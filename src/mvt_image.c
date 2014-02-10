/*
 * mvt_image.c - Image utilities
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
#include "mvt_memory.h"

// Ensures private image data is allocated
MvtImagePrivate *
mvt_image_priv_ensure(MvtImage *image)
{
    if (!image)
        return NULL;

    if (MVT_UNLIKELY(!image->priv))
        image->priv = calloc(1, sizeof(MvtImagePrivate));
    return image->priv;
}

// Clears private image data
static void
mvt_image_priv_clear(MvtImage *image)
{
    MvtImagePrivate *priv;

    if (!image || !image->priv)
        return;

    priv = image->priv;
    mem_freep(&priv->copy_cache);
    mem_freep(&priv->data_base);
    mem_freep(&image->priv);
}

// Copies MvtImage objects, except the private data
static void
mvt_image_copy_struct(MvtImage *dst_image, MvtImage *src_image)
{
    *dst_image = *src_image;
    dst_image->priv = NULL;
}

// Creates a new MvtImage object and allocates data for it
MvtImage *
mvt_image_new(VideoFormat format, uint32_t width, uint32_t height)
{
    MvtImage *image;

    image = malloc(sizeof(*image));
    if (!image)
        return NULL;

    if (!mvt_image_init(image, format, width, height))
        goto error;

    if (!mvt_image_priv_ensure(image))
        goto error;

    image->priv->data_base = malloc(image->data_size);
    if (!image->priv->data_base)
        goto error;

    image->data = image->priv->data_base;
    if (!mvt_image_init_pixels(image))
        goto error;
    return image;

error:
    mvt_image_free(image);
    return NULL;
}

// Deallocates MvtImage object and any associated data from it
void
mvt_image_free(MvtImage *image)
{
    if (!image)
        return;

    mvt_image_clear(image);
    free(image);
}

// Frees the MvtImage object and reset the supplied pointer to NULL
void
mvt_image_freep(MvtImage **image_ptr)
{
    if (image_ptr) {
        mvt_image_free(*image_ptr);
        *image_ptr = NULL;
    }
}

// Initializes MvtImage object with the specified format and size
bool
mvt_image_init(MvtImage *image, VideoFormat format, uint32_t width,
    uint32_t height)
{
    const VideoFormatInfo *vip;
    uint32_t i, awidth, aheight, heights[VIDEO_FORMAT_MAX_PLANES] = { 0, };

    if (!image || width == 0 || height == 0)
        return false;

    vip = video_format_get_info(format);
    if (!vip)
        return false;

    memset(image, 0, sizeof(*image));
    image->num_planes = vip->num_planes;

    awidth  = round_up(width, 16);
    aheight = round_up(height, 16);
    for (i = 0; i < vip->num_components; i++) {
        const VideoFormatComponentInfo * const cip = &vip->components[i];

        const uint32_t pitch = (cip->pixel_stride * awidth) >>
            (i > 0 ? vip->chroma_w_shift : 0);
        if (image->pitches[cip->plane] && image->pitches[cip->plane] != pitch)
            return false;
        image->pitches[cip->plane] = pitch;

        const uint32_t h = aheight >> (i > 0 ? vip->chroma_h_shift : 0);
        if (heights[cip->plane] && heights[cip->plane] != h)
            return false;
        heights[cip->plane] = h;
    }
    if (!heights[0])
        return false;

    image->offsets[0] = 0;
    if (!image->pitches[0])
        return false;

    for (i = 1; i < vip->num_planes; i++) {
        image->offsets[i] = image->offsets[i - 1] +
            heights[i - 1] * image->pitches[i - 1];
        if (!image->pitches[i])
            return false;
    }

    image->data = NULL;
    image->data_size = image->offsets[i - 1] +
        heights[i - 1] * image->pitches[i - 1];

    image->format = format;
    image->width  = width;
    image->height = height;
    return true;
}

// Initializes MvtImage object from the supplied MvtImage and subregion
bool
mvt_image_init_from_subimage(MvtImage *dst_image, MvtImage *src_image,
    const VARectangle *rect)
{
    const VideoFormatInfo *vip;
    VARectangle rect_tmp;
    uint32_t i, ofs, pixel_pitches[VIDEO_FORMAT_MAX_PLANES];

    if (!dst_image || !src_image)
        return false;

    vip = video_format_get_info(src_image->format);
    if (!vip)
        return false;

    if (!rect) {
        rect_tmp.x = 0;
        rect_tmp.y = 0;
        rect_tmp.width = src_image->width;
        rect_tmp.height = src_image->height;
        rect = &rect_tmp;
    }

    mvt_image_copy_struct(dst_image, src_image);
    dst_image->width = rect->width;
    dst_image->height = rect->height;
    dst_image->data = NULL; // the parent image owns the underlying data

    // Validate chroma coords
    if (vip->num_components > 1 && video_format_is_yuv(src_image->format)) {
        const uint32_t mx = (1 << vip->chroma_w_shift) - 1;
        const uint32_t my = (1 << vip->chroma_h_shift) - 1;
        if ((rect->x & mx) || (rect->y & my))
            return false;
    }

    if (!video_format_get_pixel_pitches(src_image->format, pixel_pitches))
        return false;

    ofs = rect->y * src_image->pitches[0] + rect->x * pixel_pitches[0];
    dst_image->pixels[0] += ofs;
    dst_image->offsets[0] += ofs;
    for (i = 1; i < vip->num_planes; i++) {
        ofs = (rect->y * src_image->pitches[i] >> vip->chroma_h_shift) +
            (rect->x * pixel_pitches[i] >> vip->chroma_w_shift);
        dst_image->pixels[i] += ofs;
        dst_image->offsets[i] += ofs;
    }
    return true;
}

// Initializes MvtImage object from the supplied MvtImage and field selection
bool
mvt_image_init_from_field(MvtImage *dst_image, MvtImage *src_image,
    uint32_t field)
{
    const VideoFormatInfo *vip;
    uint32_t i;

    if (!dst_image || !src_image)
        return false;

    vip = video_format_get_info(src_image->format);
    if (!vip)
        return false;

    mvt_image_copy_struct(dst_image, src_image);
    switch (field & (VA_TOP_FIELD|VA_BOTTOM_FIELD)) {
    case VA_BOTTOM_FIELD:
        for (i = 0; i < vip->num_planes; i++)
            dst_image->pixels[i] += src_image->pitches[i];
        // fall-through
    case VA_TOP_FIELD:
        for (i = 0; i < vip->num_planes; i++)
            dst_image->pitches[i] <<= 1;
        break;
    }
    dst_image->height >>= 1;
    dst_image->data = NULL; // the parent image owns the underlying data
    return true;
}

// Initializes object from a VA image object
bool
mvt_image_init_from_va_image(MvtImage *image, const VAImage *va_image)
{
    VideoFormat format;
    uint32_t i;

    if (!image || !va_image)
        return false;

    format = video_format_from_va_format(&va_image->format);
    if (!format)
        return false;

    image->format       = format;
    image->width        = va_image->width;
    image->height       = va_image->height;
    image->data         = NULL; /* initialized in mvt_image_init_pixels() */
    image->data_size    = va_image->data_size;
    image->num_planes   = va_image->num_planes;
    memset(image->pixels, 0, sizeof(image->pixels));
    image->priv         = NULL;

    for (i = 0; i < image->num_planes; i++) {
        image->offsets[i] = va_image->offsets[i];
        image->pitches[i] = va_image->pitches[i];
    }
    for (; i < VIDEO_FORMAT_MAX_PLANES; i++) {
        image->offsets[i] = 0;
        image->pitches[i] = 0;
    }
    return true;
}

// Initializes pixels from data and offsets
bool
mvt_image_init_pixels(MvtImage *image)
{
    uint32_t i;

    if (!image || !image->data)
        return false;

    for (i = 0; i < image->num_planes; i++)
        image->pixels[i] = image->data + image->offsets[i];
    return true;
}

// Clears MvtImage object, thus deallocating any possible data from it
void
mvt_image_clear(MvtImage *image)
{
    uint32_t i;

    if (!image)
        return;

    image->data = NULL;
    image->data_size = 0;

    image->format = VIDEO_FORMAT_UNKNOWN;
    image->width  = 0;
    image->height = 0;

    for (i = 0; i < image->num_planes; i++) {
        image->pixels[i] = NULL;
        image->offsets[i] = 0;
        image->pitches[i] = 0;
    }
    image->num_planes = 0;
    mvt_image_priv_clear(image);
}
