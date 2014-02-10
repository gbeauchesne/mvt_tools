/*
 * mvt_image.h - Image utilities
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

#ifndef MVT_IMAGE_H
#define MVT_IMAGE_H

#include <va/va.h>
#include "video_format.h"
#include "mvt_hash.h"

MVT_BEGIN_DECLS

struct MvtImagePrivate_s;
typedef struct MvtImagePrivate_s MvtImagePrivate;

typedef struct {
    VideoFormat         format;         ///< Video format
    uint32_t            width;          ///< MvtImage width, in pixels
    uint32_t            height;         ///< MvtImage height, in pixels
    /**
     * \brief MvtImage allocated data to hold the pixels.
     *
     * This field is only valid if this is called with mvt_image_new(). An
     * MvtImage object wrapping an existing one shall set \ref data to \c
     * NULL and fill in the \ref pixels array.
     */
    uint8_t *           data;
    uint32_t            data_size;      ///< Size of the image data
    uint32_t            num_planes;     ///< Number of planes
    uint8_t *           pixels[VIDEO_FORMAT_MAX_PLANES];
    uint32_t            offsets[VIDEO_FORMAT_MAX_PLANES];
    uint32_t            pitches[VIDEO_FORMAT_MAX_PLANES];
    MvtImagePrivate *   priv;           ///< Private image data
} MvtImage;

enum {
    /** Image is in Uncacheable Speculative Write Combining (USWC) memory */
    MVT_IMAGE_FLAG_FROM_USWC = 1 << 31,
};

/** Creates a new MvtImage object and allocates data for it */
MvtImage *
mvt_image_new(VideoFormat format, uint32_t width, uint32_t height);

/** Deallocates MvtImage object and any associated data from it */
void
mvt_image_free(MvtImage *image);

/** Frees the MvtImage object and reset the supplied pointer to NULL */
void
mvt_image_freep(MvtImage **image_ptr);

/** Initializes MvtImage object with the specified format and size */
bool
mvt_image_init(MvtImage *image, VideoFormat format, uint32_t width,
    uint32_t height);

/***
 * \brief Initializes MvtImage object from the supplied MvtImage and subregion.
 *
 * Initializes the \ref dst_image object with the supplied \ref
 * src_image and \rect region. This assumes the parent MvtImage object
 * \ref src_image will still be live until \ref dst_image is no longer
 * needed. i.e. there is no copy of the underlying data, just a
 * different representation.
 *
 * Note: it is possible for this function to fail if the supplied \ref
 * src_image won't fit the \ref rect constraints in terms of chroma
 * components for YUV images.
 */
bool
mvt_image_init_from_subimage(MvtImage *dst_image, MvtImage *src_image,
    const VARectangle *rect);

/** Initializes MvtImage object from the supplied image and selected field */
bool
mvt_image_init_from_field(MvtImage *dst_image, MvtImage *src_image,
    uint32_t field);

/** Initializes MvtImage object from a VA image object */
bool
mvt_image_init_from_va_image(MvtImage *image, const VAImage *va_image);

/** Initializes pixels from data and offsets */
bool
mvt_image_init_pixels(MvtImage *image);

/** Clears MvtImage object, thus deallocating any possible data from it */
void
mvt_image_clear(MvtImage *image);

/** Converts images with the same size */
bool
mvt_image_convert(MvtImage *image, MvtImage *src_image);

/** Converts images with the same size, while allowing interleaving */
bool
mvt_image_convert_full(MvtImage *image, MvtImage *src_image, uint32_t flags);

/** Computes the checksum from the supplied MvtImage object and hash function */
bool
mvt_image_hash(MvtImage *image, MvtHash *hash);

MVT_END_DECLS

#endif /* MVT_IMAGE_H */
