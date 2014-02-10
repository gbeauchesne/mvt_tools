/*
 * ffmpeg_compat.h - FFmpeg compatibility glue
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

#ifndef FFMPEG_COMPAT_H
#define FFMPEG_COMPAT_H

#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51,42,0)
enum AVPixelFormat {
    AV_PIX_FMT_NONE             = PIX_FMT_NONE,
    AV_PIX_FMT_GRAY8            = PIX_FMT_GRAY8,
    AV_PIX_FMT_YUV420P          = PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV422P          = PIX_FMT_YUV422P,
    AV_PIX_FMT_YUV444P          = PIX_FMT_YUV444P,
    AV_PIX_FMT_NV12             = PIX_FMT_NV12,
    AV_PIX_FMT_YUYV422          = PIX_FMT_YUYV422,
    AV_PIX_FMT_UYVY422          = PIX_FMT_UYVY422,
};
#endif

#endif /* FFMPEG_COMPAT_H */
