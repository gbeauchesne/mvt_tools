# Makefile.am - Rules for the built-in libvpx sources
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
if ENABLE_BUILTIN_LIBVPX
lib_LTLIBRARIES += libmvt_vpx.la
endif

vpx_upstream	= upstream
vpx_srcdir	= $(srcdir)/$(vpx_upstream)
vpx_builddir	= $(builddir)/$(vpx_upstream)

vpx_libs = \
	-Wl,-Bsymbolic \
	-Wl,--whole-archive \
	-Wl,$(vpx_builddir)/libvpx.a \
	-Wl,--no-whole-archive \
	-Wl,--version-script,$(srcdir)/libmvt_vpx.vers \
	-Wl,-lpthread \
	-Wl,-lm \
	$(NULL)

libmvt_vpx_la_SOURCES			=
nodist_EXTRA_libmvt_vpx_la_SOURCES	= dummy.c
libmvt_vpx_la_DEPENDENCIES		= vpx.build
libmvt_vpx_la_LINK			= $(LINK) $(vpx_libs)

VPX_DEBUG	= yes

# Codecs
VP8_DECODER	= enable
VP8_ENCODER	= disable
VP9_DECODER	= enable
VP9_ENCODER	= disable

clean-local: vpx.clean
maintainer-clean-local: vpx.maintainer.clean

vpx.build: vpx.configure.stamp
	@$(MAKE) -C $(vpx_builddir)

vpx.clean:
	@[ -d $(vpx_builddir) ] && \
	$(MAKE) -C $(vpx_builddir) clean || :
	rm -f vpx.build.stamp vpx.configure.stamp

vpx.maintainer.clean: vpx.clean
	rm -rf $(vpx_builddir)

vpx.configure.stamp:
	@[ -d $(vpx_builddir) ] || mkdir $(vpx_builddir);		 \
	cd $(vpx_builddir)						;  \
	test "$(VPX_DEBUG)" = "yes"					&& \
	CONFIGURE_FLAGS="$$CONFIGURE_FLAGS --enable-debug"		; \
	$(abs_srcdir)/$(vpx_upstream)/configure $$CONFIGURE_FLAGS	\
		--enable-static						\
		--enable-pic						\
		--disable-shared					\
		--$(VP8_DECODER)-vp8-decoder				\
		--$(VP8_ENCODER)-vp8-encoder				\
		--$(VP9_DECODER)-vp9-decoder				\
		--$(VP9_ENCODER)-vp9-encoder				\
		--enable-runtime-cpu-detect				\
		--disable-md5						\
		--disable-examples					\
		--disable-docs						\
		--disable-unit-tests					&& \
	cd ..								&& \
	touch $@

CLEANFILES = vpx.build.stamp

EXTRA_DIST = \
	libmvt_vpx.vers		\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in vpx.configure.stamp
