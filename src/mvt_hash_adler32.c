/*
 * mvt_hash_adler32.c - Hash functions used in MVT (Adler32)
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
#include <libyuv/cpu_id.h>
#include "mvt_hash.h"
#include "mvt_hash_priv.h"

typedef struct {
    MvtHash     base;
    uint32_t    value;
} MvtHashAdler32;

/* Initial Adler-32 value. Normally 1 */
#define ADLER32_INIT 0

/* Largest prime number that is smaller than 65536 */
#define ADLER32_BASE 65521

static bool
adler32_init(MvtHashAdler32 *hash)
{
    hash->value = ADLER32_INIT;
    return true;
}

static void
adler32_finalize(MvtHashAdler32 *hash)
{
    const uint32_t value = hash->value;

    hash->base.value[0] = value >> 24;
    hash->base.value[1] = value >> 16;
    hash->base.value[2] = value >> 8;
    hash->base.value[3] = value;
}

#define ADLER32_UNPACK(h, s1, s2) \
    s1 = ((h)->value & 0xffff), s2 = ((h)->value >> 16)

#define ADLER32_PACK(h, s1, s2) \
    (h)->value = (((s2) << 16) | (s1))

/* Special case for one byte at a time */
static void
adler32_update_1(MvtHashAdler32 *hash, const uint8_t *buf)
{
    uint32_t s1, s2;

    ADLER32_UNPACK(hash, s1, s2);
    s1 += buf[0];
    if (s1 >= ADLER32_BASE)
        s1 -= ADLER32_BASE;
    s2 += s1;
    if (s2 >= ADLER32_BASE)
        s2 -= ADLER32_BASE;
    ADLER32_PACK(hash, s1, s2);
}

#define DO1(buf, i)     do { s1 += buf[i]; s2 += s1; } while (0)
#define DO2(buf, i)     DO1(buf, i); DO1(buf, i+1);
#define DO4(buf, i)     DO2(buf, i); DO2(buf, i+2);
#define DO8(buf, i)     DO4(buf, i); DO4(buf, i+4);

/* Original algorithm (naive C version) */
static void
adler32_update_c(MvtHashAdler32 *hash, const uint8_t *buf, uint32_t len)
{
    uint32_t i, s1, s2;

    ADLER32_UNPACK(hash, s1, s2);
    for (i = 0; i < len; i++) {
        DO1(buf, i);
        s1 %= ADLER32_BASE;
        s2 %= ADLER32_BASE;
    }
    ADLER32_PACK(hash, s1, s2);
}

/* SWAR optimized version, with a virtual vector register of 8 bytes */
static void
adler32_update_c_swar(MvtHashAdler32 *hash, const uint8_t *buf, uint32_t len)
{
    uint32_t s1, s2;

    if (len == 1) {
        adler32_update_1(hash, buf);
        return;
    }

    /* Theory of operations for a given sequence:
       v[0] v[1] v[2] v[3] v[4] ... v[len]

       The Adler-32 series looks as follows:
       I: s1 = ADLER32_INIT s2 = 0
       0: s1 += v[0]        s2 += v[0]
       1: s1 += v[1]        s2 += v[0] + v[1] = 2*v[0] + v[1]
       k: s1 += v[k]        s2 += \sum{i=0..k} v[k] = \sum{i=0..k} (k+1-i)*v[i]

       We want to deal with partitions of 8 bytes (uint64_t).

       v00 v01 v02 v03 v04 v05 v06 v07 | v08 v09 v10 v11 v12 v13 v14 v15 | ...
       (a1,a2) will represent the Adler32 for the even bytes
       (b1,b2) will represent the Adler32 for the odd bytes

       The merge/reduce after n partitions of k=8 bytes are processed yields:
       s1 += a1 + b1;
       s2 += (8,6,4,2) * a1 + (7,5,3,1) * b1 + 8 * (a2 + b2)

       with the provision that n is chosen so that 16-bit elements of
       a2 and b2 don't overflow as they have exponential growth. In
       practice, n = 22 is the smallest integer that satisfies the
       following equation: MAX(v[i]) * n * (n + 1) / 2 <= 65535. And,
       since we pre-add a1 (resp. b1) to a2 (resp. b2), we can
       increase n to 23 as a1 and b1 are initially set to zero.
    */
    ADLER32_UNPACK(hash, s1, s2);
    while (len > 8) {
        const uint32_t n = MVT_MIN((len & ~7), 23 * 8);
        uint64_t a1 = 0, a2 = 0, b1 = 0, b2 = 0;
        uint32_t i;

        /* Accumulate interleaved Adler32 values */
        for (i = 0; i < n; i += 8) {
            const uint64_t c = *(uint64_t *)&buf[i];
            a2 += a1;
            b2 += b1;
            a1 += (c >> 8) & UINT64_C(0x00ff00ff00ff00ff);
            b1 +=  c       & UINT64_C(0x00ff00ff00ff00ff);
        }
        len -= n;
        buf += n;

        s2 += n * s1;
        s1 += ((a1 + b1) * UINT64_C(0x0001000100010001)) >> 48;
        /* Reduce the original expression so that we don't overflow.

           If the original coefficients where kept [(8,6,4,2),(7,5,3,1)],
           the worst case scenario for n is then 7 iterations, which
           is very low.

           Here, the (4,3,2,1) coefficients are the largest ones to be
           applied prior to the final multiplication by 2. So, this
           represents a maximal iteration count of 25 in the accumulator
           loop before a1 and b1 overflow. Since we capped n to 23 to
           avoid a2 and b2 overflows, that's not an issue then */
#ifdef WORDS_BIGENDIAN
        s2 += (((a1 * UINT64_C(0x0001000200030004)) >> 48) * 2 +
               ((b1 * UINT64_C(0x0000000100020003)) >> 48) * 2 +
               ((b1 * UINT64_C(0x0001000100010001)) >> 48));
#else
        s2 += (((b1 * UINT64_C(0x0004000300020001)) >> 48) * 2 +
               ((a1 * UINT64_C(0x0003000200010000)) >> 48) * 2 +
               ((a1 * UINT64_C(0x0001000100010001)) >> 48));
#endif
        s2 += (((((a2 >> 16) & UINT64_C(0x0000ffff0000ffff)) +
                 (a2 & UINT64_C(0x0000ffff0000ffff))) +
                (((b2 >> 16) & UINT64_C(0x0000ffff0000ffff)) +
                 (b2 & UINT64_C(0x0000ffff0000ffff)))) *
               UINT64_C(0x0000000800000008)) >> 32;
        s1 %= ADLER32_BASE;
        s2 %= ADLER32_BASE;
    }

    while (len > 0) {
        DO1(buf, 0);
        s1 %= ADLER32_BASE;
        s2 %= ADLER32_BASE;
        buf++; len--;
    }
    ADLER32_PACK(hash, s1, s2);
}

#if (defined(__x86_64__))
typedef long long vec128i __attribute__((__vector_size__(16), __may_alias__));

/* SSSE3 implementation */
static void
OPT_TARGET("ssse3")
adler32_update_ssse3(MvtHashAdler32 *hash, const uint8_t *buf, uint32_t len)
{
    uint32_t s1, s2, rlen;

    if (len == 1) {
        adler32_update_1(hash, buf);
        return;
    }

    ADLER32_UNPACK(hash, s1, s2);

    rlen = (((uintptr_t)buf + 0x0f) & ~0x0fUL) - (uintptr_t)buf;
    if (rlen & 0x01) { DO1(buf, 0); buf++; }
    if (rlen & 0x02) { DO2(buf, 0); buf += 2; }
    if (rlen & 0x04) { DO4(buf, 0); buf += 4; }
    if (rlen & 0x08) { DO8(buf, 0); buf += 8; }

    len -= rlen;
    while (len >= 16) {
        static const uint8_t KSB_E[16] MVT_ALIGNED(16) = // even bytes
            { 0x00, 0x80, 0x02, 0x80, 0x04, 0x80, 0x06, 0x80,
              0x08, 0x80, 0x0a, 0x80, 0x0c, 0x80, 0x0e, 0x80 };
        static const uint8_t KSB_O[16] MVT_ALIGNED(16) = // odd bytes
            { 0x01, 0x80, 0x03, 0x80, 0x05, 0x80, 0x07, 0x80,
              0x09, 0x80, 0x0b, 0x80, 0x0d, 0x80, 0x0f, 0x80 };
        static const uint8_t ZXW_L[16] MVT_ALIGNED(16) = // lower half words
            { 0x00, 0x01, 0x80, 0x80, 0x02, 0x03, 0x80, 0x80,
              0x04, 0x05, 0x80, 0x80, 0x06, 0x07, 0x80, 0x80 };
        static const uint8_t ZXW_U[16] MVT_ALIGNED(16) = // upper half words
            { 0x08, 0x09, 0x80, 0x80, 0x0a, 0x0b, 0x80, 0x80,
              0x0c, 0x0d, 0x80, 0x80, 0x0e, 0x0f, 0x80, 0x80 };
        static const uint16_t M_A1[16] MVT_ALIGNED(16) = // coeffs for a1
            { 0x0f, 0x0d, 0x0b, 0x09, 0x07, 0x05, 0x03, 0x01 };
        static const uint16_t M_B1[16] MVT_ALIGNED(16) = // coeffs for b1
            { 0x10, 0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02 };

        const uintptr_t n = MVT_MIN((len & ~0x0f), 23 * 16);
        vec128i a1, a2, b1, b2, v1, v2, z0, z1;
        uintptr_t i;
        uint32_t r1, r2;

        asm volatile(
            "   pxor    %[a1], %[a1]\n"
            "   pxor    %[a2], %[a2]\n"
            "   pxor    %[b1], %[b1]\n"
            "   pxor    %[b2], %[b2]\n"
            "   xor     %[i], %[i]\n"
            "   movdqa  %[KSB_E], %[z0]\n"
            "   movdqa  %[KSB_O], %[z1]\n"
            "0:\n"
            "   movdqa (%[buf],%[i],1), %[v1]\n"
            "   paddw   %[a1], %[a2]\n"
            "   movdqa  %[v1], %[v2]\n"
            "   paddw   %[b1], %[b2]\n"
            "   pshufb  %[z1], %[v1]\n"
            "   pshufb  %[z0], %[v2]\n"
            "   paddw   %[v1], %[a1]\n"
            "   paddw   %[v2], %[b1]\n"
            "   addl    $16, %k[i]\n"
            "   cmpl    %k[n], %k[i]\n"
            "   jl      0b\n"
            "   subl    %k[n], %k[len]\n"
            "   add     %[n], %[buf]\n"

            /* v1 = a1 + a2 */
            "   movdqa  %[a1], %[v1]\n"
            "   movdqa  %[ZXW_L], %[z0]\n"
            "   phaddw  %[b1], %[v1]\n"
            //  reduction in epilogue

            /* v2 = [M_A1=(1,3,5,7,9,11,13,15)]*a1 + [M_A2=(2,4,6,8,10,12,14,16)]*b1 */
            "   movdqa  %[b1], %[v2]\n"
            "   pmaddwd %[M_A1], %[a1]\n"
            "   pmaddwd %[M_B1], %[v2]\n"
            "   movdqa  %[ZXW_U], %[z1]\n"
            "   phaddd  %[a1], %[v2]\n"
            //  reduction in epilogue

            /* 16*(a2 + b2) without overflows */
            "   movdqa  %[a2], %[a1]\n"
            "   movdqa  %[b2], %[b1]\n"
            "   pshufb  %[z1], %[a1]\n"
            "   pshufb  %[z1], %[b1]\n"
            "   pshufb  %[z0], %[a2]\n"
            "   pshufb  %[z0], %[b2]\n"
            "   phaddd  %[b1], %[a1]\n"
            "   phaddd  %[b2], %[a2]\n"
            "   phaddd  %[a2], %[a1]\n"
            "   phaddw  %[v1], %[v1]\n"
            "   pslld   $4, %[a1]\n"

            /* Epilogue: reduction to (r1, r2) */
            "   phaddw  %[v1], %[v1]\n"
            "   phaddd  %[a1], %[v2]\n"
            "   pshufb  %[z0], %[v1]\n"
            "   phaddd  %[v2], %[v2]\n"
            "   phaddd  %[v1], %[v1]\n"
            "   phaddd  %[v2], %[v2]\n"
            "   movd    %[v1], %[r1]\n"
            "   movd    %[v2], %[r2]\n"
            : [i] "=&r" (i),
              [buf] "+r" (buf), [len] "+r" (len),
              [r1] "=r"  (r1), [r2] "=r"  (r2),
              [v1] "=&x" (v1), [v2] "=&x" (v2),
              [z0] "=&x" (z0), [z1] "=&x" (z1),
              [a1] "=&x" (a1), [a2] "=&x" (a2),
              [b1] "=&x" (b1), [b2] "=&x" (b2)
            : [n] "r" (n),
              [KSB_O] "m" (*(vec128i *)KSB_O),
              [KSB_E] "m" (*(vec128i *)KSB_E),
              [ZXW_L] "m" (*(vec128i *)ZXW_L),
              [ZXW_U] "m" (*(vec128i *)ZXW_U),
              [M_A1]  "m" (*(vec128i *)M_A1),
              [M_B1]  "m" (*(vec128i *)M_B1));

        s2 += n * s1 + r2;
        s1 += r1;
        s1 %= ADLER32_BASE;
        s2 %= ADLER32_BASE;
    }

    while (len > 0) {
        DO1(buf, 0);
        s1 %= ADLER32_BASE;
        s2 %= ADLER32_BASE;
        buf++; len--;
    }
    ADLER32_PACK(hash, s1, s2);
}
#endif

const MvtHashClass *
mvt_hash_class_adler32(void)
{
    static bool g_klass_initialized;
    static MvtHashClass g_klass = {
        .size           = sizeof(MvtHashAdler32),
        .value_length   = 4,
        .op_init        = (MvtHashInitFunc)adler32_init,
        .op_finalize    = (MvtHashFinalizeFunc)adler32_finalize,
        .op_update      = (MvtHashUpdateFunc)adler32_update_c,
    };

    if (!g_klass_initialized) {
#if (defined(__x86_64__) || defined(__i386__))
        /* Always enable SWAR optimizations on Intel Architectures */
        g_klass.op_update = (MvtHashUpdateFunc)adler32_update_c_swar;
#endif
#if (defined(__x86_64__))
        if (TestCpuFlag(kCpuHasSSSE3))
            g_klass.op_update = (MvtHashUpdateFunc)adler32_update_ssse3;
#endif
        g_klass_initialized = true;
    }
    return &g_klass;
}
