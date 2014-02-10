/*
 * mvt_decoder_ffmpeg.h - Decoder module (FFmpeg)
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

#ifndef MVT_DECODER_FFMPEG_H
#define MVT_DECODER_FFMPEG_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "mvt_decoder.h"

#define MVT_DECODER_FFMPEG(dec) \
    ((MvtDecoderFFmpeg *)(dec))
#define MVT_DECODER_FFMPEG_CLASS(klass) \
    ((MvtDecoderFFmpegClass *)(klass))
#define MVT_DECODER_FFMPEG_GET_CLASS(dec) \
    MVT_DECODER_FFMPEG_CLASS(MVT_DECODER_GET_CLASS(dec))

typedef bool (*MvtDecoderFFmpegInitContextFunc)(MvtDecoder *dec);

/** FFmpeg decoder object */
typedef struct {
    MvtDecoder base;
    AVFormatContext *fmtctx;
    AVStream *stream;
    AVCodecContext *avctx;
    AVFrame *frame;
} MvtDecoderFFmpeg;

/** FFmpeg decoder class */
typedef struct {
    MvtDecoderClass base;
    MvtDecoderInitFunc init;
    MvtDecoderFinalizeFunc finalize;
    MvtDecoderFFmpegInitContextFunc init_context;
} MvtDecoderFFmpegClass;

void
mvt_decoder_ffmpeg_class_init(MvtDecoderFFmpegClass *klass, uint32_t size);

bool
mvt_decoder_ffmpeg_handle_frame(MvtDecoderFFmpeg *dec, AVFrame *frame);

#endif /* MVT_DECODER_FFMPEG_H */
