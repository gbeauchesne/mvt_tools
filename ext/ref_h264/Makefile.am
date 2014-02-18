# Makefile.am - Rules for the built-in H.264 reference decoders
#
# Copyright (C) 2014 Intel Corporation
#   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1
# of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 

lib_LTLIBRARIES =
if ENABLE_REF_H264
lib_LTLIBRARIES += libmvt_ref_jm.la
endif

FILES_ROOT	= $(top_srcdir)/ext/files
fetch_sources	= $(FILES_ROOT)/fetch_sources
jm_tarball	= $(FILES_ROOT)/ref_h264_avc_sources.zip

# -----------------------------------------------------------------------------
# --- JM Reference Software (H.264 AVC)                                     ---
# -----------------------------------------------------------------------------

# Files for packaging
include $(srcdir)/sources.jm.frag

jm_cflags = \
	-I$(jm_srcdir)/ldecod/inc		\
	-I$(jm_srcdir)/lcommon/inc		\
	-std=gnu99				\
	-ffloat-store				\
	-fno-strict-aliasing			\
	-fsigned-char				\
	$(NULL)

jm_libs = \
	$(NULL)

jm_ldflags = \
	-Wl,--version-script,$(srcdir)/libmvt_ref_jm.vers \
	$(NULL)

jm_sources = \
	$(jm_srcdir)/COPYRIGHT_ISO_IEC.txt	\
	$(jm_srcdir)/COPYRIGHT_ITU.txt		\
	$(jm_srcdir)/disclaimer.txt		\
	$(jm_source_c)				\
	$(jm_source_h)				\
	$(NULL)

libmvt_ref_jm_la_SOURCES	= $(jm_source_c)
libmvt_ref_jm_la_CFLAGS		= $(jm_cflags)
libmvt_ref_jm_la_LIBADD		= $(jm_libs)
libmvt_ref_jm_la_LDFLAGS	= $(jm_ldflags)

BUILT_SOURCES = .jm.extract

.jm.extract:
	-rm -f .jm.extract.tmp
	-touch .jm.extract.tmp
	$(MAKE) .jm.sources
	-mv -f .jm.extract.tmp .jm.extract
$(jm_sources): .jm.extract
.jm.sources: $(jm_tarball)
	-rm -f .jm.sources.tmp
	-touch .jm.sources.tmp
	-$(UNZIP) -o -d $(srcdir) $(jm_tarball)
	-mv -f .jm.sources.tmp $@
$(jm_tarball): $(fetch_sources)
	-$(fetch_sources) REF_H264_AVC

CLEANFILES = \
	.jm.sources		\
	.jm.sources.tmp		\
	.jm.extract.tmp		\
	$(NULL)

EXTRA_DIST = \
	libmvt_ref_jm.vers	\
	.jm.extract		\
	$(jm_sources)		\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
