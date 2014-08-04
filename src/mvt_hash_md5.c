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

#if USE_FFMPEG
# include <libavutil/md5.h>
#endif

typedef struct {
    MvtHash     base;
#if USE_FFMPEG
    struct AVMD5 *avctx;
#endif
} MvtHashMD5;

static bool
md5_init(MvtHashMD5 *hash)
{
#if USE_FFMPEG
    hash->avctx = malloc(av_md5_size);
    if (!hash->avctx)
        return false;
    return true;
#else
    /* XXX: unimplemented */
    assert(0 && "unimplemented MD5 hashing");
    return false;
#endif
}

static void
md5_finalize(MvtHashMD5 *hash)
{
#if USE_FFMPEG
    free(hash->avctx);
    hash->avctx = NULL;
#endif
}

static void
md5_op_init(MvtHashMD5 *hash)
{
#if USE_FFMPEG
    av_md5_init(hash->avctx);
#endif
}

static void
md5_op_finalize(MvtHashMD5 *hash)
{
#if USE_FFMPEG
    av_md5_final(hash->avctx, hash->base.value);
#endif
}

static void
md5_op_update(MvtHashMD5 *hash, const uint8_t *buf, uint32_t len)
{
#if USE_FFMPEG
    av_md5_update(hash->avctx, buf, len);
#endif
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
