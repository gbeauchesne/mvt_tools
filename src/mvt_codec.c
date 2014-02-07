/*
 * mvt_codec.c - Codec utilities
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
#include "mvt_codec.h"
#include "mvt_map.h"

// Codecs
static const MvtMap codec_map[] = {
    { "mpeg1",  MVT_CODEC_MPEG1     },
    { "mpeg2",  MVT_CODEC_MPEG2     },
    { "mpeg4",  MVT_CODEC_MPEG4     },
    { "jpeg",   MVT_CODEC_JPEG      },
    { "h263",   MVT_CODEC_H263      },
    { "h264",   MVT_CODEC_H264      },
    { "vc1",    MVT_CODEC_VC1       },
    { "vp8",    MVT_CODEC_VP8       },
    { "vp9",    MVT_CODEC_VP9       },
    { "hevc",   MVT_CODEC_HEVC      },
    { NULL, }
};

// MPEG-2 profiles
static const MvtMap mpeg2_profiles[] = {
    { "simple",                 MVT_MPEG2_PROFILE_SIMPLE                    },
    { "main",                   MVT_MPEG2_PROFILE_MAIN                      },
    { "snr-scalable",           MVT_MPEG2_PROFILE_SNR_SCALABLE              },
    { "spatially-scalable",     MVT_MPEG2_PROFILE_SPATIALLY_SCALABLE        },
    { "high",                   MVT_MPEG2_PROFILE_HIGH                      },
    { NULL, -1 }
};

// MPEG-4 profiles
static const MvtMap mpeg4_profiles[] = {
    { "simple",                 MVT_MPEG4_PROFILE_SIMPLE                    },
    { "main",                   MVT_MPEG4_PROFILE_MAIN                      },
    { "advanced-simple",        MVT_MPEG4_PROFILE_ADVANCED_SIMPLE           },
    { NULL, -1 }
};

// H.264 profiles
static const MvtMap h264_profiles[] = {
    { "baseline",               MVT_H264_PROFILE_BASELINE                   },
    { "constrained-baseline",   MVT_H264_PROFILE_CONSTRAINED_BASELINE       },
    { "main",                   MVT_H264_PROFILE_MAIN                       },
    { "extended",               MVT_H264_PROFILE_EXTENDED                   },
    { "high",                   MVT_H264_PROFILE_HIGH                       },
    { "progressive-high",       MVT_H264_PROFILE_PROGRESSIVE_HIGH           },
    { "constrained-high",       MVT_H264_PROFILE_CONSTRAINED_HIGH           },
    { "high-10",                MVT_H264_PROFILE_HIGH10                     },
    { "high-4:2:2",             MVT_H264_PROFILE_HIGH_422                   },
    { "high-4:4:4",             MVT_H264_PROFILE_HIGH_444                   },
    { "high-10-intra",          MVT_H264_PROFILE_HIGH10_INTRA               },
    { "high-4:2:2-intra",       MVT_H264_PROFILE_HIGH_422_INTRA             },
    { "high-4:4:4-intra",       MVT_H264_PROFILE_HIGH_444_INTRA             },
    { "scalable-baseline",      MVT_H264_PROFILE_SCALABLE_BASELINE          },
    { "scalable-constrained-baseline",
      MVT_H264_PROFILE_SCALABLE_CONSTRAINED_BASELINE },
    { "scalable-high",          MVT_H264_PROFILE_SCALABLE_HIGH              },
    { "scalable-constrained-high",
      MVT_H264_PROFILE_SCALABLE_CONSTRAINED_HIGH },
    { "scalable-high-intra",    MVT_H264_PROFILE_SCALABLE_HIGH_INTRA        },
    { "multiview-high",         MVT_H264_PROFILE_MULTIVIEW_HIGH             },
    { "stereo-high",            MVT_H264_PROFILE_STEREO_HIGH                },
    { NULL, -1 }
};

// VC-1 profiles
static const MvtMap vc1_profiles[] = {
    { "simple",                 MVT_VC1_PROFILE_SIMPLE                      },
    { "main",                   MVT_VC1_PROFILE_MAIN                        },
    { "advanced",               MVT_VC1_PROFILE_ADVANCED                    },
    { NULL, -1 }
};

// VP9 profiles
static const MvtMap vp9_profiles[] = {
    { "profile0",               MVT_VP9_PROFILE_0                           },
    { "profile1",               MVT_VP9_PROFILE_1                           },
    { NULL, -1 }
};

// HEVC profiles
static const MvtMap hevc_profiles[] = {
    { "main",                   MVT_HEVC_PROFILE_MAIN                       },
    { "main-10",                MVT_HEVC_PROFILE_MAIN10                     },
    { "main-still-picture",     MVT_HEVC_PROFILE_MAIN_STILL_PICTURE         },
    { NULL, -1 }
};

// Determines the codec id from the supplied name
MvtCodec
mvt_codec_from_name(const char *name)
{
    return mvt_map_lookup(codec_map, name);
}

// Determines the codec name from the supplied id
const char *
mvt_codec_to_name(MvtCodec codec)
{
    return mvt_map_lookup_value(codec_map, codec);
}

// Determines the profile name from the supplied codec and profile id pair
const char *
mvt_profile_to_name(MvtCodec codec, int profile)
{
    const char *str;

    switch (codec) {
    case MVT_CODEC_MPEG2:
        str = mvt_mpeg2_profile_to_name(profile);
        break;
    case MVT_CODEC_MPEG4:
        str = mvt_mpeg4_profile_to_name(profile);
        break;
    case MVT_CODEC_H264:
        str = mvt_h264_profile_to_name(profile);
        break;
    case MVT_CODEC_VC1:
        str = mvt_vc1_profile_to_name(profile);
        break;
    case MVT_CODEC_VP9:
        str = mvt_vp9_profile_to_name(profile);
        break;
    case MVT_CODEC_HEVC:
        str = mvt_hevc_profile_to_name(profile);
        break;
    default:
        str = NULL;
        break;
    }
    return str;
}

// Translates MvtProfile id to VA profile id for the supplied codec
bool
mvt_profile_to_va_profile(MvtCodec codec, int profile,
    VAProfile *va_profile_ptr)
{
    VAProfile va_profile;

#define DEFINE_PROFILE(CODEC, MVT_PROFILE, VA_PROFILE)                  \
    case MVT_GEN_CONCAT4(MVT_,CODEC,_PROFILE_,MVT_PROFILE):             \
        va_profile = MVT_GEN_CONCAT3(VAProfile,CODEC,VA_PROFILE);       \
        break

    switch (codec) {
    case MVT_CODEC_MPEG2:
        switch (profile) {
            DEFINE_PROFILE(MPEG2, SIMPLE, Simple);
            DEFINE_PROFILE(MPEG2, MAIN, Main);
        default: return false;
        }
        break;
    case MVT_CODEC_MPEG4:
        switch (profile) {
            DEFINE_PROFILE(MPEG4, SIMPLE, Simple);
            DEFINE_PROFILE(MPEG4, ADVANCED_SIMPLE, AdvancedSimple);
            DEFINE_PROFILE(MPEG4, MAIN, Main);
        default: return false;
        }
        break;
    case MVT_CODEC_H264:
        switch (profile) {
            DEFINE_PROFILE(H264, BASELINE, Baseline);
            DEFINE_PROFILE(H264, CONSTRAINED_BASELINE, ConstrainedBaseline);
            DEFINE_PROFILE(H264, MAIN, Main);
            DEFINE_PROFILE(H264, HIGH, High);
        default: return false;
        }
        break;
    case MVT_CODEC_VC1:
        switch (profile) {
            DEFINE_PROFILE(VC1, SIMPLE, Simple);
            DEFINE_PROFILE(VC1, MAIN, Main);
            DEFINE_PROFILE(VC1, ADVANCED, Advanced);
        default: return false;
        }
        break;
    default:
        return false;
    }
#undef DEFINE_PROFILE

    if (va_profile_ptr)
        *va_profile_ptr = va_profile;
    return true;
}

// Determines the MPEG-2 profile id from the supplied name
MvtMpeg2Profile
mvt_mpeg2_profile_from_name(const char *name)
{
    return mvt_map_lookup(mpeg2_profiles, name);
}

// Determines the MPEG-2 profile name from the supplied id
const char *
mvt_mpeg2_profile_to_name(MvtMpeg2Profile profile)
{
    return mvt_map_lookup_value(mpeg2_profiles, profile);
}

// Determines the MPEG-4 profile id from the supplied name
MvtMpeg4Profile
mvt_mpeg4_profile_from_name(const char *name)
{
    return mvt_map_lookup(mpeg4_profiles, name);
}

// Determines the MPEG-4 profile name from the supplied id
const char *
mvt_mpeg4_profile_to_name(MvtMpeg4Profile profile)
{
    return mvt_map_lookup_value(mpeg4_profiles, profile);
}

// Determines the H.264 profile id from the supplied name
MvtH264Profile
mvt_h264_profile_from_name(const char *name)
{
    return mvt_map_lookup(h264_profiles, name);
}

// Determines the H.264 profile name from the supplied id
const char *
mvt_h264_profile_to_name(MvtH264Profile profile)
{
    return mvt_map_lookup_value(h264_profiles, profile);
}

// Checks whether the supplied profile is valid (i.e. it has a valid name)
static inline bool
mvt_h264_profile_is_valid(MvtH264Profile profile)
{
    return mvt_h264_profile_to_name(profile) != NULL;
}

// Determines the H.264 profile id from the codec-data buffer (avcC format)
bool
mvt_h264_profile_from_codec_data(const uint8_t *buf, uint32_t buf_size,
    int *profile_ptr)
{
    int profile, profile_ext;

    if (!buf || buf_size < 3)
        return false;

    if (buf[0] != 1)    // configurationVersion = 1
        return false;

    profile = buf[1];   // AVCProfileIndication
    profile_ext = ((uint32_t)buf[2]) << 16;
    switch (profile) {
    case MVT_H264_PROFILE_BASELINE:
        profile_ext &= MVT_H264_CONSTRAINT_SET1_FLAG;   // Constrained
        break;
    case MVT_H264_PROFILE_HIGH:
        profile_ext &= (MVT_H264_CONSTRAINT_SET4_FLAG|  // Progressive
            MVT_H264_CONSTRAINT_SET5_FLAG);             // Constrained
        break;
    case MVT_H264_PROFILE_HIGH10:
    case MVT_H264_PROFILE_HIGH_422:
    case MVT_H264_PROFILE_HIGH_444:
    case MVT_H264_PROFILE_SCALABLE_HIGH:
        profile_ext &= MVT_H264_CONSTRAINT_SET3_FLAG;   // Intra
        break;
    case MVT_H264_PROFILE_SCALABLE_BASELINE:
        profile_ext &= MVT_H264_CONSTRAINT_SET5_FLAG;   // Constrained
        break;
    default:
        profile_ext = 0;
        break;
    }
    profile |= profile_ext;
    if (!mvt_h264_profile_is_valid(profile))
        return false;

    if (profile_ptr)
        *profile_ptr = profile;
    return true;
}

// Determines the VC-1 profile id from the supplied name
MvtVc1Profile
mvt_vc1_profile_from_name(const char *name)
{
    return mvt_map_lookup(vc1_profiles, name);
}

// Determines the VC-1 profile name from the supplied id
const char *
mvt_vc1_profile_to_name(MvtVc1Profile profile)
{
    return mvt_map_lookup_value(vc1_profiles, profile);
}

// Checks whether the supplied profile is valid (i.e. it has a valid name)
static inline bool
mvt_vc1_profile_is_valid(MvtVc1Profile profile)
{
    return mvt_vc1_profile_to_name(profile) != NULL;
}

// Determines the VC-1 profile id from the codec-data buffer (avcC format)
bool
mvt_wmv3_profile_from_codec_data(const uint8_t *buf, uint32_t buf_size,
    int *profile_ptr)
{
    int profile;

    if (!buf || buf_size < 1)
        return false;

    profile = buf[0] >> 6;      // PROFILE (4 bits, minimum 2 bits)
    if (!mvt_vc1_profile_is_valid(profile))
        return false;

    if (profile_ptr)
        *profile_ptr = profile;
    return true;
}

// Determines the HEVC profile id from the supplied name
MvtHevcProfile
mvt_hevc_profile_from_name(const char *name)
{
    return mvt_map_lookup(hevc_profiles, name);
}

// Determines the HEVC profile name from the supplied id
const char *
mvt_hevc_profile_to_name(MvtHevcProfile profile)
{
    return mvt_map_lookup_value(hevc_profiles, profile);
}

// Determines the VP9 profile id from the supplied name
MvtVp9Profile
mvt_vp9_profile_from_name(const char *name)
{
    return mvt_map_lookup(vp9_profiles, name);
}

// Determines the VP9 profile name from the supplied id
const char *
mvt_vp9_profile_to_name(MvtVp9Profile profile)
{
    return mvt_map_lookup_value(vp9_profiles, profile);
}
