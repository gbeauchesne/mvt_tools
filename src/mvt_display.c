/*
 * mvt_display.c - VA display abstraction
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
#include "mvt_display.h"
#include "va_utils.h"

typedef bool (*MvtDisplayOpenFunc)(MvtDisplay *display);
typedef void (*MvtDisplayCloseFunc)(MvtDisplay *display);

/** Base display class */
typedef struct {
    uint32_t size;
    MvtDisplayOpenFunc open;
    MvtDisplayCloseFunc close;
} MvtDisplayClass;

/* ------------------------------------------------------------------------ */
/* --- DRM Display                                                      --- */
/* ------------------------------------------------------------------------ */

#if USE_VA_DRM
#include <va/va_drm.h>
#include <fcntl.h>
#include <unistd.h>

/* Define the default DRM device */
#define DEFAULT_DRM_DEVICE "/dev/dri/card0"

typedef struct {
    MvtDisplay base;
} MvtDisplayDRM;

static bool
mvt_display_drm_open(MvtDisplayDRM *display)
{
    MvtDisplay * const base_display = (MvtDisplay*)display;
    const char *device_name = base_display->display_name;
    int fd;

    base_display->native_display = (void *)(intptr_t)-1;

    if (!device_name)
        device_name = DEFAULT_DRM_DEVICE;

    fd = open(device_name, O_RDWR | O_CLOEXEC);
    if (fd < 0)
        goto error_open_device;
    base_display->native_display = (void *)(intptr_t)fd;

    base_display->va_display = vaGetDisplayDRM(fd);
    return true;

    /* ERRORS */
error_open_device:
    mvt_error("failed to open device `%s'", device_name);
    return false;
}

static void
mvt_display_drm_close(MvtDisplayDRM *display)
{
    MvtDisplay * const base_display = (MvtDisplay*)display;
    int fd;

    fd = (intptr_t)base_display->native_display;
    if (fd >= 0)
        close(fd);
}

static const MvtDisplayClass *
mvt_display_drm_class(void)
{
    static const MvtDisplayClass g_class = {
        .size = sizeof(MvtDisplayDRM),
        .open = (MvtDisplayOpenFunc)mvt_display_drm_open,
        .close = (MvtDisplayCloseFunc)mvt_display_drm_close,
    };
    return &g_class;
}
#endif

/* ------------------------------------------------------------------------ */
/* --- X11 Display                                                      --- */
/* ------------------------------------------------------------------------ */

#if USE_VA_X11
#include <va/va_x11.h>

typedef struct {
    MvtDisplay base;
} MvtDisplayX11;

static bool
mvt_display_x11_open(MvtDisplayX11 *display)
{
    MvtDisplay * const base_display = (MvtDisplay*)display;

    base_display->native_display = XOpenDisplay(base_display->display_name);
    if (!base_display->native_display)
        goto error_open_display;

    base_display->va_display = vaGetDisplay(base_display->native_display);
    return true;

    /* ERRORS */
error_open_display:
    mvt_error("failed to open display `%s'", base_display->display_name);
    return false;
}

static void
mvt_display_x11_close(MvtDisplayX11 *display)
{
    MvtDisplay * const base_display = (MvtDisplay*)display;

    if (base_display->native_display)
        XCloseDisplay(base_display->native_display);
}

static const MvtDisplayClass *
mvt_display_x11_class(void)
{
    static const MvtDisplayClass g_class = {
        .size = sizeof(MvtDisplayX11),
        .open = (MvtDisplayOpenFunc)mvt_display_x11_open,
        .close = (MvtDisplayCloseFunc)mvt_display_x11_close,
    };
    return &g_class;
}
#endif

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

static inline const MvtDisplayClass *
mvt_display_class(void)
{
#if USE_VA_DRM
    return mvt_display_drm_class();
#endif
#if USE_VA_X11
    return mvt_display_x11_class();
#endif
    assert(0 && "unsupported VA backend");
    return NULL;
}

static bool
display_init(MvtDisplay *display, const char *name)
{
    const MvtDisplayClass * const klass = mvt_display_class();
    int major_version, minor_version;
    VAStatus va_status;

    if (name) {
        display->display_name = strdup(name);
        if (!display->display_name)
            return false;
    }

    if (klass->open && (!klass->open(display) || !display->va_display))
        return false;

    va_status = vaInitialize(display->va_display,
        &major_version, &minor_version);
    if (!va_check_status(va_status, "vaInitialize()"))
        return false;
    return true;
}

static void
display_finalize(MvtDisplay *display)
{
    const MvtDisplayClass * const klass = mvt_display_class();

    if (display->va_display)
        vaTerminate(display->va_display);
    if (klass->close)
        klass->close(display);
    free(display->display_name);
}

// Creates a new display object and opens a connection to the native display
MvtDisplay *
mvt_display_new(const char *name)
{
    const MvtDisplayClass * const klass = mvt_display_class();
    MvtDisplay *display;

    display = calloc(1, klass->size);
    if (!display)
        return NULL;
    if (!display_init(display, name))
        goto error;
    return display;

error:
    mvt_display_free(display);
    return NULL;
}

// Closes the native display and deallocates all resources from MvtDisplay
void
mvt_display_free(MvtDisplay *display)
{
    if (!display)
        return;
    display_finalize(display);
}

// Releases MvtDisplay object and resets the supplied pointer to NULL
void
mvt_display_freep(MvtDisplay **display_ptr)
{
    if (display_ptr) {
        mvt_display_free(*display_ptr);
        *display_ptr = NULL;
    }
}
