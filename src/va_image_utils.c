/*
 * va_image_utils.c - VA image utilities
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

#include "sysdeps.h"
#include "va_utils.h"
#include "va_image_utils.h"

static inline void
va_image_init_defaults_unchecked(VAImage *image)
{
    image->image_id = VA_INVALID_ID;
    image->buf = VA_INVALID_ID;
}

// Initializes image with safe default values
void
va_image_init_defaults(VAImage *image)
{
    if (image)
        va_image_init_defaults_unchecked(image);
}

// Initializes VA image resources
bool
va_image_init(VAImage *image, VADisplay dpy, const VAImageFormat *format,
    uint32_t width, uint32_t height)
{
    VAStatus va_status;

    if (!image || !format)
        return false;

    va_status = vaCreateImage(dpy, (VAImageFormat *)format, width, height,
        image);
    if (!va_check_status(va_status, "vaCreateImage()"))
        goto error;
    return true;

error:
    va_image_init_defaults_unchecked(image);
    return false;
}

// Destroys VA image resources, if any
void
va_image_clear(VAImage *image, VADisplay dpy)
{
    if (!image)
        return;

    if (image->image_id != VA_INVALID_ID)
        vaDestroyImage(dpy, image->image_id);
    va_image_init_defaults_unchecked(image);
}

// Maps the VA image and fills in the corresponding MvtImage object
bool
va_map_image(VADisplay dpy, VAImage *va_image, MvtImage *image)
{
    if (!va_image || !image)
        return false;

    if (!mvt_image_init_from_va_image(image, va_image))
        return false;

    image->data = va_map_buffer(dpy, va_image->buf);
    if (!image->data)
        return false;

    if (!mvt_image_init_pixels(image))
        goto error_init_pixels;
    return true;

    /* ERRORS */
error_init_pixels:
    va_unmap_buffer(dpy, va_image->buf, (void **)&image->data);
    mvt_image_clear(image);
    return false;
}

// Unmaps the VA image
bool
va_unmap_image(VADisplay dpy, VAImage *va_image, MvtImage *image)
{
    if (!va_image || !image)
        return false;

    if (image->data)
        va_unmap_buffer(dpy, va_image->buf, (void **)&image->data);
    mvt_image_clear(image);
    return true;
}
