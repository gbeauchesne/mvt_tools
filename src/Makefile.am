lib_LTLIBRARIES = libmvt_utils.la

noinst_LTLIBRARIES =
if ENABLE_FFMPEG
noinst_LTLIBRARIES += libdec_ffmpeg.la
endif

pkglibexec_PROGRAMS =
if ENABLE_FFMPEG
pkglibexec_PROGRAMS += dec_ffmpeg
endif
if ENABLE_GSTREAMER
pkglibexec_PROGRAMS += dec_gstreamer
endif

# -----------------------------------------------------------------------------
# --- MVT Utilities                                                         ---
# -----------------------------------------------------------------------------

mvt_utils_cflags = \
	$(LIBVA_CFLAGS)		\
	-I$(top_srcdir)/ext/libyuv/upstream/include \
	$(NULL)

mvt_utils_libs = \
	$(LIBVA_LIBS)		\
	$(top_builddir)/ext/libyuv/libmvt_yuv.la \
	$(NULL)

mvt_utils_source_c = \
	mvt_codec.c		\
	mvt_display.c		\
	mvt_hash.c		\
	mvt_hash_adler32.c	\
	mvt_hash_md5.c		\
	mvt_image.c		\
	mvt_image_convert.c	\
	mvt_image_hash.c	\
	mvt_map.c		\
	mvt_memory.c		\
	mvt_messages.c		\
	mvt_report.c		\
	va_image_utils.c	\
	va_utils.c		\
	video_format.c		\
	$(NULL)

mvt_utils_source_h = \
	mvt_codec.h		\
	mvt_display.h		\
	mvt_hash.h		\
	mvt_hash_priv.h		\
	mvt_image.h		\
	mvt_image_priv.h	\
	mvt_macros.h		\
	mvt_map.h		\
	mvt_memory.h		\
	mvt_messages.h		\
	mvt_report.h		\
	sysdeps.h		\
	va_compat.h		\
	va_image_utils.h	\
	va_utils.h		\
	video_format.h		\
	$(NULL)

if USE_VA_DRM
mvt_utils_cflags	+= $(LIBVA_DRM_CFLAGS)
mvt_utils_libs		+= $(LIBVA_DRM_LIBS)
endif

if USE_VA_X11
mvt_utils_cflags	+= $(LIBVA_X11_CFLAGS)
mvt_utils_libs		+= $(LIBVA_X11_LIBS)
endif

libmvt_utils_la_SOURCES	= $(mvt_utils_source_c)
libmvt_utils_la_CFLAGS	= $(mvt_utils_cflags)
libmvt_utils_la_LIBADD	= $(mvt_utils_libs)

mvt_decoder_source_c = mvt_decoder.c
mvt_decoder_source_h = mvt_decoder.h

# -----------------------------------------------------------------------------
# --- FFmpeg decoder module                                                 ---
# -----------------------------------------------------------------------------

FFMPEG_CFLAGS	= $(LIBAVUTIL_CFLAGS) $(LIBAVFORMAT_CFLAGS) $(LIBAVCODEC_CFLAGS)
FFMPEG_LIBS	= $(LIBAVUTIL_LIBS) $(LIBAVFORMAT_LIBS) $(LIBAVCODEC_LIBS)

libdec_ffmpeg_cflags = \
	$(mvt_utils_cflags)	\
	$(FFMPEG_CFLAGS)	\
	$(NULL)

libdec_ffmpeg_libs = \
	libmvt_utils.la		\
	$(mvt_utils_libs)	\
	$(FFMPEG_LIBS)		\
	$(NULL)

libdec_ffmpeg_source_c = \
	$(mvt_decoder_source_c)	\
	ffmpeg_utils.c		\
	mvt_decoder_ffmpeg.c	\
	$(NULL)

libdec_ffmpeg_source_h = \
	$(mvt_decoder_source_h)	\
	ffmpeg_compat.h		\
	ffmpeg_utils.h		\
	mvt_decoder_ffmpeg.h	\
	$(NULL)

libdec_ffmpeg_la_SOURCES	= $(libdec_ffmpeg_source_c)
libdec_ffmpeg_la_CFLAGS		= $(libdec_ffmpeg_cflags)
libdec_ffmpeg_la_LIBADD		= $(libdec_ffmpeg_libs)

dec_ffmpeg_source_c		= dec_ffmpeg.c
dec_ffmpeg_SOURCES		= $(dec_ffmpeg_source_c)
dec_ffmpeg_CFLAGS		= $(libdec_ffmpeg_cflags)
dec_ffmpeg_LDADD		= libdec_ffmpeg.la

# -----------------------------------------------------------------------------
# --- GStreamer decoder module                                              ---
# -----------------------------------------------------------------------------

dec_gstreamer_cflags = \
	$(mvt_utils_cflags)	\
	$(GLIB_CFLAGS)		\
	$(NULL)

dec_gstreamer_libs = \
	libmvt_utils.la		\
	$(mvt_utils_libs)	\
	$(GLIB_LIBS)		\
	$(NULL)

dec_gstreamer_source_c = \
	$(mvt_decoder_source_c)	\
	dec_gstreamer.c		\
	$(NULL)

dec_gstreamer_source_h = \
	$(mvt_decoder_source_h)	\
	$(NULL)

dec_gstreamer_cflags	+= -DGST_USE_UNSTABLE_API
dec_gstreamer_cflags	+= $(GST_VAAPI_CFLAGS) $(GST_VIDEO_CFLAGS)
dec_gstreamer_libs	+= $(GST_VAAPI_LIBS) $(GST_VIDEO_LIBS)

if GST_VAAPI_BACKEND_DRM
dec_gstreamer_cflags	+= $(GST_VAAPI_DRM_CFLAGS)
dec_gstreamer_libs	+= $(GST_VAAPI_DRM_LIBS)
endif

if GST_VAAPI_BACKEND_X11
dec_gstreamer_cflags	+= $(GST_VAAPI_X11_CFLAGS)
dec_gstreamer_libs	+= $(GST_VAAPI_X11_LIBS)
endif

dec_gstreamer_SOURCES	= $(dec_gstreamer_source_c)
dec_gstreamer_CFLAGS	= $(dec_gstreamer_cflags)
dec_gstreamer_LDADD	= $(dec_gstreamer_libs)

EXTRA_DIST = \
	$(mvt_decoder_source_c)		\
	$(mvt_decoder_source_h)		\
	$(mvt_utils_source_h)		\
	$(libdec_ffmpeg_source_c)	\
	$(libdec_ffmpeg_source_h)	\
	$(dec_ffmpeg_source_c)		\
	$(dec_ffmpeg_source_h)		\
	$(dec_gstreamer_source_c)	\
	$(dec_gstreamer_source_h)	\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
