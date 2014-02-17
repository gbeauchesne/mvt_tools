/*
 * mvt_image_file.h - Image utilities (file operations)
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

#ifndef MVT_IMAGE_FILE_H
#define MVT_IMAGE_FILE_H

#include "mvt_image.h"

MVT_BEGIN_DECLS

struct MvtImageFile_s;
typedef struct MvtImageFile_s MvtImageFile;

/** Image file access mode */
typedef enum {
    MVT_IMAGE_FILE_MODE_READ  = 1 << 0, ///< Read-only
    MVT_IMAGE_FILE_MODE_WRITE = 1 << 1, ///< Write-only
} MvtImageFileMode;

/** Image file info descriptor */
typedef struct {
    VideoFormat         format;         ///< Video format
    uint32_t            width;          ///< MvtImage width, in pixels
    uint32_t            height;         ///< MvtImage height, in pixels
    uint32_t            fps_n;          ///< Framerate (numerator)
    uint32_t            fps_d;          ///< Framerate (denominator)
    uint32_t            par_n;          ///< Pixel aspect ratio (numerator)
    uint32_t            par_d;          ///< Pixel aspect ratio (denominator)
} MvtImageInfo;

/** Initializes MvtImageInfo descriptor to the supplied parameters */
void
mvt_image_info_init(MvtImageInfo *info, VideoFormat format,
    uint32_t width, uint32_t height);

/** Initializes MvtImageInfo descriptor to its default values */
void
mvt_image_info_init_defaults(MvtImageInfo *info);

/** Creates a new image file with the supplied access mode */
MvtImageFile *
mvt_image_file_open(const char *path, MvtImageFileMode mode);

/** Closes the image file and all resources */
void
mvt_image_file_close(MvtImageFile *fp);

/** Writes the image file headers based on the supplied info descriptor */
bool
mvt_image_file_write_headers(MvtImageFile *fp, const MvtImageInfo *info);

/** Writes the image to file */
bool
mvt_image_file_write_image(MvtImageFile *fp, MvtImage *image);

/** Reads the image file headers */
bool
mvt_image_file_read_headers(MvtImageFile *fp, MvtImageInfo *info);

/** Reads the next image stored in file */
bool
mvt_image_file_read_image(MvtImageFile *fp, MvtImage *image);

MVT_END_DECLS

#endif /* MVT_IMAGE_FILE_H */
