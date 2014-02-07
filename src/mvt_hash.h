/*
 * mvt_hash.h - Hash functions used in MVT
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

#ifndef MVT_HASH_H
#define MVT_HASH_H

MVT_BEGIN_DECLS

struct MvtHash_s;
typedef struct MvtHash_s MvtHash;

/** Hash types */
typedef enum {
    MVT_HASH_TYPE_ADLER32 = 1,
    MVT_HASH_TYPE_MD5,
} MvtHashType;

/** Determines the hash type from the supplied name */
MvtHashType
mvt_hash_type_from_name(const char *name);

/** Determines the hash name from the supplied hash id */
const char *
mvt_hash_type_to_name(MvtHashType type);

/** Creates a new hash context of the supplied type */
MvtHash *
mvt_hash_new(MvtHashType type);

/** Deallocates all resources associated with the supplied hash context */
void
mvt_hash_free(MvtHash *hash);

/** Deallocates hash context, if any, and resets the pointer to NULL */
void
mvt_hash_freep(MvtHash **hash_ptr);

/** Initializes or reset a hash context */
void
mvt_hash_init(MvtHash *hash);

/** Finalizes a hash context and compute the resulting hash value */
void
mvt_hash_finalize(MvtHash *hash);

/** Updates the hash context with the supplied data */
void
mvt_hash_update(MvtHash *hash, const uint8_t *buf, uint32_t len);

/** Exposes the hash value */
void
mvt_hash_get_value(MvtHash *hash, const uint8_t **value_ptr, uint32_t *len_ptr);

MVT_END_DECLS

#endif /* MVT_HASH_H */
