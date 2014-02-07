/*
 * va_image_utils.h - VA image utilities
 *
 * Copyright (C) 2013 Intel Corporation
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

#ifndef VA_IMAGE_UTILS_H
#define VA_IMAGE_UTILS_H

#include <va/va.h>
#include "mvt_image.h"

MVT_BEGIN_DECLS

/** Initializes image with safe default values */
void
va_image_init_defaults(VAImage *image);

/** Initializes VA image resources */
bool
va_image_init(VAImage *image, VADisplay dpy, const VAImageFormat *format,
    uint32_t width, uint32_t height);

/** Destroys VA image resources, if any */
void
va_image_clear(VAImage *image, VADisplay dpy);

/** Maps the VA image and fills in the corresponding MvtImage object */
bool
va_map_image(VADisplay dpy, VAImage *va_image, MvtImage *image);

/** Unmaps the VA image */
bool
va_unmap_image(VADisplay dpy, VAImage *va_image, MvtImage *image);

MVT_END_DECLS

#endif /* VA_IMAGE_UTILS_H */
