/*
 * mvt_decoder_ffmpeg.c - Decoder module (FFmpeg)
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
#include "mvt_decoder_ffmpeg.h"
#include "ffmpeg_utils.h"
#include <libavutil/pixdesc.h>
#include "mvt_report.h"
#include "mvt_image.h"
#include "va_image_utils.h"

static bool
init_context(MvtDecoderFFmpeg *dec, AVCodecContext *avctx)
{
    const MvtDecoderFFmpegClass * klass = MVT_DECODER_FFMPEG_GET_CLASS(dec);

    dec->avctx = avctx;
    avctx->opaque = dec;
    if (klass->init_context && !klass->init_context(MVT_DECODER(dec)))
        return false;
    return true;
}

static bool
decoder_init(MvtDecoderFFmpeg *dec)
{
    const MvtDecoderFFmpegClass * klass = MVT_DECODER_FFMPEG_GET_CLASS(dec);
    const MvtDecoderOptions * const options = &dec->base.options;
    AVFormatContext *fmtctx;
    AVCodecContext *avctx;
    AVCodec *codec;
    char errbuf[BUFSIZ];
    int i, ret;

    if (klass->init && !klass->init(MVT_DECODER(dec)))
        return false;

    av_register_all();

    // Open and identify media file
    ret = avformat_open_input(&dec->fmtctx, options->filename, NULL, NULL);
    if (ret != 0)
        goto error_open_file;
    ret = avformat_find_stream_info(dec->fmtctx, NULL);
    if (ret < 0)
        goto error_identify_file;
    av_dump_format(dec->fmtctx, 0, options->filename, 0);
    fmtctx = dec->fmtctx;

    // Find the video stream and identify the codec
    for (i = 0; i < fmtctx->nb_streams; i++) {
        if (fmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
            !dec->stream)
            dec->stream = fmtctx->streams[i];
        else
            fmtctx->streams[i]->discard = AVDISCARD_ALL;
    }
    if (!dec->stream)
        goto error_no_video_stream;

    avctx = dec->stream->codec;
    if (!init_context(dec, avctx))
        return false;

    codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec)
        goto error_no_codec;
    if (avcodec_open2(avctx, codec, NULL) < 0)
        goto error_open_codec;

    dec->frame = avcodec_alloc_frame();
    if (!dec->frame)
        goto error_alloc_frame;
    return true;

    /* ERRORS */
error_open_file:
    mvt_error("failed to open file `%s': %s", options->filename,
        ffmpeg_strerror(ret, errbuf));
    return false;
error_identify_file:
    mvt_error("failed to identify file `%s': %s", options->filename,
        ffmpeg_strerror(ret, errbuf));
    return false;
error_no_video_stream:
    mvt_error("failed to find a video stream");
    return false;
error_no_codec:
    mvt_error("failed to find codec info for codec %d", avctx->codec_id);
    return false;
error_open_codec:
    mvt_error("failed to open codec %d", avctx->codec_id);
    return false;
error_alloc_frame:
    mvt_error("failed to allocate video frame");
    return false;
}

static void
decoder_finalize(MvtDecoderFFmpeg *dec)
{
    const MvtDecoderFFmpegClass * klass = MVT_DECODER_FFMPEG_GET_CLASS(dec);

    if (dec->avctx)
        avcodec_close(dec->avctx);
    if (dec->fmtctx)
        avformat_close_input(&dec->fmtctx);
    av_freep(&dec->frame);

    if (klass->finalize)
        klass->finalize(MVT_DECODER(dec));
}

static bool
handle_frame(MvtDecoderFFmpeg *dec, AVFrame *frame)
{
    return mvt_decoder_ffmpeg_handle_frame(dec, frame);
}

static bool
decode_packet(MvtDecoderFFmpeg *dec, AVPacket *packet, int *got_frame_ptr)
{
    char errbuf[BUFSIZ];
    int got_frame, ret;

    if (!got_frame_ptr)
        got_frame_ptr = &got_frame;

    ret = avcodec_decode_video2(dec->avctx, dec->frame, got_frame_ptr, packet);
    if (ret < 0)
        goto error_decode_frame;
    if (*got_frame_ptr && !handle_frame(dec, dec->frame))
        return false;
    return true;

    /* ERRORS */
error_decode_frame:
    mvt_error("failed to decode frame: %s", ffmpeg_strerror(ret, errbuf));
    return false;
}

static bool
decoder_run(MvtDecoderFFmpeg *dec)
{
    AVPacket packet;
    char errbuf[BUFSIZ];
    int got_frame, ret;
    bool success;

    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    for (;;) {
        // Read frame from file
        ret = av_read_frame(dec->fmtctx, &packet);
        if (ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            goto error_read_frame;

        // Decode video packet
        success = packet.stream_index != dec->stream->index ||
            decode_packet(dec, &packet, NULL);
        av_free_packet(&packet);
        if (!success)
            return false;
    }

    // Decode cached frames
    packet.data = NULL;
    packet.size = 0;
    do {
        if (!decode_packet(dec, &packet, &got_frame))
            return false;
    } while (got_frame);
    return true;

    /* ERRORS */
error_read_frame:
    mvt_error("failed to read frame: %s", ffmpeg_strerror(ret, errbuf));
    return false;
}

void
mvt_decoder_ffmpeg_class_init(MvtDecoderFFmpegClass *klass, uint32_t size)
{
    MvtDecoderClass * const dec_class = MVT_DECODER_CLASS(klass);

    dec_class->size = size;
    dec_class->init = (MvtDecoderInitFunc)decoder_init;
    dec_class->finalize = (MvtDecoderFinalizeFunc)decoder_finalize;
    dec_class->run = (MvtDecoderRunFunc)decoder_run;
}

bool
mvt_decoder_ffmpeg_handle_frame(MvtDecoderFFmpeg *dec, AVFrame *frame)
{
    VideoFormat format;
    VAImage va_image;
    const VAImageFormat *va_format;
    MvtImage src_image;
    uint32_t i;
    bool success;

    if (!ffmpeg_to_mvt_video_format(frame->format, &format))
        goto error_unsupported_format;
    va_format = video_format_to_va_format(format);
    if (!va_format)
        goto error_unsupported_format;

    va_image_init_defaults(&va_image);
    va_image.format = *va_format;
    va_image.width = frame->width;
    va_image.height = frame->height;
    va_image.data_size = 0;
    va_image.num_planes = video_format_get_info(format)->num_planes;
    for (i = 0; i < va_image.num_planes; i++) {
        va_image.pitches[i] = frame->linesize[i];
        va_image.offsets[i] = 0;
    }
    if (!mvt_image_init_from_va_image(&src_image, &va_image))
        goto error_unsupported_image;

    for (i = 0; i < va_image.num_planes; i++)
        src_image.pixels[i] = frame->data[i];

    success = mvt_decoder_handle_image(&dec->base, &src_image, 0);
    mvt_image_clear(&src_image);
    return success;

    /* ERRORS */
error_unsupported_format:
    mvt_error("unsupported video format %s",
        av_get_pix_fmt_name(frame->format));
    return false;
error_unsupported_image:
    mvt_error("failed to extract image from frame %p", frame);
    return false;
}
