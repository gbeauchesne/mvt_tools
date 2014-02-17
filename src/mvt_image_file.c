/*
 * mvt_image_file.c - Image utilities (file operations)
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

#include "sysdeps.h"
#include "mvt_image_file.h"
#include "mvt_image_priv.h"

typedef bool (*MvtImageFileWriteHeaderFunc)(MvtImageFile *fp);
typedef bool (*MvtImageFileWriteImageFunc)(MvtImageFile *fp, MvtImage *image);

typedef struct {
    MvtImageFileWriteHeaderFunc write_header;
    MvtImageFileWriteImageFunc  write_image;
} MvtImageFileClass;

struct MvtImageFile_s {
    FILE *                      file;
    MvtImageFileMode            mode;
    MvtImageInfo                info;
    bool                        info_ready;
    const MvtImageFileClass *   klass;
};

// Default framerate (60 fps)
#define DEFAULT_FPS_N 60
#define DEFAULT_FPS_D 1

// Default pixel aspect ratio (1:1)
#define DEFAULT_PAR_N 1
#define DEFAULT_PAR_D 1

/* ------------------------------------------------------------------------ */
/* --- Y4M Format (YUV)                                                 --- */
/* ------------------------------------------------------------------------ */

// Y4M file format identifier
#define Y4M_HEADER_TAG "YUV4MPEG2"

// Retrieves the Y4M picture structure string
static char
y4m_get_picture_structure(uint32_t fields)
{
    char picture_structure;

    switch (fields & (VA_TOP_FIELD|VA_BOTTOM_FIELD)) {
    case VA_TOP_FIELD:
        picture_structure = 't';
        break;
    case VA_BOTTOM_FIELD:
        picture_structure = 'b';
        break;
    case VA_TOP_FIELD|VA_BOTTOM_FIELD:
        picture_structure = 'm';
        break;
    default:
        picture_structure = 'p';
        break;
    }
    return picture_structure;
}

// Retrieves the Y4M colorspace string
static const char *
y4m_get_colorspace(const VideoFormatInfo *vip)
{
    const char *colorspace;

    switch (vip->chroma_type) {
#if VA_CHECK_VERSION(0,34,0)
    case VA_RT_FORMAT_YUV400:
        colorspace = "mono";
        break;
    case VA_RT_FORMAT_YUV411:
        colorspace = "411";
        break;
#endif
    case VA_RT_FORMAT_YUV420:
        colorspace = "420jpeg"; /* XXX: handle chroma sites */
        break;
    case VA_RT_FORMAT_YUV422:
        colorspace = "422";
        break;
    case VA_RT_FORMAT_YUV444:
        colorspace = video_format_has_alpha(vip->format) ? "444alpha" : "444";
        break;
    default:
        colorspace = NULL;
        break;
    }
    return colorspace;
}

// Writes Y4M file header
static bool
y4m_write_header(MvtImageFile *fp)
{
    const MvtImageInfo * const info = &fp->info;
    const VideoFormatInfo * const vip = video_format_get_info(info->format);
    const char *colorspace;
    char picture_structure;

    colorspace = y4m_get_colorspace(vip);
    if (!colorspace)
        return false;

    picture_structure = y4m_get_picture_structure(0);
    if (!picture_structure)
        return false;

    fprintf(fp->file, "%s W%u H%u F%u:%u A%u:%u I%c C%s",
        Y4M_HEADER_TAG, info->width, info->height, info->fps_n, info->fps_d,
        info->par_n, info->par_d, picture_structure, colorspace);
    fprintf(fp->file, "\n");
    return true;
}

// Writes image component to Y4M file
static bool
y4m_write_image_component(MvtImageFile *fp, MvtImage *image,
    const VideoFormatInfo *vip, uint32_t n)
{
    const VideoFormatComponentInfo * const cip = &vip->components[n];
    const uint8_t *p;
    uint32_t x, y, w, h, stride;

    w = image->width;
    h = image->height;
    if (n > 0) {
        w = (w + (1U << vip->chroma_w_shift) - 1) >> vip->chroma_w_shift;
        h = (h + (1U << vip->chroma_h_shift) - 1) >> vip->chroma_h_shift;
    }
    stride = image->pitches[cip->plane];

    p = get_component_ptr(image, cip, 0, 0);
    if (cip->pixel_stride == 1) {
        for (y = 0; y < h; y++) {
            if (fwrite(p, w, 1, fp->file) != 1)
                return false;
            p += stride;
        }
    }
    else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                if (putc(p[x * cip->pixel_stride], fp->file) == EOF)
                    return false;
            }
            p += stride;
        }
    }
    return true;
}

// Writes image to Y4M file
static bool
y4m_write_image(MvtImageFile *fp, MvtImage *image)
{
    const VideoFormatInfo * const vip = video_format_get_info(fp->info.format);

    fprintf(fp->file, "FRAME\n");
    if (!y4m_write_image_component(fp, image, vip, 0)) // Y
        return false;
    if (vip->num_components > 1) {
        if (!y4m_write_image_component(fp, image, vip, 1)) // Cb
            return false;
        if (!y4m_write_image_component(fp, image, vip, 2)) // Cr
            return false;
    }
    if (vip->num_components > 3) {
        if (!y4m_write_image_component(fp, image, vip, 3)) // Alpha
            return false;
    }
    return true;
}

static const MvtImageFileClass mvt_image_file_class_Y4M = {
    .write_header = y4m_write_header,
    .write_image = y4m_write_image,
};

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

// Initializes framerate to its default value
static inline void
mvt_image_info_init_default_fps(MvtImageInfo *info)
{
    info->fps_n = DEFAULT_FPS_N;
    info->fps_d = DEFAULT_FPS_D;
}

// Initializes pixel-aspect-ratio to its default value
static inline void
mvt_image_info_init_default_par(MvtImageInfo *info)
{
    info->par_n = DEFAULT_PAR_N;
    info->par_d = DEFAULT_PAR_D;
}

// Initializes MvtImageInfo descriptor to the supplied parameters
void
mvt_image_info_init(MvtImageInfo *info, VideoFormat format,
    uint32_t width, uint32_t height)
{
    info->format = format;
    info->width  = width;
    info->height = height;
    mvt_image_info_init_default_fps(info);
    mvt_image_info_init_default_par(info);
}

// Initializes MvtImageInfo descriptor to its default values
void
mvt_image_info_init_defaults(MvtImageInfo *info)
{
    mvt_image_info_init(info, VIDEO_FORMAT_UNKNOWN, 0, 0);
}

// Creates a new image file with the supplied access mode
MvtImageFile *
mvt_image_file_open(const char *path, MvtImageFileMode mode)
{
    MvtImageFile *fp;
    const char *mode_str;

    if (!path)
        return NULL;

    switch (mode) {
    case MVT_IMAGE_FILE_MODE_WRITE:
        mode_str = "w";
        break;
    default:
        mvt_error("invalid access mode (0x%x)", mode);
        return NULL;
    }

    fp = calloc(1, sizeof(*fp));
    if (!fp)
        return NULL;

    fp->file = fopen(path, mode_str);
    if (!fp->file)
        goto error;
    fp->mode = mode;
    mvt_image_info_init_defaults(&fp->info);
    return fp;

error:
    mvt_image_file_close(fp);
    return NULL;
}

// Closes the image file and all resources
void
mvt_image_file_close(MvtImageFile *fp)
{
    if (!fp)
        return;

    if (fp->file)
        fclose(fp->file);
    free(fp);
}

// Writes the image file headers based on the supplied info descriptor
bool
mvt_image_file_write_headers(MvtImageFile *fp, const MvtImageInfo *info)
{
    const MvtImageFileClass *klass;
    const VideoFormatInfo *vip;

    if (!fp || !info)
        return false;

    if (fp->info_ready)
        return true;

    fp->info = *info;
    if (info->fps_n == 0 || info->fps_d == 0)
        mvt_image_info_init_default_fps(&fp->info);
    if (info->par_n == 0 || info->par_d == 0)
        mvt_image_info_init_default_par(&fp->info);

    vip = video_format_get_info(info->format);
    if (!vip)
        return false;
    if (video_format_is_yuv(vip->format))
        klass = &mvt_image_file_class_Y4M;
    else {
        mvt_error("unsupported format %s", video_format_get_name(vip->format));
        return false;
    }

    fp->klass = klass;
    if (klass->write_header && !klass->write_header(fp))
        return false;

    fp->info_ready = true;
    return true;
}

// Writes the image to file
bool
mvt_image_file_write_image(MvtImageFile *fp, MvtImage *image)
{
    const MvtImageFileClass *klass;

    if (!fp || !image)
        return false;

    if (!fp->info_ready) {
        mvt_image_info_init(&fp->info, image->format, image->width,
            image->height);
        if (!mvt_image_file_write_headers(fp, &fp->info))
            return false;
    }

    klass = fp->klass;
    return klass->write_image && klass->write_image(fp, image);
}
