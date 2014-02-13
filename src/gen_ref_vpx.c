/*
 * gen_ref_vpx.c - Reference VPx (libvpx) decoder
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

#define VPX_CODEC_DISABLE_COMPAT 1
#include "sysdeps.h"
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "mvt_decoder_ffmpeg.h"
#include "ffmpeg_compat.h"

typedef struct {
    MvtDecoderFFmpeg base;
    struct vpx_codec_dec_cfg config;
    struct vpx_codec_ctx decoder;
    bool is_open;
} Decoder;

// Translates VPX image format to FFmpeg pixel format
static enum AVPixelFormat
ff_get_pix_fmt(vpx_img_fmt_t fmt)
{
    enum AVPixelFormat pix_fmt;

    switch (fmt) {
    case VPX_IMG_FMT_UYVY:
        pix_fmt = AV_PIX_FMT_UYVY422;
        break;
    case VPX_IMG_FMT_YUY2:
        pix_fmt = AV_PIX_FMT_YUYV422;
        break;
    case VPX_IMG_FMT_I420:
        pix_fmt = AV_PIX_FMT_YUV420P;
        break;
    default:
        pix_fmt = AV_PIX_FMT_NONE;
        break;
    }
    return pix_fmt;
}

// Initializes VPX decoder
static int
vpx_init(AVCodecContext *avctx, const struct vpx_codec_iface *iface)
{
    Decoder * const decoder = avctx->opaque;
    vpx_codec_err_t err;

    memset(&decoder->config, 0, sizeof(decoder->config));
    decoder->config.threads = MVT_MIN(avctx->thread_count, 16);

    err = vpx_codec_dec_init(&decoder->decoder, iface, &decoder->config, 0);
    if (err != VPX_CODEC_OK)
        goto error_init_decoder;
    decoder->is_open = true;

    avctx->pix_fmt = AV_PIX_FMT_YUV420P;
    return 0;

    /* ERRORS */
error_init_decoder:
    mvt_error("failed to initialize decoder: %s",
        vpx_codec_error(&decoder->decoder));
    return AVERROR(EINVAL);
}

// Releases VPX decoder resources
static int
vpx_finalize(AVCodecContext *avctx)
{
    Decoder * const decoder = avctx->opaque;

    if (decoder->is_open)
        vpx_codec_destroy(&decoder->decoder);
    return 0;
}

// Decodes any frame that was parsed by FFmpeg
static int
vpx_decode(AVCodecContext *avctx, void *data, int *got_frame, AVPacket *pkt)
{
    Decoder * const decoder = avctx->opaque;
    AVFrame * const frame = data;
    const void *iter;
    struct vpx_image *img;
    vpx_codec_err_t err;
    int i;

    err = vpx_codec_decode(&decoder->decoder, pkt->data, pkt->size, NULL, 0);
    if (err != VPX_CODEC_OK)
        goto error_decode;

    iter = NULL;
    img = vpx_codec_get_frame(&decoder->decoder, &iter);
    if (img) {
        frame->format = ff_get_pix_fmt(img->fmt);
        if (frame->format == AV_PIX_FMT_NONE || frame->format != avctx->pix_fmt)
            goto error_unsupported_format;

        frame->width = img->d_w;
        frame->height = img->d_h;
        if (frame->width != avctx->width || frame->height != avctx->height) {
            avcodec_set_dimensions(avctx, frame->width, frame->height);
            if (frame->width != avctx->width || frame->height != avctx->height)
                goto error_size_change;
        }

        frame->interlaced_frame = 0;
        for (i = 0; i < 4; i++) {
            frame->data[i] = img->planes[i];
            frame->linesize[i] = img->stride[i];
        }
        *got_frame = 1;
    }
    return pkt->size;

    /* ERRORS */
error_decode:
    mvt_error("failed to decode frame: %s", vpx_codec_error(&decoder->decoder));
    return AVERROR_INVALIDDATA;
error_unsupported_format:
    mvt_error("unsupported image format (%d)", img->fmt);
    return AVERROR_INVALIDDATA;
error_size_change:
    mvt_error("failed to change dimensions (%dx%d)",
        frame->width, frame->height);
    return AVERROR_INVALIDDATA;
}

static int
vp8_init(AVCodecContext *avctx)
{
    return vpx_init(avctx, &vpx_codec_vp8_dx_algo);
}

static AVCodec libvpx_vp8_decoder = {
    .name               = "libvpx-vp8",
    .long_name          = "libvpx VP8",
    .type               = AVMEDIA_TYPE_VIDEO,
    .id                 = AV_CODEC_ID_VP8,
    .init               = vp8_init,
    .close              = vpx_finalize,
    .decode             = vpx_decode,
    .capabilities       = CODEC_CAP_AUTO_THREADS | CODEC_CAP_DR1,
};

static AVCodec *
decoder_find_decoder(Decoder *decoder, int codec_id)
{
    AVCodec *codec;

    switch (codec_id) {
    case AV_CODEC_ID_VP8:
        codec = &libvpx_vp8_decoder;
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
        .find_decoder = (MvtDecoderFFmpegFindDecoderFunc)decoder_find_decoder,
    };
    if (!g_class.base.size)
        mvt_decoder_ffmpeg_class_init(&g_class, sizeof(Decoder));
    return &g_class.base;
}
