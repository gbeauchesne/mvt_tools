/*
 * mvt_display.h - VA display abstraction
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

#ifndef MVT_DISPLAY_H
#define MVT_DISPLAY_H

#include <va/va.h>

/** Base display object */
typedef struct {
    void *native_display;
    VADisplay va_display;
    char *display_name;
} MvtDisplay;

/** Creates a new display object and opens a connection to the native display */
MvtDisplay *
mvt_display_new(const char *name);

/** Closes the native display and deallocates all resources from MvtDisplay */
void
mvt_display_free(MvtDisplay *display);

/** Releases MvtDisplay object and resets the supplied pointer to NULL */
void
mvt_display_freep(MvtDisplay **display_ptr);

#endif /* MVT_DISPLAY_H */
