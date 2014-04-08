/*
 * mvt_image_convert.c - Image utilities (color conversion)
 *
 * Copyright (C) 2010 VLC authors and VideoLAN
 *   Author: Laurent Aimar <fenrir@videolan.org>
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

#include "sysdeps.h"
#include "mvt_image.h"
#include "mvt_image_priv.h"
#include "mvt_memory.h"
#include <libyuv/cpu_id.h>
#include <libyuv/convert.h>
#include <libyuv/convert_from.h>

/*****************************************************************************
 * copy.c: Fast I420/NV12 copy
 *****************************************************************************
 * Copyright (C) 2010 Laurent Aimar
 * $Id: 9eed17f7d72fcb9e163244f9c43c3afe69558eb3 $
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

// Initializes copy cache buffer
static bool
ensure_copy_cache(MvtImage *image)
{
    MvtImagePrivate * const priv = mvt_image_priv_ensure(image);

    if (!priv)
        return false;

    if (MVT_UNLIKELY(!priv->copy_cache)) {
        priv->copy_cache_size = MVT_MAX(round_up(image->width, 16), 4096);
        priv->copy_cache = mem_alloc_aligned(priv->copy_cache_size, 16);
        if (!priv->copy_cache)
            return false;
    }
    return true;
}

/* Copy 64 bytes from srcp to dstp loading data with the SSE>=2 instruction
 * load and storing data with the SSE>=2 instruction store.
 */
#define COPY64(dstp, srcp, load, store) \
    asm volatile (                      \
        load "  0(%[src]), %%xmm1\n"    \
        load " 16(%[src]), %%xmm2\n"    \
        load " 32(%[src]), %%xmm3\n"    \
        load " 48(%[src]), %%xmm4\n"    \
        store " %%xmm1,    0(%[dst])\n" \
        store " %%xmm2,   16(%[dst])\n" \
        store " %%xmm3,   32(%[dst])\n" \
        store " %%xmm4,   48(%[dst])\n" \
        : : [dst]"r"(dstp), [src]"r"(srcp) : "memory", "xmm1", "xmm2", "xmm3", "xmm4")

/* Optimized copy from "Uncacheable Speculative Write Combining" memory
 * as used by some video surface.
 * XXX It is really efficient only when SSE4.1 is available.
 */
static void
CopyFromUswc(uint8_t *dst, size_t dst_pitch,
    const uint8_t *src, size_t src_pitch,
    unsigned width, unsigned height, unsigned cpu)
{
    unsigned y;

    assert(((intptr_t)dst & 0x0f) == 0 && (dst_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (y = 0; y < height; y++) {
        const unsigned unaligned = (-(uintptr_t)src) & 0x0f;
        unsigned x = 0;

        for (; x < unaligned; x++)
            dst[x] = src[x];

        if (cpu & kCpuHasSSE41) {
            if (!unaligned) {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movntdqa", "movdqa");
            }
            else {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movntdqa", "movdqu");
            }
        }
        else {
            if (!unaligned) {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movdqa", "movdqa");
            }
            else {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movdqa", "movdqu");
            }
        }

        for (; x < width; x++)
            dst[x] = src[x];

        src += src_pitch;
        dst += dst_pitch;
    }
}

static void
Copy2d(uint8_t *dst, size_t dst_pitch,
    const uint8_t *src, size_t src_pitch,
    unsigned width, unsigned height)
{
    unsigned y;

    assert(((intptr_t)src & 0x0f) == 0 && (src_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (y = 0; y < height; y++) {
        unsigned x = 0;

        bool unaligned = ((intptr_t)dst & 0x0f) != 0;
        if (!unaligned) {
            for (; x+63 < width; x += 64)
                COPY64(&dst[x], &src[x], "movdqa", "movntdq");
        }
        else {
            for (; x+63 < width; x += 64)
                COPY64(&dst[x], &src[x], "movdqa", "movdqu");
        }

        for (; x < width; x++)
            dst[x] = src[x];

        src += src_pitch;
        dst += dst_pitch;
    }
}

static void
SSE_SplitUV(uint8_t *dstu, size_t dstu_pitch,
    uint8_t *dstv, size_t dstv_pitch,
    const uint8_t *src, size_t src_pitch,
    unsigned width, unsigned height, unsigned cpu)
{
    const uint8_t shuffle[] = { 0, 2, 4, 6, 8, 10, 12, 14,
                                1, 3, 5, 7, 9, 11, 13, 15 };
    const uint8_t mask[] = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
                             0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };
    unsigned y;

    assert(((intptr_t)src & 0x0f) == 0 && (src_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (y = 0; y < height; y++) {
        unsigned x = 0;

#define LOAD64 \
    "movdqa  0(%[src]), %%xmm0\n" \
    "movdqa 16(%[src]), %%xmm1\n" \
    "movdqa 32(%[src]), %%xmm2\n" \
    "movdqa 48(%[src]), %%xmm3\n"

#define STORE2X32 \
    "movq   %%xmm0,   0(%[dst1])\n" \
    "movq   %%xmm1,   8(%[dst1])\n" \
    "movhpd %%xmm0,   0(%[dst2])\n" \
    "movhpd %%xmm1,   8(%[dst2])\n" \
    "movq   %%xmm2,  16(%[dst1])\n" \
    "movq   %%xmm3,  24(%[dst1])\n" \
    "movhpd %%xmm2,  16(%[dst2])\n" \
    "movhpd %%xmm3,  24(%[dst2])\n"

        if (cpu & kCpuHasSSSE3) {
            for (x = 0; x < (width & ~31); x += 32) {
                asm volatile (
                    "movdqu (%[shuffle]), %%xmm7\n"
                    LOAD64
                    "pshufb  %%xmm7, %%xmm0\n"
                    "pshufb  %%xmm7, %%xmm1\n"
                    "pshufb  %%xmm7, %%xmm2\n"
                    "pshufb  %%xmm7, %%xmm3\n"
                    STORE2X32
                    : : [dst1]"r"(&dstu[x]), [dst2]"r"(&dstv[x]), [src]"r"(&src[2*x]), [shuffle]"r"(shuffle) : "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm7");
            }
        }
        else {
            for (x = 0; x < (width & ~31); x += 32) {
                asm volatile (
                    "movdqu (%[mask]), %%xmm7\n"
                    LOAD64
                    "movdqa   %%xmm0, %%xmm4\n"
                    "movdqa   %%xmm1, %%xmm5\n"
                    "movdqa   %%xmm2, %%xmm6\n"
                    "psrlw    $8,     %%xmm0\n"
                    "psrlw    $8,     %%xmm1\n"
                    "pand     %%xmm7, %%xmm4\n"
                    "pand     %%xmm7, %%xmm5\n"
                    "pand     %%xmm7, %%xmm6\n"
                    "packuswb %%xmm4, %%xmm0\n"
                    "packuswb %%xmm5, %%xmm1\n"
                    "pand     %%xmm3, %%xmm7\n"
                    "psrlw    $8,     %%xmm2\n"
                    "psrlw    $8,     %%xmm3\n"
                    "packuswb %%xmm6, %%xmm2\n"
                    "packuswb %%xmm7, %%xmm3\n"
                    STORE2X32
                    : : [dst2]"r"(&dstu[x]), [dst1]"r"(&dstv[x]), [src]"r"(&src[2*x]), [mask]"r"(mask) : "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
            }
        }
#undef STORE2X32
#undef LOAD64

        for (; x < width; x++) {
            dstu[x] = src[2*x+0];
            dstv[x] = src[2*x+1];
        }
        src  += src_pitch;
        dstu += dstu_pitch;
        dstv += dstv_pitch;
    }
}

static void
SSE_CopyPlane(uint8_t *dst, size_t dst_pitch,
    const uint8_t *src, size_t src_pitch,
    uint8_t *cache, size_t cache_size,
    unsigned width, unsigned height, unsigned cpu)
{
    const unsigned w16 = round_up(width, 16);
    const unsigned hstep = cache_size / w16;
    unsigned y;

    assert(hstep > 0);

    for (y = 0; y < height; y += hstep) {
        const unsigned hblock =  MVT_MIN(hstep, height - y);

        /* Copy a bunch of line into our cache */
        CopyFromUswc(cache, w16,
                     src, src_pitch,
                     width, hblock, cpu);

        /* Copy from our cache to the destination */
        Copy2d(dst, dst_pitch,
               cache, w16,
               width, hblock);

        /* */
        src += src_pitch * hblock;
        dst += dst_pitch * hblock;
    }
    asm volatile ("mfence");
}

static void
SSE_SplitPlanes(uint8_t *dstu, size_t dstu_pitch,
    uint8_t *dstv, size_t dstv_pitch,
    const uint8_t *src, size_t src_pitch,
    uint8_t *cache, size_t cache_size,
    unsigned width, unsigned height, unsigned cpu)
{
    const unsigned w2_16 = round_up(2*width, 16);
    const unsigned hstep = cache_size / w2_16;
    unsigned y;

    assert(hstep > 0);

    for (y = 0; y < height; y += hstep) {
        const unsigned hblock =  MVT_MIN(hstep, height - y);

        /* Copy a bunch of line into our cache */
        CopyFromUswc(cache, w2_16, src, src_pitch,
                     2*width, hblock, cpu);

        /* Copy from our cache to the destination */
        SSE_SplitUV(dstu, dstu_pitch, dstv, dstv_pitch,
                    cache, w2_16, width, hblock, cpu);

        /* */
        src  += src_pitch  * hblock;
        dstu += dstu_pitch * hblock;
        dstv += dstv_pitch * hblock;
    }
    asm volatile ("mfence");
}

static bool
SSE_CopyFromNV12(MvtImage *dst_image, MvtImage *src_image)
{
    MvtImagePrivate * const priv = mvt_image_priv_ensure(dst_image);
    unsigned cpu = TestCpuFlag(kCpuHasSSSE3|kCpuHasSSE41);

    if (!priv || !ensure_copy_cache(dst_image))
        return false;

    SSE_CopyPlane(dst_image->pixels[0], dst_image->pitches[0],
        src_image->pixels[0], src_image->pitches[0],
        priv->copy_cache, priv->copy_cache_size,
        dst_image->width, dst_image->height, cpu);
    SSE_SplitPlanes(dst_image->pixels[1], dst_image->pitches[1],
        dst_image->pixels[2], dst_image->pitches[2],
        src_image->pixels[1], src_image->pitches[1],
        priv->copy_cache, priv->copy_cache_size,
        (dst_image->width + 1)/2, (dst_image->height + 1)/2, cpu);
    asm volatile ("emms");
    return true;
}

static bool
SSE_CopyFromI420(MvtImage *dst_image, MvtImage *src_image)
{
    MvtImagePrivate * const priv = mvt_image_priv_ensure(dst_image);
    unsigned n, cpu = TestCpuFlag(kCpuHasSSSE3|kCpuHasSSE41);

    if (!priv || !ensure_copy_cache(dst_image))
        return false;

    for (n = 0; n < 3; n++) {
        const unsigned d = n > 0 ? 2 : 1;
        SSE_CopyPlane(dst_image->pixels[n], dst_image->pitches[n],
            src_image->pixels[n], src_image->pitches[n],
            priv->copy_cache, priv->copy_cache_size,
            (dst_image->width + d - 1)/d, (dst_image->height + d - 1)/d, cpu);
    }
    asm volatile ("emms");
    return true;
}
#undef COPY64

/* -------------------------------------------------------------------------- */
/* --- libyuv based conversions                                           --- */
/* -------------------------------------------------------------------------- */

// Converts images with the same size
static bool
image_convert_internal(MvtImage *dst_image, const VideoFormatInfo *dst_vip,
    MvtImage *src_image, const VideoFormatInfo *src_vip, uint32_t flags)
{
    if ((flags & MVT_IMAGE_FLAG_FROM_USWC) &&
        dst_image->format == VIDEO_FORMAT_I420) {
        switch (src_image->format) {
        case VIDEO_FORMAT_NV12:
            if (SSE_CopyFromNV12(dst_image, src_image))
                return true;
            break;
        case VIDEO_FORMAT_I420:
            if (SSE_CopyFromI420(dst_image, src_image))
                return true;
            break;
        default:
            break;
        }
    }

    if (src_image->format == VIDEO_FORMAT_NV12 &&
        dst_image->format == VIDEO_FORMAT_I420)
        return NV12ToI420(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->pixels[2], dst_image->pitches[2],
            dst_image->width, dst_image->height) == 0;

    if (src_image->format == VIDEO_FORMAT_I420 &&
        dst_image->format == VIDEO_FORMAT_I420)
        return I420Copy(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            src_image->pixels[2], src_image->pitches[2],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->pixels[2], dst_image->pitches[2],
            dst_image->width, dst_image->height) == 0;

    if (src_image->format == VIDEO_FORMAT_I420 &&
        dst_image->format == VIDEO_FORMAT_NV12)
        return I420ToNV12(src_image->pixels[0], src_image->pitches[0],
            src_image->pixels[1], src_image->pitches[1],
            src_image->pixels[2], src_image->pitches[2],
            dst_image->pixels[0], dst_image->pitches[0],
            dst_image->pixels[1], dst_image->pitches[1],
            dst_image->width, dst_image->height) == 0;

    mvt_error("image: unsupported conversion (%s -> %s)",
        src_vip->name, dst_vip->name);
    return false;
}

// Converts images with the same size
bool
mvt_image_convert(MvtImage *dst_image, MvtImage *src_image)
{
    return mvt_image_convert_full(dst_image, src_image, 0);
}

// Converts images with the same size, while allowing interleaving
bool
mvt_image_convert_full(MvtImage *dst_image, MvtImage *src_image, uint32_t flags)
{
    const VideoFormatInfo *src_vip, *dst_vip;
    MvtImage src_image_tmp, dst_image_tmp;
    uint32_t field_flags;

    if (!dst_image || !src_image)
        return false;

    if (dst_image->width  != src_image->width ||
        dst_image->height != src_image->height) {
        mvt_fatal_error("only images with the same size are allowed");
        return false;
    }

    src_vip = video_format_get_info(src_image->format);
    if (!src_vip)
        return false;

    dst_vip = video_format_get_info(dst_image->format);
    if (!dst_vip)
        return false;

    if (src_vip->chroma_type != dst_vip->chroma_type) {
        mvt_fatal_error("only images with the same chroma type are allowed");
        return false;
    }

    field_flags = flags & (VA_TOP_FIELD|VA_BOTTOM_FIELD);
    if (!field_flags)
        return image_convert_internal(dst_image, dst_vip, src_image, src_vip,
            flags);

    if (flags & VA_TOP_FIELD) {
        mvt_image_init_from_field(&src_image_tmp, src_image, VA_TOP_FIELD);
        mvt_image_init_from_field(&dst_image_tmp, dst_image, VA_TOP_FIELD);
        if (!image_convert_internal(&dst_image_tmp, dst_vip,
                &src_image_tmp, src_vip, flags))
            return false;
    }

    if (flags & VA_BOTTOM_FIELD) {
        mvt_image_init_from_field(&src_image_tmp, src_image, VA_BOTTOM_FIELD);
        mvt_image_init_from_field(&dst_image_tmp, dst_image, VA_BOTTOM_FIELD);
        if (!image_convert_internal(&dst_image_tmp, dst_vip,
                &src_image_tmp, src_vip, flags))
            return false;
    }
    return true;
}
