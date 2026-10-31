/*
 *  arGetMarkerInfo.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.0
 * Date: 03/08/13
 *
 *******************************************************/

#include <ARX/AR/ar.h>

int arGetMarkerInfo( ARUint8 *image, int xsize, int ysize, int pixelFormat, ARMarkerInfo2 *markerInfo2, int marker2_num,
                     ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode, ARParamLTf *arParamLTf, ARdouble pattRatio,
                     ARMarkerInfo *markerInfo, int *marker_num,
                     const AR_MATRIX_CODE_TYPE matrixCodeType )
{
    int            i, j, result;
#ifndef ARDOUBLE_IS_FLOAT
    float pos0, pos1;
#endif

    for( i = j = 0; i < marker2_num; i++ ) { // marker2_num is capped at AR_SQUARE_MAX by arLabeling().
        markerInfo[j].area   = markerInfo2[i].area;
#ifdef ARDOUBLE_IS_FLOAT
        if (arParamObserv2IdealLTf(arParamLTf, markerInfo2[i].pos[0], markerInfo2[i].pos[1],
                                   &(markerInfo[j].pos[0]), &(markerInfo[j].pos[1]) ) < 0) continue;
#else
        if (arParamObserv2IdealLTf(arParamLTf, (float)markerInfo2[i].pos[0], (float)markerInfo2[i].pos[1], &pos0, &pos1) < 0) continue;
        markerInfo[j].pos[0] = (ARdouble)pos0;
        markerInfo[j].pos[1] = (ARdouble)pos1;
#endif
        //arParamObserv2Ideal( dist_factor, markerInfo2[i].pos[0], markerInfo2[i].pos[1],
        //                     &(markerInfo[j].pos[0]), &(markerInfo[j].pos[1]), dist_function_version );

        if( arGetLine(markerInfo2[i].x_coord, markerInfo2[i].y_coord, markerInfo2[i].coord_num,
                      markerInfo2[i].vertex, arParamLTf,
                      markerInfo[j].line, markerInfo[j].vertex) < 0 ) continue;

        result = arPattGetIDGlobal( pattHandle, imageProcMode, pattDetectMode, image, xsize, ysize, pixelFormat, arParamLTf, markerInfo[j].vertex, pattRatio, 
                     &markerInfo[j].idPatt, &markerInfo[j].dirPatt, &markerInfo[j].cfPatt,
                     &markerInfo[j].idMatrix, &markerInfo[j].dirMatrix, &markerInfo[j].cfMatrix,
                      matrixCodeType, &markerInfo[j].errorCorrected, &markerInfo[j].globalID );

        if      (result == 0)  markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_NONE;
        else if (result == -1) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_GENERIC;
        else if (result == -2) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONTRAST;
        else if (result == -3) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_NOT_FOUND;
        else if (result == -4) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_EDC_FAIL;
        else if (result == -5) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_HEURISTIC_TROUBLESOME_MATRIX_CODES;
        else if (result == -6) markerInfo[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_PATTERN_EXTRACTION;
  
        // If not mixing template matching and matrix code detection, then copy id, dir and cf
        // from values in appropriate type.
        if (pattDetectMode == AR_TEMPLATE_MATCHING_COLOR || pattDetectMode == AR_TEMPLATE_MATCHING_MONO) {
            markerInfo[j].id  = markerInfo[j].idPatt;
            markerInfo[j].dir = markerInfo[j].dirPatt;
            markerInfo[j].cf  = markerInfo[j].cfPatt;
        } else if( pattDetectMode == AR_MATRIX_CODE_DETECTION ) {
            markerInfo[j].id  = markerInfo[j].idMatrix;
            markerInfo[j].dir = markerInfo[j].dirMatrix;
            markerInfo[j].cf  = markerInfo[j].cfMatrix;
        }
        markerInfo[j].matched = 0;

        j++;
    }
    *marker_num = j;

    return 0;
}
