/*
 * mvt_image_convert.c - Image utilities (color conversion)
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
#include "mvt_image.h"
#include "mvt_image_priv.h"
#include <libyuv/convert.h>
#include <libyuv/convert_from.h>

/* -------------------------------------------------------------------------- */
/* --- libyuv based conversions                                           --- */
/* -------------------------------------------------------------------------- */

// Converts images with the same size
static bool
image_convert_internal(MvtImage *dst_image, const VideoFormatInfo *dst_vip,
    MvtImage *src_image, const VideoFormatInfo *src_vip, uint32_t flags)
{
    if (src_image->format == VIDEO_FORMAT_NV12 &&
        dst_image->format == VIDEO_FORMAT_I420)
        return NV12ToI420(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->pixels[2], dst_image->pitches[2],
            dst_image->width, dst_image->height) == 0;

    if (src_image->format == VIDEO_FORMAT_I420 &&
        dst_image->format == VIDEO_FORMAT_I420)
        return I420Copy(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            src_image->pixels[2], src_image->pitches[2],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->pixels[2], dst_image->pitches[2],
            dst_image->width, dst_image->height) == 0;

    if (src_image->format == VIDEO_FORMAT_I420 &&
        dst_image->format == VIDEO_FORMAT_NV12)
        return I420ToNV12(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            src_image->pixels[2], src_image->pitches[2],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->width, dst_image->height) == 0;

    mvt_error("image: unsupported conversion (%s -> %s)",
        src_vip->name, dst_vip->name);
    return false;
}

// Converts images with the same size
bool
mvt_image_convert(MvtImage *dst_image, MvtImage *src_image)
{
    return mvt_image_convert_full(dst_image, src_image, 0);
}

// Converts images with the same size, while allowing interleaving
bool
mvt_image_convert_full(MvtImage *dst_image, MvtImage *src_image, uint32_t flags)
{
    const VideoFormatInfo *src_vip, *dst_vip;
    MvtImage src_image_tmp, dst_image_tmp;
    uint32_t field_flags;

    if (!dst_image || !src_image)
        return false;

    if (dst_image->width  != src_image->width ||
        dst_image->height != src_image->height) {
        mvt_fatal_error("only images with the same size are allowed");
        return false;
    }

    src_vip = video_format_get_info(src_image->format);
    if (!src_vip)
        return false;

    dst_vip = video_format_get_info(dst_image->format);
    if (!dst_vip)
        return false;

    if (src_vip->chroma_type != dst_vip->chroma_type) {
        mvt_fatal_error("only images with the same chroma type are allowed");
        return false;
    }

    field_flags = flags & (VA_TOP_FIELD|VA_BOTTOM_FIELD);
    if (!field_flags)
        return image_convert_internal(dst_image, dst_vip, src_image, src_vip,
            flags);

    if (flags & VA_TOP_FIELD) {
        mvt_image_init_from_field(&src_image_tmp, src_image, VA_TOP_FIELD);
        mvt_image_init_from_field(&dst_image_tmp, dst_image, VA_TOP_FIELD);
        if (!image_convert_internal(&dst_image_tmp, dst_vip,
                &src_image_tmp, src_vip, flags))
            return false;
    }

    if (flags & VA_BOTTOM_FIELD) {
        mvt_image_init_from_field(&src_image_tmp, src_image, VA_BOTTOM_FIELD);
        mvt_image_init_from_field(&dst_image_tmp, dst_image, VA_BOTTOM_FIELD);
        if (!image_convert_internal(&dst_image_tmp, dst_vip,
                &src_image_tmp, src_vip, flags))
            return false;
    }
    return true;
}
