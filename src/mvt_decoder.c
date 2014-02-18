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
    free(options->config_filename);
    free(options->report_filename);
    free(options->output_filename);
    memset(options, 0, sizeof(*options));
}

static const char *
get_basename(const char *filename)
{
    const char * const s = strrchr(filename, '/');

    return s ? s + 1 : filename;
}

static bool
is_dev_null(const char *filename)
{
    return filename && strcmp(filename, "/dev/null") == 0;
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
    printf("  %-28s  define the config filename (default: stdout)\n",
           "    --gen-config[=PATH]");
    printf("  %-28s  define the output filename (default: <video>.raw)\n",
           "    --gen-output[=PATH]");
    printf("  %-28s  enable benchmark mode (decode only)\n",
           "    --benchmark");

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
    if (decoder->output_file)
        mvt_image_file_close(decoder->output_file);
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
    mvt_image_info_init_defaults(&decoder->output_info);
    return decoder;
}

static bool
mvt_decoder_init_options(MvtDecoder *decoder, int argc, char *argv[])
{
    MvtDecoderOptions * const options = &decoder->options;
    bool gen_output = false;

    enum {
        OPT_HWACCEL = 1000,
        OPT_VAAPI,
        OPT_GEN_CONF,
        OPT_BENCHMARK,
    };

    static const struct option long_options[] = {
        { "help",       no_argument,        NULL, 'h'           },
        { "checksum",   required_argument,  NULL, 'c'           },
        { "hwaccel",    required_argument,  NULL, OPT_HWACCEL   },
        { "vaapi",      no_argument,        NULL, OPT_VAAPI     },
        { "report",     required_argument,  NULL, 'r'           },
        { "gen-config", optional_argument,  NULL, OPT_GEN_CONF  },
        { "gen-output", optional_argument,  NULL, 'o'           },
        { "benchmark",  no_argument,        NULL, OPT_BENCHMARK },
        { NULL, }
    };

    for (;;) {
        int v = getopt_long(argc, argv, "-hc:r:o:", long_options, NULL);
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
        case OPT_GEN_CONF:
            free(options->config_filename);
            options->config_filename = strdup(optarg ? optarg : "-");
            if (!options->config_filename)
                goto error_alloc_memory;
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
        case 'o':
            free(options->output_filename);
            if (optarg) {
                options->output_filename = strdup(optarg);
                if (!options->output_filename)
                    goto error_alloc_memory;
            }
            gen_output = true;
            break;
        case OPT_BENCHMARK:
            options->benchmark = true;
            break;
        default:
            break;
        }
    }

    if (gen_output && !options->output_filename && options->filename) {
        const char *filename = get_basename(options->filename);
        const size_t len = strlen(filename) + 4 /* ".raw" */ + 1;

        options->output_filename = malloc(len);
        if (!options->output_filename)
            goto error_alloc_memory;
        options->output_filename[0] = '\0';

        strcpy(options->output_filename, filename);
        strcat(options->output_filename, ".raw");
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

    if (!is_dev_null(options->report_filename)) {
        decoder->report = mvt_report_new(options->report_filename);
        if (!decoder->report)
            goto error_init_report;

        decoder->hash = mvt_hash_new(options->hash_type);
        if (!decoder->hash)
            goto error_init_hash;
    }

    if (options->output_filename && !is_dev_null(options->output_filename)) {
        decoder->output_file = mvt_image_file_open(options->output_filename,
            MVT_IMAGE_FILE_MODE_WRITE);
        if (!decoder->output_file)
            goto error_open_output_file;
    }
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
error_open_output_file:
    mvt_error("failed to open raw decoded output file `%s'",
        options->output_filename);
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
    const MvtDecoderOptions * const options = &decoder->options;

    if (decoder->max_width < image->width)
        decoder->max_width = image->width;
    if (decoder->max_height < image->height)
        decoder->max_height = image->height;

    if (options->benchmark)
        goto done;

    if (decoder->hash && decoder->report) {
        if (!mvt_image_hash(image, decoder->hash))
            return false;
        mvt_report_write_image_hash(decoder->report, image, decoder->hash, 0);
    }

    if (decoder->output_file) {
        if (decoder->num_frames == 0) {
            MvtImageInfo * const info = &decoder->output_info;
            bool image_changed;

            image_changed = info->format != image->format ||
                info->width != image->width || info->height != image->height;
            if (image_changed) {
                const MvtImageInfo old_info = *info;

                mvt_image_info_init(info, image->format,
                    image->width, image->height);
                info->fps_n = old_info.fps_n;
                info->fps_d = old_info.fps_d;
                info->par_n = old_info.par_n;
                info->par_d = old_info.par_d;
            }
            if (!mvt_image_file_write_headers(decoder->output_file, info))
                return false;
        }
        if (!mvt_image_file_write_image(decoder->output_file, image))
            return false;
    }

done:
    decoder->num_frames++;
    return true;
}

// Dumps test config
static bool
mvt_decoder_dump_config(MvtDecoder *decoder)
{
    const MvtDecoderOptions * const options = &decoder->options;
    FILE *out;
    const char *str;
    const uint8_t *value;
    uint32_t i, value_length;
    MvtHash *hash;
    bool success = true;

    if (options->benchmark)
        return true;
    if (!options->config_filename || is_dev_null(options->config_filename))
        return true;

    out = options->config_filename[0] == '-' ? stdout :
        fopen(options->config_filename, "w");
    if (!out) {
        mvt_error("failed to create config file `%s'",
            out != stdout ? "<stdout>" : options->filename);
        goto cleanup;
    }

    fprintf(out, "#!/bin/sh\n");
    fprintf(out, "# This file is part of the Media Validation Tools (MVT)\n");
    fprintf(out, "FILE='%s'\n", get_basename(options->filename));

    hash = mvt_hash_file(MVT_HASH_TYPE_MD5, options->filename);
    if (!hash) {
        mvt_error("failed to compute hash of file `%s'", options->filename);
        goto cleanup;
    }
    mvt_hash_get_value(hash, &value, &value_length);
    fprintf(out, "FILE_HASH='");
    for (i = 0; i < value_length; i++)
        fprintf(out, "%02x", value[i]);
    fprintf(out, "'\n");
    mvt_hash_freep(&hash);
    fprintf(out, "\n");

    if (!decoder->codec || !(str = mvt_codec_to_name(decoder->codec))) {
        mvt_error("invalid codec (%d)", decoder->codec);
        goto cleanup;
    }
    fprintf(out, "CODEC='%s'\n", str);

    if (decoder->profile != -1) {
        str = mvt_profile_to_name(decoder->codec, decoder->profile);
        if (str)
            fprintf(out, "CODEC_PROFILE='%s'\n", str);
    }
    fprintf(out, "CODEC_HASH='%s'\n", mvt_hash_type_to_name(options->hash_type));
    fprintf(out, "CODEC_MAX_WIDTH=%u\n", decoder->max_width);
    fprintf(out, "CODEC_MAX_HEIGHT=%u\n", decoder->max_height);
    success = true;

cleanup:
    if (out != stdout)
        fclose(out);
    return success;
}

int
main(int argc, char *argv[])
{
    MvtDecoder *decoder;
    bool success = false;

    decoder = mvt_decoder_new();
    if (!decoder || !mvt_decoder_init(decoder, argc, argv))
        goto cleanup;
    if (!mvt_decoder_run(decoder))
        goto cleanup;
    mvt_decoder_dump_config(decoder);
    success = true;

cleanup:
    if (decoder)
        mvt_decoder_free(decoder);
    return !success;
}
