/*
 * mvt_decoder.c - Decoder module
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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include <getopt.h>
#include "mvt_decoder.h"
#include "mvt_map.h"

// Default hash function
#define DEFAULT_HASH MVT_HASH_TYPE_ADLER32

// Default hardware acceleration mode
#define DEFAULT_HWACCEL MVT_HWACCEL_NONE

static const MvtMap hwaccel_map[] = {
    { "none",   MVT_HWACCEL_NONE    },
    { "vaapi",  MVT_HWACCEL_VAAPI   },
    { NULL, }
};

// Determines the hwaccel id from the supplied name
MvtHwaccel
mvt_hwaccel_from_name(const char *name)
{
    return mvt_map_lookup(hwaccel_map, name);
}

// Determines the hwaccel name from the supplied hash id
const char *
mvt_hwaccel_to_name(MvtHwaccel hwaccel)
{
    return mvt_map_lookup_value(hwaccel_map, hwaccel);
}

// Initializes the decoder options to their defaults
void
mvt_decoder_options_init(MvtDecoderOptions *options)
{
    memset(options, 0, sizeof(*options));
    options->hash_type = DEFAULT_HASH;
    options->hwaccel = DEFAULT_HWACCEL;
}

// Clears the decoder options
static void
mvt_decoder_options_clear(MvtDecoderOptions *options)
{
    free(options->filename);
    free(options->report_filename);
    memset(options, 0, sizeof(*options));
}

static const char *
get_basename(const char *filename)
{
    const char * const s = strrchr(filename, '/');

    return s ? s + 1 : filename;
}

static void
print_help(const char *prog)
{
    printf("Usage: %s [<option>]* <video>\n", get_basename(prog));
    printf("\n");
    printf("Options:\n");
    printf("  %-28s  display this help and exit\n",
           "-h, --help");
    printf("  %-28s  define the hash function (default: %s)\n",
           "-c, --checksum=HASH", mvt_hash_type_to_name(DEFAULT_HASH));
    printf("  %-28s  enable hardware acceleration (default: %s)\n",
           "    --hwaccel=API", mvt_hwaccel_to_name(DEFAULT_HWACCEL));
    printf("  %-28s  define the report filename (default: stdout)\n",
           "-r, --report=PATH");

    exit(EXIT_FAILURE);
}

static void
mvt_decoder_free(MvtDecoder *decoder)
{
    const MvtDecoderClass * const klass = mvt_decoder_class();

    if (!decoder)
        return;

    if (klass->finalize)
        klass->finalize(decoder);
    if (decoder->report)
        mvt_report_free(decoder->report);
    if (decoder->hash)
        mvt_hash_free(decoder->hash);
    mvt_decoder_options_clear(&decoder->options);
    free(decoder);
}

static MvtDecoder *
mvt_decoder_new(void)
{
    const MvtDecoderClass * const klass = mvt_decoder_class();
    MvtDecoder *decoder;

    mvt_return_val_if_fail(klass->size >= sizeof(*decoder), NULL);

    decoder = calloc(1, klass->size);
    if (!decoder)
        return NULL;

    decoder->profile = -1;
    mvt_decoder_options_init(&decoder->options);
    return decoder;
}

static bool
mvt_decoder_init_options(MvtDecoder *decoder, int argc, char *argv[])
{
    MvtDecoderOptions * const options = &decoder->options;

    enum {
        OPT_HWACCEL = 1000,
        OPT_VAAPI,
    };

    static const struct option long_options[] = {
        { "help",       no_argument,        NULL, 'h'           },
        { "checksum",   required_argument,  NULL, 'c'           },
        { "hwaccel",    required_argument,  NULL, OPT_HWACCEL   },
        { "vaapi",      no_argument,        NULL, OPT_VAAPI     },
        { "report",     required_argument,  NULL, 'r'           },
        { NULL, }
    };

    for (;;) {
        int v = getopt_long(argc, argv, "-hc:r:", long_options, NULL);
        if (v < 0)
            break;

        switch (v) {
        case '?':
            return false;
        case 'h':
            print_help(argv[0]);
            break;
        case 'c':
            options->hash_type = mvt_hash_type_from_name(optarg);
            if (!options->hash_type)
                goto error_invalid_hash;
            break;
        case OPT_HWACCEL:
            options->hwaccel = mvt_hwaccel_from_name(optarg);
            break;
        case OPT_VAAPI:
            options->hwaccel = MVT_HWACCEL_VAAPI;
            break;
        case '\1':
            free(options->filename);
            options->filename = strdup(optarg);
            if (!options->filename)
                goto error_alloc_memory;
            break;
        case 'r':
            free(options->report_filename);
            options->report_filename = strdup(optarg);
            if (!options->report_filename)
                goto error_alloc_memory;
            break;
        default:
            break;
        }
    }
    return true;

    /* ERRORS */
error_alloc_memory:
    mvt_error("failed to allocate memory");
    return false;
error_invalid_hash:
    mvt_error("invalid hash name ('%s')", optarg);
    return false;
}

static bool
mvt_decoder_init(MvtDecoder *decoder, int argc, char *argv[])
{
    const MvtDecoderClass * const klass = mvt_decoder_class();
    const MvtDecoderOptions * const options = &decoder->options;

    if (!mvt_decoder_init_options(decoder, argc, argv))
        return false;
    if (!options->filename)
        goto error_no_filename;

    decoder->report = mvt_report_new(options->report_filename);
    if (!decoder->report)
        goto error_init_report;

    decoder->hash = mvt_hash_new(options->hash_type);
    if (!decoder->hash)
        goto error_init_hash;

    return !klass->init || klass->init(decoder);

    /* ERRORS */
error_no_filename:
    mvt_error("no filename provided on the command line");
    return false;
error_init_report:
    mvt_error("failed to initialize report file");
    return false;
error_init_hash:
    mvt_error("failed to initialize hash");
    return false;
}

static bool
mvt_decoder_run(MvtDecoder *decoder)
{
    const MvtDecoderClass * const klass = mvt_decoder_class();

    return !klass->run || klass->run(decoder);
}

// Hashes the supplied image and reports result
bool
mvt_decoder_handle_image(MvtDecoder *decoder, MvtImage *image, uint32_t flags)
{
    if (!mvt_image_hash(image, decoder->hash))
        return false;
    mvt_report_write_image_hash(decoder->report, image, decoder->hash);
    return true;
}

int
main(int argc, char *argv[])
{
    MvtDecoder *decoder;
    bool success = false;

    decoder = mvt_decoder_new();
    if (!decoder || !mvt_decoder_init(decoder, argc, argv))
        goto cleanup;
    success = mvt_decoder_run(decoder);

cleanup:
    if (decoder)
        mvt_decoder_free(decoder);
    return !success;
}
