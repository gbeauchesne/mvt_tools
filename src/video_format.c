/*
 * video_format.c - Video formats
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
#include "video_format.h"
#include "va_compat.h"

#define DEF_YUV(FORMAT, FOURCC, ENDIAN, BPP, SUB)                       \
    [MVT_GEN_CONCAT(VIDEO_FORMAT_,FORMAT)] = {                          \
        MVT_GEN_STRING(FORMAT),                                         \
        MVT_GEN_CONCAT(VIDEO_FORMAT_,FORMAT),                           \
        MVT_GEN_CONCAT(VA_RT_FORMAT_YUV,SUB),                           \
        MVT_GEN_CONCAT(R_YUV,SUB),                                      \
        { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, },                \
        MVT_GEN_CONCAT(C_,FORMAT)                                       \
    }

#define DEF_YUVp(BIT_DEPTH, FOURCC, ENDIAN, BPP, SUB)                   \
    [MVT_GEN_CONCAT4(VIDEO_FORMAT_I,SUB,P,BIT_DEPTH)] = {               \
        MVT_GEN_STRING(MVT_GEN_CONCAT4(I,SUB,p,BIT_DEPTH)),             \
        MVT_GEN_CONCAT4(VIDEO_FORMAT_I,SUB,P,BIT_DEPTH),                \
        MVT_GEN_CONCAT(VA_RT_FORMAT_YUV,SUB),                           \
        MVT_GEN_CONCAT(R_YUV,SUB),                                      \
        { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, },                \
        C_YUVp(BIT_DEPTH)                                               \
    }

#define DEF_RGB(FORMAT, FOURCC, ENDIAN, BPP, DEPTH, R,G,B,A)            \
    [MVT_GEN_CONCAT(VIDEO_FORMAT_,FORMAT)] = {                          \
        MVT_GEN_STRING(FORMAT),                                         \
        MVT_GEN_CONCAT(VIDEO_FORMAT_,FORMAT),                           \
        MVT_GEN_CONCAT(VA_RT_FORMAT_RGB,BPP),                           \
        MVT_GEN_CONCAT(R_RGB,BPP),                                      \
        { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, DEPTH, R,G,B,A }, \
        MVT_GEN_CONCAT(C_,FORMAT)                                       \
    }

#define R_FF    0,0     // Full horizontal resolution, full vertical resolution
#define R_HF    1,0     // Half horizontal resolution, full vertical resolution
#define R_HH    1,1     // Half horizontal resolution, half vertical resolution

#define R_YUV420        R_HH
#define R_YUV422        R_HF
#define R_YUV444        R_FF
#define R_YUV400        R_FF
#define R_RGB32         R_FF

#define C_NV12          2, 3, {{0,0,1,8},{1,0,2,8},{1,1,2,8},}
#define C_I420          3, 3, {{0,0,1,8},{1,0,1,8},{2,0,1,8},}
#define C_YV12          3, 3, {{0,0,1,8},{2,0,1,8},{1,0,1,8},}
#define C_YUY2          1, 3, {{0,0,2,8},{0,1,4,8},{0,3,4,8},}
#define C_UYVY          1, 3, {{0,1,2,8},{0,0,4,8},{0,2,4,8},}
#define C_AYUV          1, 4, {{0,1,4,8},{0,2,4,8},{0,3,4,8},{0,0,4,8},}
#define C_Y800          1, 1, {{0,0,1,8},}
#define C_ARGB          1, 4, {{0,1,4,8},{0,2,4,8},{0,3,4,8},{0,0,4}}
#define C_xRGB          1, 3, {{0,1,4,8},{0,2,4,8},{0,3,4,8},}
#define C_ABGR          1, 4, {{0,3,4,8},{0,2,4,8},{0,1,4,8},{0,0,4}}
#define C_xBGR          1, 3, {{0,3,4,8},{0,2,4,8},{0,1,4,8},}
#define C_RGBA          1, 4, {{0,0,4,8},{0,1,4,8},{0,2,4,8},{0,3,4}}
#define C_RGBx          1, 3, {{0,0,4,8},{0,1,4,8},{0,2,4,8},}
#define C_BGRA          1, 4, {{0,2,4,8},{0,1,4,8},{0,0,4,8},{0,3,4}}
#define C_BGRx          1, 3, {{0,2,4,8},{0,1,4,8},{0,0,4,8},}
#define C_YUVp(n)       3, 3, {{0,0,2,n},{1,0,2,n},{2,0,2,n},}

#ifdef WORDS_BIGENDIAN
#define VA_NSB_FIRST VA_MSB_FIRST
#else
#define VA_NSB_FIRST VA_LSB_FIRST
#endif

static const VideoFormatInfo g_video_formats[] = {
    DEF_YUV(NV12, ('N','V','1','2'), LSB, 12, 420),
    DEF_YUV(YV12, ('Y','V','1','2'), LSB, 12, 420),
    DEF_YUV(I420, ('I','4','2','0'), LSB, 12, 420),
    DEF_YUV(YUY2, ('Y','U','Y','2'), LSB, 16, 422),
    DEF_YUV(UYVY, ('U','Y','V','Y'), LSB, 16, 422),
    DEF_YUV(AYUV, ('A','Y','U','V'), LSB, 32, 444),
    DEF_YUV(Y800, ('Y','8','0','0'), LSB,  8, 400),
#ifdef WORDS_BIGENDIAN
    DEF_RGB(xRGB, ('X','R','G','B'), MSB, 32,
            24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000),
    DEF_RGB(xBGR, ('X','B','G','R'), MSB, 32,
            24, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000),
    DEF_RGB(RGBx, ('R','G','B','X'), MSB, 32,
            24, 0xff000000, 0x00ff0000, 0x0000ff00, 0x00000000),
    DEF_RGB(BGRx, ('B','G','R','X'), MSB, 32,
            24, 0x0000ff00, 0x00ff0000, 0x0000ff00, 0x00000000),
    DEF_RGB(ARGB, ('A','R','G','B'), MSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(ABGR, ('A','B','G','R'), MSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
    DEF_RGB(RGBA, ('R','G','B','A'), MSB, 32,
            32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff),
    DEF_RGB(BGRA, ('B','G','R','A'), MSB, 32,
            32, 0x0000ff00, 0x00ff0000, 0x0000ff00, 0x000000ff),
#else
    DEF_RGB(xRGB, ('B','G','R','X'), LSB, 32,
            24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000),
    DEF_RGB(xBGR, ('R','G','B','X'), LSB, 32,
            24, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000),
    DEF_RGB(RGBx, ('X','B','G','R'), LSB, 32,
            24, 0xff000000, 0x00ff0000, 0x0000ff00, 0x00000000),
    DEF_RGB(BGRx, ('X','R','G','B'), LSB, 32,
            24, 0x0000ff00, 0x00ff0000, 0xff000000, 0x00000000),
    DEF_RGB(ARGB, ('B','G','R','A'), LSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(ABGR, ('R','G','B','A'), LSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
    DEF_RGB(RGBA, ('A','B','G','R'), LSB, 32,
            32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff),
    DEF_RGB(BGRA, ('A','R','G','B'), LSB, 32,
            32, 0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff),
#endif
    DEF_YUVp(10,  ('P','0','1','0'), NSB, 15, 420),
    DEF_YUVp(12,  ('P','0','1','2'), NSB, 18, 420),
    DEF_YUVp(16,  ('P','0','1','6'), NSB, 24, 420),
    DEF_YUVp(10,  ('P','2','1','0'), NSB, 20, 422),
    DEF_YUVp(12,  ('P','2','1','2'), NSB, 24, 422),
    DEF_YUVp(16,  ('P','2','1','6'), NSB, 32, 422),
    DEF_YUVp(10,  ('P','4','1','0'), NSB, 30, 444),
    DEF_YUVp(12,  ('P','4','1','2'), NSB, 36, 444),
    DEF_YUVp(16,  ('P','4','1','6'), NSB, 48, 444),
    { NULL, }
};

#undef DEF_RGB
#undef DEF_YUV
#undef DEF_YUVp

static inline bool
va_format_is_rgb(const VAImageFormat *va_format)
{
    return va_format->depth != 0;
}

static inline bool
va_format_is_yuv(const VAImageFormat *va_format)
{
    return va_format->depth == 0;
}

static inline bool
va_format_is_same_rgb(const VAImageFormat *fmt1, const VAImageFormat *fmt2)
{
    return (fmt1->byte_order == fmt2->byte_order &&
            fmt1->red_mask   == fmt2->red_mask   &&
            fmt1->green_mask == fmt2->green_mask &&
            fmt1->blue_mask  == fmt2->blue_mask  &&
            fmt1->alpha_mask == fmt2->alpha_mask);
}

static inline bool
va_format_is_same(const VAImageFormat *fmt1, const VAImageFormat *fmt2)
{
    if (fmt1->fourcc != fmt2->fourcc)
        return false;
    return va_format_is_rgb(fmt1) ? va_format_is_same_rgb(fmt1, fmt2) : true;
}

// Checks whether the video format is valid
static inline bool
is_valid(const VideoFormatInfo *vip)
{
    return vip->name != NULL;
}

// Lookups video format in the list of supported mappings
static const VideoFormatInfo *
get_info(int format)
{
    const VideoFormatInfo *vip;

    if (format < 0 || format >= VIDEO_FORMAT_COUNT)
        return NULL;

    vip = &g_video_formats[format];
    return is_valid(vip) ? vip : NULL;
}

// Checks whether the format is an RGB format
bool
video_format_is_rgb(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip && va_format_is_rgb(&vip->va_format);
}

// Checks whether the format is a YUV format
bool
video_format_is_yuv(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip && va_format_is_yuv(&vip->va_format);
}

// Checks whether the format is grayscale
bool
video_format_is_grayscale(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip && vip->chroma_type == VA_RT_FORMAT_YUV400;
}

// Checks whether the format is subsampled (for YUV)
bool
video_format_is_subsampled(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip && (vip->chroma_w_shift > 0 || vip->chroma_h_shift > 0);
}

// Checks whether the format includes an alpha channel
bool
video_format_has_alpha(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip && vip->num_components == 4;
}

// Converts a video format name to its unique identifier
VideoFormat
video_format_from_name(const char *name)
{
    uint32_t i;

    if (!name)
        return VIDEO_FORMAT_UNKNOWN;

    for (i = 0; i < MVT_ARRAY_LENGTH(g_video_formats); i++) {
        const VideoFormatInfo * const vip = &g_video_formats[i];
        if (is_valid(vip) && strcmp(vip->name, name) == 0)
            return vip->format;
    }
    return VIDEO_FORMAT_UNKNOWN;
}

// Converts a VA fourcc value to a video format
VideoFormat
video_format_from_va_fourcc(uint32_t fourcc)
{
    uint32_t i;

    if (!fourcc)
        return VIDEO_FORMAT_UNKNOWN;

    for (i = 0; i < MVT_ARRAY_LENGTH(g_video_formats); i++) {
        const VideoFormatInfo * const vip = &g_video_formats[i];
        if (is_valid(vip) && vip->va_format.fourcc == fourcc)
            return vip->format;
    }
    return VIDEO_FORMAT_UNKNOWN;
}

// Converts a VA image format to a video format
VideoFormat
video_format_from_va_format(const VAImageFormat *va_format)
{
    uint32_t i;

    for (i = 0; i < MVT_ARRAY_LENGTH(g_video_formats); i++) {
        const VideoFormatInfo * const vip = &g_video_formats[i];
        if (is_valid(vip) && va_format_is_same(&vip->va_format, va_format))
            return vip->format;
    }
    return VIDEO_FORMAT_UNKNOWN;
}

// Converts a video format to a VA image format
const VAImageFormat *
video_format_to_va_format(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip ? &vip->va_format : NULL;
}

// Retrieves information for the specified format
const VideoFormatInfo *
video_format_get_info(VideoFormat format)
{
    return get_info(format);
}

// Converts video format to its string representation
const char *
video_format_get_name(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip ? vip->name : NULL;
}

// Converts a video format to a chroma type
uint32_t
video_format_get_chroma_type(VideoFormat format)
{
    const VideoFormatInfo * const vip = get_info(format);

    return vip ? vip->chroma_type : 0;
}

// Retrieves the pixel pitches for each plane
bool
video_format_get_pixel_pitches(VideoFormat format,
    uint32_t pixel_pitches[VIDEO_FORMAT_MAX_PLANES])
{
    const VideoFormatInfo * const vip = get_info(format);
    uint32_t i, pitches[VIDEO_FORMAT_MAX_PLANES];

    if (!vip)
        return false;

    memset(pitches, 0, sizeof(pitches));
    for (i = 0; i < vip->num_components; i++) {
        const VideoFormatComponentInfo * const cip = &vip->components[i];
        const uint32_t pitch =
            cip->pixel_stride >> (i > 0 ? vip->chroma_w_shift : 0);
        if (!pitches[cip->plane]) {
            pitches[cip->plane] = pitch;
            pixel_pitches[cip->plane] = cip->pixel_stride;
        }
        else if (pitches[cip->plane] != pitch)
            return false;
    }
    return true;
}
