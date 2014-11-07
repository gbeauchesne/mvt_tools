/*
 * cmp_video.c - Raw video decoder
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
#include "mvt_image_file.h"
#include "mvt_image_compare.h"
#include "mvt_map.h"

// Default image quality metric
#define DEFAULT_METRIC MVT_IMAGE_QUALITY_METRIC_PSNR

static const MvtMap image_qm_map[] = {
    { "psnr",   MVT_IMAGE_QUALITY_METRIC_PSNR   },
    { "y_psnr", MVT_IMAGE_QUALITY_METRIC_Y_PSNR },
    { NULL, }
};

typedef struct {
    char *filename;
    MvtImageFile *file;
    MvtImageInfo image_info;
    MvtImage *image;
} VideoStream;

typedef struct {
    MvtImageQualityMetric metric;
    VideoStream src_video;
    VideoStream ref_video;
    bool calc_average;
} App;

static App g_app;

static const char *
get_basename(const char *filename)
{
    const char * const s = strrchr(filename, '/');

    return s ? s + 1 : filename;
}

static void
print_help(const char *prog)
{
    printf("Usage: %s [<option>]* -r <ref_video> <video>\n",
           get_basename(prog));
    printf("\n");
    printf("Options:\n");
    printf("  %-28s  display this help and exit\n",
           "-h, --help");
    printf("  %-28s  define the reference video file in Y4M format\n",
           "-r, --reference");
    printf("  %-28s  define the image quality metric to use (default: %s)\n",
           "-m, --metric", mvt_map_lookup_value(image_qm_map, DEFAULT_METRIC));
    printf("  %-28s  compute the average over the file (default: false)\n",
           "-a, --average");

    exit(EXIT_FAILURE);
}

static bool
app_init_args(App *app, int argc, char *argv[])
{
    static const struct option long_options[] = {
        { "help",       no_argument,        NULL, 'h'                   },
        { "reference",  required_argument,  NULL, 'r'                   },
        { "metric",     required_argument,  NULL, 'm'                   },
        { "average",    no_argument,        NULL, 'a'                   },
        { NULL, }
    };

    for (;;) {
        int v = getopt_long(argc, argv, "-hr:m:a", long_options, NULL);
        if (v < 0)
            break;

        switch (v) {
        case '?':
            return false;
        case 'h':
            print_help(argv[0]);
            break;
        case '\1':
            free(app->src_video.filename);
            app->src_video.filename = strdup(optarg);
            if (!app->src_video.filename)
                goto error_alloc_memory;
            break;
        case 'r':
            free(app->ref_video.filename);
            app->ref_video.filename = strdup(optarg);
            if (!app->ref_video.filename)
                goto error_alloc_memory;
            break;
        case 'm': {
            const MvtImageQualityMetric metric =
                mvt_map_lookup(image_qm_map, optarg);

            if (!metric)
                goto error_parse_metric;
            app->metric = metric;
            break;
        }
        case 'a':
            app->calc_average = true;
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
error_parse_metric:
    mvt_error("failed to parse image quality metric ('%s')", optarg);
    return false;
}

static bool
app_init_video(App *app, VideoStream *vsp, const char *name)
{
    MvtImageInfo * const info = &vsp->image_info;

    if (!vsp->filename)
        goto error_no_filename;

    vsp->file = mvt_image_file_open(vsp->filename, MVT_IMAGE_FILE_MODE_READ);
    if (!vsp->file)
        goto error_open_file;
    if (!mvt_image_file_read_headers(vsp->file, &vsp->image_info))
        goto error_read_headers;

    vsp->image = mvt_image_new(info->format, info->width, info->height);
    if (!vsp->image)
        goto error_alloc_image;
    return true;

    /* ERRORS */
error_no_filename:
    mvt_error("no %s video filename supplied", name);
    return false;
error_open_file:
    mvt_error("failed to open video file ('%s')", vsp->filename);
    return false;
error_read_headers:
    mvt_error("failed to read video file headers");
    return false;
error_alloc_image:
    mvt_error("failed to allocate video frame");
    return false;
}

static bool
app_init(App *app, int argc, char *argv[])
{
    app->metric = DEFAULT_METRIC;
    if (!app_init_args(app, argc, argv))
        return false;

    if (!app_init_video(app, &app->src_video, "source"))
        return false;
    if (!app_init_video(app, &app->ref_video, "reference"))
        return false;
    return true;
}

static void
app_finalize_video(App *app, VideoStream *vsp)
{
    if (vsp->file) {
        mvt_image_file_close(vsp->file);
        vsp->file = NULL;
    }
    mvt_image_freep(&vsp->image);
    free(vsp->filename);
    vsp->filename = NULL;
}

static void
app_finalize(App *app)
{
    if (!app)
        return;

    app_finalize_video(app, &app->src_video);
    app_finalize_video(app, &app->ref_video);
}

static bool
app_run(App *app)
{
    VideoStream * const src = &app->src_video;
    VideoStream * const ref = &app->ref_video;
    double qvalue, qvalue_sum = 0.0;
    uint32_t n = 0;

    while (mvt_image_file_read_image(src->file, src->image)) {
        if (!mvt_image_file_read_image(ref->file, ref->image))
            goto error_read_ref_frame;
        if (!mvt_image_compare(src->image, ref->image, app->metric, &qvalue))
            goto error_calc_quality;
        if (app->calc_average)
            qvalue_sum += qvalue;
        else
            printf("%7u %.4f\n", n, qvalue);
        n++;
    }

    if (app->calc_average)
        printf("%.4f\n", qvalue_sum / n);
    return true;

    /* ERRORS */
error_read_ref_frame:
    mvt_error("failed to read reference frame %u", n);
    return false;
error_calc_quality:
    mvt_error("failed to compute quality for frame %u", n);
    return false;
}

int
main(int argc, char *argv[])
{
    App * const app = &g_app;
    bool success = false;

    if (!app_init(app, argc, argv))
        goto cleanup;
    if (!app_run(app))
        goto cleanup;
    success = true;

cleanup:
    app_finalize(app);
    return !success;
}
