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

#define    AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE 0
#define    AR_MULTI_PATTERN_DETECTION_MODE_MATRIX 1
#define    AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX 2

#define    AR_MULTI_CONFIDENCE_PATTERN_CUTOFF_DEFAULT   0.5
#define    AR_MULTI_CONFIDENCE_MATRIX_CUTOFF_DEFAULT    0.5
#define    AR_MULTI_POSE_ERROR_CUTOFF_EACH_DEFAULT      4.0 // Maximum allowable pose estimation error for each marker.
#define    AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT 20.0 // Maximum allowable pose estimation error for combined marker set.


typedef struct {
    int     patt_id;
    int     patt_type; // Either AR_MULTI_PATTERN_TYPE_TEMPLATE or AR_MULTI_PATTERN_TYPE_MATRIX.
    ARdouble  width;
    ARdouble  trans[3][4];
    ARdouble  itrans[3][4];
    ARdouble  pos3d[4][3];
    int     visible;
    int     visibleR;
    uint64_t globalID;
} ARMultiEachMarkerInfoT;

typedef struct {
    ARMultiEachMarkerInfoT *marker;
    int                     marker_num;
    ARdouble                trans[3][4];
    int                     prevF;
    int                     patt_type;
    ARdouble                cfPattCutoff;
    ARdouble                cfMatrixCutoff;
    int                     min_submarker;
} ARMultiMarkerInfoT;

AR_EXTERN ARMultiMarkerInfoT *arMultiReadConfigFile( const char *filename, ARPattHandle *pattHandle );

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
