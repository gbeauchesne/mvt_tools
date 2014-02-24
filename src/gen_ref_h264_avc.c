/*
 * gen_ref_h264_avc.c - Reference H.264 (JM) decoder
 *
 * Copyright (C) 2014 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301
 */

#include "sysdeps.h"
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#undef VERSION
#include <h264decoder.h>
#include <annexb.h>
#include "mvt_decoder_ffmpeg.h"
#include "ffmpeg_compat.h"
#include "mvt_string.h"

// Default time to wait for thread initialization
#define DEFAULT_THREAD_INIT_TIME (3)

// Defined to 1 if libavutil features refcounted buffers for AVFrame
#define AV_FEATURE_REFCOUNTED_BUFFERS \
    (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,43,0))

/* ------------------------------------------------------------------------ */
/* --- Utilities                                                        --- */
/* ------------------------------------------------------------------------ */

// Determines the name of the temporary directory
static const char *
get_tmpdir(void)
{
    const char *tmpdir;

    tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = P_tmpdir;
    if (!tmpdir)
        tmpdir = "/tmp";
    return tmpdir;
}

// Adds the specified number of milliseconds to the supplied timespec argument
static void
ts_add_msec(struct timespec *ts, int64_t msec)
{
    const uint64_t nsec = ts->tv_nsec + msec * 1000000;

    ts->tv_sec += nsec / 1000000000;
    ts->tv_nsec = nsec % 1000000000;
}

enum {
    THREAD_STATE_ACTIVE = 1 << 0,       ///< Flag: thread was created
    THREAD_STATE_INIT   = 1 << 1,       ///< Flag: thread got initialized
    THREAD_STATE_IDLE   = 1 << 2,       ///< Flag: thread is not doing anything
    THREAD_STATE_ERROR  = 1 << 3,       ///< Flag: thread in error state
} ThreadState;

typedef struct {
    pthread_t id;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    volatile uint32_t state;
    int error_value;                    ///< Error value if in "error" state
    bool valid;                         ///< Flag: thread context valid?
} ThreadContext;

// Initializes thread context
static bool
thread_context_init(ThreadContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    if (pthread_mutex_init(&ctx->lock, NULL) != 0)
        goto error_init_lock;
    if (pthread_cond_init(&ctx->cond, NULL) != 0)
        goto error_init_cond;
    ctx->valid = true;
    return true;

    /* ERRORS */
error_init_cond:
    pthread_mutex_destroy(&ctx->lock);
error_init_lock:
    return false;
}

// Releases any thread context resources
static void
thread_context_clear(ThreadContext *ctx)
{
    if (ctx->valid) {
        pthread_cond_destroy(&ctx->cond);
        pthread_mutex_destroy(&ctx->lock);
    }
    ctx->state = 0;
    ctx->valid = false;
}

// Removes the supplied flags from the thread state
static void
thread_context_clear_state(ThreadContext *ctx, uint32_t flags)
{
    pthread_mutex_lock(&ctx->lock);
    ctx->state &= ~flags;
    pthread_cond_broadcast(&ctx->cond);
    pthread_mutex_unlock(&ctx->lock);
}

// Adds the supplied flags to the thread state
static void
thread_context_set_state(ThreadContext *ctx, uint32_t flags)
{
    pthread_mutex_lock(&ctx->lock);
    ctx->state |= flags;
    pthread_cond_broadcast(&ctx->cond);
    pthread_mutex_unlock(&ctx->lock);
}

// Sets thread context in error state with the supplied value
static void
thread_context_set_error(ThreadContext *ctx, int value)
{
    pthread_mutex_lock(&ctx->lock);
    ctx->state |= THREAD_STATE_ERROR;
    ctx->error_value = value;
    pthread_cond_broadcast(&ctx->cond);
    pthread_mutex_unlock(&ctx->lock);
}

// Constructs the return value to thread_context_*wait() functions
static int
thread_context_wait_return(ThreadContext *ctx, uint32_t state, int ret)
{
    if (ret != 0) // pthread_cond_*wait() return value
        ret = AVERROR(ret);
    else if ((ctx->state & state) & THREAD_STATE_ERROR)
        ret = ctx->error_value;
    return ret;
}

// Waits for the thread to reach the specified state
static int
thread_context_wait(ThreadContext *ctx, uint32_t state)
{
    int ret = 0;

    pthread_mutex_lock(&ctx->lock);
    while (!(ctx->state & state) && ret == 0)
        ret = pthread_cond_wait(&ctx->cond, &ctx->lock);
    ret = thread_context_wait_return(ctx, state, ret);
    pthread_mutex_unlock(&ctx->lock);
    return ret;
}

// Waits for the thread to reach the specified state, or timeout
static int
thread_context_timedwait(ThreadContext *ctx, uint32_t state, int64_t timeout)
{
    struct timespec ts;
    int ret;

    if (timeout < 0)
        return thread_context_wait(ctx, state);

    clock_gettime(CLOCK_REALTIME, &ts);
    ts_add_msec(&ts, timeout);
    ret = 0;

    pthread_mutex_lock(&ctx->lock);
    while (!(ctx->state & state) && ret == 0)
        ret = pthread_cond_timedwait(&ctx->cond, &ctx->lock, &ts);
    ret = thread_context_wait_return(ctx, state, ret);
    pthread_mutex_unlock(&ctx->lock);
    return ret;
}

/* ------------------------------------------------------------------------ */
/* --- Wrapper around the JM Reference Software                         --- */
/* ------------------------------------------------------------------------ */

typedef struct {
    MvtDecoderFFmpeg base;
    const uint8_t *extradata;           ///< Additional data (SPS/PPS headers)
    bool is_avcC;                       ///< Bitstream is in avcC format (MP4)
    uint32_t nal_length_size;           ///< Length of a NALU size element
    int pipe;                           ///< FIFO for the raw bitstream (data)
    char *pipe_name;                    ///< "$TMPDIR/gen_ref_h264.<PID>.pipe"

    ThreadContext parser_thread;        ///< Temporary thread to initialize pipe
    ThreadContext decoder_thread;       ///< Main decoder thread: DecodeOneFrame
    DecodedPicList *decoder_pic_list;   ///< List of decoded pictures to JM
    DecodedPicList **decoder_pic_list_tail;
    DecodedPicList *output_pic_list;    ///< List of output pictures for JM
    DecodedPicList **output_pic_list_tail;

    /* JM decoder data */
    int jm_pipe;
    InputParameters jm_input;
    DecoderParams *jm_decoder;
    bool jm_decoder_flushed;
} Decoder;

// Writes data to the bitstream pipe
static inline bool
bs_write(Decoder *decoder, const uint8_t *buf, uint32_t buf_size)
{
    return write(decoder->pipe, buf, buf_size) == buf_size;
}

// Writes byte to the bitstream pipe
static inline bool
bs_write_uint8(Decoder *decoder, uint8_t value)
{
    return bs_write(decoder, &value, 1);
}

// Writes start code prefix to the bitstream pipe
static bool
bs_write_start_code_prefix(Decoder *decoder)
{
    static const uint8_t start_code_prefix[4] = { 0x00, 0x00, 0x00, 0x01 };

    return bs_write(decoder, start_code_prefix, sizeof(start_code_prefix));
}

// Writes NAL unit to the bitstream pipe
static bool
bs_write_nal_unit(Decoder *decoder, const uint8_t *nal, uint32_t nal_size)
{
    if (decoder->is_avcC && !bs_write_start_code_prefix(decoder))
        return false;
    return bs_write(decoder, nal, nal_size);
}

// Writes filler_data() NAL unit
static bool
bs_write_filler_data(Decoder *decoder, int length)
{
    if (!bs_write_start_code_prefix(decoder))
        return false;

    if (!bs_write_uint8(decoder, 0x0c))
        return false;
    while (length-- > 0) {
        if (!bs_write_uint8(decoder, 0xff))
            return false;
    }
    return bs_write_uint8(decoder, 0x80);
}

// Writes end_of_stream() NAL unit
static bool
bs_write_eos(Decoder *decoder)
{
    if (!bs_write_start_code_prefix(decoder))
        return false;
    return bs_write_uint8(decoder, 0x0b);
}

// Decodes extradata buffer (SPS/PPS headers)
static int
decode_extradata(Decoder *decoder)
{
    AVCodecContext * const avctx = MVT_DECODER_FFMPEG(decoder)->avctx;
    const uint8_t *buf = avctx->extradata;
    const uint8_t * const buf_end = buf + avctx->extradata_size;
    uint32_t i, num_headers, nal_size;

    if (decoder->extradata == buf)
        return 0;
    decoder->extradata = buf;

    decoder->is_avcC = buf[0] == 1; /* configurationVersion */
    if (!decoder->is_avcC) {
        nal_size = avctx->extradata_size;
        if (!bs_write(decoder, buf, nal_size))
            goto error_write_pipe;
        return 0;
    }

    decoder->nal_length_size = (buf[4] & 0x03) + 1;

    /* SPS */
    num_headers = buf[5] & 0x1f;
    buf += 6;
    for (i = 0; i < num_headers; i++) {
        if (buf + 2 > buf_end)
            goto error_no_data;
        nal_size = ((uint32_t)buf[0] << 8) | buf[1]; buf += 2;
        if (buf + nal_size > buf_end)
            goto error_no_data;
        if (!bs_write_nal_unit(decoder, buf, nal_size))
            goto error_write_pipe;
        buf += nal_size;
    }

    /* PPS */
    num_headers = *buf++;
    for (i = 0; i < num_headers; i++) {
        if (buf + 2 > buf_end)
            goto error_no_data;
        nal_size = ((uint32_t)buf[0] << 8) | buf[1]; buf += 2;
        if (buf + nal_size > buf_end)
            goto error_no_data;
        if (!bs_write_nal_unit(decoder, buf, nal_size))
            goto error_write_pipe;
        buf += nal_size;
    }
    return 0;

    /* ERRORS */
error_no_data:
    mvt_warning("failed to parse enough data from extradata");
    return AVERROR_INVALIDDATA;
error_write_pipe:
    mvt_error("failed to write NAL unit (%u bytes) to pipe", nal_size);
    return AVERROR_INVALIDDATA;
}

// Decodes source packet and forward it to the JM decoder
static int
decode_packet(Decoder *decoder, const uint8_t *buf, uint32_t buf_size)
{
    const uint8_t * const buf_end = buf + buf_size;
    uint32_t i, nal_size;
    int ret;

    if ((ret = decode_extradata(decoder)) < 0)
        return ret;

    if (decoder->is_avcC) {
        do {
            if (buf + decoder->nal_length_size > buf_end)
                break;
            for (i = 0, nal_size = 0; i < decoder->nal_length_size; i++)
                nal_size = (nal_size << 8) | buf[i];
            buf += decoder->nal_length_size;
            if (buf + nal_size > buf_end)
                goto error_invalid_buffer;
            if (!bs_write_nal_unit(decoder, buf, nal_size))
                goto error_write_pipe;
            buf += nal_size;
        } while (buf < buf_end);
    }
    else if (!bs_write_nal_unit(decoder, buf, buf_size))
        goto error_write_pipe;
    return 0;

    /* ERRORS */
error_invalid_buffer:
    mvt_error("failed to parse NAL unit (%u bytes)", nal_size);
    return AVERROR_INVALIDDATA;
error_write_pipe:
    mvt_error("failed to write NAL units to pipe");
    return AVERROR_INVALIDDATA;
}

// Parser thread
static void *
parser_thread_func(void *arg)
{
    Decoder * const decoder = arg;
    ThreadContext * const ctx = &decoder->parser_thread;
    int ret;

    decoder->pipe = open(decoder->pipe_name, O_WRONLY);
    if (!decoder->pipe)
        goto error_open_pipe;

    /* OpenDecoder() expects some initial data in the source bitstream */
    if (!bs_write_filler_data(decoder, 0))
        goto error_fill_pipe;

    /* Submit extradata headers now, should they exist */
    ret = decode_extradata(decoder);
    if (ret < 0)
        goto error;

    thread_context_set_state(ctx, THREAD_STATE_INIT);
    return NULL;

    /* ERRORS */
error:
    thread_context_set_error(ctx, ret);
    return NULL;
error_open_pipe:
    mvt_error("failed to open pipe `%s' for writing", decoder->pipe_name);
    thread_context_set_error(ctx, AVERROR(EIO));
    return NULL;
error_fill_pipe:
    mvt_error("failed to send filler_data() NAL unit");
    thread_context_set_error(ctx, AVERROR(EIO));
    return NULL;
}

// Initializes JM decoder input params
static int
init_input_params(Decoder *decoder)
{
    InputParameters * const params = &decoder->jm_input;
    int n;

    memset(params, 0, sizeof(*params));

    n = strlen(decoder->pipe_name) + 1;
    if (sizeof(params->infile) < n)
        return AVERROR(ENOMEM);
    memcpy(params->infile, decoder->pipe_name, n);

    params->outfile[0] = '\0';
    params->reffile[0] = '\0';

    params->FileFormat = PAR_OF_ANNEXB;
    params->poc_scale = 2;
    params->write_uv = 0; /* don't write chroma components for grayscale */
    params->silent = 1;

    params->conceal_mode = 0;
    params->ref_poc_gap = 2;
    params->poc_gap = 2;
    return 0;
}

// Initializes JM decoder video params
static int
init_video_params(Decoder *decoder, int out_fd)
{
    VideoParameters *params;
    int i, pipe;

    if (!decoder->jm_decoder)
        return 0;
    params = decoder->jm_decoder->p_Vid;
    if (!params)
        return 0;

    pipe = decoder->jm_pipe;
    if (pipe < 0) {
        if (params && params->annex_b)
            pipe = params->annex_b->BitStreamFile;
        if (pipe < 0)
            goto error_open_pipe;
        decoder->jm_pipe = pipe;
    }

    /* This is a hack valid at least for JM 18.6.

       We want output.c(write_out_picture) to meet all the following
       constraints: (i) generate the DecodedPicList buffers, i.e. call
       p_Vid->img2buf(), (ii) do not write the buffers to disk, and
       (iii) keep the bValid bit set to 1 */
    params->p_out = out_fd;
#if (MVC_EXTENSION_ENABLE)
    for (i = 0; i < MVT_ARRAY_LENGTH(params->p_out_mvc); i++)
        params->p_out_mvc[i] = params->p_out;
#endif
    return 0;

    /* ERRORS */
error_open_pipe:
    mvt_error("failed to determine file descriptor to JM input");
    return AVERROR(EIO);
}

// Checks whether the JM decoder has pending data in its bitstream file (pipe)
static int
has_pending_data(Decoder *decoder, int timeout)
{
    struct pollfd pfd;
    int ret;

    if (decoder->jm_pipe < 0)
        return 0;

    pfd.fd = decoder->jm_pipe;
    pfd.events = POLLIN;
    pfd.revents = 0;
    ret = poll(&pfd, 1, timeout);
    if (ret > 0)
        return (pfd.revents & POLLIN) != 0;
    if (ret < 0)
        return AVERROR(errno);
    return 0;
}

// Appends picture to the list of decoded pictures
static inline DecodedPicList *
dp_queue_push(Decoder *decoder, DecodedPicList *pic)
{
    *decoder->decoder_pic_list_tail = pic;
    decoder->decoder_pic_list_tail = &pic->pNext;
    return pic;
}

// Appends picture copy to the list of decoded pictures
static DecodedPicList *
dp_queue_push_copy(Decoder *decoder, DecodedPicList *pic)
{
    DecodedPicList *copy;

    copy = malloc(sizeof(*copy));
    if (!copy)
        return NULL;
    *copy = *pic;
    copy->pNext = NULL;

    pic->bValid = 0;
    pic->pY = NULL;
    pic->pU = NULL;
    pic->pV = NULL;
    pic->iBufSize = 0;

    dp_queue_push(decoder, copy);
    return copy;
}

// Returns the next decoded picture from the queue
static DecodedPicList *
dp_queue_pop(Decoder *decoder)
{
    DecodedPicList *pic;

    if (!(pic = decoder->decoder_pic_list))
        return NULL;
    if (!(decoder->decoder_pic_list = pic->pNext))
        decoder->decoder_pic_list_tail = &decoder->decoder_pic_list;
    pic->pNext = NULL;
    return pic;
}

// Decoder thread
static void *
decoder_thread_func(void *arg)
{
    Decoder * const decoder = arg;
    ThreadContext * const ctx = &decoder->decoder_thread;
    DecodedPicList *pic_list;
    int ret;

    ret = init_input_params(decoder);
    if (ret < 0)
        goto error;

    if (OpenDecoder(&decoder->jm_input) != DEC_OPEN_NOERR)
        goto error_open_decoder;
    decoder->jm_decoder = p_Dec;

    ret = init_video_params(decoder, -2);
    if (ret < 0)
        goto error;

    thread_context_set_state(ctx, THREAD_STATE_INIT);

    /* Wait for packets to decode */
    for (;;) {
        thread_context_set_state(ctx, THREAD_STATE_IDLE);
        ret = has_pending_data(decoder, 100);
        if (ret == AVERROR(ETIMEDOUT))
            continue;
        if (ret < 0)
            goto error;

        thread_context_clear_state(ctx, THREAD_STATE_IDLE);
        ret = DecodeOneFrame(&pic_list);
        switch (ret) {
        case DEC_EOS:
            ret = AVERROR_EOF;
            goto submit_decoded_picture;
        case DEC_SUCCEED:
            ret = 0;
        submit_decoded_picture:
            pthread_mutex_lock(&ctx->lock);
            if (pic_list->bValid && !dp_queue_push_copy(decoder, pic_list))
                ret = AVERROR(ENOMEM);

            /* Release output pics back to the JM decoder pool */
            if (decoder->output_pic_list) {
                VideoParameters * const vid_params = p_Dec->p_Vid;
                *decoder->output_pic_list_tail = vid_params->pDecOuputPic;
                vid_params->pDecOuputPic = decoder->output_pic_list;
                decoder->output_pic_list = NULL;
                decoder->output_pic_list_tail = &decoder->output_pic_list;
            }
            pthread_mutex_unlock(&ctx->lock);
            break;
        default:
            ret = AVERROR_INVALIDDATA;
            break;
        }
        if (ret < 0)
            goto error;
    }
    return NULL;

    /* ERRORS */
error:
    thread_context_set_error(ctx, ret);
    return NULL;
error_open_decoder:
    mvt_error("failed to open decoder");
    thread_context_set_error(ctx, AVERROR_DECODER_NOT_FOUND);
    return NULL;
}

// Initializes JM decoder resources
static int
jm_init(Decoder *decoder)
{
    int ret;

    decoder->pipe = -1;
    decoder->jm_pipe = -1;
    if (!thread_context_init(&decoder->parser_thread))
        goto error_alloc_thread_resources;
    if (!thread_context_init(&decoder->decoder_thread))
        goto error_alloc_thread_resources;

    decoder->pipe_name = str_dup_printf("%s/gen_ref_h264.%d.pipe",
        get_tmpdir(), getpid());
    if (!decoder->pipe_name)
        goto error_alloc_memory;

    ret = mkfifo(decoder->pipe_name, 0600);
    if (ret < 0)
        goto error_create_pipe;

    /* Start parser and decoder threads at the same time since opening
       the named pipe (FIFO) is a blocking operation, i.e. writer end and
       consumer end need to be opened at the same time */
    if (pthread_create(&decoder->parser_thread.id, NULL,
            parser_thread_func, decoder) != 0)
        goto error_alloc_thread_resources;
    decoder->parser_thread.state |= THREAD_STATE_ACTIVE;

    if (pthread_create(&decoder->decoder_thread.id, NULL,
            decoder_thread_func, decoder) != 0)
        goto error_alloc_thread_resources;
    decoder->decoder_thread.state |= THREAD_STATE_ACTIVE;

    /* Wait for threads to complete initialization (FIFO and decoder opened) */
    ret = thread_context_timedwait(&decoder->parser_thread,
        THREAD_STATE_INIT|THREAD_STATE_ERROR, DEFAULT_THREAD_INIT_TIME * 1000);
    if (ret < 0)
        return ret;

    ret = thread_context_timedwait(&decoder->decoder_thread,
        THREAD_STATE_INIT|THREAD_STATE_ERROR, DEFAULT_THREAD_INIT_TIME * 1000);
    if (ret < 0)
        return ret;

    decoder->decoder_pic_list_tail = &decoder->decoder_pic_list;
    decoder->output_pic_list_tail = &decoder->output_pic_list;
    decoder->extradata = NULL;
    return 0;

    /* ERRORS */
error_alloc_thread_resources:
    mvt_error("failed to allocate thread resources: %s", strerror(errno));
    return AVERROR(errno);
error_alloc_memory:
    mvt_error("failed to allocate memory");
    return AVERROR(ENOMEM);
error_create_pipe:
    mvt_error("failed to create pipe `%s': %s", decoder->pipe_name,
        strerror(errno));
    return AVERROR(errno);
}

// Releases JM decoder resources
static void
jm_finalize(Decoder *decoder)
{
    ThreadContext *ctx;

    ctx = &decoder->parser_thread;
    if (ctx->state & THREAD_STATE_ACTIVE) {
        pthread_cancel(ctx->id);
        pthread_join(ctx->id, NULL);
    }
    thread_context_clear(ctx);

    ctx = &decoder->decoder_thread;
    if (ctx->state & THREAD_STATE_ACTIVE) {
        pthread_cancel(ctx->id);
        pthread_join(ctx->id, NULL);
    }
    thread_context_clear(ctx);

    if (decoder->output_pic_list && decoder->jm_decoder) {
        VideoParameters * const vid_params = decoder->jm_decoder->p_Vid;
        *decoder->output_pic_list_tail = vid_params->pDecOuputPic;
        vid_params->pDecOuputPic = decoder->output_pic_list;
        decoder->output_pic_list = NULL;
        decoder->output_pic_list_tail = &decoder->output_pic_list;
    }

    if (decoder->jm_decoder) {
        init_video_params(decoder, -1);
        CloseDecoder();
        decoder->jm_decoder = NULL;
        decoder->jm_pipe = -1;
    }

    if (decoder->pipe >= 0) {
        close(decoder->pipe);
        decoder->pipe = -1;
    }

    if (decoder->pipe_name) {
        unlink(decoder->pipe_name);
        free(decoder->pipe_name);
        decoder->pipe_name = NULL;
    }
}

// Moves the picture to the list of pictures to re-inject into JM decoder
static void
jm_release_picture_buffer(Decoder *decoder, DecodedPicList *pic)
{
    ThreadContext * const ctx = &decoder->decoder_thread;

    pthread_mutex_lock(&ctx->lock);
    pic->bValid = 0;
    *decoder->output_pic_list_tail = pic;
    decoder->output_pic_list_tail = &pic->pNext;
    pthread_mutex_unlock(&ctx->lock);
}

static void
jm_release_picture_buffer_cb(void *opaque, uint8_t *data)
{
    jm_release_picture_buffer(opaque, (DecodedPicList *)data);
}

// Acquires a new frame buffer for the supplied picture (libavutil >= 51.43.0)
static int
jm_acquire_picture_buffer(Decoder *decoder, DecodedPicList *pic, AVFrame *frame)
{
#if AV_FEATURE_REFCOUNTED_BUFFERS
    AVBufferRef *buf;

    buf = av_buffer_create((uint8_t *)pic, 0, jm_release_picture_buffer_cb,
        decoder, AV_BUFFER_FLAG_READONLY);
    if (!buf)
        return AVERROR(ENOMEM);

    frame->buf[0] = buf;
#endif
    return 0;
}

// Flushes the JM decoder output queue
static int
jm_flush_pictures(Decoder *decoder)
{
    DecodedPicList *pic_list;
    int n;

    if (FinitDecoder(&pic_list) != DEC_GEN_NOERR)
        return AVERROR_UNKNOWN;

    for (n = 0; pic_list && pic_list->bValid; pic_list = pic_list->pNext) {
        if (!dp_queue_push_copy(decoder, pic_list))
            return AVERROR(ENOMEM);
        n++;
    }
    return n;
}

// Determines an FFmpeg compatible pixel format for the supplied picture
static enum AVPixelFormat
jm_get_pixel_format(DecodedPicList *pic)
{
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;

    if (pic->iYUVStorageFormat != 0)
        return AV_PIX_FMT_NONE;

    /* For high bit depths, little endian byte order is always used,
       even on big endian systems (as of JM 18.6) */
    switch (pic->iBitDepth) {
    case 8:
        switch (pic->iYUVFormat) {
        case YUV400: pix_fmt = AV_PIX_FMT_GRAY8;       break;
        case YUV420: pix_fmt = AV_PIX_FMT_YUV420P;     break;
        case YUV422: pix_fmt = AV_PIX_FMT_YUV422P;     break;
        case YUV444: pix_fmt = AV_PIX_FMT_YUV444P;     break;
        }
        break;
    case 10:
        switch (pic->iYUVFormat) {
        case YUV420: pix_fmt = AV_PIX_FMT_YUV420P10LE; break;
        case YUV422: pix_fmt = AV_PIX_FMT_YUV422P10LE; break;
        case YUV444: pix_fmt = AV_PIX_FMT_YUV444P10LE; break;
        }
        break;
#if LIBAVUTIL_VERSION_INT > AV_VERSION_INT(51,34,0)
    case 12:
        switch (pic->iYUVFormat) {
        case YUV420: pix_fmt = AV_PIX_FMT_YUV420P12LE; break;
        case YUV422: pix_fmt = AV_PIX_FMT_YUV422P12LE; break;
        case YUV444: pix_fmt = AV_PIX_FMT_YUV444P12LE; break;
        }
        break;
#endif
    case 16:
        switch (pic->iYUVFormat) {
        case YUV400: pix_fmt = AV_PIX_FMT_GRAY16;      break;
        case YUV420: pix_fmt = AV_PIX_FMT_YUV420P16LE; break;
        case YUV422: pix_fmt = AV_PIX_FMT_YUV422P16LE; break;
        case YUV444: pix_fmt = AV_PIX_FMT_YUV444P16LE; break;
        }
        break;
    }
    return pix_fmt;
}

// Handles the decoded picture
static int
jm_handle_picture(Decoder *decoder, DecodedPicList *pic, AVFrame *frame)
{
    AVCodecContext * const avctx = MVT_DECODER_FFMPEG(decoder)->avctx;
    int ret;

    if (!pic || !pic->bValid)
        return AVERROR_INVALIDDATA;

    ret = jm_acquire_picture_buffer(decoder, pic, frame);
    if (ret < 0)
        return ret;

    frame->format = jm_get_pixel_format(pic);
    if (frame->format == AV_PIX_FMT_NONE)
        return AVERROR(EINVAL);

    frame->width        = pic->iWidth;
    frame->height       = pic->iHeight;
    frame->data[0]      = pic->pY;
    frame->linesize[0]  = pic->iYBufStride;
    frame->data[1]      = pic->pU;
    frame->linesize[1]  = pic->iUVBufStride;
    frame->data[2]      = pic->pV;
    frame->linesize[2]  = pic->iUVBufStride;
    frame->opaque       = pic;

    if (avctx->pix_fmt != frame->format)
        avctx->pix_fmt = frame->format;
    if (avctx->width != frame->width || avctx->height != frame->height)
        avcodec_set_dimensions(avctx, frame->width, frame->height);
    return 0;
}

// Releases pictures back to the JM decoder
static void
jm_release_frame(Decoder *decoder, AVFrame *frame)
{
    DecodedPicList * const pic = frame->opaque;

    if (!pic)
        return;

    jm_release_picture_buffer(decoder, pic);
    frame->opaque = NULL;
    avcodec_get_frame_defaults(frame);
}

// Decodes any frame that was parsed by FFmpeg
static int
jm_decode_packet(Decoder *decoder, AVPacket *pkt, AVFrame *frame,
    int *got_frame)
{
    const bool at_eos = pkt->data == NULL && pkt->size == 0;
    ThreadContext *ctx;
    DecodedPicList *pic;
    int ret;

    jm_release_frame(decoder, frame);

    /* Submit the packet to the JM decoder */
    if (!at_eos)
        ret = decode_packet(decoder, pkt->data, pkt->size);
    else if (decoder->pipe < 0)
        ret = 0;
    else {
        ret = bs_write_eos(decoder) ? 0 : AVERROR(EIO);
        close(decoder->pipe);
        decoder->pipe = -1;
    }
    if (ret < 0)
        return ret;

    /* Wait for a decoded frame, if any */
    ctx = &decoder->decoder_thread;
    pthread_mutex_lock(&ctx->lock);
    do {
        if ((pic = dp_queue_pop(decoder)) != NULL)
            ret = 0;
        else if (at_eos) {
            if (!(ctx->state & THREAD_STATE_ERROR))
                pthread_cond_wait(&ctx->cond, &ctx->lock);
            if (!(ctx->state & THREAD_STATE_ERROR))
                ret = 1;
            else if ((ret = ctx->error_value) == AVERROR_EOF)
                ret = jm_flush_pictures(decoder);
        }
        else if (!has_pending_data(decoder, 0))
            ret = 0;
        else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts_add_msec(&ts, 100); /* 100 ms */
            ret = pthread_cond_timedwait(&ctx->cond, &ctx->lock, &ts);
            if (ret != 0 && ret != ETIMEDOUT)
                ret = AVERROR(ret);
            else if (ctx->state & THREAD_STATE_ERROR)
                ret = ctx->error_value;
            else
                ret = 1;
        }
    } while (ret > 0);
    pthread_mutex_unlock(&ctx->lock);
    if (pic)
        *got_frame = (ret = jm_handle_picture(decoder, pic, frame)) == 0;
    if (ret < 0)
        return ret;
    return pkt->size;
}

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

static int
h264_init(AVCodecContext *avctx)
{
    Decoder * const decoder = avctx->opaque;

    return jm_init(decoder);
}

static int
h264_finalize(AVCodecContext *avctx)
{
    Decoder * const decoder = avctx->opaque;

    jm_finalize(decoder);
    return 0;
}

static int
h264_decode(AVCodecContext *avctx, void *data, int *got_frame, AVPacket *pkt)
{
    Decoder * const decoder = avctx->opaque;
    AVFrame * const frame = data;

    return jm_decode_packet(decoder, pkt, frame, got_frame);
}

static AVCodec h264_decoder = {
    .name               = "h264_ref_avc",
    .long_name          = "H.264 AVC Reference Decoder (JM)",
    .type               = AVMEDIA_TYPE_VIDEO,
    .id                 = AV_CODEC_ID_H264,
    .init               = h264_init,
    .close              = h264_finalize,
    .decode             = h264_decode,
    .capabilities       = CODEC_CAP_DELAY,
};

static void
decoder_finalize(Decoder *decoder)
{
    jm_finalize(decoder);
}

static AVCodec *
decoder_find_decoder(Decoder *decoder, int codec_id)
{
    AVCodec *codec;

    switch (codec_id) {
    case AV_CODEC_ID_H264:
        codec = &h264_decoder;
        break;
    default:
        codec = NULL;
        break;
    }
    return codec;
}

const MvtDecoderClass *
mvt_decoder_class(void)
{
    static MvtDecoderFFmpegClass g_class = {
        .finalize = (MvtDecoderFinalizeFunc)decoder_finalize,
        .find_decoder = (MvtDecoderFFmpegFindDecoderFunc)decoder_find_decoder,
    };
    if (!g_class.base.size)
        mvt_decoder_ffmpeg_class_init(&g_class, sizeof(Decoder));
    return &g_class.base;
}
