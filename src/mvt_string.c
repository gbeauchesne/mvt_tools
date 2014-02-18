/*
 * mvt_string.c - String utilities
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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include "mvt_string.h"

// Frees array of strings
void
str_freev(char **str_array)
{
    char **str_ptr;

    if (str_array) {
        for (str_ptr = str_array; *str_ptr != NULL; str_ptr++)
            free(*str_ptr);
        free(str_array);
    }
}

// Frees array of strings and resets pointer to NULL
void
str_freevp(char ***str_array_ptr)
{
    if (!str_array_ptr)
        return;

    str_freev(*str_array_ptr);
    *str_array_ptr = NULL;
}

// Duplicates a string
char *
str_dup(const char *str)
{
    if (!str)
        return NULL;

#if __USE_GNU || (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600)
    return strdup(str);
#else
# error "strdup() is not implemented"
#endif
}

// Duplicates a string, capped by the supplied number of characters
char *
str_dup_n(const char *str, size_t len)
{
    if (!str)
        return NULL;

#if __USE_GNU || (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700)
    return strndup(str, len);
#else
# error "strndup() is not implemented"
#endif
}

// Generates a newly allocated string with printf()-like format
char *
str_dup_printf(const char *format, ...)
{
    va_list args;
    char *out_str;
    int ret;

#if __USE_GNU
    va_start(args, format);
    ret = vasprintf(&out_str, format, args);
    va_end(args);

    if (ret < 0)
        return NULL;
    return out_str;
#else
    char dummy[1];
    int len;

    va_start(args, format);
    ret = vsnprintf(dummy, sizeof(dummy), format, args);
    va_end(args);

    if (ret < 0)
        return NULL;

    len = ret + 1;
    out_str = malloc(len);
    if (!out_str)
        return NULL;

    va_start(args, format);
    ret = vsnprintf(out_str, len, format, args);
    va_end(args);

    if (ret < 0 || ret >= len) {
        free(out_str);
        return NULL;
    }
    return out_str;
#endif
}

// Checks whether a a string starts with a certain prefix
bool
str_has_prefix(const char *str, const char *prefix)
{
    size_t str_len, prefix_len;

    if (!str || !prefix)
        return false;

    str_len = strlen(str);
    prefix_len = strlen(prefix);
    if (prefix_len > str_len)
        return false;
    return strncmp(str, prefix, prefix_len) == 0;
}

// Parses an unsigned integer value
bool
str_parse_uint(const char *str, unsigned int *value_ptr, int base)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(str, &end, base);
    if (!(*str && *end == '\0') || (value == ULONG_MAX && errno == ERANGE))
        return false;

    *value_ptr = value;
    return true;
}

// Parses a pair of unsigned integers representing a size
bool
str_parse_size(const char *str, unsigned int *width_ptr,
    unsigned int *height_ptr)
{
    unsigned int width, height;

    if (sscanf(str, "%ux%u", &width, &height) != 2)
        return false;

    if (width_ptr)
        *width_ptr = width;
    if (height_ptr)
        *height_ptr = height;
    return true;
}
