/*
 * mvt_hash_priv.h - Hash functions used in MVT (private definitions)
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

#ifndef MVT_HASH_PRIV_H
#define MVT_HASH_PRIV_H

#include "mvt_hash.h"

/* Defines the maximum length in bytes of a hash value */
#define MVT_HASH_VALUE_MAX_LENGTH 64

typedef bool (*MvtHashInitFunc)(MvtHash *hash);
typedef void (*MvtHashFinalizeFunc)(MvtHash *hash);
typedef void (*MvtHashUpdateFunc)(MvtHash *hash, const uint8_t *buf,
    uint32_t len);

/* Hash object class */
typedef struct {
    uint32_t            size;
    uint32_t            value_length;
    MvtHashInitFunc     init;
    MvtHashFinalizeFunc finalize;
    MvtHashInitFunc     op_init;
    MvtHashFinalizeFunc op_finalize;
    MvtHashUpdateFunc   op_update;
} MvtHashClass;

/* Private definition of a hash context */
struct MvtHash_s {
    const MvtHashClass *klass;
    uint8_t value[MVT_HASH_VALUE_MAX_LENGTH];
};

DLL_HIDDEN
const MvtHashClass *
mvt_hash_class_adler32(void);

DLL_HIDDEN
const MvtHashClass *
mvt_hash_class_md5(void);

#endif /* MVT_HASH_PRIV_H */
