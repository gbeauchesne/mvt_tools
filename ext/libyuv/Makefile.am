# Makefile.am - Rules for the built-in libyuv
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

lib_LTLIBRARIES = libmvt_yuv.la

# Files for packaging
include $(srcdir)/sources.frag

libyuv_cflags = \
	-I$(libyuv_srcdir)/include \
	$(NULL)

libyuv_libs = \
	$(NULL)

libyuv_ldflags = \
	-Wl,--version-script,$(srcdir)/libmvt_yuv.vers \
	$(NULL)

libyuv_sources = \
	$(libyuv_srcdir)/AUTHORS		\
	$(libyuv_srcdir)/LICENSE		\
	$(libyuv_srcdir)/LICENSE_THIRD_PARTY	\
	$(libyuv_srcdir)/OWNERS			\
	$(libyuv_srcdir)/README.chromium	\
	$(libyuv_source_c)			\
	$(libyuv_source_h)			\
	$(NULL)

libmvt_yuv_la_SOURCES	= $(libyuv_source_c)
libmvt_yuv_la_CXXFLAGS	= $(libyuv_cflags)
libmvt_yuv_la_LIBADD	= $(libyuv_libs)
libmvt_yuv_la_LDFLAGS	= $(libyuv_ldflags)

EXTRA_DIST = \
	libmvt_yuv.vers		\
	$(libyuv_sources)	\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
