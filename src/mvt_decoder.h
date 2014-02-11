/*
 * mvt_decoder.h - Decoder module
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

#ifndef MVT_DECODER_H
#define MVT_DECODER_H

#include "mvt_hash.h"
#include "mvt_report.h"
#include "mvt_codec.h"

MVT_BEGIN_DECLS

#define MVT_DECODER(dec) \
    ((MvtDecoder *)(dec))
#define MVT_DECODER_CLASS(klass) \
    ((MvtDecoderClass *)(klass))
#define MVT_DECODER_GET_CLASS(dec) \
    (mvt_decoder_class())

/** Hardware acceleration modes */
typedef enum {
    MVT_HWACCEL_NONE = 0,       ///< No hardware acceleration (software only)
    MVT_HWACCEL_VAAPI,          ///< Hardware acceleration through VA-API
} MvtHwaccel;

/** Determines the hwaccel id from the supplied name */
MvtHwaccel
mvt_hwaccel_from_name(const char *name);

/** Determines the hwaccel name from the supplied hash id */
const char *
mvt_hwaccel_to_name(MvtHwaccel hwaccel);

/** Decoder options passed on to the command line */
typedef struct {
    char *filename;             ///< Input filename
    char *config_filename;      ///< Filename of the generated test config
    char *report_filename;      ///< Report filename
    MvtHashType hash_type;      ///< Codec hash type to use
    MvtHwaccel hwaccel;         ///< Hardware acceleration mode
} MvtDecoderOptions;

/** Base decoder object */
typedef struct {
    MvtDecoderOptions options;  ///< Decoder options (parsed)
    MvtHash *hash;              ///< Codec hash to use
    MvtReport *report;          ///< Report (per-frame test results)
    MvtCodec codec;             ///< Identified codec
    int profile;                ///< Identified profile
    uint32_t max_width;         ///< Max decoded width in pixels
    uint32_t max_height;        ///< Max decoded height in pixels
} MvtDecoder;

typedef bool (*MvtDecoderInitFunc)(MvtDecoder *decoder);
typedef void (*MvtDecoderFinalizeFunc)(MvtDecoder *decoder);
typedef bool (*MvtDecoderRunFunc)(MvtDecoder *decoder);

/** Decoder object class */
typedef struct {
    uint32_t size;
    MvtDecoderInitFunc init;
    MvtDecoderFinalizeFunc finalize;
    MvtDecoderRunFunc run;
} MvtDecoderClass;

const MvtDecoderClass *
mvt_decoder_class(void);

/** Hashes the supplied image and reports result */
bool
mvt_decoder_handle_image(MvtDecoder *decoder, MvtImage *image, uint32_t flags);

MVT_END_DECLS

#endif /* MVT_DECODER_H */
