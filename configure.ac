# Media Validation Tool (MVT) package version number
m4_define([mvt_major_version], [0])
m4_define([mvt_minor_version], [1])
m4_define([mvt_micro_version], [0])
m4_define([mvt_pre_version],   [1])
m4_define([mvt_version],
          [mvt_major_version.mvt_minor_version.mvt_micro_version])
m4_if(mvt_pre_version, [0], [], [
m4_append([mvt_version], mvt_pre_version, [.pre])
])

# Configure defaults
m4_define([default_renderer],           [drm])

# VA-API minimum version number
m4_define([va_api_version],             [0.30.4])
m4_define([va_api_drm_version],         [0.33.0])
m4_define([va_api_x11_version],         [0.31.0])

# libva package version number
m4_define([libva_drm_package_version],  [1.1.0])
m4_define([libva_x11_package_version],  [1.0.3])

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
AC_ARG_WITH([renderer],
    AC_HELP_STRING([--with-renderer=API],
                   [build with the specified rendering API
                    @<:@default=default_renderer@:>@]),
    [MVT_RENDERER="$with_renderer"], [MVT_RENDERER=default_renderer])

dnl Check for tools
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

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
dnl -- Generate files and summary                                            --
dnl ---------------------------------------------------------------------------

AC_CONFIG_FILES([
    Makefile
    ext/Makefile
    ext/files/Makefile
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
echo
