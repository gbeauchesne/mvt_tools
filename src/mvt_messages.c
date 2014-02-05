/*
 * mvt_messages.c - Utility functions used for logging (printing messages)
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
#include <stdarg.h>
#include "mvt_messages.h"

#define DEFINE_LOGGER(NAME, PREFIX, STREAM, CODE)       \
void                                                    \
NAME(const char *format, ...)                           \
{                                                       \
    va_list args;                                       \
                                                        \
    fprintf(STREAM, PREFIX ": ");                       \
    va_start(args, format);                             \
    vfprintf(STREAM, format, args);                     \
    va_end(args);                                       \
    fprintf(STREAM, "\n");                              \
    CODE;                                               \
}

DEFINE_LOGGER(mvt_warning,      "warning",      stdout, )
DEFINE_LOGGER(mvt_error,        "error",        stderr, )
DEFINE_LOGGER(mvt_fatal_error,  "fatal error",  stderr, exit(1))

#undef DEFINE_LOGGER
