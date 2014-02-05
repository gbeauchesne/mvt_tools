/*
 * sysdeps.h - System-dependent definitions
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

#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "mvt_macros.h"
#include "mvt_messages.h"

/* Visibility attributes */
#if defined __GNUC__ && __GNUC__ >= 4
# define DLL_PUBLIC __attribute__((visibility("default")))
# define DLL_HIDDEN __attribute__((visibility("hidden")))
#else
# define DLL_PUBLIC
# define DLL_HIDDEN
#endif

#endif /* SYSDEPS_H */
