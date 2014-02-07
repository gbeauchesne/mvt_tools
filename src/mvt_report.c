/*
 * mvt_report.c - Report generator
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
#include "mvt_report.h"
#include "mvt_hash_priv.h"

struct MvtReport_s {
    FILE *file;
    uint32_t image_index;
    uint32_t warned_image_index : 1;
    uint32_t warned_image_size  : 1;
};

// Creates a new MvtReport object and opens the associated file for writing
MvtReport *
mvt_report_new(const char *filename)
{
    MvtReport *report;

    report = calloc(1, sizeof(*report));
    if (!report)
        return NULL;

    report->file = filename ? fopen(filename, "w") : stdout;
    if (!report->file)
        goto error;
    return report;

error:
    mvt_report_free(report);
    return NULL;
}

// Releases MvtReport object resources. This flushes and closes report file
void
mvt_report_free(MvtReport *report)
{
    if (!report)
        return;

    if (report->file) {
        fflush(report->file);
        if (report->file != stdout)
            fclose(report->file);
        report->file = NULL;
    }
    free(report);
}

// Writes a comment to the report file
bool
mvt_report_write_comment(MvtReport *report, const char *format, ...)
{
    va_list args;
    char *out, *str, *sep, dummy[1];
    int ret, len;
    bool success = false;

    mvt_return_val_if_fail(report != NULL, false);

    va_start(args, format);
    ret = vsnprintf(dummy, sizeof(dummy), format, args);
    va_end(args);
    if (ret < 0)
        return false;

    len = ret + 1;
    str = malloc(len);
    if (!str)
        return false;

    va_start(args, format);
    ret = vsnprintf(str, len, format, args);
    va_end(args);
    if (ret < 0 || ret >= len)
        goto cleanup;

    for (out = str; (sep = strchr(out, '\n')) != NULL; out = sep) {
        *sep++ = '\0';
        fprintf(report->file, "# %s\n", out);
    }
    fprintf(report->file, "# %s\n", out);
    success = true;

cleanup:
    free(str);
    return success;
}

// Writes headers to the report file
static bool
mvt_report_write_headers(MvtReport *report)
{
    mvt_return_val_if_fail(report != NULL, false);

    if (report->image_index > 0)
        return true;
    return mvt_report_write_comment(report, "%5s %10s %-20s",
        "frame", "size", "hash");
}

// Writes image hash to the report file
bool
mvt_report_write_image_hash(MvtReport *report, MvtImage *image, MvtHash *hash)
{
    char value_string[2*MVT_HASH_VALUE_MAX_LENGTH+1], size_string[20];
    const uint8_t *value;
    uint32_t i, value_length;

    mvt_return_val_if_fail(report != NULL, false);
    mvt_return_val_if_fail(image != NULL, false);
    mvt_return_val_if_fail(hash != NULL, false);

    if (!mvt_report_write_headers(report))
        return false;

    if (report->image_index >= 10000000 && !report->warned_image_index) {
        mvt_warning("image index (%u) is too large", report->image_index);
        report->warned_image_index = true;
    }

    // Image size
    if ((image->width >= 10000 || image->height >= 10000) &&
        !report->warned_image_size) {
        mvt_warning("image dimensions (%ux%u) are too large",
            image->width, image->height);
        report->warned_image_size = true;
    }
    sprintf(size_string, "%ux%u", image->width, image->height);

    // Image hash
    mvt_hash_get_value(hash, &value, &value_length);
    if (value_length > MVT_HASH_VALUE_MAX_LENGTH)
        mvt_fatal_error("inconsistent hash value length (%u > max:%d)",
            value_length, MVT_HASH_VALUE_MAX_LENGTH);

    for (i = 0; i < value_length; i++)
        sprintf(&value_string[2*i], "%02x", value[i]);
    value_string[2*value_length] = '\0';

    fprintf(report->file, "%7d %10s 0x%-18s\n", report->image_index,
        size_string, value_string);

    report->image_index++;
    return true;
}
