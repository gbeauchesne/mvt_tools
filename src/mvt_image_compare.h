/*
 * mvt_image_compare.h - Image comparison utilities
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

#ifndef MVT_IMAGE_COMPARE_H
#define MVT_IMAGE_COMPARE_H

#include "mvt_image.h"

MVT_BEGIN_DECLS

/** Image quality metrics */
typedef enum {
    /** Peak Signal to Noise Ratio */
    MVT_IMAGE_QUALITY_METRIC_PSNR = 1,
    /** Peak Signal to Noise Ratio (Y-channel only) */
    MVT_IMAGE_QUALITY_METRIC_Y_PSNR,
    /** Number of image quality metrics */
    MVT_IMAGE_QUALITY_METRIC_COUNT
} MvtImageQualityMetric;

/** Flags specific to the PSNR metric */
enum {
    /** Asses the Y-channel only */
    MVT_IMAGE_QUALITY_METRIC_FLAG_Y_PSNR        = 1 << 0,
};

/** Compares two images with the supplied quality metric */
bool
mvt_image_compare(MvtImage *image, MvtImage *ref_image,
    MvtImageQualityMetric metric, double *value_ptr);

/** Compares two images with the PSNR metric */
bool
mvt_image_compare_psnr(MvtImage *image, MvtImage *ref_image, uint32_t flags,
    double *psnr_ptr);

MVT_END_DECLS

#endif /* MVT_IMAGE_COMPARE_H */
