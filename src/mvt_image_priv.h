/*
 * mvt_image_priv.h - Image utilities (private)
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

#ifndef MVT_IMAGE_PRIV_H
#define MVT_IMAGE_PRIV_H

// Private image data
struct MvtImagePrivate_s {
    uint8_t *           data_base;      ///< Base memory buffer (allocated)
    uint8_t *           copy_cache;     ///< Cache buffer used for image copies
    uint32_t            copy_cache_size; ///< Size of the cache buffer
};

// Ensures private image data is allocated
DLL_HIDDEN
MvtImagePrivate *
mvt_image_priv_ensure(MvtImage *image);

// Round up the supplied value. Alignment must be a power of two
static inline uint32_t
round_up(uint32_t v, uint32_t a)
{
    return (v + a - 1) & ~(a - 1);
}

// Get pointer to component at the specified coordinates
static inline void *
get_component_ptr(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y)
{
    return &image->pixels[cip->plane][y * image->pitches[cip->plane] +
        x * cip->pixel_stride + cip->pixel_offset];
}

// Get 8-bit component at the specified coordinates
static inline const uint8_t
get_component8(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y)
{
    return *((uint8_t *)get_component_ptr(image, cip, x, y));
}

// Put 8-bit component to the specified coordinates
static inline void
put_component8(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y, uint8_t v)
{
    *((uint8_t *)get_component_ptr(image, cip, x, y)) = v;
}

// Get 16-bit component at the specified coordinates
static inline const uint16_t
get_component16(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y)
{
    const uint16_t * const p = (uint16_t *)get_component_ptr(image, cip, x, y);

    return *p & ((1U << cip->bit_depth) - 1);
}

// Put 16-bit component to the specified coordinates
static inline void
put_component16(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y, uint16_t v)
{
    uint16_t * const p = get_component_ptr(image, cip, x, y);

    *p = v & ((1U << cip->bit_depth) - 1);
}

// Get component value at the specified coordinates
static inline const uint32_t
get_component(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y)
{
    return (MVT_LIKELY(cip->bit_depth <= 8) ?
        get_component8(image, cip, x, y) : get_component16(image, cip, x, y));
}

// Put component value to the specified coordinates
static inline void
put_component(MvtImage *image, const VideoFormatComponentInfo *cip,
    int x, int y, uint32_t v)
{
    if (MVT_LIKELY(cip->bit_depth <= 8))
        put_component8(image, cip, x, y, v);
    else
        put_component16(image, cip, x, y, v);
}

// Gets RGB pixel at the specified coordinates
static inline void
get_rgb_pixel(MvtImage *image, const VideoFormatInfo *vip, int x, int y,
    uint8_t *R_ptr, uint8_t *G_ptr, uint8_t *B_ptr)
{
    *R_ptr = get_component8(image, &vip->components[0], x, y);
    *G_ptr = get_component8(image, &vip->components[1], x, y);
    *B_ptr = get_component8(image, &vip->components[2], x, y);
}

// Puts RGB pixel at the specified coordinates
static inline void
put_rgb_pixel(MvtImage *image, const VideoFormatInfo *vip, int x, int y,
    uint8_t R, uint8_t G, uint8_t B)
{
    put_component8(image, &vip->components[0], x, y, R);
    put_component8(image, &vip->components[1], x, y, G);
    put_component8(image, &vip->components[2], x, y, B);
}

#endif /* MVT_IMAGE_PRIV_H */
