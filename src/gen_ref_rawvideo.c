/*
 * gen_ref_rawvideo.c - Raw video decoder
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
#include "mvt_decoder.h"
#include "mvt_image_file.h"

typedef struct {
    MvtDecoder base;
    MvtImageFile *input_file;
    MvtImageInfo input_info;
    MvtImage *image;
} Decoder;

static bool
decoder_init(Decoder *decoder)
{
    const MvtDecoderOptions * const options = &decoder->base.options;

    decoder->input_file = mvt_image_file_open(options->filename,
        MVT_IMAGE_FILE_MODE_READ);
    if (!decoder->input_file)
        return false;
    if (!mvt_image_file_read_headers(decoder->input_file, &decoder->input_info))
        return false;

    decoder->image = mvt_image_new(decoder->input_info.format,
        decoder->input_info.width, decoder->input_info.height);
    if (!decoder->image)
        return false;

    decoder->base.output_info = decoder->input_info;
    return true;
}

static void
decoder_finalize(Decoder *decoder)
{
    if (decoder->input_file) {
        mvt_image_file_close(decoder->input_file);
        decoder->input_file = NULL;
    }
    mvt_image_freep(&decoder->image);
}

static bool
decoder_run(Decoder *decoder)
{
    while (mvt_image_file_read_image(decoder->input_file, decoder->image)) {
        if (!mvt_decoder_handle_image(&decoder->base, decoder->image, 0))
            return false;
    }
    return true;
}

const MvtDecoderClass *
mvt_decoder_class(void)
{
    static MvtDecoderClass g_class = {
        .size = sizeof(Decoder),
        .init = (MvtDecoderInitFunc)decoder_init,
        .finalize = (MvtDecoderFinalizeFunc)decoder_finalize,
        .run = (MvtDecoderRunFunc)decoder_run,
    };
    return &g_class;
}
