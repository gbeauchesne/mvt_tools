/*
 * mvt_image_hash.c - Image utilities (hash)
 *
 * Copyright (C) 2013-2014 Intel Corporation
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
#include <wchar.h>
#include "mvt_image.h"
#include "mvt_image_priv.h"
#include "mvt_memory.h"

// Updates the checksum for the specified component
static void
mvt_image_hash_component(MvtImage *image, MvtHash *hash,
    const VideoFormatInfo *vip, uint32_t component)
{
    const VideoFormatComponentInfo * const cip = &vip->components[component];
    const uint8_t *p;
    uint32_t x, y, w, h, stride, bpc;

    w = image->width;
    h = image->height;
    if (component != 0) {
        w = (w + (1U << vip->chroma_w_shift) - 1) >> vip->chroma_w_shift;
        h = (h + (1U << vip->chroma_h_shift) - 1) >> vip->chroma_h_shift;
    }
    stride = image->pitches[cip->plane];

    bpc = (cip->bit_depth + 7) / 8; // bytes per component

    p = get_component_ptr(image, cip, 0, 0);
    if (cip->pixel_stride == bpc) {
        for (y = 0; y < h; y++) {
            mvt_hash_update(hash, p, w * bpc);
            p += stride;
        }
    }
    else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++)
                mvt_hash_update(hash, p + x * cip->pixel_stride, bpc);
            p += stride;
        }
    }
}

// Hashes grayscale images as if they were 4:2:0 with 0.0 chroma
static bool
mvt_image_hash_grayscale(MvtImage *image, MvtHash *hash,
    const VideoFormatInfo *vip)
{
    MvtImagePrivate * const priv = mvt_image_priv_ensure(image);
    const VideoFormatComponentInfo * const cip = &vip->components[0];
    uint32_t w, n, bpc, stride;
    wchar_t c;

    if (!priv)
        return false;

    w = (image->width + 1) / 2;
    bpc = (cip->bit_depth + 7) / 8;
    stride = round_up(w * bpc, sizeof(wchar_t));

    if (!priv->hash_data || priv->hash_data_size < stride) {
        if (!mem_reallocp(&priv->hash_data, &priv->hash_data_size, stride))
            return false;

        /* In MVT, high bit depth components are always stored in
           native endian byte order */
#define INIT_CHROMA(bpc, type)                          \
        case bpc: {                                     \
            type * const p = (type *)&c;                \
            int i;                                      \
            for (i = 0; i < sizeof(c) / bpc; i++)       \
                p[i] = 1U << (cip->bit_depth - 1);      \
            break;                                      \
        }

        switch (bpc) {
            INIT_CHROMA(1, uint8_t);
            INIT_CHROMA(2, uint16_t);
            INIT_CHROMA(4, uint32_t);
        default: return false;
        }
#undef INIT_CHROMA
        wmemset((wchar_t *)priv->hash_data, c, stride / sizeof(c));
    }

    mvt_hash_init(hash);
    mvt_image_hash_component(image, hash, vip, 0);
    for (n = 2 * ((image->height + 1) / 2); n > 0; n--)
        mvt_hash_update(hash, priv->hash_data, w * bpc);
    mvt_hash_finalize(hash);
    return true;
}

// Computes the checksum from the supplied MvtImage object and hash function
bool
mvt_image_hash(MvtImage *image, MvtHash *hash)
{
    const VideoFormatInfo *vip;

    if (!image || !hash)
        return false;

    vip = video_format_get_info(image->format);
    if (!vip || !video_format_is_yuv(image->format))
        goto error_unsupported_format;

    if (MVT_UNLIKELY(vip->num_components == 1))
        return mvt_image_hash_grayscale(image, hash, vip);

    mvt_hash_init(hash);
    mvt_image_hash_component(image, hash, vip, 0);
    mvt_image_hash_component(image, hash, vip, 1);
    mvt_image_hash_component(image, hash, vip, 2);
    mvt_hash_finalize(hash);
    return true;

    /* ERRORS */
error_unsupported_format:
    mvt_error("unsupported image format (%s)",
              video_format_get_name(image->format));
    return false;
}
