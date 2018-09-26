/*
 *  arMulti.h
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 1.0
 * Date: 01/09/05
 *
 *******************************************************/

#ifndef AR_MULTI_H
#define AR_MULTI_H

#include <ARX/AR/ar.h>


#ifdef __cplusplus
extern "C" {
#endif

#define    AR_MULTI_PATTERN_TYPE_TEMPLATE    0
#define    AR_MULTI_PATTERN_TYPE_MATRIX      1

#define    AR_MULTI_PATTERN_DETECTION_MODE_NONE -1
#define    AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE 0
#define    AR_MULTI_PATTERN_DETECTION_MODE_MATRIX 1
#define    AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX 2

#define    AR_MULTI_CONFIDENCE_PATTERN_CUTOFF_DEFAULT   0.5
#define    AR_MULTI_CONFIDENCE_MATRIX_CUTOFF_DEFAULT    0.5
#define    AR_MULTI_POSE_ERROR_CUTOFF_EACH_DEFAULT      4.0 // Maximum allowable pose estimation error for each marker.
#define    AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT 20.0 // Maximum allowable pose estimation error for combined marker set.


typedef struct {
    int      patt_id;      // ID of the template pattern or matrix barcode.
    int      patt_type;    // Type of this pattern. Either AR_MULTI_PATTERN_TYPE_TEMPLATE or AR_MULTI_PATTERN_TYPE_MATRIX.
    ARdouble width;
    ARdouble trans[3][4];  // Pose of this marker, expressed in multimarker coordinate system.
    ARdouble itrans[3][4]; // Inverse of trans, i.e. pose of the multimarker, expressed in this marker's coordinate system.
    ARdouble pos3d[4][3];  // Position of each corner (in order: upper-left, upper-right, lower-right, lower right), expressed in multimarker coordinate system.
    int      visible;      // Used internally in arGetTransMatMultiSquare2/arGetTransMatMultiSquareStereo2. Set to index into ARMarkerInfo array of the matched marker, or -1 if no match.
    int      visibleR;     // Used internally in arGetTransMatMultiSquareStereo2. Set to index into ARMarkerInfo array for the right camera of the matched marker, or -1 if no match.
    uint64_t globalID;     // If patt_type == AR_MULTI_PATTERN_TYPE_MATRIX, the globalID of the matrix or 0 if not a global ID.
} ARMultiEachMarkerInfoT;

typedef struct {
    ARMultiEachMarkerInfoT *marker;         // Array of markers in this set.
    int                     marker_num;     // Number of markers present in this set (i.e. length of marker array).
    ARdouble                trans[3][4];    // Transform (i.e. pose) of this multimarker set, expressed in camera coordinate system.
    int                     prevF;          // 1 if the most recent call to arGetTransMatMulti estimated a valid pose. 0 otherwise.
    int                     patt_type;      // Aggregate type of patterns present in this set. Either AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE or AR_MULTI_PATTERN_DETECTION_MODE_MATRIX or AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX.
    ARdouble                cfPattCutoff;   // Minimum matching confidence required for any pattern markers in order to consider them when calculating the multimarker pose. Default value is AR_MULTI_CONFIDENCE_PATTERN_CUTOFF_DEFAULT.
    ARdouble                cfMatrixCutoff; // Minimum matching confidence required for any matrix markers in order to consider them when calculating the multimarker pose. Default value is AR_MULTI_CONFIDENCE_MATRIX_CUTOFF_DEFAULT.
    int                     min_submarker;  // Minimum number of markers in this set that must be detected in order to consider it a valid multimarker detection.
    ARdouble                minInlierProb;  // Minimum allowable inlier probability when performing robust multimarker pose estimation.
} ARMultiMarkerInfoT;

/**
 *  Creates a new empty multi-marker configuration.
 *  The returned value should be freed by calling arMultiFreeConfig when done.
 */
AR_EXTERN ARMultiMarkerInfoT *arMultiAllocConfig(void);
    
/**
 *  Takes a deep copy of the multi-marker configuration passed in.
 *  The returned value should be freed by calling arMultiFreeConfig when done.
 */
AR_EXTERN ARMultiMarkerInfoT *arMultiCopyConfig(const ARMultiMarkerInfoT *marker_info);

/**
 *  Creates a new multi-marker configuration and fills it with the config from the multi-marker config file.
 *  The returned value should be freed by calling arMultiFreeConfig when done.
 */
AR_EXTERN ARMultiMarkerInfoT *arMultiReadConfigFile( const char *filename, ARPattHandle *pattHandle );
    
AR_EXTERN int arMultiAddOrUpdateSubmarker(ARMultiMarkerInfoT *marker_info, int patt_id, int patt_type, ARdouble width, const ARdouble trans[3][4], uint64_t globalID);
    
AR_EXTERN void arMultiUpdateSubmarkerPose(ARMultiEachMarkerInfoT *submarker, const ARdouble trans[3][4]);
    
AR_EXTERN int arMultiRemoveSubmarker(ARMultiMarkerInfoT *marker_info, int patt_id, int patt_type, uint64_t globalID);

/**
 *  Frees the multi-marker configuration passed in.
 */
AR_EXTERN int arMultiFreeConfig( ARMultiMarkerInfoT *config );

AR_EXTERN ARdouble  arGetTransMatMultiSquare(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                 ARMultiMarkerInfoT *config);

AR_EXTERN ARdouble  arGetTransMatMultiSquareRobust(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                       ARMultiMarkerInfoT *config);

AR_EXTERN ARdouble  arGetTransMatMultiSquareStereo(AR3DStereoHandle *handle,
                                       ARMarkerInfo *marker_infoL, int marker_numL,
                                       ARMarkerInfo *marker_infoR, int marker_numR,
                                       ARMultiMarkerInfoT *config);

AR_EXTERN ARdouble  arGetTransMatMultiSquareStereoRobust(AR3DStereoHandle *handle,
                                             ARMarkerInfo *marker_infoL, int marker_numL,
                                             ARMarkerInfo *marker_infoR, int marker_numR,
                                             ARMultiMarkerInfoT *config);


#ifdef __cplusplus
}
#endif
#endif
