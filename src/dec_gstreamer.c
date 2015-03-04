/*
 * dec_gstreamer.c - Decoder based on GStreamer
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
#include <gst/gst.h>
#if GST_CHECK_VERSION(1,1,0)
#include <gst/gstcontext.h>
#else
#include <gst/video/videocontext.h>
#endif
#include <gst/vaapi/gstvaapisurfaceproxy.h>
#if GST_VAAPI_BACKEND_DRM
# include <gst/vaapi/gstvaapidisplay_drm.h>
# include <gst/vaapi/gstvaapiwindow_drm.h>
#endif
#if GST_VAAPI_BACKEND_X11
# include <gst/vaapi/gstvaapidisplay_x11.h>
# include <gst/vaapi/gstvaapiwindow_x11.h>
#endif
#include "mvt_report.h"
#include "mvt_decoder.h"
#include "mvt_string.h"
#include "mvt_image.h"
#include "va_image_utils.h"

/* GstContext type name for GstVaapiDisplay */
#ifndef GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME
#define GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME "gst.vaapi.Display"
#endif

/* ------------------------------------------------------------------------ */
/* --- Windowing System Helpers                                         --- */
/* ------------------------------------------------------------------------ */

typedef GstVaapiDisplay *(*CreateDisplayFunc)(const gchar *name);

typedef struct _WinsysClass WinsysClass;
struct _WinsysClass {
    CreateDisplayFunc   create_display;
};

static inline const WinsysClass *
winsys_class(void)
{
    static const WinsysClass g_winsys_class = {
#if GST_VAAPI_BACKEND_DRM
        gst_vaapi_display_drm_new,
#endif
#if GST_VAAPI_BACKEND_X11
        gst_vaapi_display_x11_new,
#endif
    };
    return &g_winsys_class;
};

static inline GstVaapiDisplay *
winsys_create_display(const gchar *name)
{
    return winsys_class()->create_display(name);
}

/* ------------------------------------------------------------------------ */
/* --- GStreamer utilities                                              --- */
/* ------------------------------------------------------------------------ */

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GST_VIDEO_FORMAT_NE(fmt) G_PASTE(G_PASTE(GST_VIDEO_FORMAT_,fmt),LE)
#else
#define GST_VIDEO_FORMAT_NE(fmt) G_PASTE(G_PASTE(GST_VIDEO_FORMAT_,fmt),BE)
#endif

typedef int (*ProfileFromNameFunc)(const gchar *name);
typedef bool (*ProfileFromCodecDataFunc)(const uint8_t *buf, uint32_t buf_size,
    int *profile_ptr);

typedef struct {
    const gchar *media_str;
    const gchar *profile_str;
    MvtCodec codec;
    int profile;
    ProfileFromNameFunc profile_from_name;
    ProfileFromCodecDataFunc profile_from_codec_data;
} CodecInfo;

static const CodecInfo codec_info[] = {
    { "video/mpeg, mpegversion=(int)2", NULL,
      MVT_CODEC_MPEG2, MVT_MPEG2_PROFILE_SIMPLE,
      .profile_from_name = (ProfileFromNameFunc)mvt_mpeg2_profile_from_name },
    { "video/mpeg, mpegversion=(int)4", NULL,
      MVT_CODEC_MPEG4, MVT_MPEG4_PROFILE_SIMPLE,
      .profile_from_name = (ProfileFromNameFunc)mvt_mpeg4_profile_from_name },
    { "video/x-divx, divxversion=(int)5", NULL,
      MVT_CODEC_MPEG4, MVT_MPEG4_PROFILE_ADVANCED_SIMPLE, },
    { "video/x-xvid", NULL,
      MVT_CODEC_MPEG4, MVT_MPEG4_PROFILE_ADVANCED_SIMPLE, },
    { "image/jpeg", NULL,
      MVT_CODEC_JPEG, -1 },
    { "video/x-h264", NULL,
      MVT_CODEC_H264, MVT_H264_PROFILE_BASELINE,
      .profile_from_name = (ProfileFromNameFunc)mvt_h264_profile_from_name,
      .profile_from_codec_data = mvt_h264_profile_from_codec_data },
    { "video/x-wmv, wmvversion=(int)3", NULL,
      MVT_CODEC_VC1, MVT_VC1_PROFILE_SIMPLE,
      .profile_from_name = (ProfileFromNameFunc)mvt_vc1_profile_from_name,
      .profile_from_codec_data = mvt_wmv3_profile_from_codec_data },
    { "video/x-wmv, wmvversion=(int)3, format=(string)WVC1", NULL,
      MVT_CODEC_VC1, MVT_VC1_PROFILE_ADVANCED, },
    { "video/x-vp8", NULL,
      MVT_CODEC_VP8, -1, },
    { "video/x-vp9", NULL,
      MVT_CODEC_VP9, MVT_VP9_PROFILE_0, },
    { "video/x-h265", NULL,
      MVT_CODEC_HEVC, MVT_HEVC_PROFILE_MAIN,
      .profile_from_name = (ProfileFromNameFunc)mvt_hevc_profile_from_name },
    { NULL, }
};

// Determines the codec and profile ids from the GStreamer caps
static gboolean
parse_codec_caps(GstCaps *caps, MvtCodec *codec_ptr, int *profile_ptr)
{
    const CodecInfo *cip;
    GstCaps *ref_caps;
    GstStructure *structure;
    const GValue *value;
    guint8 codec_data[BUFSIZ];
    gsize codec_data_size = 0;
    const gchar *name;
    gsize namelen;
    gboolean compatible_caps;
    MvtCodec codec = 0;
    const gchar *profile_str;
    int v, profile = -1;

    if (!gst_caps_is_fixed(caps))
        return FALSE;

    structure = gst_caps_get_structure(caps, 0);
    if (!structure)
        return FALSE;

    name = gst_structure_get_name(structure);
    namelen = strlen(name);

    profile_str = gst_structure_get_string(structure, "profile");

    value = gst_structure_get_value(structure, "codec_data");
    if (value && G_VALUE_TYPE(value) == GST_TYPE_BUFFER) {
        GstBuffer * const buffer = gst_value_get_buffer(value);
        if (buffer)
            codec_data_size = gst_buffer_extract(buffer, 0,
                codec_data, sizeof(codec_data));
    }

    for (cip = codec_info; cip->media_str != NULL; cip++) {
        if (strncmp(name, cip->media_str, namelen) != 0)
            continue;
        ref_caps = gst_caps_from_string(cip->media_str);
        if (!ref_caps)
            continue;
        compatible_caps = gst_caps_is_always_compatible(caps, ref_caps);
        gst_caps_unref(ref_caps);
        if (!compatible_caps)
            continue;
        codec = cip->codec;
        profile = cip->profile;
        if (profile_str && cip->profile_from_name)
            profile = cip->profile_from_name(profile_str);
        if (cip->profile_from_codec_data &&
            cip->profile_from_codec_data(codec_data, codec_data_size, &v))
            profile = v;
    }

    if (codec_ptr)
        *codec_ptr = codec;
    if (profile_ptr)
        *profile_ptr = profile;
    return codec != 0;
}

typedef struct {
    GstVideoFormat gst_format;
    VideoFormat mvt_format;
} VideoFormatMap;

static const VideoFormatMap g_video_format_map[] = {
    { GST_VIDEO_FORMAT_I420, VIDEO_FORMAT_I420 },
    { GST_VIDEO_FORMAT_Y42B, VIDEO_FORMAT_I422 },
    { GST_VIDEO_FORMAT_Y444, VIDEO_FORMAT_I444 },
    { GST_VIDEO_FORMAT_YV12, VIDEO_FORMAT_YV12 },
    { GST_VIDEO_FORMAT_NV12, VIDEO_FORMAT_NV12 },
    { GST_VIDEO_FORMAT_YUY2, VIDEO_FORMAT_YUY2 },
    { GST_VIDEO_FORMAT_UYVY, VIDEO_FORMAT_UYVY },
    { GST_VIDEO_FORMAT_AYUV, VIDEO_FORMAT_AYUV },
    { GST_VIDEO_FORMAT_NE(I420_10), VIDEO_FORMAT_I420P10 },
    { GST_VIDEO_FORMAT_NE(I422_10), VIDEO_FORMAT_I422P10 },
#if GST_CHECK_VERSION(1,1,0)
    { GST_VIDEO_FORMAT_NE(Y444_10), VIDEO_FORMAT_I444P10 },
#endif
    { GST_VIDEO_FORMAT_UNKNOWN, VIDEO_FORMAT_UNKNOWN }
};

// Translates MVT video format to GstVideoFormat
static bool
mvt_to_gst_video_format(VideoFormat format, GstVideoFormat *out_format_ptr)
{
    const VideoFormatMap *m;

    for (m = g_video_format_map; m->mvt_format != VIDEO_FORMAT_UNKNOWN; m++) {
        if (m->mvt_format == format) {
            if (out_format_ptr)
                *out_format_ptr = m->gst_format;
            return true;
        }
    }
    return false;
}

// Translates GstVideoFormat to MVT video format
static bool
gst_to_mvt_video_format(GstVideoFormat format, VideoFormat *out_format_ptr)
{
    const VideoFormatMap *m;

    for (m = g_video_format_map; m->gst_format != GST_VIDEO_FORMAT_UNKNOWN; m++) {
        if (m->gst_format == format) {
            if (out_format_ptr)
                *out_format_ptr = m->mvt_format;
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------------ */
/* --- GStreamer based decoder                                          --- */
/* ------------------------------------------------------------------------ */

/* Maximum number of chained video processing elements (used for validation) */
#define MAX_VPP_CHAIN 2

typedef struct {
    guint               method;
    guint               width;
    guint               height;
    gboolean            is_valid;
} VideoScaleOptions;

typedef struct {
    VideoFormat         format;
} VideoConvertOptions;

typedef enum {
    APP_ERROR_NONE,
    APP_ERROR_PIPELINE,
    APP_ERROR_BUS,
    APP_ERROR_WINSYS,
    APP_ERROR_GENERIC
} AppError;

typedef struct {
    MvtDecoder          decoder;
    GError             *error;
    GMainLoop          *loop;
    GstElement         *pipeline;
    GstElement         *source;
    GstElement         *decodebin;
    GstElement         *postprocbin;
    GstElement         *postprocbin_head;
    GstElement         *vparse;
    gchar              *vparse_name;
    GstCaps            *vparse_capsfilter;
    GstElement         *vdecode;
    GstElement         *vsink;
    GstCaps            *vsink_caps;
    GstElement         *vapostproc[MAX_VPP_CHAIN];
    guint               vapostproc_mask;
    GstVideoInfo        vinfo;
    guint               bus_watch_id;
    GstVaapiDisplay    *display;
#if GST_CHECK_VERSION(1,1,0)
    GstContext         *context;
#endif
    MvtImage           *image;

    /** @name Video post-processing options */
    /*@{*/
    VideoScaleOptions   vscale_options[MAX_VPP_CHAIN];
    VideoConvertOptions vconvert_options;
    /*@}*/

    guint               gst_init_done : 1;
} App;

#define APP_ERROR app_error_quark()
static GQuark
app_error_quark(void)
{
    static gsize g_quark;

    if (g_once_init_enter(&g_quark)) {
        gsize quark = (gsize)g_quark_from_static_string("AppError");
        g_once_init_leave(&g_quark, quark);
    }
    return g_quark;
}

static void
app_set_error_message(App *app, gint code, const gchar *message)
{
    g_clear_error(&app->error);
    app->error = g_error_new_literal(APP_ERROR, code, message);
}

static void
app_error(App *app, const gchar *format, ...)
{
    g_clear_error(&app->error);
    {
        va_list args;
        va_start(args, format);
        app->error = g_error_new_valist(APP_ERROR, APP_ERROR_GENERIC,
            format, args);
        va_end(args);
    }
    g_main_loop_quit(app->loop);
}

static bool
app_init_gst(App *app)
{
    if (!app->gst_init_done) {
        if (!gst_init_check(NULL, NULL, &app->error))
            return false;
        app->gst_init_done = true;
    }
    return true;
}

static bool
app_init(App *app)
{
    app->loop = g_main_loop_new(NULL, FALSE);
    if (!app->loop)
        return false;

    gst_video_info_init(&app->vinfo);
    return true;
}

static void
app_finalize(App *app)
{
    guint i;

#if GST_CHECK_VERSION(1,1,0)
    if (app->context)
        gst_context_unref(app->context);
#endif
    if (app->display)
        gst_vaapi_display_unref(app->display);
    if (app->pipeline)
        gst_object_unref(app->pipeline);
    if (app->bus_watch_id)
        g_source_remove(app->bus_watch_id);
    if (app->loop)
        g_main_loop_unref(app->loop);
    mvt_image_freep(&app->image);
    g_free(app->vparse_name);
    gst_caps_replace(&app->vparse_capsfilter, NULL);
    gst_object_replace((GstObject **)&app->vparse, NULL);
    gst_object_replace((GstObject **)&app->vdecode, NULL);
    for (i = 0; i < G_N_ELEMENTS(app->vapostproc); i++)
        gst_object_replace((GstObject **)&app->vapostproc[i], NULL);
    gst_caps_replace(&app->vsink_caps, NULL);
    g_clear_error(&app->error);
}

static inline gboolean
app_pipeline_error(App *app, const gchar *message)
{
    app_set_error_message(app, APP_ERROR_PIPELINE, message);
    return FALSE;
}

// Update video info
static gboolean
app_update_video_info(App *app, GstCaps *caps)
{
    GstVideoInfo * const vip = &app->vinfo;
    MvtImageInfo * const info = &MVT_DECODER(app)->output_info;

    if (!gst_video_info_from_caps(vip, caps))
        goto error_invalid_caps;
    GST_INFO("new caps: format %s, size %dx%d",
        gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(vip)),
        GST_VIDEO_INFO_WIDTH(vip), GST_VIDEO_INFO_HEIGHT(vip));

    info->fps_n = GST_VIDEO_INFO_FPS_N(vip);
    info->fps_d = GST_VIDEO_INFO_FPS_D(vip);
    info->par_n = GST_VIDEO_INFO_PAR_N(vip);
    info->par_d = GST_VIDEO_INFO_PAR_D(vip);
    return TRUE;

    /* ERRORS */
error_invalid_caps:
    mvt_error("invalid or unsupported caps received");
    return FALSE;
}

// Update codec info
static gboolean
app_update_codec_info(App *app)
{
    MvtDecoder * const dec = &app->decoder;
    GstPad *pad;
    GstCaps *caps;
    MvtCodec codec;
    int profile;
    gboolean success;

    if (!app->vdecode)
        return FALSE;

    if (!GST_IS_VIDEO_DECODER(app->vdecode))
        goto error_unsupported_decoder;

    pad = GST_VIDEO_DECODER_SINK_PAD(app->vdecode);
    if (!pad)
        goto error_no_sinkpad;

    caps = gst_pad_get_current_caps(pad);
    if (!caps)
        goto error_no_caps;

    success = parse_codec_caps(caps, &codec, &profile);
    gst_caps_unref(caps);
    if (!success)
        return FALSE;

    if (codec != dec->codec)
        dec->codec = codec;
    if (profile != -1 && profile != dec->profile)
        dec->profile = profile;
    return TRUE;

    /* ERRORS */
error_unsupported_decoder:
    app_error(app, "unsupported decoder %" GST_PTR_FORMAT, app->vdecode);
    return FALSE;
error_no_sinkpad:
    app_error(app, "fatal: no sink pad found for %" GST_PTR_FORMAT,
        app->vdecode);
    return FALSE;
error_no_caps:
    app_error(app, "fatal: no sink pad caps found for %" GST_PTR_FORMAT,
        app->vdecode);
    return FALSE;
}

// Handle SW surfaces
static gboolean
app_handle_sw_surface(App *app, GstBuffer *buffer,
    const VARectangle *crop_rect)
{
    GstVideoFrame frame;
    VideoFormat format;
    MvtImage src_image, dst_image;
    VAImage va_image;
    const VAImageFormat *va_format;
    uint32_t i;
    gboolean success;

    if (!gst_to_mvt_video_format(GST_VIDEO_INFO_FORMAT(&app->vinfo), &format))
        goto error_unsupported_format;
    if (!(va_format = video_format_to_va_format(format)))
        goto error_unsupported_format;

    va_image_init_defaults(&va_image);
    va_image.format = *va_format;
    va_image.width = GST_VIDEO_INFO_WIDTH(&app->vinfo);
    va_image.height = GST_VIDEO_INFO_HEIGHT(&app->vinfo);
    va_image.data_size = gst_buffer_get_size(buffer);
    va_image.num_planes = GST_VIDEO_INFO_N_PLANES(&app->vinfo);
    for (i = 0; i < va_image.num_planes; i++) {
        va_image.pitches[i] = GST_VIDEO_INFO_PLANE_STRIDE(&app->vinfo, i);
        va_image.offsets[i] = GST_VIDEO_INFO_PLANE_OFFSET(&app->vinfo, i);
    }
    if (!mvt_image_init_from_va_image(&src_image, &va_image))
        goto error_unsupported_image;

    if (!gst_video_frame_map(&frame, &app->vinfo, buffer, GST_MAP_READ))
        goto error_map_buffer;

    for (i = 0; i < src_image.num_planes; i++)
        src_image.pixels[i] = frame.data[i];

    if (!mvt_image_init_from_subimage(&dst_image, &src_image, crop_rect))
        goto error_crop_image;

    success = mvt_decoder_handle_image(&app->decoder, &dst_image, 0);
    mvt_image_clear(&dst_image);
    mvt_image_clear(&src_image);
    gst_video_frame_unmap(&frame);
    return success;

    /* ERRORS */
error_unsupported_format:
    app_error(app, "unsupported video format %s",
        gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&app->vinfo)));
    return FALSE;
error_unsupported_image:
    app_error(app, "failed to extract image from buffer %p", buffer);
    return FALSE;
error_map_buffer:
    app_error(app, "failed to map buffer %p", buffer);
    mvt_image_clear(&src_image);
    return FALSE;
error_crop_image:
    app_error(app, "failed to extract cropped image from buffer %p", buffer);
    mvt_image_clear(&src_image);
    gst_video_frame_unmap(&frame);
    return FALSE;
}

// Handle HW surfaces (VA-API)
static gboolean
app_handle_hw_surface_vaapi(App *app, GstBuffer *buffer,
    const VARectangle *crop_rect)
{
    MvtImage src_image, dst_image;
    GstVaapiSurfaceProxy *proxy;
    GstVaapiSurface *surface;
    GstMapInfo map_info;
    GstVaapiImage *image;
    VAImage va_image;
    VADisplay va_display;
    gboolean success;

    if (!gst_buffer_map(buffer, &map_info, 0))
        goto error_map_buffer;

    proxy = (GstVaapiSurfaceProxy *)map_info.data;
    if (!proxy)
        goto error_invalid_proxy;

    surface = gst_vaapi_surface_proxy_get_surface(proxy);
    if (!surface)
        goto error_invalid_surface;

    va_display = gst_vaapi_display_get_display(
        GST_VAAPI_OBJECT_DISPLAY(surface));

    image = gst_vaapi_surface_derive_image(surface);
    if (!image || !gst_vaapi_image_get_image(image, &va_image))
        goto error_unsupported_image;
    if (!mvt_image_init_from_va_image(&src_image, &va_image))
        goto error_unsupported_image;
    if (!va_map_image(va_display, &va_image, &src_image))
        goto error_unsupported_image;

    if (!mvt_image_init_from_subimage(&dst_image, &src_image, crop_rect))
        goto error_crop_image;

    if (!app->image || (app->image->width != dst_image.width ||
            app->image->height != dst_image.height)) {
        mvt_image_freep(&app->image);
        app->image = mvt_image_new(video_format_normalize(dst_image.format),
            dst_image.width, dst_image.height);
        if (!app->image)
            goto error_convert_image;
    }
    if (!mvt_image_convert_full(app->image, &dst_image,
            MVT_IMAGE_FLAG_FROM_USWC))
        goto error_convert_image;

    success = mvt_decoder_handle_image(&app->decoder, app->image, 0);
    mvt_image_clear(&dst_image);
    va_unmap_image(va_display, &va_image, &src_image);
    gst_vaapi_object_unref(image);
    gst_buffer_unmap(buffer, &map_info);
    return success;

    /* ERRORS */
error_map_buffer:
    app_error(app, "failed to map buffer %p", buffer);
    return FALSE;
error_invalid_proxy:
    app_error(app, "failed to extract VA surface proxy from buffer %p", buffer);
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
error_invalid_surface:
    app_error(app, "failed to extract VA surface from buffer %p", buffer);
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
error_unsupported_image:
    app_error(app, "failed to extract raw VA image from buffer %p", buffer);
    gst_vaapi_object_replace(&image, NULL);
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
error_crop_image:
    app_error(app, "failed to extract cropped image from buffer %p", buffer);
    va_unmap_image(va_display, &va_image, &src_image);
    gst_vaapi_object_replace(&image, NULL);
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
error_convert_image:
    app_error(app, "failed to convert VA image to native image");
    mvt_image_clear(&dst_image);
    va_unmap_image(va_display, &va_image, &src_image);
    gst_vaapi_object_replace(&image, NULL);
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
}

static gboolean
on_autoplug_query(GstElement *element, GstPad *pad, GstElement *child,
    GstQuery *query, App *app)
{
    GstCaps *filter, *outcaps;

    /* Detect video parser 'caps' query on src pad */
    if (child != app->vparse || !app->vparse_capsfilter)
        return FALSE;
    if (!GST_PAD_IS_SRC(pad) || GST_QUERY_TYPE(query) != GST_QUERY_CAPS)
        return FALSE;

    gst_query_parse_caps(query, &filter);
    outcaps = gst_caps_intersect(filter, app->vparse_capsfilter);
    gst_query_set_caps_result(query, outcaps);
    gst_caps_unref(outcaps);
    return TRUE;
}

typedef enum {
    GST_AUTOPLUG_SELECT_TRY,
    GST_AUTOPLUG_SELECT_EXPOSE,
    GST_AUTOPLUG_SELECT_SKIP
} GstAutoplugSelectResult;

static GstAutoplugSelectResult
on_autoplug_select(GstElement *element, GstPad *pad, GstCaps *caps,
    GstElementFactory *factory, App *app)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    const gchar *element_name;

    /* Determine the name of the element this factory refers to */
    if (GST_IS_PLUGIN_FEATURE(factory))
        element_name = gst_plugin_feature_get_name(factory);
    else if (GST_IS_OBJECT(factory))
        element_name = GST_OBJECT_NAME(factory);
    else {
        GST_ERROR("failed to determine element name");
        return GST_AUTOPLUG_SELECT_SKIP;
    }

    /* Filter out audio parser or audio decoder */
    if (gst_element_factory_list_is_type(factory,
            (GST_ELEMENT_FACTORY_TYPE_PARSER |
             GST_ELEMENT_FACTORY_TYPE_DECODER) |
            GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
        return GST_AUTOPLUG_SELECT_SKIP;

    /* Work out video parser specifics */
    if (gst_element_factory_list_is_type(factory,
            GST_ELEMENT_FACTORY_TYPE_PARSER |
            GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO)) {
        if (app->vparse || (app->vparse_name &&
                !g_str_has_prefix(element_name, app->vparse_name)))
            return GST_AUTOPLUG_SELECT_SKIP;
        return GST_AUTOPLUG_SELECT_TRY;
    }

    /* Try out anything that is not a video decoder */
    if (!gst_element_factory_list_is_type(factory,
            GST_ELEMENT_FACTORY_TYPE_DECODER |
            (GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO |
             GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE)))
        return GST_AUTOPLUG_SELECT_TRY;

    /* Filter out/in hardware accelerated decoders when needed */
    if (!strncmp(element_name, "vaapi", 5))
        return options->hwaccel == MVT_HWACCEL_VAAPI ?
            GST_AUTOPLUG_SELECT_TRY : GST_AUTOPLUG_SELECT_SKIP;

    /* Try whatever is left */
    return options->hwaccel == MVT_HWACCEL_NONE ?
        GST_AUTOPLUG_SELECT_TRY : GST_AUTOPLUG_SELECT_SKIP;
}

static void
on_pad_added(GstElement *element, GstPad *pad, GstElement *vsink)
{
    GstPad *sinkpad;

    sinkpad = gst_element_get_static_pad(vsink, "sink");
    gst_pad_link(pad, sinkpad);
    gst_object_unref(sinkpad);
}

static inline gboolean
is_video_decoder_factory(GstElementFactory *factory)
{
    return gst_element_factory_list_is_type(factory,
        GST_ELEMENT_FACTORY_TYPE_DECODER |
        (GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO |
         GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE));
}

static inline gboolean
is_video_parser_factory(GstElementFactory *factory)
{
    return gst_element_factory_list_is_type(factory,
        GST_ELEMENT_FACTORY_TYPE_PARSER |
        (GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO |
         GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE));
}

static void
on_element_added(GstBin *bin, GstElement *element, App *app)
{
    GstElementFactory * const factory = gst_element_get_factory(element);
    gpointer element_ptr = NULL;

    /* Video decoder */
    if (is_video_decoder_factory(factory))
        element_ptr = &app->vdecode;

    /* Video parser */
    else if (is_video_parser_factory(factory))
        element_ptr = &app->vparse;

    if (element_ptr)
        gst_object_replace(element_ptr, (GstObject *)element);
}

static gboolean
on_handoff(GstElement *element, GstBuffer *buffer, GstPad *pad, App *app)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    VARectangle tmp_crop_rect, *crop_rect = NULL;
    GstCaps *caps;
    gboolean caps_changed;

    /* Update video crop info */
#if GST_CHECK_VERSION(1,0,0)
    GstVideoCropMeta * const crop_meta =
        gst_buffer_get_video_crop_meta(buffer);
    if (crop_meta) {
        crop_rect = &tmp_crop_rect;
        crop_rect->x = crop_meta->x;
        crop_rect->y = crop_meta->y;
        crop_rect->width = crop_meta->width;
        crop_rect->height = crop_meta->height;
    }
#endif

    /* Update the video caps */
    caps = gst_pad_get_current_caps(pad);
    caps_changed = caps != app->vsink_caps;
    gst_caps_replace(&app->vsink_caps, caps);
    gst_caps_unref(caps);
    if (caps_changed) {
        if (!app_update_video_info(app, app->vsink_caps))
            return FALSE;
        if (!app_update_codec_info(app))
            return FALSE;
    }

    if (options->benchmark)
        return TRUE;

    /* Handle the decoded buffer */
    switch (options->hwaccel) {
    case MVT_HWACCEL_NONE:
        if (!app_handle_sw_surface(app, buffer, crop_rect))
            return FALSE;
        break;
    case MVT_HWACCEL_VAAPI:
        if (!app_handle_hw_surface_vaapi(app, buffer, crop_rect))
            return FALSE;
        break;
    }
    return TRUE;
}

static gboolean
app_init_postprocbin_for_vaapipostproc(App *app, guint n)
{
    GstElement *vapostproc;

    g_return_val_if_fail(app->pipeline != NULL, FALSE);

    vapostproc = app->vapostproc[n];
    if (!vapostproc) {
        vapostproc = gst_element_factory_make("vaapipostproc", NULL);
        if (!vapostproc)
            return app_pipeline_error(app, "vaapi video processing");
        app->vapostproc[n] = vapostproc;
    }

    if (!(app->vapostproc_mask & (1U << n))) {
        if (!gst_bin_add(GST_BIN(app->pipeline), g_object_ref(vapostproc)))
            return app_pipeline_error(app, "failed to append vaapipostproc");
        if (app->postprocbin && !gst_element_link(app->postprocbin, vapostproc))
            return app_pipeline_error(app, "postprocbin to vaapipostproc link");
        app->postprocbin = vapostproc;
        if (!app->postprocbin_head)
            app->postprocbin_head = vapostproc;
        app->vapostproc_mask |= 1U << n;
    }
    return TRUE;
}

/* Initializes pipeline for optional "videoconvert" elements */
static gboolean
app_init_postprocbin_for_videoconvert(App *app, const VideoConvertOptions *vco)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    GstElement *vconvert, *capsfilter;
    GstVideoFormat out_format;
    GstCaps *caps;

    if (!vco->format || vco->format == VIDEO_FORMAT_ENCODED)
        return TRUE;

    if (!mvt_to_gst_video_format(vco->format, &out_format))
        return app_pipeline_error(app, "unsupported video format");

    switch (options->hwaccel) {
    case MVT_HWACCEL_NONE:
        vconvert = gst_element_factory_make("videoconvert", NULL);
        if (!gst_bin_add(GST_BIN(app->pipeline), vconvert))
            return app_pipeline_error(app, "failed to append videoconvert");

        capsfilter = gst_element_factory_make("capsfilter", NULL);
        if (!gst_bin_add(GST_BIN(app->pipeline), capsfilter))
            return app_pipeline_error(app, "failed to append capsfilter");

        caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
            gst_video_format_to_string(out_format), NULL);
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        if (app->postprocbin && !gst_element_link(app->postprocbin, vconvert))
            return app_pipeline_error(app, "postprocbin to vconvert link");
        if (!gst_element_link(vconvert, capsfilter))
            return app_pipeline_error(app, "videoconvert to capsfilter link");

        app->postprocbin = capsfilter;
        if (!app->postprocbin_head)
            app->postprocbin_head = vconvert;
        break;
    case MVT_HWACCEL_VAAPI:
        if (!app_init_postprocbin_for_vaapipostproc(app, 0))
            return FALSE;

        g_object_set(app->vapostproc[0], "format", out_format, NULL);
        break;
    }
    return TRUE;
}

/* Initializes pipeline for optional "videoscale" elements */
static gboolean
app_init_postprocbin_for_videoscale(App *app, const VideoScaleOptions *vso,
    guint n)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    GstElement *vscale, *capsfilter;
    GstCaps *caps;

    if (!vso->width || !vso->height)
        return TRUE;

    switch (options->hwaccel) {
    case MVT_HWACCEL_NONE:
        vscale = gst_element_factory_make("videoscale", NULL);
        if (!gst_bin_add(GST_BIN(app->pipeline), vscale))
            return app_pipeline_error(app, "failed to append videoscale");

        g_object_set(vscale, "method", vso->method, NULL);

        capsfilter = gst_element_factory_make("capsfilter", NULL);
        if (!gst_bin_add(GST_BIN(app->pipeline), capsfilter))
            return app_pipeline_error(app, "failed to append capsfilter");

        caps = gst_caps_new_simple("video/x-raw",
            "width", G_TYPE_INT, vso->width,
            "height", G_TYPE_INT, vso->height, NULL);
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        if (app->postprocbin && !gst_element_link(app->postprocbin, vscale))
            return app_pipeline_error(app, "postprocbin to vscale link");
        if (!gst_element_link(vscale, capsfilter))
            return app_pipeline_error(app, "videoscale to capsfilter link");

        app->postprocbin = capsfilter;
        if (!app->postprocbin_head)
            app->postprocbin_head = vscale;
        break;
    case MVT_HWACCEL_VAAPI:
        if (!app_init_postprocbin_for_vaapipostproc(app, n))
            return FALSE;

        g_object_set(app->vapostproc[n], "scale-method", vso->method,
            "width", vso->width, "height", vso->height, NULL);

        /* XXX: force NV12 format negotiation */
        if (n == 0 && !app->vconvert_options.format)
            g_object_set(app->vapostproc[n], "format", GST_VIDEO_FORMAT_NV12,
                NULL);
        break;
    }
    return TRUE;
}

static gboolean
app_init_pipeline(App *app)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    guint i;

    struct map_entry {
        GstElement    **element_ptr;    // pointer to resulting element
        const gchar    *factoryname;    // named factory to instantiate
        const gchar    *name;           // element name, or NULL
        const char     *desc;           // description for errors
    };
    struct map_entry g_build_map[] = {
        { &app->source,     "filesrc",      NULL,   "source"            },
        { &app->decodebin,  "decodebin",    NULL,   "decodebin"         },
        { &app->vsink,      "fakesink",     NULL,   "video sink (fake)" },
        { NULL, }
    };
    const struct map_entry *m;

    app->pipeline = gst_pipeline_new("video-player");
    if (!app->pipeline)
        return app_pipeline_error(app, "top-level");

    /* Setup the pipeline */
    for (m = g_build_map; m->element_ptr != NULL; m++) {
        GstElement * const e =
            gst_element_factory_make(m->factoryname, m->name);
        if (!e)
            return app_pipeline_error(app, m->desc);
        *m->element_ptr = e;
    }

    g_object_set(app->source, "location", options->filename, NULL);
    g_object_set(app->vsink, "sync", FALSE, NULL);
    g_object_set(app->vsink, "signal-handoffs", TRUE, NULL);

    gst_bin_add_many(GST_BIN(app->pipeline), app->source, app->decodebin, NULL);

    app->postprocbin = NULL;
    app->postprocbin_head = NULL;

    if (!app_init_postprocbin_for_videoconvert(app, &app->vconvert_options))
        return FALSE;
    for (i = 0; i < G_N_ELEMENTS(app->vscale_options); i++) {
        const VideoScaleOptions * const vso = &app->vscale_options[i];
        if (vso->is_valid && !app_init_postprocbin_for_videoscale(app, vso, i))
            return FALSE;
    }

    if (!gst_bin_add(GST_BIN(app->pipeline), app->vsink))
        return FALSE;
    if (app->postprocbin && !gst_element_link(app->postprocbin, app->vsink))
        return FALSE;
    if (!app->postprocbin_head)
        app->postprocbin_head = app->vsink;

    if (!gst_element_link(app->source, app->decodebin))
        return app_pipeline_error(app, "source to decodebin link");
    g_signal_connect(app->decodebin, "pad-added",
        G_CALLBACK(on_pad_added), app->postprocbin_head);
    g_signal_connect(app->decodebin, "element-added",
        G_CALLBACK(on_element_added), app);
    g_signal_connect(app->decodebin, "autoplug-query",
        G_CALLBACK(on_autoplug_query), app);
    g_signal_connect(app->decodebin, "autoplug-select",
        G_CALLBACK(on_autoplug_select), app);
    g_signal_connect(app->vsink, "handoff",
        G_CALLBACK(on_handoff), app);
    return TRUE;
}

static inline gboolean
app_bus_error(App *app, const gchar *message)
{
    app_set_error_message(app, APP_ERROR_BUS, message);
    return FALSE;
}

static gboolean
on_bus_message(GstBus *bus, GstMessage *msg, gpointer data)
{
    App * const app = data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        g_main_loop_quit(app->loop);
        break;
    case GST_MESSAGE_ERROR: {
        GError *error;
        gchar *debug;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        /* XXX: make sure to return a sensible error code */
        app_error(app, "failed to play back file (%s)", error->message);
        g_error_free(error);
        break;
    }
#if GST_CHECK_VERSION(1,1,0)
    case GST_MESSAGE_NEED_CONTEXT: {
        const gchar *type = NULL;

        if (!app->context)
            break;
        if (!gst_message_parse_context_type(msg, &type))
            break;
        if (g_strcmp0(type, GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME) != 0)
            break;
        gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(msg)),
            app->context);
        break;
    }
#elif GST_CHECK_VERSION(1,0,0)
    case GST_MESSAGE_ELEMENT: {
        GstVideoContext *context;
        const gchar **types;

        if (gst_video_context_message_parse_prepare(msg, &types, &context)) {
            gint i;
            for (i = 0; types[i] != NULL; i++) {
                if (strcmp(types[i], "gst-vaapi-display") == 0)
                    break;
            }
            if (types[i] == 0) {
                app_error(app, "failed to send GstVaapiDisplay upstream");
                break;
            }
            gst_video_context_set_context_pointer(context,
                "gst-vaapi-display", app->display);
        }
        break;
    }
#endif
    default:
        break;
    }
    return TRUE;
}

static gboolean
app_init_bus(App *app)
{
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE(app->pipeline));
    if (!bus)
        return app_bus_error(app, "pipeline error");

    app->bus_watch_id = gst_bus_add_watch(bus, on_bus_message, app);
    gst_object_unref(bus);
    return TRUE;
}

static gboolean
app_ensure_context(App *app)
{
#if GST_CHECK_VERSION(1,1,0) && 0
    /* XXX: enable this code only if gst_vaapi_display_get_type() is
       exposed from libgstvaapi core library */
    GstStructure *structure;

    if (app->context)
        return TRUE;

    app->context = gst_context_new(GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME, TRUE);
    if (!app->context)
        return FALSE;

    structure = gst_context_writable_structure(app->context);
    gst_structure_set(structure, "display", GST_VAAPI_TYPE_DISPLAY,
        app->display, NULL);
#endif
    return TRUE;
}

static inline gboolean
app_winsys_error(App *app, const gchar *message)
{
    app_set_error_message(app, APP_ERROR_WINSYS, message);
    return FALSE;
}

static gboolean
app_init_winsys(App *app)
{
    app->display = winsys_create_display(NULL);
    if (!app->display)
        return app_winsys_error(app, "display");

    if (!app_ensure_context(app))
        return FALSE;
    return TRUE;
}

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

static bool
decoder_init(MvtDecoder *decoder)
{
    App * const app = (App *)decoder;

    if (!app_init_gst(app))
        goto error_init_gst;
    if (!app_init(app))
        goto error_alloc_memory;
    if (!app_init_pipeline(app))
        goto error_init_pipeline;
    if (!app_init_bus(app))
        goto error_init_bus;
    if (!app_init_winsys(app))
        goto error_init_winsys;
    return true;

    /* ERRORS */
error_alloc_memory:
    mvt_error("failed to allocate memory");
    return false;
error_init_gst:
    mvt_error("failed to parse arguments (%s)", app->error->message);
    return false;
error_init_pipeline:
    mvt_error("failed to initialize pipeline (%s)", app->error->message);
    return false;
error_init_bus:
    mvt_error("failed to initialize bus (%s)", app->error->message);
    return false;
error_init_winsys:
    mvt_error("failed to initialize windowing system (%s)", app->error->message);
    return false;
}

static bool
decoder_init_option(MvtDecoder *decoder, const char *key, const char *value)
{
    App * const app = (App *)decoder;
    const char *subkey;
    GstCaps *caps;

    if (!app_init_gst(app))
        return false;

    if (!g_strcmp0(key, "vparse_capsfilter") && value) {
        do {
            caps = gst_caps_from_string(value);
            if (!caps)
                break;
            gst_caps_replace(&app->vparse_capsfilter, caps);
            gst_caps_unref(caps);
            return true;
        } while (0);
    }

    /* Allow "vparse" option to select the video parser element */
    else if (!g_strcmp0(key, "vparse") && value) {
        g_free(app->vparse_name);
        app->vparse_name = g_strdup(value);
        if (app->vparse_name)
            return true;
    }

    /* Video convert */
    else if (!g_strcmp0(key, "vconvert") && value) {
        VideoConvertOptions * const vco = &app->vconvert_options;
        vco->format = video_format_from_name(value);
        if (!vco->format)
            goto error_parse_format;
        return true;
    }

    /* Video scaling */
    else if (!strncmp(key, "vscale", 6) && value) {
        VideoScaleOptions *vso;
        guint n = 0;

        for (subkey = &key[6]; *subkey && g_ascii_isdigit(*subkey); subkey++)
            n = (n * 10) + (*subkey - '0');

        if (n > 0 && --n >= G_N_ELEMENTS(app->vscale_options))
            goto error_max_vpp_chain_reached;
        vso = &app->vscale_options[n];

        /* output size */
        if (!*subkey) {
            if (!str_parse_size(value, &vso->width, &vso->height))
                goto error_parse_scale_size;
            if (vso->width == 0 || vso->height == 0)
                goto error_parse_scale_size;
            vso->is_valid = true;
            return true;
        }

        /* method */
        else if (!g_strcmp0(subkey, "_method")) {
            if (!str_parse_uint(value, &vso->method, 10))
                goto error_parse_scale_quality;
            return true;
        }
    }
    return false;

    /* ERRORS */
error_parse_format:
    mvt_error("failed to parse format argument ('%s')", value);
    return false;
error_parse_scale_size:
    mvt_error("failed to parse scale size argument ('%s')", value);
    return false;
error_parse_scale_quality:
    mvt_error("failed to parse scale quality argument ('%s')", value);
    return false;
error_max_vpp_chain_reached:
    mvt_error("reached max number of chained VPP elements ('%s')", key);
    return false;
}

static void
decoder_finalize(MvtDecoder *decoder)
{
    App * const app = (App *)decoder;

    app_finalize(app);
    if (app->gst_init_done)
        gst_deinit();
}

static bool
decoder_run(MvtDecoder *decoder)
{
    App * const app = (App *)decoder;

    gst_element_set_state(app->pipeline, GST_STATE_PLAYING);
    g_main_loop_run(app->loop);
    gst_element_set_state(app->pipeline, GST_STATE_NULL);
    if (!app->error)
        return true;

    mvt_error("%s", app->error->message);
    return false;
}

const MvtDecoderClass *
mvt_decoder_class(void)
{
    static const MvtDecoderClass g_class = {
        .size = sizeof(App),
        .init = decoder_init,
        .init_option = decoder_init_option,
        .finalize = decoder_finalize,
        .run = decoder_run
    };
    return &g_class;
}
