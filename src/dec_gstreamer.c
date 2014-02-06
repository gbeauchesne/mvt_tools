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
/* --- GStreamer based decoder                                          --- */
/* ------------------------------------------------------------------------ */

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
    GstElement         *vsink;
    GstCaps            *vsink_caps;
    GstVideoInfo        vinfo;
    guint               bus_watch_id;
    GstVaapiDisplay    *display;
#if GST_CHECK_VERSION(1,1,0)
    GstContext         *context;
#endif
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

    if (!gst_video_info_from_caps(vip, caps))
        goto error_invalid_caps;
    GST_INFO("new caps: format %s, size %dx%d",
        gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(vip)),
        GST_VIDEO_INFO_WIDTH(vip), GST_VIDEO_INFO_HEIGHT(vip));
    return TRUE;

    /* ERRORS */
error_invalid_caps:
    mvt_error("invalid or unsupported caps received");
    return FALSE;
}

// Handle SW surfaces
static gboolean
app_handle_sw_surface(App *app, GstBuffer *buffer)
{
    GstVideoFrame frame;
    VideoFormat format;
    MvtImage src_image;
    VAImage va_image;
    const VAImageFormat *va_format;
    uint32_t i;
    gboolean success;

    switch (GST_VIDEO_INFO_FORMAT(&app->vinfo)) {
    case GST_VIDEO_FORMAT_I420:
        format = VIDEO_FORMAT_I420;
        break;
    case GST_VIDEO_FORMAT_YV12:
        format = VIDEO_FORMAT_YV12;
        break;
    case GST_VIDEO_FORMAT_NV12:
        format = VIDEO_FORMAT_NV12;
        break;
    case GST_VIDEO_FORMAT_YUY2:
        format = VIDEO_FORMAT_YUY2;
        break;
    case GST_VIDEO_FORMAT_UYVY:
        format = VIDEO_FORMAT_UYVY;
        break;
    case GST_VIDEO_FORMAT_AYUV:
        format = VIDEO_FORMAT_AYUV;
        break;
    default:
        format = VIDEO_FORMAT_UNKNOWN;
        break;
    }
    if (!format || !(va_format = video_format_to_va_format(format)))
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

    success = mvt_decoder_handle_image(&app->decoder, &src_image, 0);
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

    /* Filter out factories that are not "media-video" + "decoder" */
    if (!gst_element_factory_list_is_type(factory,
            GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO) &&
        !gst_element_factory_list_is_type(factory,
            GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE))
        return GST_AUTOPLUG_SELECT_TRY;
    if (!gst_element_factory_list_is_type(factory,
            GST_ELEMENT_FACTORY_TYPE_DECODER))
        return GST_AUTOPLUG_SELECT_TRY;

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

static gboolean
on_handoff(GstElement *element, GstBuffer *buffer, GstPad *pad, App *app)
{
    const MvtDecoderOptions * const options = &app->decoder.options;
    GstCaps *caps;
    gboolean caps_changed;

    /* Update the video caps */
    caps = gst_pad_get_current_caps(pad);
    caps_changed = caps != app->vsink_caps;
    gst_caps_replace(&app->vsink_caps, caps);
    gst_caps_unref(caps);
    if (caps_changed) {
        if (!app_update_video_info(app, app->vsink_caps))
            return FALSE;
    }

    /* Handle the decoded buffer */
    switch (options->hwaccel) {
    case MVT_HWACCEL_NONE:
        if (!app_handle_sw_surface(app, buffer))
            return FALSE;
        break;
    }
    return TRUE;
}

static gboolean
app_init_pipeline(App *app)
{
    const MvtDecoderOptions * const options = &app->decoder.options;

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

    gst_bin_add_many(GST_BIN(app->pipeline), app->source, app->decodebin,
        app->vsink, NULL);

    if (!gst_element_link(app->source, app->decodebin))
        return app_pipeline_error(app, "source to decodebin link");
    g_signal_connect(app->decodebin, "pad-added",
        G_CALLBACK(on_pad_added), app->vsink);
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
    GError *error = NULL;

    if (!gst_init_check(NULL, NULL, &error))
        goto error_init_gst;
    app->gst_init_done = TRUE;

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
    mvt_error("failed to parse arguments (%s)", error->message);
    g_error_free(error);
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
        .finalize = decoder_finalize,
        .run = decoder_run
    };
    return &g_class;
}
