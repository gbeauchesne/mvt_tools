/*
 * mvt_memory.h - Memory utilities
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

#ifndef MVT_MEMORY_H
#define MVT_MEMORY_H

MVT_BEGIN_DECLS

/**
 * \brief Allocates memory aligned on bounardy specified by alignment
 *
 * Allocates \c size bytes aligned on a boundary specified by the \c
 * alignment argument. The value of \c alignment shall be a multiple
 * of sizeof(void *), that is also a power-of-two.
 *
 * @param[in] size              the minimum size in bytes of the resulting block
 * @param[in] alignment         the required memory alignment
 * @return the newly allocated memory, or \c NULL on error
 */
void *
mem_alloc_aligned(size_t size, size_t alignment);

/**
 * \brief Reallocates memory to the expected size
 *
 * Reallocates the memory pointed by \c *mem_ptr and updates the \c
 * size object on success. If memory allocation fails, the original
 * memory block pointed is preserved and the \c size value is not
 * updated.
 *
 * @param[in,out] mem_ptr       the pointer to memory (any pointer)
 * @param[in,out] size_ptr      the pointer to size value to update on success
 * @param[in] size              the new size in bytes
 * @return \c true on success
 */
#if SIZE_MAX == UINT32_MAX
#define mem_reallocp(mem_ptr, size_ptr, new_size)                       \
    (MVT_LIKELY(sizeof(*(size_ptr)) == 4) ?                             \
     mem_reallocp32((mem_ptr), (uint32_t *)(size_ptr), (new_size)) :    \
     mem_reallocp64((mem_ptr), (uint64_t *)(size_ptr), (new_size)))
#elif SIZE_MAX == UINT64_MAX
#define mem_reallocp(mem_ptr, size_ptr, new_size)                       \
    (MVT_LIKELY(sizeof(*(size_ptr)) == 8) ?                             \
     mem_reallocp64((mem_ptr), (uint64_t *)(size_ptr), (new_size)) :    \
     mem_reallocp32((mem_ptr), (uint32_t *)(size_ptr), (new_size)))
#else
#error "unsupported sizeof(size_t)"
#endif

bool
mem_reallocp32(void *mem_ptr, uint32_t *size_ptr, size_t new_size);

bool
mem_reallocp64(void *mem_ptr, uint64_t *size_ptr, size_t new_size);

/**
 * \brief Duplicates a memory block.
 *
 * Duplicates a memory block. If \ref mem is \c NULL, the function
 * returns \c NULL. The returned memory block should be deallocated
 * with free() when it is no longer needed.
 *
 * @param[in] mem       the memory block to duplicate
 * @param[in] size      the size of the memory block
 * @return a newly allocated copy of \ref mem
 */
void *
mem_dup(const void *mem, size_t size);

/**
 * \brief Frees memory and resets the pointer to memory to NULL.
 *
 * Frees the memory pointed by \c *mem_ptr and resets that pointer to
 * NULL.  \ref mem_ptr is a pointer to the memory block. This is a \c
 * void * type as a convenience to hold a pointer to any memory block
 * (pointer).
 *
 * @param[in,out] mem_ptr       the pointer to memory (any pointer)
 */
void
mem_freep(void *mem_ptr);

MVT_END_DECLS

#endif /* MVT_MEMORY_H */
