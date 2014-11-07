/*
 * mvt_image_compare.c - Image comparison utilities
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
#include <math.h>
#include "mvt_image.h"
#include "mvt_image_priv.h"
#include "mvt_image_compare.h"

typedef bool (*MvtImageCompareFunc)(MvtImage *image, MvtImage *ref_image,
    uint32_t flags, double *val);

// Compares two images with the supplied quality metric
bool
mvt_image_compare(MvtImage *image, MvtImage *ref_image,
    MvtImageQualityMetric metric, double *value_ptr)
{
    const VideoFormatInfo *vip, *ref_vip;
    MvtImageCompareFunc image_compare_func;
    uint32_t flags;

    if (!image || !ref_image)
        return false;
    if (image->width != ref_image->width || image->height != ref_image->height)
        return false;

    vip = video_format_get_info(image->format);
    ref_vip = video_format_get_info(ref_image->format);
    if (vip->chroma_type != ref_vip->chroma_type)
        return false;

    if (!value_ptr)
        return false;

    flags = 0;
    switch (metric) {
    case MVT_IMAGE_QUALITY_METRIC_Y_PSNR:
        flags |= MVT_IMAGE_QUALITY_METRIC_FLAG_Y_PSNR;
        // fall-through
    case MVT_IMAGE_QUALITY_METRIC_PSNR:
        image_compare_func = mvt_image_compare_psnr;
        break;
    default:
        assert(0 && "unsupported image quality metric");
        return false;
    }
    return image_compare_func(image, ref_image, flags, value_ptr);
}

// Computes the squared error
static inline uint32_t
calc_se(uint32_t val, uint32_t ref)
{
    const int32_t diff = (int32_t)val - (int32_t)ref;
    return diff * diff;
}

// Computes the PSNR
static inline double
calc_psnr(uint64_t se, uint32_t num_samples, uint32_t max_intensity)
{
    return se > 0 ? (20.0 * log10(max_intensity) -
        10.0 * log10((double)se / num_samples)) : INFINITY;
}

// Compares two images with the PSNR metric
bool
mvt_image_compare_psnr(MvtImage *image, MvtImage *ref_image, uint32_t flags,
    double *psnr_ptr)
{
    const VideoFormatInfo * const vip =
        video_format_get_info(image->format);
    const VideoFormatInfo * const ref_vip =
        video_format_get_info(ref_image->format);
    uint32_t max_intensity, bit_depth = 0, ref_bit_depth = 0;
    uint32_t i, j, w, h, n, num_components, num_samples = 0;
    uint64_t se = 0;

    if (vip->chroma_w_shift != ref_vip->chroma_w_shift ||
        vip->chroma_h_shift != ref_vip->chroma_h_shift)
        return false;

    num_components = MVT_MIN(vip->num_components, ref_vip->num_components);

    // Limit comparison range for Y-PSNR
    if (flags & MVT_IMAGE_QUALITY_METRIC_FLAG_Y_PSNR) {
        if (!video_format_is_yuv(image->format))
            return false;
        num_components = 1;
    }

    // Check component bit depths are compatible
    for (n = 0; n < num_components; n++) {
        const VideoFormatComponentInfo * const cip =
            &vip->components[n];
        const VideoFormatComponentInfo * const ref_cip =
            &ref_vip->components[n];

        if (MVT_UNLIKELY(!bit_depth))
            bit_depth = cip->bit_depth;
        else if (bit_depth != cip->bit_depth)
            return false;

        if (MVT_UNLIKELY(!ref_bit_depth))
            ref_bit_depth = ref_cip->bit_depth;
        else if (ref_bit_depth != ref_cip->bit_depth)
            return false;
    }
    if (bit_depth != ref_bit_depth)
        return false;
    max_intensity = (1U << bit_depth) - 1;

    // Compare main components
    for (n = 0; n < num_components; n++) {
        const VideoFormatComponentInfo * const cip =
            &vip->components[n];
        const VideoFormatComponentInfo * const ref_cip =
            &ref_vip->components[n];

        w = image->width;
        h = image->height;
        if (n > 0) {
            w = (w + (1U << vip->chroma_w_shift) - 1) >> vip->chroma_w_shift;
            h = (h + (1U << vip->chroma_h_shift) - 1) >> vip->chroma_h_shift;
        }

        for (j = 0; j < h; j++) {
            for (i = 0; i < w; i++)
                se += calc_se(get_component(image, cip, i, j),
                    get_component(ref_image, ref_cip, i, j));
        }
        num_samples += w * h;
    }

    // Compare alpha components
    if (video_format_has_alpha(image->format) && num_components > 1) {
        MvtImage *a_image;
        const VideoFormatInfo *a_vip;

        if (vip->num_components == 3 && ref_vip->num_components == 4)
            a_image = ref_image, a_vip = ref_vip;
        else if (vip->num_components == 4 && ref_vip->num_components == 3)
            a_image = image, a_vip = vip;
        else if (vip->num_components == ref_vip->num_components &&
                 vip->num_components == num_components)
            a_image = NULL;
        else
            return false;
        if (a_image) {
            const VideoFormatComponentInfo * const cip = &a_vip->components[3];
            for (j = 0; j < a_image->height; j++) {
                for (i = 0; i < a_image->width; i++)
                    se += calc_se(get_component(a_image, cip, i, j),
                        max_intensity);
            }
            num_samples += a_image->width * a_image->height;
        }
    }

    *psnr_ptr = calc_psnr(se, num_samples, max_intensity);
    return true;
}
