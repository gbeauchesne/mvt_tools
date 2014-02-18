# sources.frag - Generated list of source files for JM (-*- makefile -*-)
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

jm_srcdir = $(srcdir)/JM

jm_source_c = \
	$(jm_srcdir)/lcommon/src/blk_prediction.c \
	$(jm_srcdir)/lcommon/src/config_common.c \
	$(jm_srcdir)/lcommon/src/img_io.c \
	$(jm_srcdir)/lcommon/src/img_process.c \
	$(jm_srcdir)/lcommon/src/input.c \
	$(jm_srcdir)/lcommon/src/io_raw.c \
	$(jm_srcdir)/lcommon/src/io_tiff.c \
	$(jm_srcdir)/lcommon/src/mbuffer_common.c \
	$(jm_srcdir)/lcommon/src/memalloc.c \
	$(jm_srcdir)/lcommon/src/mv_prediction.c \
	$(jm_srcdir)/lcommon/src/nalucommon.c \
	$(jm_srcdir)/lcommon/src/parsetcommon.c \
	$(jm_srcdir)/lcommon/src/resize.c \
	$(jm_srcdir)/lcommon/src/transform.c \
	$(jm_srcdir)/lcommon/src/win32.c \
	$(jm_srcdir)/ldecod/src/annexb.c \
	$(jm_srcdir)/ldecod/src/biaridecod.c \
	$(jm_srcdir)/ldecod/src/block.c \
	$(jm_srcdir)/ldecod/src/cabac.c \
	$(jm_srcdir)/ldecod/src/configfile.c \
	$(jm_srcdir)/ldecod/src/context_ini.c \
	$(jm_srcdir)/ldecod/src/dec_statistics.c \
	$(jm_srcdir)/ldecod/src/erc_api.c \
	$(jm_srcdir)/ldecod/src/erc_do_i.c \
	$(jm_srcdir)/ldecod/src/erc_do_p.c \
	$(jm_srcdir)/ldecod/src/errorconcealment.c \
	$(jm_srcdir)/ldecod/src/filehandle.c \
	$(jm_srcdir)/ldecod/src/fmo.c \
	$(jm_srcdir)/ldecod/src/header.c \
	$(jm_srcdir)/ldecod/src/image.c \
	$(jm_srcdir)/ldecod/src/intra16x16_pred.c \
	$(jm_srcdir)/ldecod/src/intra16x16_pred_mbaff.c \
	$(jm_srcdir)/ldecod/src/intra16x16_pred_normal.c \
	$(jm_srcdir)/ldecod/src/intra4x4_pred.c \
	$(jm_srcdir)/ldecod/src/intra4x4_pred_mbaff.c \
	$(jm_srcdir)/ldecod/src/intra4x4_pred_normal.c \
	$(jm_srcdir)/ldecod/src/intra8x8_pred.c \
	$(jm_srcdir)/ldecod/src/intra8x8_pred_mbaff.c \
	$(jm_srcdir)/ldecod/src/intra8x8_pred_normal.c \
	$(jm_srcdir)/ldecod/src/intra_chroma_pred.c \
	$(jm_srcdir)/ldecod/src/intra_chroma_pred_mbaff.c \
	$(jm_srcdir)/ldecod/src/intra_pred_common.c \
	$(jm_srcdir)/ldecod/src/ldecod.c \
	$(jm_srcdir)/ldecod/src/leaky_bucket.c \
	$(jm_srcdir)/ldecod/src/loopFilter.c \
	$(jm_srcdir)/ldecod/src/loop_filter_mbaff.c \
	$(jm_srcdir)/ldecod/src/loop_filter_normal.c \
	$(jm_srcdir)/ldecod/src/macroblock.c \
	$(jm_srcdir)/ldecod/src/mb_access.c \
	$(jm_srcdir)/ldecod/src/mb_prediction.c \
	$(jm_srcdir)/ldecod/src/mb_read.c \
	$(jm_srcdir)/ldecod/src/mbuffer.c \
	$(jm_srcdir)/ldecod/src/mbuffer_mvc.c \
	$(jm_srcdir)/ldecod/src/mc_direct.c \
	$(jm_srcdir)/ldecod/src/mc_prediction.c \
	$(jm_srcdir)/ldecod/src/nal.c \
	$(jm_srcdir)/ldecod/src/nalu.c \
	$(jm_srcdir)/ldecod/src/output.c \
	$(jm_srcdir)/ldecod/src/parset.c \
	$(jm_srcdir)/ldecod/src/quant.c \
	$(jm_srcdir)/ldecod/src/read_comp_cabac.c \
	$(jm_srcdir)/ldecod/src/read_comp_cavlc.c \
	$(jm_srcdir)/ldecod/src/rtp.c \
	$(jm_srcdir)/ldecod/src/sei.c \
	$(jm_srcdir)/ldecod/src/transform8x8.c \
	$(jm_srcdir)/ldecod/src/vlc.c \
	$(NULL)

jm_source_h = \
	$(jm_srcdir)/lcommon/inc/blk_prediction.h \
	$(jm_srcdir)/lcommon/inc/config_common.h \
	$(jm_srcdir)/lcommon/inc/ctx_tables.h \
	$(jm_srcdir)/lcommon/inc/distortion.h \
	$(jm_srcdir)/lcommon/inc/enc_statistics.h \
	$(jm_srcdir)/lcommon/inc/fast_memory.h \
	$(jm_srcdir)/lcommon/inc/frame.h \
	$(jm_srcdir)/lcommon/inc/ifunctions.h \
	$(jm_srcdir)/lcommon/inc/img_io.h \
	$(jm_srcdir)/lcommon/inc/img_process.h \
	$(jm_srcdir)/lcommon/inc/input.h \
	$(jm_srcdir)/lcommon/inc/io_image.h \
	$(jm_srcdir)/lcommon/inc/io_raw.h \
	$(jm_srcdir)/lcommon/inc/io_tiff.h \
	$(jm_srcdir)/lcommon/inc/io_video.h \
	$(jm_srcdir)/lcommon/inc/lagrangian.h \
	$(jm_srcdir)/lcommon/inc/mb_access.h \
	$(jm_srcdir)/lcommon/inc/mbuffer_common.h \
	$(jm_srcdir)/lcommon/inc/memalloc.h \
	$(jm_srcdir)/lcommon/inc/mv_prediction.h \
	$(jm_srcdir)/lcommon/inc/nalucommon.h \
	$(jm_srcdir)/lcommon/inc/parsetcommon.h \
	$(jm_srcdir)/lcommon/inc/quant_params.h \
	$(jm_srcdir)/lcommon/inc/report.h \
	$(jm_srcdir)/lcommon/inc/resize.h \
	$(jm_srcdir)/lcommon/inc/transform.h \
	$(jm_srcdir)/lcommon/inc/typedefs.h \
	$(jm_srcdir)/lcommon/inc/types.h \
	$(jm_srcdir)/lcommon/inc/win32.h \
	$(jm_srcdir)/ldecod/inc/annexb.h \
	$(jm_srcdir)/ldecod/inc/biaridecod.h \
	$(jm_srcdir)/ldecod/inc/block.h \
	$(jm_srcdir)/ldecod/inc/cabac.h \
	$(jm_srcdir)/ldecod/inc/configfile.h \
	$(jm_srcdir)/ldecod/inc/context_ini.h \
	$(jm_srcdir)/ldecod/inc/contributors.h \
	$(jm_srcdir)/ldecod/inc/dec_statistics.h \
	$(jm_srcdir)/ldecod/inc/defines.h \
	$(jm_srcdir)/ldecod/inc/elements.h \
	$(jm_srcdir)/ldecod/inc/erc_api.h \
	$(jm_srcdir)/ldecod/inc/erc_do.h \
	$(jm_srcdir)/ldecod/inc/erc_globals.h \
	$(jm_srcdir)/ldecod/inc/errorconcealment.h \
	$(jm_srcdir)/ldecod/inc/filehandle.h \
	$(jm_srcdir)/ldecod/inc/fmo.h \
	$(jm_srcdir)/ldecod/inc/global.h \
	$(jm_srcdir)/ldecod/inc/h264decoder.h \
	$(jm_srcdir)/ldecod/inc/header.h \
	$(jm_srcdir)/ldecod/inc/image.h \
	$(jm_srcdir)/ldecod/inc/intra16x16_pred.h \
	$(jm_srcdir)/ldecod/inc/intra4x4_pred.h \
	$(jm_srcdir)/ldecod/inc/intra8x8_pred.h \
	$(jm_srcdir)/ldecod/inc/leaky_bucket.h \
	$(jm_srcdir)/ldecod/inc/loop_filter.h \
	$(jm_srcdir)/ldecod/inc/loopfilter.h \
	$(jm_srcdir)/ldecod/inc/macroblock.h \
	$(jm_srcdir)/ldecod/inc/mb_prediction.h \
	$(jm_srcdir)/ldecod/inc/mbuffer.h \
	$(jm_srcdir)/ldecod/inc/mbuffer_mvc.h \
	$(jm_srcdir)/ldecod/inc/mc_prediction.h \
	$(jm_srcdir)/ldecod/inc/nalu.h \
	$(jm_srcdir)/ldecod/inc/output.h \
	$(jm_srcdir)/ldecod/inc/parset.h \
	$(jm_srcdir)/ldecod/inc/quant.h \
	$(jm_srcdir)/ldecod/inc/rtp.h \
	$(jm_srcdir)/ldecod/inc/sei.h \
	$(jm_srcdir)/ldecod/inc/transform8x8.h \
	$(jm_srcdir)/ldecod/inc/vlc.h \
	$(NULL)
