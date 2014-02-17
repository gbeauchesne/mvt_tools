/*
 * video_format.h - Video formats
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

#ifndef VIDEO_FORMAT_H
#define VIDEO_FORMAT_H

#include <va/va.h>

MVT_BEGIN_DECLS

/* Helper macro to build native endian byte-order video format */
#ifdef WORDS_BIGENDIAN
# define VIDEO_FORMAT_NE(be, le) MVT_GEN_CONCAT(VIDEO_FORMAT_,be)
#else
# define VIDEO_FORMAT_NE(be, le) MVT_GEN_CONCAT(VIDEO_FORMAT_,le)
#endif

typedef enum {
    /** Unknown video format */
    VIDEO_FORMAT_UNKNOWN = 0,
    /** Unknown video format (internally encoded) */
    VIDEO_FORMAT_ENCODED,

    /** Planar YUV 4:2:0, 12-bit, 1 plane for Y and 1 plane for UV */
    VIDEO_FORMAT_NV12,
    /** Planar YUV 4:2:0, 12-bit, 3 planes for Y U V */
    VIDEO_FORMAT_I420,
    /** Planar YUV 4:2:0, 12-bit, 3 planes for Y V U */
    VIDEO_FORMAT_YV12,
    /** 8 bit grayscale */
    VIDEO_FORMAT_Y800,
    /** Packed YUV 4:4:4, 32-bit, A Y U V */
    VIDEO_FORMAT_AYUV,
    /** Packed YUV 4:2:2, 16-bit, Y0 Cb Y1 Cr */
    VIDEO_FORMAT_YUY2,
    /** Packed YUV 4:2:2, 16-bit, Cb Y0 Cr Y1 */
    VIDEO_FORMAT_UYVY,
    /** Packed RGB 8:8:8, 32-bit, x R G B */
    VIDEO_FORMAT_xRGB,
    /** Packed RGB 8:8:8, 32-bit, x B G R */
    VIDEO_FORMAT_xBGR,
    /** Packed RGB 8:8:8, 32-bit, R G B x */
    VIDEO_FORMAT_RGBx,
    /** Packed RGB 8:8:8, 32-bit, B G R x */
    VIDEO_FORMAT_BGRx,
    /** Packed RGB 8:8:8, 32-bit, A R G B */
    VIDEO_FORMAT_ARGB,
    /** Packed RGB 8:8:8, 32-bit, A B G R */
    VIDEO_FORMAT_ABGR,
    /** Packed RGB 8:8:8, 32-bit, R G B A */
    VIDEO_FORMAT_RGBA,
    /** Packed RGB 8:8:8, 32-bit, B G R A */
    VIDEO_FORMAT_BGRA,
    /** Planar YUV 4:2:0, 15-bit, 3 planes for Y U V, 10 bits per sample */
    VIDEO_FORMAT_I420P10,
    /** Planar YUV 4:2:0, 18-bit, 3 planes for Y U V, 12 bits per sample */
    VIDEO_FORMAT_I420P12,
    /** Planar YUV 4:2:0, 24-bit, 3 planes for Y U V, 16 bits per sample */
    VIDEO_FORMAT_I420P16,
    /** Planar YUV 4:2:2, 20-bit, 3 planes for Y U V, 10 bits per sample */
    VIDEO_FORMAT_I422P10,
    /** Planar YUV 4:2:2, 24-bit, 3 planes for Y U V, 12 bits per sample */
    VIDEO_FORMAT_I422P12,
    /** Planar YUV 4:2:2, 32-bit, 3 planes for Y U V, 16 bits per sample */
    VIDEO_FORMAT_I422P16,
    /** Planar YUV 4:4:4, 30-bit, 3 planes for Y U V, 10 bits per sample */
    VIDEO_FORMAT_I444P10,
    /** Planar YUV 4:4:4, 36-bit, 3 planes for Y U V, 12 bits per sample */
    VIDEO_FORMAT_I444P12,
    /** Planar YUV 4:4:4, 48-bit, 3 planes for Y U V, 16 bits per sample */
    VIDEO_FORMAT_I444P16,
    /** Number of video formats */
    VIDEO_FORMAT_COUNT,

    /** Packed RGB 8:8:8, 32-bit, x R G B, native endian byte-order */
    VIDEO_FORMAT_RGB32 = VIDEO_FORMAT_NE(xRGB, BGRx),
    /** Packed RGB 8:8:8, 32-bit, A R G B, native endian byte-order */
    VIDEO_FORMAT_ARGB32 = VIDEO_FORMAT_NE(ARGB, BGRA),
} VideoFormat;

/** Maximum number of supported planes */
#define VIDEO_FORMAT_MAX_PLANES         4

/** Maximum number of supported components */
#define VIDEO_FORMAT_MAX_COMPONENTS     4

typedef struct {
    uint8_t             plane;          ///< Plane identifier
    uint8_t             pixel_offset;   ///< Byte offset within the pixel
    uint8_t             pixel_stride;   ///< Number of bytes for a pixel
    uint8_t             bit_depth;      ///< Number of bits for a sample
} VideoFormatComponentInfo;

typedef struct {
    const char *        name;           ///< String representation
    VideoFormat         format;         ///< Video format
    uint32_t            chroma_type;    ///< VA chroma type
    uint8_t             chroma_w_shift; ///< Shift count for chroma width
    uint8_t             chroma_h_shift; ///< Shift count for chroma height
    VAImageFormat       va_format;      ///< VA image format specification
    uint8_t             num_planes;     ///< Number of \ref planes
    uint8_t             num_components; ///< Number of \ref components
    /*
     * Components in the \ref components[] array are ordered such a
     * way that, for YUV formats, we have Y U V A components in that
     * order (up to 4 components); and for RGB formats, we have R G B
     * A components in that order (up to 4 components).
     */
    VideoFormatComponentInfo components[VIDEO_FORMAT_MAX_COMPONENTS];
} VideoFormatInfo;

/** Checks whether the format is an RGB format */
bool
video_format_is_rgb(VideoFormat format);

/** Checks whether the format is a YUV format */
bool
video_format_is_yuv(VideoFormat format);

/** Checks whether the format is grayscale */
bool
video_format_is_grayscale(VideoFormat format);

/** Checks whether the format is subsampled (for YUV) */
bool
video_format_is_subsampled(VideoFormat format);

/** Checks whether the format includes an alpha channel */
bool
video_format_has_alpha(VideoFormat format);

/** Converts a video format name to its unique identifier */
VideoFormat
video_format_from_name(const char *name);

/** Converts a VA fourcc value to a video format */
VideoFormat
video_format_from_va_fourcc(uint32_t fourcc);

/** Converts a VA image format to a video format */
VideoFormat
video_format_from_va_format(const VAImageFormat *va_format);

/** Converts a video format to a VA image format */
const VAImageFormat *
video_format_to_va_format(VideoFormat format);

/** Retrieves extended information for the specified format */
const VideoFormatInfo *
video_format_get_info(VideoFormat format);

/** Converts a video format to its string representation */
const char *
video_format_get_name(VideoFormat format);

/** Converts a video format to a chroma type */
uint32_t
video_format_get_chroma_type(VideoFormat format);

/** Retrieves the pixel pitches for each plane */
bool
video_format_get_pixel_pitches(VideoFormat format,
    uint32_t pixel_pitches[VIDEO_FORMAT_MAX_PLANES]);

MVT_END_DECLS

#endif /* VIDEO_FORMAT_H */
