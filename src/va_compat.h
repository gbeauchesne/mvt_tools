/*
 * va_compat.h - VA compatibility glue
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

#ifndef VA_COMPAT_GLUE_H
#define VA_COMPAT_GLUE_H

#include <va/va.h>

#if VA_CHECK_VERSION(0,34,0)
# include <va/va_compat.h>
#endif

/* Chroma formats */
#ifndef VA_RT_FORMAT_YUV411
#define VA_RT_FORMAT_YUV411     0x00000008
#endif
#ifndef VA_RT_FORMAT_YUV400
#define VA_RT_FORMAT_YUV400     0x00000010
#endif
#ifndef VA_RT_FORMAT_RGB16
#define VA_RT_FORMAT_RGB16      0x00010000
#endif
#ifndef VA_RT_FORMAT_RGB32
#define VA_RT_FORMAT_RGB32      0x00020000
#endif

/* Profiles */
#if !VA_CHECK_VERSION(0,34,0)
enum {
    VAProfileNone = -1,
};
#endif

/* Entrypoints */
#if !VA_CHECK_VERSION(0,34,0)
enum {
    VAEntrypointVideoProc = 10,
};
#endif

#endif /* VA_COMPAT_GLUE_H */
