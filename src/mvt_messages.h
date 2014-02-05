/*
 * mvt_messages.h - Utility functions used for logging (printing messages)
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

#ifndef MVT_MESSAGES_H
#define MVT_MESSAGES_H

void
mvt_warning(const char *format, ...);

void
mvt_error(const char *format, ...);

void
mvt_fatal_error(const char *format, ...);

#define mvt_return_if_fail(expr) do {           \
        if (!(expr))                            \
            return;                             \
    } while (0)

#define mvt_return_val_if_fail(expr, val) do {  \
        if (!(expr))                            \
            return (val);                       \
    } while (0)

#endif /* MVT_MESSAGES_H */
