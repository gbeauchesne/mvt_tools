/*
 * mvt_hash.c - Hash functions used in MVT
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
#include "mvt_map.h"

static const MvtMap hash_types[] = {
    { "adler32",    MVT_HASH_TYPE_ADLER32   },
    { "md5",        MVT_HASH_TYPE_MD5       },
    { NULL, }
};

static bool
check_class(const MvtHashClass *klass)
{
    return klass &&
        (klass->value_length > 0 &&
         klass->value_length < MVT_HASH_VALUE_MAX_LENGTH) &&
        klass->op_init && klass->op_finalize && klass->op_update;
}

// Determines the hash id from the supplied name
MvtHashType
mvt_hash_type_from_name(const char *name)
{
    return mvt_map_lookup(hash_types, name);
}

// Determines the hash name from the supplied hash id
const char *
mvt_hash_type_to_name(MvtHashType type)
{
    return mvt_map_lookup_value(hash_types, type);
}

// Creates a new hash context of the supplied type
MvtHash *
mvt_hash_new(MvtHashType type)
{
    const MvtHashClass *klass;
    MvtHash *hash;

    switch (type) {
    case MVT_HASH_TYPE_ADLER32:
        klass = mvt_hash_class_adler32();
        break;
    case MVT_HASH_TYPE_MD5:
        klass = mvt_hash_class_md5();
        break;
    default:
        klass = NULL;
        break;
    }
    if (!check_class(klass))
        return NULL;

    mvt_return_val_if_fail(klass->size >= sizeof(*hash), NULL);

    hash = calloc(1, klass->size);
    if (!hash)
        return NULL;

    hash->klass = klass;
    if (klass->init && !klass->init(hash))
        goto error;
    return hash;

error:
    mvt_hash_free(hash);
    return NULL;
}

// Deallocates all resources associated with the supplied hash context
void
mvt_hash_free(MvtHash *hash)
{
    if (!hash)
        return;

    if (hash->klass->finalize)
        hash->klass->finalize(hash);
    free(hash);
}

// Deallocates hash context, if any, and resets the pointer to NULL
void
mvt_hash_freep(MvtHash **hash_ptr)
{
    if (hash_ptr) {
        mvt_hash_free(*hash_ptr);
        *hash_ptr = NULL;
    }
}

// Initializes or reset a hash context
void
mvt_hash_init(MvtHash *hash)
{
    mvt_return_if_fail(hash != NULL);

    hash->klass->op_init(hash);
}

// Finalizes a hash context and compute the resulting hash value
void
mvt_hash_finalize(MvtHash *hash)
{
    mvt_return_if_fail(hash != NULL);

    hash->klass->op_finalize(hash);
}

// Updates the hash context with the supplied data
void
mvt_hash_update(MvtHash *hash, const uint8_t *buf, uint32_t len)
{
    mvt_return_if_fail(hash != NULL);

    if (!buf || len < 1)
        return;
    hash->klass->op_update(hash, buf, len);
}

// Exposes the hash value
void
mvt_hash_get_value(MvtHash *hash, const uint8_t **value_ptr, uint32_t *len_ptr)
{
    mvt_return_if_fail(hash != NULL);

    if (value_ptr)
        *value_ptr = hash->value;
    if (len_ptr)
        *len_ptr = hash->klass->value_length;
}
