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
#include "mvt_decoder_ffmpeg.h"

typedef struct {
    MvtDecoderFFmpeg base;
} Decoder;

/* ------------------------------------------------------------------------ */
/* --- Software Decoder                                                 --- */
/* ------------------------------------------------------------------------ */

static bool
decoder_init(Decoder *decoder)
{
    return true;
}

static void
decoder_finalize(Decoder *decoder)
{
}

const MvtDecoderClass *
mvt_decoder_class(void)
{
    static MvtDecoderFFmpegClass g_class = {
        .init = (MvtDecoderInitFunc)decoder_init,
        .finalize = (MvtDecoderFinalizeFunc)decoder_finalize,
    };
    if (!g_class.base.size)
        mvt_decoder_ffmpeg_class_init(&g_class, sizeof(Decoder));
    return &g_class.base;
}
