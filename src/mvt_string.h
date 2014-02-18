/*
 * mvt_string.h - String utilities
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

#ifndef MVT_STRING_H
#define MVT_STRING_H

MVT_BEGIN_DECLS

/**
 * \brief Frees array of strings.
 *
 * Frees a \c NULL terminated array of strings, and the \ref str_array
 * memory itself. If \ref str_array is \c NULL, then str_freev() is a
 * no-op and simply returns.
 *
 * @param[in] str_array the array of strings to free
 */
void
str_freev(char **str_array);

/**
 * \brief Frees array of strings and resets pointer to NULL.
 *
 * Frees a \c NULL terminated array of strings, and the memory pointed
 * to by \ref str_array_ptr. If \ref str_array_ptr is \c NULL, then
 * str_freevp() is a no-op and simply returns.
 *
 * @param[in,out] str_array_ptr the pointer to the array of strings
 */
void
str_freevp(char ***str_array_ptr);

/**
 * \brief Duplicates a string.
 *
 * Duplicates a string. If \ref str is \c NULL, the function returns
 * \c NULL. The returned string should be deallocated with free() when
 * it is no longer needed.
 *
 * @param[in] str       the string to duplicate
 * @return a newly allocated copy of \ref str
 */
char *
str_dup(const char *str);

/**
 * \brief Duplicates a string, capped by the supplied number of characters.
 *
 * Duplicates a string, up to the specified number \ref len of
 * characters. If \ref str is \c NULL, the function returns \c
 * NULL. The returned string should be deallocated with free() when it
 * is no longer needed.
 *
 * @param[in] str       the string to duplicate
 * @param[in] len       the maximum number of characters to copy
 * @return a newly allocated copy of \ref str
 */
char *
str_dup_n(const char *str, size_t len);

/**
 * \brief Generates a newly allocated string with printf()-like format.
 *
 * This function is similar to sprintf() but it calculates the maximum
 * space required and allocates memory to hold the result. The
 * returned string should be deallocated with free() when it is no
 * longer needed.
 *
 * @param[in] format    a standard printf() format string
 * @param[in] ...       the parameters to insert into the formatted string
 * @return the newly allocated string result, or \ref NULL if an error occurred
 */
char *
str_dup_printf(const char *format, ...);

/**
 * \brief Checks whether a a string starts with a certain prefix.
 *
 * This function checks whether the string \ref str begins with the
 * specified string \ref prefix.
 *
 * @param[in] str       a nul-terminated string
 * @param[in] prefix    the nul-terminated prefix to look for
 * @return \c true if the string begins with the supplied prefix string
 */
bool
str_has_prefix(const char *str, const char *prefix);

/**
 * \brief Parses an unsigned integer value.
 *
 * This function parses an unsigned integer value from a string, and
 * with the supplied base. A base of 0 will handle hexadecimal (base
 * 16) or octal (base 8) integer values indifferently.
 *
 * @param[in] str       a nul-terminated string
 * @param[out] value_ptr the pointer to the resulting integer value
 * @param[in] base      the desired base
 * @return \c true if the string was correctly and fully parsed
 */
bool
str_parse_uint(const char *str, unsigned int *value_ptr, int base);

/**
 * \brief Parses a pair of unsigned integers representing a size.
 *
 * This function parses a pair of unsigned integers meant to represent
 * a size. The format is <WIDTH> 'x' <HEIGHT>. Only decimal values
 * (base 10) are supported.
 *
 * @param[in]   str             a nul-terminated string
 * @param[out]  width_ptr       a pointer to the resulting width value
 * @param[out]  height_ptr      a pointer to the resulting height value
 * @return \c true if the string was correctly and fully parsed
 */
bool
str_parse_size(const char *str, unsigned int *width_ptr,
    unsigned int *height_ptr);

MVT_END_DECLS

#endif /* MVT_STRING_H */
