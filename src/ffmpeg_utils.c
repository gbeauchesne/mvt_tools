/*
 * ffmpeg_utils.c - FFmpeg utilities
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
#include "ffmpeg_utils.h"
#include "ffmpeg_compat.h"

// Returns a string representation of the supplied FFmpeg error id
const char *
ffmpeg_strerror(int errnum, char errbuf[BUFSIZ])
{
    if (av_strerror(errnum, errbuf, BUFSIZ) != 0)
        sprintf(errbuf, "error %d", errnum);
    return errbuf;
}

// Translates FFmpeg pixel format to MVT video format
bool
ffmpeg_to_mvt_video_format(enum AVPixelFormat pix_fmt, VideoFormat *format_ptr)
{
    VideoFormat format;

    switch (pix_fmt) {
    case AV_PIX_FMT_GRAY8:
        format = VIDEO_FORMAT_Y800;
        break;
    case AV_PIX_FMT_YUV420P:
        format = VIDEO_FORMAT_I420;
        break;
    case AV_PIX_FMT_YUV420P10:
        format = VIDEO_FORMAT_I420P10;
        break;
    case AV_PIX_FMT_YUV422P:
        format = VIDEO_FORMAT_I422;
        break;
    case AV_PIX_FMT_YUV422P10:
        format = VIDEO_FORMAT_I422P10;
        break;
    case AV_PIX_FMT_YUV444P:
        format = VIDEO_FORMAT_I444;
        break;
    case AV_PIX_FMT_YUV444P10:
        format = VIDEO_FORMAT_I444P10;
        break;
#if LIBAVUTIL_VERSION_INT > AV_VERSION_INT(51,34,0)
    case AV_PIX_FMT_YUV420P12:
        format = VIDEO_FORMAT_I420P12;
        break;
    case AV_PIX_FMT_YUV422P12:
        format = VIDEO_FORMAT_I422P12;
        break;
    case AV_PIX_FMT_YUV444P12:
        format = VIDEO_FORMAT_I444P12;
        break;
#endif
    case AV_PIX_FMT_YUV420P16:
        format = VIDEO_FORMAT_I420P16;
        break;
    case AV_PIX_FMT_YUV422P16:
        format = VIDEO_FORMAT_I422P16;
        break;
    case AV_PIX_FMT_YUV444P16:
        format = VIDEO_FORMAT_I444P16;
        break;
    case AV_PIX_FMT_NV12:
        format = VIDEO_FORMAT_NV12;
        break;
    case AV_PIX_FMT_YUYV422:
        format = VIDEO_FORMAT_YUY2;
        break;
    case AV_PIX_FMT_UYVY422:
        format = VIDEO_FORMAT_UYVY;
        break;
    default:
        format = VIDEO_FORMAT_UNKNOWN;
        break;
    }
    if (!format)
        return false;

    if (format_ptr)
        *format_ptr = format;
    return true;
}

// Translates FFmpeg codec id to MVT codec
bool
ffmpeg_to_mvt_codec(enum AVCodecID codec_id, MvtCodec *codec_ptr)
{
    MvtCodec codec;

#define DEFINE_CODEC(FFMPEG_CODEC, MVT_CODEC)           \
    case MVT_GEN_CONCAT(AV_CODEC_ID_,FFMPEG_CODEC):     \
        codec = MVT_GEN_CONCAT(MVT_CODEC_,MVT_CODEC);   \
        break

#define DEFINE_CODEC_I(CODEC) \
    DEFINE_CODEC(CODEC, CODEC)

    switch (codec_id) {
        DEFINE_CODEC_I(RAWVIDEO);
        DEFINE_CODEC(MPEG1VIDEO, MPEG1);
        DEFINE_CODEC(MPEG2VIDEO, MPEG2);
        DEFINE_CODEC_I(MPEG4);
        DEFINE_CODEC(MJPEG, JPEG);
        DEFINE_CODEC_I(H263);
        DEFINE_CODEC_I(H264);
        DEFINE_CODEC(WMV3, VC1);
        DEFINE_CODEC_I(VC1);
        DEFINE_CODEC_I(VP8);
#if FFMPEG_HAS_VP9_DECODER
        DEFINE_CODEC_I(VP9);
#endif
#if FFMPEG_HAS_HEVC_DECODER
        DEFINE_CODEC_I(HEVC);
#endif
    default:
        return false;
    }
#undef DEFINE_CODEC_I
#undef DEFINE_CODEC

    if (codec_ptr)
        *codec_ptr = codec;
    return true;
}

// Translates FFmpeg profile id to MVT profile
bool
ffmpeg_to_mvt_profile(MvtCodec codec, int ff_profile, int *profile_ptr)
{
    int profile = -1;

#define DEFINE_PROFILE(CODEC, FFMPEG_PROFILE, MVT_PROFILE)              \
    case MVT_GEN_CONCAT4(FF_PROFILE_,CODEC,_,FFMPEG_PROFILE):           \
        profile = MVT_GEN_CONCAT4(MVT_,CODEC,_PROFILE_,MVT_PROFILE);    \
        break

#define DEFINE_PROFILE_I(CODEC, PROFILE) \
    DEFINE_PROFILE(CODEC, PROFILE, PROFILE)

    switch (codec) {
    case MVT_CODEC_MPEG2:
        switch (ff_profile) {
            DEFINE_PROFILE_I(MPEG2, SIMPLE);
            DEFINE_PROFILE_I(MPEG2, MAIN);
            DEFINE_PROFILE_I(MPEG2, SNR_SCALABLE);
            DEFINE_PROFILE(MPEG2, SS, SPATIALLY_SCALABLE);
            DEFINE_PROFILE_I(MPEG2, HIGH);
        }
        break;
    case MVT_CODEC_MPEG4:
        switch (ff_profile) {
            DEFINE_PROFILE_I(MPEG4, SIMPLE);
            DEFINE_PROFILE_I(MPEG4, MAIN);
            DEFINE_PROFILE_I(MPEG4, ADVANCED_SIMPLE);
        }
        break;
    case MVT_CODEC_H264:
        switch (ff_profile) {
            DEFINE_PROFILE_I(H264, BASELINE);
            DEFINE_PROFILE_I(H264, CONSTRAINED_BASELINE);
            DEFINE_PROFILE_I(H264, MAIN);
            DEFINE_PROFILE_I(H264, EXTENDED);
            DEFINE_PROFILE_I(H264, HIGH);
            DEFINE_PROFILE(H264, HIGH_10, HIGH10);
            DEFINE_PROFILE(H264, HIGH_10_INTRA, HIGH10_INTRA);
            DEFINE_PROFILE_I(H264, HIGH_422);
            DEFINE_PROFILE_I(H264, HIGH_422_INTRA);
            DEFINE_PROFILE(H264, HIGH_444_PREDICTIVE, HIGH_444);
            DEFINE_PROFILE_I(H264, HIGH_444_INTRA);
        }
        break;
    case MVT_CODEC_VC1:
        switch (ff_profile) {
            DEFINE_PROFILE_I(VC1, SIMPLE);
            DEFINE_PROFILE_I(VC1, MAIN);
            DEFINE_PROFILE_I(VC1, ADVANCED);
        }
        break;
#if FFMPEG_HAS_HEVC_DECODER
    case MVT_CODEC_HEVC:
        switch (ff_profile) {
            DEFINE_PROFILE_I(HEVC, MAIN);
            DEFINE_PROFILE(HEVC, MAIN_10, MAIN10);
            DEFINE_PROFILE_I(HEVC, MAIN_STILL_PICTURE);
        }
        break;
#endif
    default:
        break;
    }
#undef DEFINE_PROFILE_I
#undef DEFINE_PROFILE

    if (profile_ptr)
        *profile_ptr = profile;
    return profile != -1;
}
