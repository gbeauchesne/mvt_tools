lib_LTLIBRARIES = libmvt_utils.la

pkglibexec_PROGRAMS =

# -----------------------------------------------------------------------------
# --- MVT Utilities                                                         ---
# -----------------------------------------------------------------------------

mvt_utils_cflags = \
	$(LIBVA_CFLAGS)		\
	$(NULL)

mvt_utils_libs = \
	$(LIBVA_LIBS)		\
	$(NULL)

mvt_utils_source_c = \
	mvt_display.c		\
	mvt_hash.c		\
	mvt_hash_adler32.c	\
	mvt_hash_md5.c		\
	mvt_map.c		\
	mvt_memory.c		\
	mvt_messages.c		\
	va_utils.c		\
	$(NULL)

mvt_utils_source_h = \
	mvt_display.h		\
	mvt_hash.h		\
	mvt_hash_priv.h		\
	mvt_macros.h		\
	mvt_map.h		\
	mvt_memory.h		\
	mvt_messages.h		\
	sysdeps.h		\
	va_compat.h		\
	va_utils.h		\
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

EXTRA_DIST = \
	$(mvt_utils_source_h)		\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
