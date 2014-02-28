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
        g_klass_initialized = true;
    }
    return &g_klass;
}
