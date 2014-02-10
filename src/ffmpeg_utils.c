/*
 * ffmpeg_utils.c - FFmpeg utilities
 *
 * Copyright (C) 2014 Intel Corporation
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
#include "ffmpeg_utils.h"
#include "ffmpeg_compat.h"

// Returns a string representation of the supplied FFmpeg error id
const char *
ffmpeg_strerror(int errnum, char errbuf[BUFSIZ])
{
    if (av_strerror(errnum, errbuf, BUFSIZ) != 0)
        sprintf(errbuf, "error %d", errnum);
    return errbuf;
}

// Translates FFmpeg pixel format to MVT video format
bool
ffmpeg_to_mvt_video_format(enum AVPixelFormat pix_fmt, VideoFormat *format_ptr)
{
    VideoFormat format;

    switch (pix_fmt) {
    case AV_PIX_FMT_GRAY8:
        format = VIDEO_FORMAT_Y800;
        break;
    case AV_PIX_FMT_YUV420P:
        format = VIDEO_FORMAT_I420;
        break;
    case AV_PIX_FMT_NV12:
        format = VIDEO_FORMAT_NV12;
        break;
    case AV_PIX_FMT_YUYV422:
        format = VIDEO_FORMAT_YUY2;
        break;
    case AV_PIX_FMT_UYVY422:
        format = VIDEO_FORMAT_UYVY;
        break;
    default:
        format = VIDEO_FORMAT_UNKNOWN;
        break;
    }
    if (!format)
        return false;

    if (format_ptr)
        *format_ptr = format;
    return true;
}
