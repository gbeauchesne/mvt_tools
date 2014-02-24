# Media Validation Tool (MVT) package version number
m4_define([mvt_major_version], [0])
m4_define([mvt_minor_version], [2])
m4_define([mvt_micro_version], [0])
m4_define([mvt_pre_version],   [1])
m4_define([mvt_version],
          [mvt_major_version.mvt_minor_version.mvt_micro_version])
m4_if(mvt_pre_version, [0], [], [
m4_append([mvt_version], mvt_pre_version, [.pre])
])

# Configure defaults
m4_define([default_renderer],           [drm])
m4_define([default_ffmpeg],             [yes])
m4_define([default_builtin_ffmpeg],     [no])
m4_define([default_gstreamer],          [yes])
m4_define([default_ref_libvpx],         [yes])
m4_define([default_builtin_libvpx],     [no])

# FFmpeg version number
m4_define([libavutil_version],          [51.22.1])
m4_define([libavformat_version],        [53.21.1])
m4_define([libavcodec_version],         [53.35.0])

# glib version number
m4_define([glib_version],               [2.28])

# gstreamer version number
m4_define([gst_api_version],            [autodetect])
m4_define([gst0_version],               [0.10.36])
m4_define([gst1_version],               [1.2.0])

# gstreamer-vaapi version number
m4_define([gst_vaapi_version],          [0.5.4])

# VA-API minimum version number
m4_define([va_api_version],             [0.30.4])
m4_define([va_api_drm_version],         [0.33.0])
m4_define([va_api_x11_version],         [0.31.0])

# libva package version number
m4_define([libva_drm_package_version],  [1.1.0])
m4_define([libva_x11_package_version],  [1.0.3])

# libvpx version number
m4_define([libvpx_version],             [1.2.0])

AC_PREREQ([2.66])
AC_INIT([mvt], [mvt_version],
    [gwenole.beauchesne@intel.com],
    [mvt])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-bzip2])

TODAY="`LC_ALL=C date +'%a, %d %b %Y %X %z'`"
AC_SUBST([TODAY])

LIBVA_PACKAGE_VERSION=libva_x11_package_version
AC_SUBST([LIBVA_PACKAGE_VERSION])

TARGET_ARCH="$target_cpu"
AC_SUBST([TARGET_ARCH])

dnl Use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

dnl Configure options
AC_ARG_ENABLE([ffmpeg],
    AS_HELP_STRING([--enable-ffmpeg],
                   [build FFmpeg module @<:@default=default_ffmpeg@:>@]),
    [], [enable_ffmpeg=default_ffmpeg])
AC_ARG_ENABLE([builtin-ffmpeg],
    AS_HELP_STRING([--enable-builtin-ffmpeg],
                   [build built-in FFmpeg module @<:@default=default_builtin_ffmpeg@:>@]),
    [], [enable_builtin_ffmpeg=default_builtin_ffmpeg])
AC_ARG_ENABLE([gstreamer],
    AS_HELP_STRING([--enable-gstreamer],
                   [build GStreamer module @<:@default=default_gstreamer@:>@]),
    [], [enable_gstreamer=default_gstreamer])

AC_ARG_ENABLE([ref-libvpx],
    AS_HELP_STRING([--enable-ref-libvpx],
        [build reference libvpx decoders @<:@default=default_ref_libvpx@:>@]),
    [], [enable_ref_libvpx=default_ref_libvpx])
AC_ARG_ENABLE([builtin-libvpx],
    AS_HELP_STRING([--enable-builtin-libvpx],
                   [build built-in Libvpx module @<:@default=default_builtin_libvpx@:>@]),
    [], [enable_builtin_libvpx=default_builtin_libvpx])

AC_ARG_WITH([renderer],
    AC_HELP_STRING([--with-renderer=API],
                   [build with the specified rendering API
                    @<:@default=default_renderer@:>@]),
    [MVT_RENDERER="$with_renderer"], [MVT_RENDERER=default_renderer])
AC_ARG_WITH([gstreamer-api],
    AC_HELP_STRING([--with-gstreamer-api=VERSION],
                   [build against the specified GStreamer API version
                    @<:@default=gst_api_version@:>@]),
    [GST_API_VERSION="$with_gstreamer_api"], [GST_API_VERSION=gst_api_version])

dnl Resolve dependencies
if test "$enable_ref_libvpx" = "yes"; then
    enable_ffmpeg="yes"
fi

dnl Check for tools
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

AC_ARG_VAR([YASM], [Path to yasm program, if any])
AC_PATH_PROG([YASM], [yasm])

AC_C_BIGENDIAN
AC_CHECK_LIB([m], [log10])

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl MVT_LT_LDFLAGS:
MVT_LT_CURRENT="$MVT_MAJOR_VERSION"
MVT_LT_REV=mvt_lt_revision
MVT_LT_AGE=mvt_lt_age
MVT_LT_VERSION="$MVT_LT_CURRENT:$MVT_LT_REV:$MVT_LT_AGE"
MVT_LT_LDFLAGS="-version-info $MVT_LT_VERSION"
AC_SUBST([MVT_LT_VERSION])
AC_SUBST([MVT_LT_LDFLAGS])

dnl Check for GLib
GLIB_VERSION_REQUIRED=glib_version
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= $GLIB_VERSION_REQUIRED])
AC_SUBST([GLIB_VERSION_REQUIRED])

dnl ---------------------------------------------------------------------------
dnl -- Renderers                                                             --
dnl ---------------------------------------------------------------------------

dnl Renderer
case "$MVT_RENDERER" in
    drm)        MVT_RENDERER_NAME="DRM";;
    x11)        MVT_RENDERER_NAME="X11";;
    *) AC_MSG_ERROR([Unsupported renderer $MVT_RENDERER]);;
esac
AC_SUBST([MVT_RENDERER])

dnl Check for DRM
USE_DRM=0
if test "$MVT_RENDERER" = "drm"; then
    PKG_CHECK_MODULES([DRM], [libdrm])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $DRM_CFLAGS"
    AC_CHECK_HEADERS([xf86drm.h],
        [USE_DRM=1], [AC_MSG_ERROR([incomplete libdrm-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for X11
USE_X11=0
if test "$MVT_RENDERER" = "x11"; then
    PKG_CHECK_MODULES([X11], [x11])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X11_CFLAGS"
    AC_CHECK_HEADERS([X11/Xlib.h X11/Xutil.h X11/Xatom.h],
        [USE_X11=1], [AC_MSG_ERROR([incomplete libx11-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl -- VA-API                                                                --
dnl ---------------------------------------------------------------------------

dnl Core API
LIBVA_PKGNAME="libva"
PKG_CHECK_MODULES([LIBVA], [$LIBVA_PKGNAME >= va_api_version])
AC_SUBST([LIBVA_PKGNAME])

VA_VERSION=`$PKG_CONFIG --modversion libva`
VA_MAJOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f1`
VA_MINOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f2`
VA_MICRO_VERSION=`echo "$VA_VERSION" | cut -d'.' -f3`
VA_VERSION_STR="$VA_VERSION"

dnl VA/DRM API
USE_VA_DRM=0
if test $USE_DRM -eq 1; then
    PKG_CHECK_MODULES([LIBVA_DRM], [libva-drm >= va_api_drm_version])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$LIBVA_DRM_CFLAGS"
    AC_CHECK_HEADERS([va/va_drm.h], [USE_VA_DRM=1],
        [AC_MSG_ERROR([incomplete libva-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi
AC_DEFINE_UNQUOTED([USE_VA_DRM], [$USE_VA_DRM],
    [Defined to 1 if libva (DRM) backend is enabled])
AM_CONDITIONAL([USE_VA_DRM], [test $USE_VA_DRM -eq 1])

dnl VA/X11 API
USE_VA_X11=0
if test $USE_X11 -eq 1; then
    PKG_CHECK_MODULES([LIBVA_X11], [libva-x11 >= va_api_x11_version])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$LIBVA_X11_CFLAGS"
    AC_CHECK_HEADERS([va/va_x11.h], [USE_VA_X11=1],
        [AC_MSG_ERROR([incomplete libva-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi
AC_DEFINE_UNQUOTED([USE_VA_X11], [$USE_VA_X11],
    [Defined to 1 if libva (X11) backend is enabled])
AM_CONDITIONAL([USE_VA_X11], [test $USE_VA_X11 -eq 1])

dnl ---------------------------------------------------------------------------
dnl -- FFmpeg                                                                --
dnl ---------------------------------------------------------------------------

dnl Check for built-in FFmpeg
if test "$enable_builtin_ffmpeg" = "yes"; then
    enable_ffmpeg="yes"
fi

dnl Check for libavutil
if test "$enable_ffmpeg:$enable_builtin_ffmpeg" = "yes:no"; then
    PKG_CHECK_MODULES([LIBAVUTIL], [libavutil >= libavutil_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVUTIL_CFLAGS"
    AC_CHECK_HEADERS([libavutil/pixfmt.h libavutil/pixdesc.h],
        [], [AC_MSG_ERROR([incomplete libavutil-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for libavformat
if test "$enable_ffmpeg:$enable_builtin_ffmpeg" = "yes:no"; then
    PKG_CHECK_MODULES([LIBAVFORMAT], [libavformat >= libavformat_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVFORMAT_CFLAGS"
    AC_CHECK_HEADERS([libavformat/avformat.h],
        [], [AC_MSG_ERROR([incomplete libavformat-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for libavcodec
if test "$enable_ffmpeg:$enable_builtin_ffmpeg" = "yes:no"; then
    PKG_CHECK_MODULES([LIBAVCODEC], [libavcodec >= libavcodec_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVCODEC_CFLAGS"
    AC_CHECK_HEADERS([libavcodec/avcodec.h],
        [], [AC_MSG_ERROR([incomplete libavcodec-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl -- GStreamer                                                             --
dnl ---------------------------------------------------------------------------

dnl Versions for GStreamer and plugins-base
dnl If GStreamer API version is to be autodetected, then always try
dnl with the latest version first
if test "$enable_gstreamer" = "yes"; then
    AC_MSG_CHECKING([for GStreamer API version])
    if test "$GST_API_VERSION" = "autodetect"; then
        gst_api_versions="1.0"
    else
        gst_api_versions="$GST_API_VERSION"
    fi
    for gst_api_version in ${gst_api_versions}; do
        if $PKG_CONFIG --exists "gstreamer-$gst_api_version"; then
            GST_API_VERSION="$gst_api_version"
            break
        fi
    done
    if test -z "$GST_API_VERSION"; then
        AC_MSG_ERROR([version $GST_API_VERSION not found])
    fi
    AC_MSG_RESULT([$GST_API_VERSION])
fi

dnl Versions for GStreamer
if test "$enable_gstreamer" = "yes"; then
    case $GST_API_VERSION in
    1.0)
        GST_VERSION_REQUIRED=gst1_version
        ;;
    *)
        AC_MSG_ERROR([unsupported GStreamer API version $GST_API_VERSION])
        ;;
    esac
fi
AC_SUBST([GST_API_VERSION])
AC_SUBST([GST_VERSION_REQUIRED])

dnl GStreamer/VA-API libraries
GST_VAAPI_VERSION=gst_vaapi_version
AC_SUBST([GST_VAAPI_VERSION])

if test "$enable_gstreamer" = "yes"; then
    PKG_CHECK_MODULES([GST_VAAPI],
        [gstreamer-vaapi-$GST_API_VERSION >= $GST_VAAPI_VERSION])
    PKG_CHECK_MODULES([GST_VIDEO],
        [gstreamer-video-$GST_API_VERSION >= $GST_VERSION_REQUIRED])
fi

dnl Check for DRM backend
USE_GST_VAAPI_BACKEND_DRM=0
if test "$enable_gstreamer" = "yes" -a "$MVT_RENDERER" = "drm"; then
    PKG_CHECK_MODULES([GST_VAAPI_DRM],
        [gstreamer-vaapi-drm-$GST_API_VERSION >= $GST_VAAPI_VERSION])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $GST_VAAPI_DRM_CFLAGS $GST_VAAPI_CFLAGS"
    AC_CHECK_HEADERS([gst/vaapi/gstvaapidisplay_drm.h],
        [USE_GST_VAAPI_BACKEND_DRM=1])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for X11 backend
USE_GST_VAAPI_BACKEND_X11=0
if test "$enable_gstreamer" = "yes" -a "$MVT_RENDERER" = "x11"; then
    PKG_CHECK_MODULES([GST_VAAPI_X11],
        [gstreamer-vaapi-x11-$GST_API_VERSION >= $GST_VAAPI_VERSION])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $GST_VAAPI_X11_CFLAGS $GST_VAAPI_CFLAGS"
    AC_CHECK_HEADERS([gst/vaapi/gstvaapidisplay_x11.h],
        [USE_GST_VAAPI_BACKEND_X11=1])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl -- Reference Decoders
dnl ---------------------------------------------------------------------------

dnl Check for built-in FFmpeg
if test "$enable_builtin_libvpx" = "yes"; then
    enable_libvpx="yes"
fi

dnl Check for libvpx
USE_LIBVPX=0
if test "$enable_ref_libvpx:$enable_builtin_libvpx" = "yes:no"; then
    PKG_CHECK_MODULES([LIBVPX], [vpx >= libvpx_version], [],
        [AC_MSG_WARN([libvpx-dev package not found. Disabling]);
         enable_ref_libvpx="no"])
fi
if test "$enable_ref_libvpx:$enable_builtin_libvpx" = "yes:no"; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$LIBVPX_CFLAGS"
    AC_CHECK_HEADERS([vpx/vpx_decoder.h], [USE_LIBVPX=1],
        [AC_MSG_WARN([incomplete libvpx-dev (headers) package. Disabling]);
         enable_ref_libvpx="no"])
    CPPFLAGS="$saved_CPPFLAGS"
fi
if test "$enable_ref_libvpx:$enable_builtin_libvpx" = "yes:yes"; then
    USE_LIBVPX=1
fi
AC_DEFINE_UNQUOTED([USE_LIBVPX], [$USE_LIBVPX],
    [Defined to 1 if VPx (libvpx) reference decoder is enabled])
AM_CONDITIONAL([ENABLE_REF_VPX], [test $USE_LIBVPX -eq 1])

dnl ---------------------------------------------------------------------------
dnl -- Generate files and summary                                            --
dnl ---------------------------------------------------------------------------

AM_CONDITIONAL([ENABLE_GSTREAMER],      [test "$enable_gstreamer" = "yes"])
AM_CONDITIONAL([ENABLE_FFMPEG],         [test "$enable_ffmpeg" = "yes"])
AM_CONDITIONAL([ENABLE_BUILTIN_FFMPEG], [test "$enable_builtin_ffmpeg" = "yes"])
AM_CONDITIONAL([ENABLE_BUILTIN_LIBVPX], [test "$enable_builtin_libvpx" = "yes"])

AC_DEFINE_UNQUOTED([GST_VAAPI_BACKEND_DRM], [$USE_GST_VAAPI_BACKEND_DRM],
    [Defined to 1 if gstreamer-vaapi (DRM) backend is enabled])
AM_CONDITIONAL([GST_VAAPI_BACKEND_DRM], [test $USE_GST_VAAPI_BACKEND_DRM -eq 1])

AC_DEFINE_UNQUOTED([GST_VAAPI_BACKEND_X11], [$USE_GST_VAAPI_BACKEND_X11],
    [Defined to 1 if gstreamer-vaapi (X11) backend is enabled])
AM_CONDITIONAL([GST_VAAPI_BACKEND_X11], [test $USE_GST_VAAPI_BACKEND_X11 -eq 1])

AC_CONFIG_FILES([
    Makefile
    ext/Makefile
    ext/ffmpeg/Makefile
    ext/files/Makefile
    ext/libvpx/Makefile
    ext/libyuv/Makefile
    src/Makefile
])
AC_OUTPUT

dnl Print summary
echo
echo $PACKAGE configuration summary:
echo
echo Renderer ......................... : $MVT_RENDERER_NAME
echo VA-API version ................... : $VA_VERSION_STR
echo Enable FFmpeg .................... : $enable_ffmpeg
echo Enable GStreamer ................. : $enable_gstreamer
if test "$enable_gstreamer" = "yes"; then
echo GStreamer API version ............ : $GST_API_VERSION
fi
echo Enable reference VPx decoder ..... : $enable_ref_libvpx
echo
