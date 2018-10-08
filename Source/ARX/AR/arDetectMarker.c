/*
 *  arDetectMarker.c
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

#include <stdio.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/arImageProc.h>
#include "arRefineCorners.h"
#if DEBUG_PATT_GETID
extern int cnt;
#endif

const char *arMarkerInfoCutoffPhaseDescriptions[AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT] = {
    "Marker OK.",
    "Pattern extraction failed.",
    "Generic error during matching phase.",
    "Insufficient contrast during matching.",
    "Barcode matching could not find correct barcode locator pattern.",
    "Barcode matching error detection/correction found unrecoverable error.",
    "Matching confidence cutoff value not reached.",
    "Maximum allowable pose error exceeded.",
    "Multi-marker pose error value exceeded.",
    "Rejected frequently misrecognised matrix marker."
};

static void confidenceCutoff(ARHandle *arHandle);

int arDetectMarker(ARHandle *arHandle, AR2VideoBufferT *frame)
{
    ARdouble    rarea, rlen, rlenmin;
    ARdouble    diff, diffmin;
    int         cid, cdir;
    int         i, j, k;
    int         detectionIsDone = 0;
    int         threshDiff;

#if DEBUG_PATT_GETID
cnt = 0;
#endif

    if (!arHandle || !frame) return (-1);
    
    arHandle->marker_num = 0;
    
    if (arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_BRACKETING) {
        if (arHandle->arLabelingThreshAutoIntervalTTL > 0) {
            arHandle->arLabelingThreshAutoIntervalTTL--;
        } else {
            int thresholds[3];
            int marker_nums[3];
            
            thresholds[0] = arHandle->arLabelingThresh + arHandle->arLabelingThreshAutoBracketOver;
            if (thresholds[0] > 255) thresholds[0] = 255;
            thresholds[1] = arHandle->arLabelingThresh - arHandle->arLabelingThreshAutoBracketUnder;
            if (thresholds[1] < 0) thresholds[1] = 0;
            thresholds[2] = arHandle->arLabelingThresh;
            
            for (i = 0; i < 3; i++) {
                if (arLabeling(frame->buffLuma, arHandle->xsize, arHandle->ysize, arHandle->arDebug, arHandle->arLabelingMode, thresholds[i], arHandle->arImageProcMode, &(arHandle->labelInfo), NULL) < 0) return -1;
                if (arDetectMarker2(arHandle->xsize, arHandle->ysize, &(arHandle->labelInfo), arHandle->arImageProcMode, arHandle->areaMax, arHandle->areaMin, arHandle->squareFitThresh, arHandle->markerInfo2, &(arHandle->marker2_num)) < 0) return -1;
                if (arGetMarkerInfo(frame->buff, arHandle->xsize, arHandle->ysize, arHandle->arPixelFormat, arHandle->markerInfo2, arHandle->marker2_num, arHandle->pattHandle, arHandle->arImageProcMode, arHandle->arPatternDetectionMode, &(arHandle->arParamLT->paramLTf), arHandle->pattRatio, arHandle->markerInfo, &(arHandle->marker_num), arHandle->matrixCodeType) < 0) return -1;
                marker_nums[i] = 0;
                for (j = 0; j < arHandle->marker_num; j++) if (arHandle->markerInfo[j].idPatt != -1 || arHandle->markerInfo[j].idMatrix != -1) marker_nums[i]++;
            }

            if (arHandle->arDebug == AR_DEBUG_ENABLE) ARLOGe("Auto threshold (bracket) marker counts -[%3d: %3d] [%3d: %3d] [%3d: %3d]+.\n", thresholds[1], marker_nums[1], thresholds[2], marker_nums[2], thresholds[0], marker_nums[0]);
        
            // If neither of the bracketed values was superior, then change the size of the bracket.
            if (marker_nums[0] <= marker_nums[2] && marker_nums[1] <= marker_nums[2]) {
                if (arHandle->arLabelingThreshAutoBracketOver < arHandle->arLabelingThreshAutoBracketUnder) {
                    arHandle->arLabelingThreshAutoBracketOver++;
                } else if (arHandle->arLabelingThreshAutoBracketOver > arHandle->arLabelingThreshAutoBracketUnder) {
                    arHandle->arLabelingThreshAutoBracketUnder++;
                } else {
                    arHandle->arLabelingThreshAutoBracketOver++;
                    arHandle->arLabelingThreshAutoBracketUnder++;
                }
                if ((thresholds[2] + arHandle->arLabelingThreshAutoBracketOver) >= 255) arHandle->arLabelingThreshAutoBracketOver = 1; // If the bracket has hit the end of the range, reset it.
                if ((thresholds[2] - arHandle->arLabelingThreshAutoBracketOver) <= 0) arHandle->arLabelingThreshAutoBracketUnder = 1; // If a bracket has hit the end of the range, reset it.
                detectionIsDone = 1;
            } else {
                arHandle->arLabelingThresh = (marker_nums[0] >= marker_nums[1] ? thresholds[0] : thresholds[1]);
                threshDiff = arHandle->arLabelingThresh - thresholds[2];
                if (threshDiff > 0) {
                    arHandle->arLabelingThreshAutoBracketOver = (threshDiff + 1) / 2;
                    arHandle->arLabelingThreshAutoBracketUnder = 1;
                } else {
                    arHandle->arLabelingThreshAutoBracketOver = 1;
                    arHandle->arLabelingThreshAutoBracketUnder = (-threshDiff + 1) / 2;
                }
                if (arHandle->arDebug == AR_DEBUG_ENABLE) ARLOGe("Auto threshold (bracket) adjusted threshold to %d.\n", arHandle->arLabelingThresh);
            }
            arHandle->arLabelingThreshAutoIntervalTTL = arHandle->arLabelingThreshAutoInterval;
        }
    }
    
    if (!detectionIsDone) {
        if (arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE) {
            
            int ret;
            ret = arImageProcLumaHistAndBoxFilterWithBias(arHandle->arImageProcInfo, frame->buffLuma, arHandle->arLabelingThreshAutoAdaptiveKernelSize, arHandle->arLabelingThreshAutoAdaptiveBias);
            if (ret < 0) return (ret);
            
            ret = arLabeling(frame->buffLuma, arHandle->arImageProcInfo->imageX, arHandle->arImageProcInfo->imageY,
                             arHandle->arDebug, arHandle->arLabelingMode,
                             0, AR_IMAGE_PROC_FRAME_IMAGE,
                             &(arHandle->labelInfo), arHandle->arImageProcInfo->image2);
            if (ret < 0) return (ret);
            
        } else { // !adaptive
            
            if (arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_MEDIAN || arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_OTSU) {
                // Do an auto-threshold operation.
                if (arHandle->arLabelingThreshAutoIntervalTTL > 0) {
                    arHandle->arLabelingThreshAutoIntervalTTL--;
                } else {
                    int ret;
                    unsigned char value;
                    if (arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_MEDIAN) ret = arImageProcLumaHistAndCDFAndMedian(arHandle->arImageProcInfo, frame->buffLuma, &value);
                    else ret = arImageProcLumaHistAndOtsu(arHandle->arImageProcInfo, frame->buffLuma, &value);
                    if (ret < 0) return (ret);
                    if (arHandle->arDebug == AR_DEBUG_ENABLE && arHandle->arLabelingThresh != value) ARLOGi("Auto threshold (%s) adjusted threshold to %d.\n", (arHandle->arLabelingThreshMode == AR_LABELING_THRESH_MODE_AUTO_MEDIAN ? "median" : "Otsu"), value);
                    arHandle->arLabelingThresh = value;
                    arHandle->arLabelingThreshAutoIntervalTTL = arHandle->arLabelingThreshAutoInterval;
                }
            }
            
            if( arLabeling(frame->buffLuma, arHandle->xsize, arHandle->ysize,
                           arHandle->arDebug, arHandle->arLabelingMode,
                           arHandle->arLabelingThresh, arHandle->arImageProcMode,
                           &(arHandle->labelInfo), NULL) < 0 ) {
                return -1;
            }
            
        }
        
        if( arDetectMarker2( arHandle->xsize, arHandle->ysize,
                            &(arHandle->labelInfo), arHandle->arImageProcMode,
                            arHandle->areaMax, arHandle->areaMin, arHandle->squareFitThresh,
                            arHandle->markerInfo2, &(arHandle->marker2_num) ) < 0 ) {
            return -1;
        }
        
        if( arGetMarkerInfo(frame->buff, arHandle->xsize, arHandle->ysize, arHandle->arPixelFormat,
                            arHandle->markerInfo2, arHandle->marker2_num,
                            arHandle->pattHandle, arHandle->arImageProcMode,
                            arHandle->arPatternDetectionMode, &(arHandle->arParamLT->paramLTf), arHandle->pattRatio,
                            arHandle->markerInfo, &(arHandle->marker_num),
                            arHandle->matrixCodeType ) < 0 ) {
            return -1;
        }
    } // !detectionIsDone
    
    if (arHandle->arCornerRefinementMode == AR_CORNER_REFINEMENT_ENABLE) {
        // Refine marker co-ordinates.
        ARfloat obVertex[4][2];
        for (int i = 0; i < 4; i++) {
            float arXIn = (float)arHandle->markerInfo->vertex[i][0];
            float arYIn = (float)arHandle->markerInfo->vertex[i][1];
            arParamIdeal2ObservLTf(&arHandle->arParamLT->paramLTf, arXIn, arYIn, &obVertex[i][0], &obVertex[i][1]);
        }
        arRefineCorners((float (*)[2])obVertex, frame->buffLuma, arHandle->xsize, arHandle->ysize);
        for (int i = 0; i < 4; i++) {
            float newX, newY;
            arParamObserv2IdealLTf(&arHandle->arParamLT->paramLTf, obVertex[i][0], obVertex[i][1], &newX, &newY);
            arHandle->markerInfo->vertex[i][0] = (ARdouble)newX;
            arHandle->markerInfo->vertex[i][1] = (ARdouble)newY;
        }
        //free(obVertex);
    }
    
    // If history mode is not enabled, just perform a basic confidence cutoff.
    if (arHandle->arMarkerExtractionMode == AR_NOUSE_TRACKING_HISTORY) {
        confidenceCutoff(arHandle);
        return 0;
    }

/*------------------------------------------------------------*/

    // For all history records, check every identified marker, to see if the position and size of the marker
    // as recorded in the history record is very similar to one of the identified markers.
    // If it is, and the history record has a higher confidence value, then use the  pattern matching
    // information (marker ID, confidence, and direction) info from the history instead.
    for( i = 0; i < arHandle->history_num; i++ ) {
        rlenmin = 0.5;
        cid = -1;
        for( j = 0; j < arHandle->marker_num; j++ ) {
            rarea = (ARdouble)arHandle->history[i].marker.area / (ARdouble)arHandle->markerInfo[j].area;
            if( rarea < 0.7 || rarea > 1.43 ) continue;
            rlen = ( (arHandle->markerInfo[j].pos[0] - arHandle->history[i].marker.pos[0])
                   * (arHandle->markerInfo[j].pos[0] - arHandle->history[i].marker.pos[0])
                   + (arHandle->markerInfo[j].pos[1] - arHandle->history[i].marker.pos[1])
                   * (arHandle->markerInfo[j].pos[1] - arHandle->history[i].marker.pos[1]) )
                   / arHandle->markerInfo[j].area;
            if( rlen < rlenmin ) {
                rlenmin = rlen;
                cid = j;
            }
        }
        if (cid >= 0) {
            if (arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR || arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO || arHandle->arPatternDetectionMode == AR_MATRIX_CODE_DETECTION) {
                // Either template or matrix, but not both.
                if (arHandle->markerInfo[cid].cf < arHandle->history[i].marker.cf) {
                    arHandle->markerInfo[cid].cf = arHandle->history[i].marker.cf;
                    arHandle->markerInfo[cid].id = arHandle->history[i].marker.id;
                    diffmin = 10000.0 * 10000.0;
                    cdir = -1;
                    for( j = 0; j < 4; j++ ) {
                        diff = 0;
                        for( k = 0; k < 4; k++ ) {
                            diff += (arHandle->history[i].marker.vertex[k][0] - arHandle->markerInfo[cid].vertex[(j+k)%4][0])
                            * (arHandle->history[i].marker.vertex[k][0] - arHandle->markerInfo[cid].vertex[(j+k)%4][0])
                            + (arHandle->history[i].marker.vertex[k][1] - arHandle->markerInfo[cid].vertex[(j+k)%4][1])
                            * (arHandle->history[i].marker.vertex[k][1] - arHandle->markerInfo[cid].vertex[(j+k)%4][1]);
                        }
                        if( diff < diffmin ) {
                            diffmin = diff;
                            cdir = (arHandle->history[i].marker.dir - j + 4) % 4;
                        }
                    }
                    arHandle->markerInfo[cid].dir = cdir;
                    // Copy the id, cf, and dir back to the appropriate mode-dependent values too.
                    if (arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR || arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO) {
                        arHandle->markerInfo[cid].idPatt  = arHandle->markerInfo[cid].id;
                        arHandle->markerInfo[cid].cfPatt  = arHandle->markerInfo[cid].cf;
                        arHandle->markerInfo[cid].dirPatt = arHandle->markerInfo[cid].dir;
                    } else {
                        arHandle->markerInfo[cid].idMatrix  = arHandle->markerInfo[cid].id;
                        arHandle->markerInfo[cid].cfMatrix  = arHandle->markerInfo[cid].cf;
                        arHandle->markerInfo[cid].dirMatrix = arHandle->markerInfo[cid].dir;
                    }
                }
            } else if (arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX) {
                // Both template and matrix.
                if (arHandle->markerInfo[cid].cfPatt < arHandle->history[i].marker.cfPatt || arHandle->markerInfo[cid].cfMatrix < arHandle->history[i].marker.cfMatrix) {
                    arHandle->markerInfo[cid].cfPatt = arHandle->history[i].marker.cfPatt;
                    arHandle->markerInfo[cid].idPatt = arHandle->history[i].marker.idPatt;
                    arHandle->markerInfo[cid].cfMatrix = arHandle->history[i].marker.cfMatrix;
                    arHandle->markerInfo[cid].idMatrix = arHandle->history[i].marker.idMatrix;
                    diffmin = 10000.0 * 10000.0;
                    cdir = -1;
                    for( j = 0; j < 4; j++ ) {
                        diff = 0;
                        for( k = 0; k < 4; k++ ) {
                            diff += (arHandle->history[i].marker.vertex[k][0] - arHandle->markerInfo[cid].vertex[(j+k)%4][0])
                            * (arHandle->history[i].marker.vertex[k][0] - arHandle->markerInfo[cid].vertex[(j+k)%4][0])
                            + (arHandle->history[i].marker.vertex[k][1] - arHandle->markerInfo[cid].vertex[(j+k)%4][1])
                            * (arHandle->history[i].marker.vertex[k][1] - arHandle->markerInfo[cid].vertex[(j+k)%4][1]);
                        }
                        if( diff < diffmin ) {
                            diffmin = diff;
                            cdir = j;
                        }
                    }
                    arHandle->markerInfo[cid].dirPatt   = (arHandle->history[i].marker.dirPatt   - cdir + 4) % 4;
                    arHandle->markerInfo[cid].dirMatrix = (arHandle->history[i].marker.dirMatrix - cdir + 4) % 4;
                }
            }
            else return -1; // Unsupported arPatternDetectionMode.
        } // cid >= 0
    }

    confidenceCutoff(arHandle);

    // Age all history records (and expire old records, i.e. where count >= 4).
    for( i = j = 0; i < arHandle->history_num; i++ ) {
        arHandle->history[i].count++;
        if( arHandle->history[i].count < 4 ) {
            if (i != j) arHandle->history[j] = arHandle->history[i];
            j++;
        }
    }
    arHandle->history_num = j;

    // Save current marker info in history.
    for( i = 0; i < arHandle->marker_num; i++ ) {
        if( arHandle->markerInfo[i].id < 0 ) continue;

        // Check if an ARTrackingHistory record already exists for this marker ID.
        for( j = 0; j < arHandle->history_num; j++ ) {
            if( arHandle->history[j].marker.id == arHandle->markerInfo[i].id ) break;
        }
        if( j == arHandle->history_num ) { // If a pre-existing ARTrackingHistory record was not found,
            if( arHandle->history_num == AR_SQUARE_MAX ) break; // exit if we've filled all available history slots,
            arHandle->history_num++; // Otherwise count the newly created record.
        }
        arHandle->history[j].marker = arHandle->markerInfo[i]; // Save the marker info.
        arHandle->history[j].count  = 1; // Reset count to indicate info is fresh.
    }

    if( arHandle->arMarkerExtractionMode == AR_USE_TRACKING_HISTORY_V2 ) {
        return 0;
    }


    for( i = 0; i < arHandle->history_num; i++ ) {
        for( j = 0; j < arHandle->marker_num; j++ ) {
            rarea = (ARdouble)arHandle->history[i].marker.area / (ARdouble)arHandle->markerInfo[j].area;
            if( rarea < 0.7 || rarea > 1.43 ) continue;
            rlen = ( (arHandle->markerInfo[j].pos[0] - arHandle->history[i].marker.pos[0])
                   * (arHandle->markerInfo[j].pos[0] - arHandle->history[i].marker.pos[0])
                   + (arHandle->markerInfo[j].pos[1] - arHandle->history[i].marker.pos[1])
                   * (arHandle->markerInfo[j].pos[1] - arHandle->history[i].marker.pos[1]) )
                   / arHandle->markerInfo[j].area;
            if( rlen < 0.5 ) break;
        }
        if( j == arHandle->marker_num ) {
            arHandle->markerInfo[arHandle->marker_num] = arHandle->history[i].marker;
            arHandle->marker_num++;
        }
    }

    return 0;
}

static void confidenceCutoff(ARHandle *arHandle)
{
    int i, cfOK;
    
    if (arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR || arHandle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO) {
        for (i = 0; i < arHandle->marker_num; i++) {
            if (arHandle->markerInfo[i].id >= 0 && arHandle->markerInfo[i].cf < AR_CONFIDENCE_CUTOFF_DEFAULT) {
                arHandle->markerInfo[i].id = arHandle->markerInfo[i].idPatt = -1;
                arHandle->markerInfo[i].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE;
            }
        }
    } else if (arHandle->arPatternDetectionMode == AR_MATRIX_CODE_DETECTION ) {
        for (i = 0; i < arHandle->marker_num; i++) {
            if (arHandle->markerInfo[i].id >= 0 && arHandle->markerInfo[i].cf < AR_CONFIDENCE_CUTOFF_DEFAULT) {
                arHandle->markerInfo[i].id = arHandle->markerInfo[i].idMatrix = -1;
                arHandle->markerInfo[i].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE;
            }
        }
    } else {
        for (i = 0; i < arHandle->marker_num; i++) {
            cfOK = 0;
            if (arHandle->markerInfo[i].idPatt >= 0 && arHandle->markerInfo[i].cfPatt < AR_CONFIDENCE_CUTOFF_DEFAULT) {
                arHandle->markerInfo[i].idPatt = -1;
            } else cfOK = 1;
            if (arHandle->markerInfo[i].idMatrix >= 0 && arHandle->markerInfo[i].cfMatrix < AR_CONFIDENCE_CUTOFF_DEFAULT) {
                arHandle->markerInfo[i].idMatrix = -1;
            } else cfOK = 1;
            if (!cfOK) arHandle->markerInfo[i].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE;
        }
    }
}

