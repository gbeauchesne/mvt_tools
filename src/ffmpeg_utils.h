/*
 * ffmpeg_utils.h - FFmpeg utilities
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

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H

#include "video_format.h"
#include "ffmpeg_compat.h"

/** Returns a string representation of the supplied FFmpeg error id */
const char *
ffmpeg_strerror(int errnum, char errbuf[BUFSIZ]);

/** Translates FFmpeg pixel format to MVT video format */
bool
ffmpeg_to_mvt_video_format(enum AVPixelFormat pix_fmt, VideoFormat *format_ptr);

#endif /* FFMPEG_UTILS_H */
