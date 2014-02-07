/*
 * mvt_hash_md5.c - Hash functions used in MVT (MD5)
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
} MvtHashMD5;

static bool
md5_init(MvtHashMD5 *hash)
{
    /* XXX: unimplemented */
    assert(0 && "unimplemented MD5 hashing");
    return false;
}

static void
md5_finalize(MvtHashMD5 *hash)
{
}

static void
md5_op_init(MvtHashMD5 *hash)
{
}

static void
md5_op_finalize(MvtHashMD5 *hash)
{
}

static void
md5_op_update(MvtHashMD5 *hash, const uint8_t *buf, uint32_t len)
{
}

const MvtHashClass *
mvt_hash_class_md5(void)
{
    static const MvtHashClass g_klass = {
        .size           = sizeof(MvtHashMD5),
        .value_length   = 16,
        .init           = (MvtHashInitFunc)md5_init,
        .finalize       = (MvtHashFinalizeFunc)md5_finalize,
        .op_init        = (MvtHashInitFunc)md5_op_init,
        .op_finalize    = (MvtHashFinalizeFunc)md5_op_finalize,
        .op_update      = (MvtHashUpdateFunc)md5_op_update,
    };
    return &g_klass;
}
