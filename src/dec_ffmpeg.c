/*
 * dec_ffmpeg.c - Decoder based on FFmpeg
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
#include <libavutil/pixdesc.h>
#include <libavcodec/vaapi.h>
#include "ffmpeg_compat.h"
#include "mvt_decoder_ffmpeg.h"
#include "mvt_display.h"
#include "mvt_memory.h"
#include "va_utils.h"
#include "va_image_utils.h"

typedef struct {
    MvtDecoderInitFunc init;
    MvtDecoderFinalizeFunc finalize;
    MvtDecoderFFmpegInitContextFunc init_context;
    MvtDecoderFFmpegHandleFrameFunc handle_frame;
} HWAccelDecoder;

typedef struct {
    MvtDecoderFFmpeg base;
    MvtDisplay *display;
    const HWAccelDecoder *hwaccel;
    MvtImage *image;

    // VA-API resources
    struct vaapi_context va_context;
    VAProfile *va_profiles;
    uint32_t num_va_profiles;
    VASurfaceID *va_surfaces;
    uint32_t num_va_surfaces;
    VASurfaceID *va_surfaces_queue;
    uint32_t va_surfaces_queue_length;
    uint32_t va_surfaces_queue_head;
    uint32_t va_surfaces_queue_tail;
} Decoder;

/* ------------------------------------------------------------------------ */
/* --- VA-API Decoder                                                   --- */
/* ------------------------------------------------------------------------ */

// Ensures the array of VA profiles is allocated and filled up correctly
static bool
vaapi_ensure_profiles(Decoder *decoder)
{
    struct vaapi_context * const vactx = &decoder->va_context;
    VAProfile *profiles;
    int num_profiles;
    VAStatus va_status;

    if (decoder->va_profiles && decoder->num_va_profiles > 0)
        return true;

    num_profiles = vaMaxNumProfiles(vactx->display);
    profiles = malloc(num_profiles * sizeof(*profiles));
    if (!profiles)
        return false;

    va_status = vaQueryConfigProfiles(vactx->display, profiles, &num_profiles);
    if (!va_check_status(va_status, "vaQueryConfigProfiles()"))
        goto error_query_profiles;

    decoder->va_profiles = profiles;
    decoder->num_va_profiles = num_profiles;
    return true;

    /* ERRORS */
error_query_profiles:
    mvt_error("failed to query the set of supported profiles");
    free(profiles);
    return false;
}

// Ensures the array of VA surfaces and queue of free VA surfaces are allocated
static bool
vaapi_ensure_surfaces(Decoder *decoder, uint32_t num_surfaces)
{
    uint32_t size, new_size = num_surfaces * sizeof(VASurfaceID);
    uint32_t i;

    size = decoder->num_va_surfaces * sizeof(*decoder->va_surfaces);
    if (!mem_reallocp32(&decoder->va_surfaces, &size, new_size))
        goto error_alloc_surfaces;
    if (decoder->num_va_surfaces < num_surfaces)
        decoder->num_va_surfaces = num_surfaces;

    size = decoder->va_surfaces_queue_length * sizeof(*decoder->va_surfaces);
    if (!mem_reallocp32(&decoder->va_surfaces_queue, &size, new_size))
        goto error_alloc_surfaces_queue;
    if (decoder->va_surfaces_queue_length < num_surfaces)
        decoder->va_surfaces_queue_length = num_surfaces;

    for (i = 0; i < decoder->va_surfaces_queue_length; i++)
        decoder->va_surfaces_queue[i] = VA_INVALID_ID;
    return true;

    /* ERRORS */
error_alloc_surfaces:
    mvt_error("failed to allocate VA surfaces array");
    return false;
error_alloc_surfaces_queue:
    mvt_error("failed to allocate VA surfaces queue");
    return false;
}

// Acquires a surface from the queue of free VA surfaces
static bool
vaapi_acquire_surface(Decoder *decoder, VASurfaceID *va_surface_ptr)
{
    VASurfaceID va_surface;

    va_surface = decoder->va_surfaces_queue[decoder->va_surfaces_queue_head];
    if (va_surface == VA_INVALID_ID)
        return false;

    decoder->va_surfaces_queue[decoder->va_surfaces_queue_head] = VA_INVALID_ID;
    decoder->va_surfaces_queue_head = (decoder->va_surfaces_queue_head + 1) %
        decoder->va_surfaces_queue_length;

    if (va_surface_ptr)
        *va_surface_ptr = va_surface;
    return true;
}

// Releases a surface back to the queue of free VA surfaces
static bool
vaapi_release_surface(Decoder *decoder, VASurfaceID va_surface)
{
    const VASurfaceID va_surface_in_queue =
        decoder->va_surfaces_queue[decoder->va_surfaces_queue_tail];

    if (va_surface_in_queue != VA_INVALID_ID)
        return false;

    decoder->va_surfaces_queue[decoder->va_surfaces_queue_tail] = va_surface;
    decoder->va_surfaces_queue_tail = (decoder->va_surfaces_queue_tail + 1) %
        decoder->va_surfaces_queue_length;
    return true;
}

// Checks whether the supplied config, i.e. (profile, entrypoint) pair, exists
static bool
vaapi_has_config(Decoder *decoder, VAProfile profile, VAEntrypoint entrypoint)
{
    uint32_t i;

    if (!vaapi_ensure_profiles(decoder))
        return false;

    for (i = 0; i < decoder->num_va_profiles; i++) {
        if (decoder->va_profiles[i] == profile)
            break;
    }
    if (i == decoder->num_va_profiles)
        return false;
    return true;
}

// Initializes VA decoder comprising of VA config, surfaces and context
static bool
vaapi_init_decoder(Decoder *decoder, VAProfile profile, VAEntrypoint entrypoint)
{
    AVCodecContext * const avctx = decoder->base.avctx;
    struct vaapi_context * const vactx = &decoder->va_context;
    VAConfigID va_config = VA_INVALID_ID;
    VAContextID va_context = VA_INVALID_ID;
    VAConfigAttrib va_attribs[1], *va_attrib;
    uint32_t i, num_va_attribs = 0;
    VAStatus va_status;

    va_attrib = &va_attribs[num_va_attribs++];
    va_attrib->type = VAConfigAttribRTFormat;
    va_status = vaGetConfigAttributes(vactx->display, profile, entrypoint,
        va_attribs, num_va_attribs);
    if (!va_check_status(va_status, "vaGetConfigAttributes()"))
        return false;

    va_attrib = &va_attribs[0];
    if (va_attrib->value == VA_ATTRIB_NOT_SUPPORTED ||
        !(va_attrib->value & VA_RT_FORMAT_YUV420))
        goto error_unsupported_chroma_format;
    va_attrib->value = VA_RT_FORMAT_YUV420;

    va_status = vaCreateConfig(vactx->display, profile, entrypoint,
        va_attribs, num_va_attribs, &va_config);
    if (!va_check_status(va_status, "vaCreateConfig()"))
        return false;

    static const int SCRATCH_SURFACES = 4;
    if (!vaapi_ensure_surfaces(decoder, avctx->refs + 1 + SCRATCH_SURFACES))
        goto error_cleanup;

    va_status = vaCreateSurfaces(vactx->display,
        avctx->coded_width, avctx->coded_height, VA_RT_FORMAT_YUV420,
        decoder->num_va_surfaces, decoder->va_surfaces);
    if (!va_check_status(va_status, "vaCreateSurfaces()"))
        goto error_cleanup;

    for (i = 0; i < decoder->num_va_surfaces; i++)
        decoder->va_surfaces_queue[i] = decoder->va_surfaces[i];
    decoder->va_surfaces_queue_head = 0;
    decoder->va_surfaces_queue_tail = 0;

    va_status = vaCreateContext(vactx->display, va_config,
        avctx->coded_width, avctx->coded_height, VA_PROGRESSIVE,
        decoder->va_surfaces, decoder->num_va_surfaces, &va_context);
    if (!va_check_status(va_status, "vaCreateContext()"))
        goto error_cleanup;

    vactx->config_id = va_config;
    vactx->context_id = va_context;
    return true;

    /* ERRORS */
error_unsupported_chroma_format:
    mvt_error("unsupported YUV 4:2:0 chroma format");
    return false;
error_cleanup:
    va_destroy_context(vactx->display, &va_context);
    va_destroy_config(vactx->display, &va_config);
    return false;
}

// AVCodecContext.get_format() implementation for VA-API
static enum AVPixelFormat
vaapi_get_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
    Decoder * const decoder = avctx->opaque;
    MvtDecoder * const base_decoder = MVT_DECODER(decoder);
    VAProfile profiles[3];
    uint32_t i, num_profiles;

    // Find a VA format
    for (i = 0; pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
        if (pix_fmts[i] == AV_PIX_FMT_VAAPI_VLD)
            break;
    }
    if (pix_fmts[i] == AV_PIX_FMT_NONE)
        return AV_PIX_FMT_NONE;

    // Find a suitable VA profile that fits FFmpeg config
    num_profiles = 0;
    if (!mvt_profile_to_va_profile(base_decoder->codec, base_decoder->profile,
            &profiles[num_profiles]))
        return AV_PIX_FMT_NONE;

    switch (profiles[num_profiles++]) {
    case VAProfileMPEG2Simple:
        profiles[num_profiles++] = VAProfileMPEG2Main;
        break;
    case VAProfileMPEG4Simple:
        profiles[num_profiles++] = VAProfileMPEG4AdvancedSimple;
        // fall-through
    case VAProfileMPEG4AdvancedSimple:
        profiles[num_profiles++] = VAProfileMPEG4Main;
        break;
    case VAProfileH264ConstrainedBaseline:
        profiles[num_profiles++] = VAProfileH264Main;
        // fall-through
    case VAProfileH264Main:
        profiles[num_profiles++] = VAProfileH264High;
        break;
    case VAProfileVC1Simple:
        profiles[num_profiles++] = VAProfileVC1Main;
        // fall-through
    case VAProfileVC1Main:
        profiles[num_profiles++] = VAProfileVC1Advanced;
        break;
    default:
        break;
    }
    for (i = 0; i < num_profiles; i++) {
        if (vaapi_has_config(decoder, profiles[i], VAEntrypointVLD))
            break;
    }
    if (i == num_profiles)
        return AV_PIX_FMT_NONE;
    if (!vaapi_init_decoder(decoder, profiles[i], VAEntrypointVLD))
        return AV_PIX_FMT_NONE;
    return AV_PIX_FMT_VAAPI_VLD;
}

// AVCodecContext.get_buffer() implementation for VA-API
static int
vaapi_get_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    Decoder * const decoder = avctx->opaque;
    VASurfaceID va_surface;

    if (!vaapi_acquire_surface(decoder, &va_surface))
        return -1;

    frame->opaque = NULL;
    frame->type = FF_BUFFER_TYPE_USER;
    frame->reordered_opaque = avctx->reordered_opaque;

    memset(frame->data, 0, sizeof(frame->data));
    frame->data[0] = (uint8_t *)(uintptr_t)va_surface;
    frame->data[3] = (uint8_t *)(uintptr_t)va_surface;
    memset(frame->linesize, 0, sizeof(frame->linesize));
    frame->linesize[0] = avctx->coded_width; /* XXX: 8-bit per sample only */
    return 0;
}

// AVCodecContext.reget_buffer() implementation for VA-API
static int
vaapi_reget_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    assert(0 && "FIXME: implement AVCodecContext::reget_buffer() [VA-API]");

    // XXX: this is not the correct implementation
    return avcodec_default_reget_buffer(avctx, frame);
}

// AVCodecContext.release_buffer() implementation for VA-API
static void
vaapi_release_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    Decoder * const decoder = avctx->opaque;
    const VASurfaceID va_surface = (uintptr_t)frame->data[3];

    memset(frame->data, 0, sizeof(frame->data));
    if (!vaapi_release_surface(decoder, va_surface))
        return;
}

// Initializes AVCodecContext for VA-API decoding purposes
static bool
decoder_vaapi_init_context(Decoder *decoder)
{
    AVCodecContext * const avctx = decoder->base.avctx;

    avctx->hwaccel_context = &decoder->va_context;
    avctx->thread_count = 1;
    avctx->draw_horiz_band = 0;
    avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

    avctx->get_format = vaapi_get_format;
    avctx->get_buffer = vaapi_get_buffer;
    avctx->reget_buffer = vaapi_reget_buffer;
    avctx->release_buffer = vaapi_release_buffer;
    return true;
}

// Initializes decoder for VA-API purposes, e.g. creates the VA display
static bool
decoder_vaapi_init(Decoder *decoder)
{
    struct vaapi_context * const vactx = &decoder->va_context;

    memset(vactx, 0, sizeof(*vactx));
    vactx->config_id = VA_INVALID_ID;
    vactx->context_id = VA_INVALID_ID;

    decoder->display = mvt_display_new(NULL);
    if (!decoder->display)
        return false;
    vactx->display = decoder->display->va_display;
    return true;
}

// Destroys all VA-API related resources
static void
decoder_vaapi_finalize(Decoder *decoder)
{
    struct vaapi_context * const vactx = &decoder->va_context;

    if (vactx->display) {
        va_destroy_context(vactx->display, &vactx->context_id);
        if (decoder->va_surfaces && decoder->num_va_surfaces > 0) {
            vaDestroySurfaces(vactx->display, decoder->va_surfaces,
                decoder->num_va_surfaces);
        }
        va_destroy_config(vactx->display, &vactx->config_id);
    }
    free(decoder->va_surfaces);
    decoder->num_va_surfaces = 0;
    free(decoder->va_surfaces_queue);
    decoder->va_surfaces_queue_length = 0;
    free(decoder->va_profiles);
    decoder->num_va_profiles = 0;
}

// Handles decoded frame, i.e. hash and report the result
static bool
decoder_vaapi_handle_frame(Decoder *decoder, AVFrame *frame)
{
    struct vaapi_context * const vactx = &decoder->va_context;
    const VASurfaceID va_surface = (uintptr_t)frame->data[3];
    MvtImage dst_image, src_image;
    VAImage va_image;
    VAStatus va_status;
    VARectangle crop_rect;
    int data_offset;
    bool success;

    if (frame->format != AV_PIX_FMT_VAAPI_VLD)
        goto error_invalid_format;

    va_status = vaDeriveImage(vactx->display, va_surface, &va_image);
    if (!va_check_status(va_status, "vaDeriveImage()"))
        return false;

    if (!va_map_image(vactx->display, &va_image, &src_image))
        goto error_unsupported_image;

    data_offset = frame->data[0] - frame->data[3];
    crop_rect.x = data_offset % frame->linesize[0];
    crop_rect.y = data_offset / frame->linesize[0];
    crop_rect.width = frame->width;
    crop_rect.height = frame->height;
    if (!mvt_image_init_from_subimage(&dst_image, &src_image, &crop_rect))
        goto error_crop_image;

    if (!decoder->image || (decoder->image->width != dst_image.width ||
            decoder->image->height != dst_image.height)) {
        mvt_image_freep(&decoder->image);
        decoder->image = mvt_image_new(VIDEO_FORMAT_I420,
            dst_image.width, dst_image.height);
        if (!decoder->image)
            goto error_download_image;
    }
    if (!mvt_image_convert_full(decoder->image, &dst_image,
            MVT_IMAGE_FLAG_FROM_USWC))
        goto error_download_image;

    success = mvt_decoder_handle_image(MVT_DECODER(decoder), decoder->image, 0);
    mvt_image_clear(&dst_image);
    va_unmap_image(vactx->display, &va_image, &src_image);
    vaDestroyImage(vactx->display, va_image.image_id);
    return success;

    /* ERRORS */
error_invalid_format:
    mvt_error("invalid format (%s)", av_get_pix_fmt_name(frame->format));
    return false;
error_unsupported_image:
    mvt_error("unsupported surface format (%.4s)", &va_image.format.fourcc);
    vaDestroyImage(vactx->display, va_image.image_id);
    return false;
error_crop_image:
    mvt_error("failed to crop VA surface to (%d,%d):%ux%u",
        crop_rect.x, crop_rect.y, crop_rect.width, crop_rect.height);
    va_unmap_image(vactx->display, &va_image, &src_image);
    vaDestroyImage(vactx->display, va_image.image_id);
    return false;
error_download_image:
    mvt_error("failed to download VA surface 0x%08x to I420 image", va_surface);
    mvt_image_clear(&dst_image);
    va_unmap_image(vactx->display, &va_image, &src_image);
    vaDestroyImage(vactx->display, va_image.image_id);
    return false;
}

static const HWAccelDecoder hwaccel_vaapi = {
    .init = (MvtDecoderInitFunc)decoder_vaapi_init,
    .finalize = (MvtDecoderFinalizeFunc)decoder_vaapi_finalize,
    .init_context = (MvtDecoderFFmpegInitContextFunc)decoder_vaapi_init_context,
    .handle_frame = (MvtDecoderFFmpegHandleFrameFunc)decoder_vaapi_handle_frame,
};

/* ------------------------------------------------------------------------ */
/* --- Software Decoder                                                 --- */
/* ------------------------------------------------------------------------ */

static bool
decoder_init_context(Decoder *decoder)
{
    const HWAccelDecoder * const hwaccel = decoder->hwaccel;
    AVCodecContext * const avctx = MVT_DECODER_FFMPEG(decoder)->avctx;

    if (hwaccel->init_context)
        return hwaccel->init_context(MVT_DECODER(decoder));

    /* Always enable bitexact output mode */
    avctx->flags |= CODEC_FLAG_BITEXACT;

    /* Also try to enable "simple" IDCT transforms. Though, only for a
       known set of video codecs where this is known to be useful */
    switch (avctx->codec_id) {
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
    case AV_CODEC_ID_MPEG4:
    case AV_CODEC_ID_MJPEG:
    case AV_CODEC_ID_H263:
        if (avctx->idct_algo == FF_IDCT_AUTO)
            avctx->idct_algo = FF_IDCT_SIMPLE;
        break;
    default:
        break;
    }
    return true;
}

static bool
decoder_init(Decoder *decoder)
{
    const MvtDecoderOptions * const options = &MVT_DECODER(decoder)->options;
    static const HWAccelDecoder hwaccel_none;

    switch (options->hwaccel) {
    case MVT_HWACCEL_VAAPI:
        decoder->hwaccel = &hwaccel_vaapi;
        break;
    default:
        decoder->hwaccel = &hwaccel_none;
        break;
    }
    if (decoder->hwaccel->init && !decoder->hwaccel->init(MVT_DECODER(decoder)))
        return false;
    return true;
}

static void
decoder_finalize(Decoder *decoder)
{
    const HWAccelDecoder * const hwaccel = decoder->hwaccel;

    if (hwaccel && hwaccel->finalize)
        hwaccel->finalize(MVT_DECODER(decoder));
    mvt_display_freep(&decoder->display);
    mvt_image_freep(&decoder->image);
}

static bool
decoder_handle_frame(Decoder *decoder, AVFrame *frame)
{
    const HWAccelDecoder * const hwaccel = decoder->hwaccel;

    if (hwaccel->handle_frame)
        return hwaccel->handle_frame(MVT_DECODER(decoder), frame);
    return mvt_decoder_ffmpeg_handle_frame(&decoder->base, frame);
}

const MvtDecoderClass *
mvt_decoder_class(void)
{
    static MvtDecoderFFmpegClass g_class = {
        .init = (MvtDecoderInitFunc)decoder_init,
        .finalize = (MvtDecoderFinalizeFunc)decoder_finalize,
        .init_context = (MvtDecoderFFmpegInitContextFunc)decoder_init_context,
        .handle_frame = (MvtDecoderFFmpegHandleFrameFunc)decoder_handle_frame,
    };
    if (!g_class.base.size)
        mvt_decoder_ffmpeg_class_init(&g_class, sizeof(Decoder));
    return &g_class.base;
}
