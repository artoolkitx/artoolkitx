/*
 *  AR2/config.h
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR2_CONFIG_H
#define AR2_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef AR2_STATIC
#    define AR2_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define AR2_EXTERN __declspec(dllexport)
#    else
#      define AR2_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define AR2_EXTERN
#endif

// The MAJOR version number defines non-backwards compatible
// changes in the ARToolKit NFT API. Range: [0-99].
#define AR2_HEADER_VERSION_MAJOR        4

// The MINOR version number defines additions to the ARToolKit NFT
// API, or (occsasionally) other significant backwards-compatible
// changes in runtime functionality. Range: [0-99].
#define AR2_HEADER_VERSION_MINOR        0

// The TINY version number defines bug-fixes to existing
// functionality. Range: [0-99].
#define AR2_HEADER_VERSION_TINY         0

// The BUILD version number will always be zero in releases,
// but may be non-zero in internal builds or in version-control
// repository-sourced code. Range: [0-99].
#define AR2_HEADER_VERSION_BUILD        0

// The string representation below must match the major, minor
// and tiny release numbers.
#define AR2_HEADER_VERSION_STRING       "4.0.0"

// The macros below are convenience macros to enable use
// of certain ARToolKit NFT header functionality by the release
// version in which it appeared.
// Each time the major version number is incremented, all
// existing macros must be removed, and just one new one
// added for the new major version.
// Each time the minor version number is incremented, a new
// AR_HAVE_HEADER_VERSION_ macro definition must be added.
// Tiny version numbers (being bug-fix releases, by definition)
// are NOT included in the AR_HAVE_HEADER_VERSION_ system.
#define AR2_HAVE_HEADER_VERSION_4
//#define AR2_HAVE_HEADER_VERSION_4_1



#define AR2_CAPABLE_ADAPTIVE_TEMPLATE               0

// Template scalefactor.
#define AR2_TEMP_SCALE                              2

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
#define AR2_CONSTANT_BLUR                           0
#define AR2_ADAPTIVE_BLUR                           2
#define AR2_DEFAULT_BLUR_METHOD                     AR2_CONSTANT_BLUR
#define AR2_DEFAULT_BLUR_LEVEL                      1
#endif


#define AR2_THREAD_MAX                              8

#define AR2_DEFAULT_SEARCH_SIZE	                    25          // Default radius of feature search window.

#define AR2_DEFAULT_SEARCH_FEATURE_NUM	            10          // May not be higher than AR2_SEARCH_FEATURE_MAX.

#define AR2_DEFAULT_TS1                             11          // Template size 1. Multiplied by AR2_TEMP_SCALE to give number of pixels outside centre pixel in negative x/y axis.
#define AR2_DEFAULT_TS2                             11          // Template size 2. Multiplied by AR2_TEMP_SCALE to give number of pixels outside centre pixel in positive x/y axis.

#define AR2_DEFAULT_SIM_THRESH                      0.6F

#define AR2_DEFAULT_TRACKING_THRESH                 2.0F



/* tracking.c */
#define    AR2_TRACKING_SURFACE_MAX                 10          // Maximum number of surfaces per surface set (i.e. maximum number of discrete surfaces with fixed relationship to each other able to be combined into a surface set.)
#define    AR2_TRACKING_CANDIDATE_MAX               200         // Maximum number of candidate feature points.

/* tracking2d.c */
#define AR2_DEFAULT_TRACKING_SD_THRESH              5.0F
#define AR2_SEARCH_FEATURE_MAX                      40


/* genFeatureSet.c */
#define AR2_DEFAULT_MAX_SIM_THRESH_L0               0.80F
#define AR2_DEFAULT_MAX_SIM_THRESH_L1               0.85F
#define AR2_DEFAULT_MAX_SIM_THRESH_L2               0.90F
#define AR2_DEFAULT_MAX_SIM_THRESH_L3               0.98F
#define AR2_DEFAULT_MIN_SIM_THRESH_L0               0.70F
#define AR2_DEFAULT_MIN_SIM_THRESH_L1               0.65F
#define AR2_DEFAULT_MIN_SIM_THRESH_L2               0.55F
#define AR2_DEFAULT_MIN_SIM_THRESH_L3               0.45F
#define AR2_DEFAULT_SD_THRESH_L0                    12.0F
#define AR2_DEFAULT_SD_THRESH_L1                    10.0F
#define AR2_DEFAULT_SD_THRESH_L2                     8.0F
#define AR2_DEFAULT_SD_THRESH_L3                     6.0F
#define AR2_DEFAULT_OCCUPANCY_SIZE                  24

/* genFeatureMap.c  genFeatureSet.c */
#define AR2_DEFAULT_GEN_FEATURE_MAP_SEARCH_SIZE1    10
#define AR2_DEFAULT_GEN_FEATURE_MAP_SEARCH_SIZE2    2
#define AR2_DEFAULT_MAX_SIM_THRESH2                 0.95F
#define AR2_DEFAULT_SD_THRESH2                      5.0F


#ifdef __cplusplus
}
#endif
#endif
