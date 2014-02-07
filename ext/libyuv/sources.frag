# sources.frag - Generated list of source files for libyuv (-*- makefile -*-)
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

libyuv_srcdir = $(srcdir)/upstream

libyuv_source_c = \
	$(libyuv_srcdir)/source/compare.cc			\
	$(libyuv_srcdir)/source/compare_common.cc		\
	$(libyuv_srcdir)/source/compare_posix.cc		\
	$(libyuv_srcdir)/source/convert.cc			\
	$(libyuv_srcdir)/source/convert_argb.cc			\
	$(libyuv_srcdir)/source/convert_from.cc			\
	$(libyuv_srcdir)/source/convert_from_argb.cc		\
	$(libyuv_srcdir)/source/convert_to_argb.cc		\
	$(libyuv_srcdir)/source/convert_to_i420.cc		\
	$(libyuv_srcdir)/source/cpu_id.cc			\
	$(libyuv_srcdir)/source/format_conversion.cc		\
	$(libyuv_srcdir)/source/planar_functions.cc		\
	$(libyuv_srcdir)/source/rotate.cc			\
	$(libyuv_srcdir)/source/rotate_argb.cc			\
	$(libyuv_srcdir)/source/rotate_mips.cc			\
	$(libyuv_srcdir)/source/row_any.cc			\
	$(libyuv_srcdir)/source/row_common.cc			\
	$(libyuv_srcdir)/source/row_mips.cc			\
	$(libyuv_srcdir)/source/row_posix.cc			\
	$(libyuv_srcdir)/source/scale.cc			\
	$(libyuv_srcdir)/source/scale_argb.cc			\
	$(libyuv_srcdir)/source/scale_common.cc			\
	$(libyuv_srcdir)/source/scale_mips.cc			\
	$(libyuv_srcdir)/source/scale_posix.cc			\
	$(libyuv_srcdir)/source/video_common.cc			\
	$(NULL)

libyuv_source_h = \
	$(libyuv_srcdir)/include/libyuv.h			\
	$(libyuv_srcdir)/include/libyuv/basic_types.h		\
	$(libyuv_srcdir)/include/libyuv/compare.h		\
	$(libyuv_srcdir)/include/libyuv/convert.h		\
	$(libyuv_srcdir)/include/libyuv/convert_argb.h		\
	$(libyuv_srcdir)/include/libyuv/convert_from.h		\
	$(libyuv_srcdir)/include/libyuv/convert_from_argb.h	\
	$(libyuv_srcdir)/include/libyuv/cpu_id.h		\
	$(libyuv_srcdir)/include/libyuv/format_conversion.h	\
	$(libyuv_srcdir)/include/libyuv/mjpeg_decoder.h		\
	$(libyuv_srcdir)/include/libyuv/planar_functions.h	\
	$(libyuv_srcdir)/include/libyuv/rotate.h		\
	$(libyuv_srcdir)/include/libyuv/rotate_argb.h		\
	$(libyuv_srcdir)/include/libyuv/row.h			\
	$(libyuv_srcdir)/include/libyuv/scale.h			\
	$(libyuv_srcdir)/include/libyuv/scale_argb.h		\
	$(libyuv_srcdir)/include/libyuv/scale_row.h		\
	$(libyuv_srcdir)/include/libyuv/version.h		\
	$(libyuv_srcdir)/include/libyuv/video_common.h		\
	$(NULL)
