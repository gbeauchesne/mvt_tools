/*
 * mvt_codec.h - Codec utilities
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

#ifndef MVT_CODEC_H
#define MVT_CODEC_H

#include <va/va.h>

/** Codecs */
typedef enum {
    MVT_CODEC_NONE,
    /** Raw video */
    MVT_CODEC_RAWVIDEO,
    /** MPEG-1 (ISO/IEC 11172) */
    MVT_CODEC_MPEG1,
    /** MPEG-2 (ISO/IEC 13818-2) */
    MVT_CODEC_MPEG2,
    /** MPEG-4 Part 2 (ISO/IEC 14496-2) */
    MVT_CODEC_MPEG4,
    /** JPEG (ITU-T 81) */
    MVT_CODEC_JPEG,
    /** H.263 */
    MVT_CODEC_H263,
    /** H.264 aka MPEG-4 Part 10 (ISO/IEC 14496-10) */
    MVT_CODEC_H264,
    /** VC-1 Advanced profile (SMPTE 421M) */
    MVT_CODEC_VC1,
    /** VP8 (RFC #6386) */
    MVT_CODEC_VP8,
    /** VP9 */
    MVT_CODEC_VP9,
    /** H.265 aka MPEG-H Part 2 (ISO/IEC 23008-2) */
    MVT_CODEC_HEVC,
} MvtCodec;

/** Determines the codec id from the supplied name */
MvtCodec
mvt_codec_from_name(const char *name);

/** Determines the codec name from the supplied id */
const char *
mvt_codec_to_name(MvtCodec codec);

/** Determines the profile name from the supplied codec and profile id pair */
const char *
mvt_profile_to_name(MvtCodec codec, int profile);

/** Translates MvtProfile id to VA profile id for the supplied codec */
bool
mvt_profile_to_va_profile(MvtCodec codec, int profile,
    VAProfile *va_profile_ptr);

/** MPEG-2 profiles */
typedef enum {
    /** Simple profile (101) */
    MVT_MPEG2_PROFILE_SIMPLE = 0x05,
    /** Main profile (100) */
    MVT_MPEG2_PROFILE_MAIN = 0x04,
    /** SNR Scalable profile (011) */
    MVT_MPEG2_PROFILE_SNR_SCALABLE = 0x03,
    /** Spatially Scalable profile (010) */
    MVT_MPEG2_PROFILE_SPATIALLY_SCALABLE = 0x02,
    /** High profile (001) */
    MVT_MPEG2_PROFILE_HIGH = 0x01,
} MvtMpeg2Profile;

/** Determines the MPEG-2 profile id from the supplied name */
MvtMpeg2Profile
mvt_mpeg2_profile_from_name(const char *name);

/** Determines the MPEG-2 profile name from the supplied id */
const char *
mvt_mpeg2_profile_to_name(MvtMpeg2Profile profile);

/** MPEG-4:2 profiles */
typedef enum {
    /** Simple profile (0000) */
    MVT_MPEG4_PROFILE_SIMPLE = 0x00,
    /** Main profile (0011) */
    MVT_MPEG4_PROFILE_MAIN = 0x03,
    /** Advanced Simple profile (1111) */
    MVT_MPEG4_PROFILE_ADVANCED_SIMPLE = 0x0f,
} MvtMpeg4Profile;

/** Determines the MPEG-4 profile id from the supplied name */
MvtMpeg4Profile
mvt_mpeg4_profile_from_name(const char *name);

/** Determines the MPEG-4 profile name from the supplied id */
const char *
mvt_mpeg4_profile_to_name(MvtMpeg4Profile profile);

/** H.264 profiles */
typedef enum {
    MVT_H264_CONSTRAINT_SET0_FLAG = (1U << (7-0)) << 16,
    MVT_H264_CONSTRAINT_SET1_FLAG = (1U << (7-1)) << 16,
    MVT_H264_CONSTRAINT_SET2_FLAG = (1U << (7-2)) << 16,
    MVT_H264_CONSTRAINT_SET3_FLAG = (1U << (7-3)) << 16,
    MVT_H264_CONSTRAINT_SET4_FLAG = (1U << (7-4)) << 16,
    MVT_H264_CONSTRAINT_SET5_FLAG = (1U << (7-5)) << 16,

    /** Baseline profile [A.2.1] */
    MVT_H264_PROFILE_BASELINE = 66,
    /** Constrained Baseline profile [A.2.1.1] */
    MVT_H264_PROFILE_CONSTRAINED_BASELINE = (
        MVT_H264_PROFILE_BASELINE |
        MVT_H264_CONSTRAINT_SET1_FLAG),
    /** Main profile [A.2.2] */
    MVT_H264_PROFILE_MAIN = 77,
    /** Extended profile [A.2.3] */
    MVT_H264_PROFILE_EXTENDED = 88,
    /** High profile [A.2.4] */
    MVT_H264_PROFILE_HIGH = 100,
    /** Progressive High profile [A.2.4.1] */
    MVT_H264_PROFILE_PROGRESSIVE_HIGH = (
        MVT_H264_PROFILE_HIGH |
        MVT_H264_CONSTRAINT_SET4_FLAG),
    /** Constrained High profile [A.2.4.2] */
    MVT_H264_PROFILE_CONSTRAINED_HIGH = (
        MVT_H264_PROFILE_HIGH |
        MVT_H264_CONSTRAINT_SET4_FLAG | MVT_H264_CONSTRAINT_SET5_FLAG),
    /** High 10 profile [A.2.5] */
    MVT_H264_PROFILE_HIGH10 = 110,
    /** High 4:2:2 profile [A.2.6] */
    MVT_H264_PROFILE_HIGH_422 = 122,
    /** High 4:4:4 Predictive profile [A.2.7] */
    MVT_H264_PROFILE_HIGH_444 = 244,
    /** High 10 Intra profile [A.2.8] */
    MVT_H264_PROFILE_HIGH10_INTRA = (
        MVT_H264_PROFILE_HIGH10 |
        MVT_H264_CONSTRAINT_SET3_FLAG),
    /** High 4:2:2 Intra profile [A.2.9] */
    MVT_H264_PROFILE_HIGH_422_INTRA = (
        MVT_H264_PROFILE_HIGH_422 |
        MVT_H264_CONSTRAINT_SET3_FLAG),
    /** High 4:4:4 Intra profile [A.2.10] */
    MVT_H264_PROFILE_HIGH_444_INTRA = (
        MVT_H264_PROFILE_HIGH_444 |
        MVT_H264_CONSTRAINT_SET3_FLAG),
    /** Scalable Baseline profile [G.10.1.1] */
    MVT_H264_PROFILE_SCALABLE_BASELINE = 83,
    /** Scalable Constrained Baseline profile [G.10.1.1.1] */
    MVT_H264_PROFILE_SCALABLE_CONSTRAINED_BASELINE = (
        MVT_H264_PROFILE_SCALABLE_BASELINE |
        MVT_H264_CONSTRAINT_SET5_FLAG),
    /** Scalable High profile [G.10.1.2] */
    MVT_H264_PROFILE_SCALABLE_HIGH = 86,
    /** Scalable Constrained High profile [G.10.1.2.1] */
    MVT_H264_PROFILE_SCALABLE_CONSTRAINED_HIGH = (
        MVT_H264_PROFILE_SCALABLE_HIGH |
        MVT_H264_CONSTRAINT_SET5_FLAG),
    /** Scalable High Intra profile [G.10.1.3] */
    MVT_H264_PROFILE_SCALABLE_HIGH_INTRA = (
        MVT_H264_PROFILE_SCALABLE_HIGH |
        MVT_H264_CONSTRAINT_SET3_FLAG),
    /** Multiview High profile [H.10.1.1] */
    MVT_H264_PROFILE_MULTIVIEW_HIGH = 118,
    /** Stereo High profile [H.10.1.2] */
    MVT_H264_PROFILE_STEREO_HIGH = 128,
} MvtH264Profile;

/** Determines the H.264 profile id from the supplied name */
MvtH264Profile
mvt_h264_profile_from_name(const char *name);

/** Determines the H.264 profile name from the supplied id */
const char *
mvt_h264_profile_to_name(MvtH264Profile profile);

/** Determines the H.264 profile id from the codec-data buffer (avcC format) */
bool
mvt_h264_profile_from_codec_data(const uint8_t *buf, uint32_t buf_size,
    int *profile_ptr);

/** VC-1 profiles */
typedef enum {
    /** Simple profile */
    MVT_VC1_PROFILE_SIMPLE = 0,
    /** Main profile */
    MVT_VC1_PROFILE_MAIN = 1,
    /** Advanced profile [6.1.1] */
    MVT_VC1_PROFILE_ADVANCED = 3,
} MvtVc1Profile;

/** Determines the VC-1 profile id from the supplied name */
MvtVc1Profile
mvt_vc1_profile_from_name(const char *name);

/** Determines the VC-1 profile name from the supplied id */
const char *
mvt_vc1_profile_to_name(MvtVc1Profile profile);

/** Determines the VC-1 profile id from the codec-data buffer (avcC format) */
bool
mvt_wmv3_profile_from_codec_data(const uint8_t *buf, uint32_t buf_size,
    int *profile_ptr);

/** VP9 profiles */
typedef enum {
    /** Profile 0 (00) */
    MVT_VP9_PROFILE_0 = 0x00,
    /** Profile 1 (10) */
    MVT_VP9_PROFILE_1 = 0x02,
} MvtVp9Profile;

/** Determines the VP9 profile id from the supplied name */
MvtVp9Profile
mvt_vp9_profile_from_name(const char *name);

/** Determines the VP9 profile name from the supplied id */
const char *
mvt_vp9_profile_to_name(MvtVp9Profile profile);

/** HEVC profiles */
typedef enum {
    /** Main profile [A.3.2] */
    MVT_HEVC_PROFILE_MAIN = 1,
    /** Main 10 profile [A.3.3] */
    MVT_HEVC_PROFILE_MAIN10 = 2,
    /** Main Still Picture profile [A.3.4] */
    MVT_HEVC_PROFILE_MAIN_STILL_PICTURE = 3,
} MvtHevcProfile;

/** Determines the HEVC profile id from the supplied name */
MvtHevcProfile
mvt_hevc_profile_from_name(const char *name);

/** Determines the HEVC profile name from the supplied id */
const char *
mvt_hevc_profile_to_name(MvtHevcProfile profile);

#endif /* MVT_CODEC_H */
