/*
 * mvt_mac.h - Basic string/integer map functions
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
#include "mvt_map.h"

// Looks up value by name in the supplied map table
int
mvt_map_lookup(const MvtMap *map, const char *name)
{
    const MvtMap *m;

    assert(map != NULL);

    for (m = map; m->name != NULL; m++) {
        if (!strcmp(name, m->name))
            return m->value;
    }
    return m->value;
}

// Looks up name of value in the supplied map table
const char *
mvt_map_lookup_value(const MvtMap *map, int value)
{
    const MvtMap *m;

    assert(map != NULL);

    for (m = map; m->name != NULL; m++) {
        if (value == m->value)
            return m->name;
    }
    return NULL;
}
