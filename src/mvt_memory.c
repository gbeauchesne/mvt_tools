/*
 * mvt_memory.c - Memory utilities
 *
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
#include "mvt_memory.h"

#undef mem_reallocp

// Allocates memory aligned on bounardy specified by alignment
void *
mem_alloc_aligned(size_t size, size_t alignment)
{
    void *mem;

    if (posix_memalign(&mem, alignment, size) == 0)
        return mem;
    return NULL;
}

// Reallocates memory to the expected size
static bool
mem_reallocp(void *mem_arg, size_t *size_ptr, size_t new_size)
{
    void ** const mem_ptr = (void **)mem_arg;
    void *new_mem;

    if (!mem_ptr)
        return false;

    new_mem = realloc(*mem_ptr, new_size);
    if (!new_mem)
        return false;

    *mem_ptr = new_mem;
    if (size_ptr)
        *size_ptr = new_size;
    return true;
}

bool
mem_reallocp32(void *mem_arg, uint32_t *size_ptr, size_t new_size)
{
    size_t tmp_size;

    if (sizeof(size_t) == 4)
        return mem_reallocp(mem_arg, (size_t *)size_ptr, new_size);

    if (!mem_reallocp(mem_arg, &tmp_size, new_size))
        return false;

    if (size_ptr)
        *size_ptr = tmp_size;
    return true;
}

bool
mem_reallocp64(void *mem_arg, uint64_t *size_ptr, size_t new_size)
{
    size_t tmp_size;

    if (sizeof(size_t) == 8)
        return mem_reallocp(mem_arg, (size_t *)size_ptr, new_size);

    if (!mem_reallocp(mem_arg, &tmp_size, new_size))
        return false;

    if (size_ptr)
        *size_ptr = tmp_size;
    return true;
}

// Duplicates a memory block.
void *
mem_dup(const void *mem, size_t size)
{
    void *new_mem;

    if (!mem || size == 0)
        return NULL;

    new_mem = malloc(size);
    if (!new_mem)
        return NULL;

    memcpy(new_mem, mem, size);
    return new_mem;
}

// Frees memory and resets the pointer to memory to NULL
void
mem_freep(void *mem_arg)
{
    void ** const mem_ptr = (void **)mem_arg;

    if (!mem_ptr)
        return;

    free(*mem_ptr);
    *mem_ptr = NULL;
}
