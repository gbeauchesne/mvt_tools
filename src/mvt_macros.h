/*
 * mvt_macros.h - Utility macros
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

#ifndef MVT_MACROS_H
#define MVT_MACROS_H

/* Determine the size of a static array */
#define MVT_ARRAY_LENGTH(a) \
    (sizeof(a) / sizeof((a)[0]))

#define MVT_MIN(a, b)                   (((a) < (b)) ? (a) : (b))
#define MVT_MAX(a, b)                   (((a) > (b)) ? (a) : (b))

#ifndef MVT_ALIGNED
#define MVT_ALIGNED(n)                  __attribute__((__aligned__(n)))
#endif

#if defined __GNUC__
# define MVT_LIKELY(expr)               (__builtin_expect(!!(expr), 1))
# define MVT_UNLIKELY(expr)             (__builtin_expect(!!(expr), 0))
#else
# define MVT_LIKELY(expr)               (expr)
# define MVT_UNLIKELY(expr)             (expr)
#endif

#define MVT_GEN_STRING(x)               MVT_GEN_STRING_I(x)
#define MVT_GEN_STRING_I(x)             #x

#define MVT_GEN_CONCAT(a1, a2)          MVT_GEN_CONCAT2_I(a1, a2)
#define MVT_GEN_CONCAT2(a1, a2)         MVT_GEN_CONCAT2_I(a1, a2)
#define MVT_GEN_CONCAT2_I(a1, a2)       a1 ## a2
#define MVT_GEN_CONCAT3(a1, a2, a3)     MVT_GEN_CONCAT3_I(a1, a2, a3)
#define MVT_GEN_CONCAT3_I(a1, a2, a3)   a1 ## a2 ## a3
#define MVT_GEN_CONCAT4(a1, a2, a3, a4) MVT_GEN_CONCAT4_I(a1, a2, a3, a4)
#define MVT_GEN_CONCAT4_I(a1, a2, a3, a4) a1 ## a2 ## a3 ## a4

#ifdef __cplusplus
# define MVT_BEGIN_DECLS                extern "C" {
# define MVT_END_DECLS                  }
#else
# define MVT_BEGIN_DECLS
# define MVT_END_DECLS
#endif

#endif /* MVT_MACROS_H */
