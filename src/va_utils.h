/*
 * va_utils.h - VA utilities
 *
 * Copyright (C) 2013 Intel Corporation
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

#ifndef VA_UTILS_H
#define VA_UTILS_H

#include <va/va.h>
#include "va_compat.h"

MVT_BEGIN_DECLS

/** Checks the VA status */
bool
va_check_status(VAStatus va_status, const char *msg);

/** Destroys a VA config */
void
va_destroy_config(VADisplay dpy, VAConfigID *cfg_ptr);

/** Destroys a VA context */
void
va_destroy_context(VADisplay dpy, VAContextID *ctx_ptr);

/** Destroys a VA buffer */
void
va_destroy_buffer(VADisplay dpy, VABufferID *buf_ptr);

/** Destroys an array of VA buffers */
void
va_destroy_buffers(VADisplay dpy, VABufferID *buf, uint32_t *len_ptr);

/** Maps the specified VA buffer */
void *
va_map_buffer(VADisplay dpy, VABufferID buf_id);

/** Unmaps the supplied VA buffer. Sets the (optional) data pointer to NULL */
void
va_unmap_buffer(VADisplay dpy, VABufferID buf_id, void **buf_ptr);

MVT_END_DECLS

#endif /* VA_UTILS_H */
