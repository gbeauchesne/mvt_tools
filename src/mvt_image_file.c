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
#include <ctype.h>
#include "mvt_image_file.h"
#include "mvt_image_priv.h"

typedef bool (*MvtImageFileWriteHeaderFunc)(MvtImageFile *fp);
typedef bool (*MvtImageFileWriteImageFunc)(MvtImageFile *fp, MvtImage *image);
typedef bool (*MvtImageFileReadHeaderFunc)(MvtImageFile *fp);
typedef bool (*MvtImageFileReadImageFunc)(MvtImageFile *fp, MvtImage *image);

typedef struct {
    MvtImageFileWriteHeaderFunc write_header;
    MvtImageFileWriteImageFunc  write_image;
    MvtImageFileReadHeaderFunc  read_header;
    MvtImageFileReadImageFunc   read_image;
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
/* --- Helpers                                                          --- */
/* ------------------------------------------------------------------------ */

typedef struct {
    char *      str;
    uint32_t    len;
    uint32_t    maxlen;
} Token;

static void
token_init(Token *token)
{
    memset(token, 0, sizeof(*token));
}

static void
token_clear(Token *token)
{
    free(token->str);
    token->len = 0;
    token->maxlen = 0;
}

static bool
token_append(Token *token, char *buf, uint32_t buf_size)
{
    char *str;
    uint32_t len;

    if (!buf || !buf_size)
        return true;

    len = token->len + buf_size + 1;
    if (len > token->maxlen) {
        len += (len >> 3) + 1; // +~12%
        str = realloc(token->str, len);
        if (!str)
            return false;
        token->str = str;
        token->maxlen = len;
    }
    memcpy(token->str + token->len, buf, buf_size);
    token->len += buf_size;
    token->str[token->len] = '\0';
    return true;
}

static bool
token_read(Token *token, FILE *fp, char *sep_ptr)
{
    char sep, buf[BUFSIZ];
    int c, buf_size = 0;

    if (!sep_ptr)
        sep_ptr = &sep;

    if (feof(fp)) {
        *sep_ptr = EOF;
        return false;
    }

    *sep_ptr = '\0';
    token->len = 0;
    while ((c = fgetc(fp)) != EOF && !isspace(c)) {
        buf[buf_size++] = c;
        if (buf_size == sizeof(buf)) {
            if (!token_append(token, buf, buf_size))
                return false;
            buf_size = 0;
        }
    }
    if (!token_append(token, buf, buf_size))
        return false;

    *sep_ptr = c;
    return true;
}

static bool
str_parse_uint(const char *str, char **end_ptr, uint32_t *value_ptr)
{
    unsigned long v;
    char *end = NULL;

    v = strtoul(str, &end, 10);
    if (end == str)
        return false;

    if (end_ptr)
        *end_ptr = end;
    if (value_ptr)
        *value_ptr = v;
    return true;
}

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

// Retrieves the Y4M colorspace bit depth (YSCSS)
static int
y4m_get_colorspace_depth(const VideoFormatInfo *vip)
{
    int i, bit_depth = 8;

    if (vip->num_planes == vip->num_components) {
        int d = vip->components[0].bit_depth;
        for (i = 1; i < vip->num_components; i++) {
            const VideoFormatComponentInfo * const cip = &vip->components[i];
            if (cip->bit_depth != d)
                break;
        }
        if (i == vip->num_components)
            bit_depth = d;
    }
    return bit_depth;
}

// Retrieves the Y4M colorspace string
static const char *
y4m_get_colorspace(const VideoFormatInfo *vip, int depth)
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
        if (depth > 8)
            colorspace = "420";
        else {
            colorspace = "420jpeg"; /* XXX: handle chroma sites */
        }
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

// Retrieves a video format matching the supplied colorspace string
static VideoFormat
y4m_get_video_format(const char *colorspace)
{
    int chroma_type, depth = 8;

    if (strcmp(colorspace, "mono") == 0)
        return VIDEO_FORMAT_Y800;
    else if (strcmp(colorspace, "444alpha") == 0)
        return VIDEO_FORMAT_AYUV;

    if (sscanf(colorspace, "%d%*[pP]%d", &chroma_type, &depth) > 0) {
        switch (chroma_type) {
        case 420:
            switch (depth) {
            case 8:  return VIDEO_FORMAT_I420;
            case 10: return VIDEO_FORMAT_I420P10;
            case 12: return VIDEO_FORMAT_I420P12;
            case 16: return VIDEO_FORMAT_I420P16;
            }
            break;
        case 422:
            switch (depth) {
            case 8:  return VIDEO_FORMAT_YUY2;
            case 10: return VIDEO_FORMAT_I422P10;
            case 12: return VIDEO_FORMAT_I422P12;
            case 16: return VIDEO_FORMAT_I422P16;
            }
            break;
        case 444:
            switch (depth) {
            case 10: return VIDEO_FORMAT_I444P10;
            case 12: return VIDEO_FORMAT_I444P12;
            case 16: return VIDEO_FORMAT_I444P16;
            }
            break;
        }
    }
    return VIDEO_FORMAT_UNKNOWN;
}

// Writes Y4M file header
static bool
y4m_write_header(MvtImageFile *fp)
{
    const MvtImageInfo * const info = &fp->info;
    const VideoFormatInfo * const vip = video_format_get_info(info->format);
    const char *colorspace;
    char picture_structure;
    int depth;

    depth = y4m_get_colorspace_depth(vip);
    colorspace = y4m_get_colorspace(vip, depth);
    if (!colorspace)
        return false;

    picture_structure = y4m_get_picture_structure(0);
    if (!picture_structure)
        return false;

    fprintf(fp->file, "%s W%u H%u F%u:%u A%u:%u I%c C%s",
        Y4M_HEADER_TAG, info->width, info->height, info->fps_n, info->fps_d,
        info->par_n, info->par_d, picture_structure, colorspace);
    if (depth > 8)
        fprintf(fp->file, "p%d XYSCSS=%sP%d", depth, colorspace, depth);
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
    uint32_t x, y, w, h, stride, bpc;

    w = image->width;
    h = image->height;
    if (n > 0) {
        w = (w + (1U << vip->chroma_w_shift) - 1) >> vip->chroma_w_shift;
        h = (h + (1U << vip->chroma_h_shift) - 1) >> vip->chroma_h_shift;
    }
    stride = image->pitches[cip->plane];

    bpc = (cip->bit_depth + 7) / 8; // bytes per component

    p = get_component_ptr(image, cip, 0, 0);
    if (cip->pixel_stride == bpc) {
        for (y = 0; y < h; y++) {
            if (fwrite(p, w, bpc, fp->file) != bpc)
                return false;
            p += stride;
        }
    }
    else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                if (fwrite(p + x * cip->pixel_stride, bpc, 1, fp->file) != 1)
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

// Reads Y4M headers
static bool
y4m_read_header(MvtImageFile *fp)
{
    MvtImageInfo * const info = &fp->info;
    Token token;
    char sep = 0, *end;
    bool success = false;
    uint32_t v0, v1;

    token_init(&token);
    if (!token_read(&token, fp->file, &sep))
        goto cleanup;
    if (strcmp(token.str, Y4M_HEADER_TAG) != 0)
        goto cleanup;
    while (!success && token_read(&token, fp->file, &sep)) {
        const char * const str = token.str;
        switch (str[0]) {
        case 'W':
            if (str_parse_uint(&str[1], NULL, &v0))
                info->width = v0;
            break;
        case 'H':
            if (str_parse_uint(&str[1], NULL, &v0))
                info->height = v0;
            break;
        case 'F':
            if (str_parse_uint(&str[1], &end, &v0) && *end == ':' &&
                str_parse_uint(&end[1], NULL, &v1))
                info->fps_n = v0, info->fps_d = v1;
            break;
        case 'A':
            if (str_parse_uint(&str[1], &end, &v0) && *end == ':' &&
                str_parse_uint(&end[1], NULL, &v1))
                info->par_n = v0, info->par_d = v1;
            break;
        case 'I':
            if (str[1] != 'p')
                mvt_warning("unsupported interlacing mode '%c'", str[1]);
            break;
        case 'C':
            v0 = y4m_get_video_format(&str[1]);
            if (v0 != VIDEO_FORMAT_UNKNOWN)
                info->format = v0;
            break;
        case 'X':
            break;
        default:
            mvt_warning("unsupported token `%s'", str);
            break;
        }
        success = sep == '\n';
    }

cleanup:
    token_clear(&token);
    return success;
}

// Reads image component from Y4M file
static bool
y4m_read_image_component(MvtImageFile *fp, MvtImage *image,
    const VideoFormatInfo *vip, uint32_t n)
{
    const VideoFormatComponentInfo * const cip = &vip->components[n];
    uint8_t *p;
    uint32_t x, y, w, h, stride, bpc;

    w = image->width;
    h = image->height;
    if (n > 0) {
        w = (w + (1U << vip->chroma_w_shift) - 1) >> vip->chroma_w_shift;
        h = (h + (1U << vip->chroma_h_shift) - 1) >> vip->chroma_h_shift;
    }
    stride = image->pitches[cip->plane];

    bpc = (cip->bit_depth + 7) / 8; // bytes per component

    p = get_component_ptr(image, cip, 0, 0);
    if (cip->pixel_stride == bpc) {
        for (y = 0; y < h; y++) {
            if (fread(p, w, bpc, fp->file) != bpc)
                return false;
            p += stride;
        }
    }
    else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                if (fread(p + x * cip->pixel_stride, bpc, 1, fp->file) != 1)
                    return false;
            }
            p += stride;
        }
    }
    return true;
}

// Reads image from Y4M file
static bool
y4m_read_image(MvtImageFile *fp, MvtImage *image)
{
    const VideoFormatInfo * const vip = video_format_get_info(fp->info.format);
    Token token;
    char sep = 0;
    bool success = false;

    token_init(&token);
    while (token_read(&token, fp->file, &sep) && sep != EOF) {
        if (strcmp(token.str, "FRAME") == 0)
            success = true;
        if (sep == '\n')
            break;
    }
    token_clear(&token);
    if (!success)
        return false;

    if (!y4m_read_image_component(fp, image, vip, 0)) // Y
        return false;
    if (vip->num_components > 1) {
        if (!y4m_read_image_component(fp, image, vip, 1)) // Cb
            return false;
        if (!y4m_read_image_component(fp, image, vip, 2)) // Cr
            return false;
    }
    if (vip->num_components > 3) {
        if (!y4m_read_image_component(fp, image, vip, 3)) // Alpha
            return false;
    }
    return true;
}

static const MvtImageFileClass mvt_image_file_class_Y4M = {
    .write_header = y4m_write_header,
    .write_image = y4m_write_image,
    .read_header = y4m_read_header,
    .read_image = y4m_read_image,
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
    case MVT_IMAGE_FILE_MODE_READ:
        mode_str = "r";
        break;
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

// Reads image file headers
bool
mvt_image_file_read_headers(MvtImageFile *fp, MvtImageInfo *info)
{
    static const MvtImageFileClass *klass_list[] = {
        &mvt_image_file_class_Y4M,
        NULL
    };

    if (!fp || !info)
        return false;

    if (!fp->info_ready) {
        uint32_t i;

        fp->klass = NULL;
        for (i = 0; !fp->klass && klass_list[i] != NULL; i++) {
            const MvtImageFileClass * const klass = klass_list[i];

            rewind(fp->file);
            if (klass->read_header && klass->read_header(fp))
                fp->klass = klass;
        }
        if (!fp->klass)
            return false;
        fp->info_ready = true;

        if (fp->info.fps_n == 0 || fp->info.fps_d == 0)
            mvt_image_info_init_default_fps(&fp->info);
        if (fp->info.par_n == 0 || fp->info.par_d == 0)
            mvt_image_info_init_default_par(&fp->info);
    }

    if (info)
        *info = fp->info;
    return true;
}

// Reads the next image stored in file
bool
mvt_image_file_read_image(MvtImageFile *fp, MvtImage *image)
{
    const MvtImageInfo *info;
    const MvtImageFileClass *klass;

    if (!fp || !image)
        return false;

    if (!fp->info_ready && !mvt_image_file_read_headers(fp, NULL))
        return false;

    info = &fp->info;
    if (image->format != info->format ||
        image->width  != info->width  ||
        image->height != info->height)
        return false;

    klass = fp->klass;
    return klass->read_image && klass->read_image(fp, image);
}
