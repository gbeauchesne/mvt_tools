/*
 * mvt_report.h - Report generator
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

#ifndef MVT_REPORT_H
#define MVT_REPORT_H

#include "mvt_image.h"

MVT_BEGIN_DECLS

struct MvtReport_s;
typedef struct MvtReport_s MvtReport;

/** Creates a new MvtReport object and opens the associated file for writing */
/* Note: if @filename is NULL, then the standard output will be used */
MvtReport *
mvt_report_new(const char *filename);

/** Releases MvtReport object resources. This flushes and closes report file */
void
mvt_report_free(MvtReport *report);

/** Writes a comment to the report file */
bool
mvt_report_write_comment(MvtReport *report, const char *format, ...);

/** Writes image hash to the report file */
bool
mvt_report_write_image_hash(MvtReport *report, MvtImage *image, MvtHash *hash);

MVT_END_DECLS

#endif /* MVT_REPORT_H */
