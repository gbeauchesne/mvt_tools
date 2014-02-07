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

/* Largest prime number that is smaller than 65536 */
#define ADLER32_BASE 65521

static bool
adler32_init(MvtHashAdler32 *hash)
{
    hash->value = 0;
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

static void
adler32_update(MvtHashAdler32 *hash, const uint8_t *buf, uint32_t len)
{
    uint32_t s1 = hash->value & 0xffff;
    uint32_t s2 = hash->value >> 16;
    uint32_t i;

    for (i = 0; i < len; i++) {
        s1 = (s1 + buf[i]) % ADLER32_BASE;
        s2 = (s2 + s1) % ADLER32_BASE;
    }
    hash->value = (s2 << 16) | s1;
}

const MvtHashClass *
mvt_hash_class_adler32(void)
{
    static const MvtHashClass g_klass = {
        .size           = sizeof(MvtHashAdler32),
        .value_length   = 4,
        .op_init        = (MvtHashInitFunc)adler32_init,
        .op_finalize    = (MvtHashFinalizeFunc)adler32_finalize,
        .op_update      = (MvtHashUpdateFunc)adler32_update,
    };
    return &g_klass;
}
