/*
 *  ARTrackableMultiSquareAuto.cpp
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
 *  Copyright 2018 Eden Networks Ltd.
 *
 *  Author(s): Philip Lamb.
 *
 */

#include <ARX/ARTrackableMultiSquareAuto.h>
#include "AR/matrixCode.h"

#if USE_GTSAM
#  include "mapper.hpp"
#endif

// This struct contains member variables we don't want to appear in the header.
struct ARTrackableMapPrivateMembers {
    ARTrackableMapPrivateMembers()
#if USE_GTSAM
    : m_mapper(0.04, 1)
#endif
    {
    }
#if USE_GTSAM
    arx_mapper::Mapper m_mapper;
#endif
};

ARTrackableMultiSquareAuto::ARTrackableMultiSquareAuto() : ARTrackable(MULTI_AUTO),
    m_OriginMarkerUid(0),
    m_markerWidth(80.0f),
    m_MultiConfig(NULL),
    m_pm(new struct ARTrackableMapPrivateMembers)
{
    m_MultiConfig = arMultiAllocConfig();
}

ARTrackableMultiSquareAuto::~ARTrackableMultiSquareAuto()
{
    if (m_MultiConfig) {
        arMultiFreeConfig(m_MultiConfig);
        m_MultiConfig = NULL;
    }
}

bool ARTrackableMultiSquareAuto::initWithOriginMarkerUID(int originMarkerUID, ARdouble markerWidth)
{
    m_OriginMarkerUid = originMarkerUID;
    m_markerWidth = markerWidth;
    return true;
}

bool ARTrackableMultiSquareAuto::updateWithDetectedMarkers(ARMarkerInfo* markerInfo, int markerNum, int videoWidth, int videoHeight, AR3DHandle *ar3DHandle) {
    
    ARLOGd("ARTrackableMultiSquareAuto::updateWithDetectedMarkers(...)\n");
    
    visiblePrev = visible;
    visible = false;
    
    // Need at least one detected marker.
    if (markerInfo && markerNum > 0) {
        
        // Shallow copy the markerInfo array, so that we can filter it.
        ARMarkerInfo *markerInfoCopy = (ARMarkerInfo *)malloc(sizeof(ARMarkerInfo) * markerNum);
        memcpy(markerInfoCopy, markerInfo, sizeof(ARMarkerInfo) * markerNum);
        
        int goodMarkers = 0;
        int discarded = 0;
        ARdouble borderX = (ARdouble)videoWidth*m_ImageBorderZone;
        ARdouble borderY = (ARdouble)videoHeight*m_ImageBorderZone;
        for (int i = 0; i < markerNum; i++) {
            if (markerInfoCopy[i].idMatrix == -1) continue;

            // Discard markers towards the edge of the image.
            if (m_ImageBorderZone > 0.0f) {
                if (markerInfoCopy[i].pos[0] < borderX || markerInfoCopy[i].pos[0] > ((ARdouble)videoWidth - borderX)
                    || markerInfoCopy[i].pos[1] < borderY || markerInfoCopy[i].pos[1] > ((ARdouble)videoHeight - borderY)) {
                    markerInfoCopy[i].id = markerInfoCopy[i].idPatt = markerInfoCopy[i].idMatrix = -1;
                    discarded++;
                    continue;
                }
            }
            
            // Marker is OK.
            goodMarkers++;
        }
        //if (discarded > 0) ARLOGd("Discarded %d markers not in image centre.\n", discarded);
        
        if (goodMarkers > 0) {
            // If map is empty, see if we've found the first marker.
            if (m_MultiConfig->marker_num == 0) {
                
                for (int i = 0; i < markerNum; i++) {
                    if (markerInfoCopy[i].idMatrix != -1 && markerInfoCopy[i].idMatrix == m_OriginMarkerUid) {
                        ARLOGi("Initing marker map with marker %d.\n", m_OriginMarkerUid);
                        ARdouble origin[3][4] = {{1.0, 0.0, 0.0, 0.0},  {0.0, 1.0, 0.0, 0.0},  {0.0, 0.0, 1.0, 0.0}};
                        arMultiAddOrUpdateSubmarker(m_MultiConfig, m_OriginMarkerUid, AR_MULTI_PATTERN_TYPE_MATRIX, m_markerWidth, origin, 0);
                    }
                }
            }
            
            // If map is not empty, calculate the pose of the multimarker in camera frame, i.e. trans_M_c.
            if (m_MultiConfig->marker_num > 0) {
                
                ARdouble multiErr;
                if (m_robustFlag) multiErr = arGetTransMatMultiSquareRobust(ar3DHandle, markerInfoCopy, markerNum, m_MultiConfig);
                else multiErr = arGetTransMatMultiSquare(ar3DHandle, markerInfoCopy, markerNum, m_MultiConfig);
                if (m_MultiConfig->prevF != 0) {
                    
                    //ARLOGi("Got multimarker pose with err=%0.3f\n", multiErr);
                    //arUtilPrintTransMat(m_MultiConfig->trans);
                    memcpy(trans, m_MultiConfig->trans, sizeof(trans));
                    visible = true;
#if !USE_GTSAM
                    // Construct map by simple inter-marker pose estimation.
                    // This approach will result in accumulation of pose errors as estimates are chained from
                    // previously estimated markers. Also, absolute pose error increases with distance from the origin.
                    
                    // Get the pose of the camera in the multimarker frame, i.e. trans_c_M.
                    ARdouble trans_c_M[3][4];
                    arUtilMatInv(m_MultiConfig->trans, trans_c_M);
                    
                    // Now add or update all markers (except never update the origin marker).
                    // Get pose in camera frame of individual marker, i.e. trans_m_c, compose with trans_c_M
                    // to get pose in multimarker local coordinate system, i.e. trans_m_M.
                    
                    for (int i = 0; i < markerNum; i++) {
                        if (markerInfoCopy[i].idMatrix != -1 && markerInfoCopy[i].idMatrix != m_OriginMarkerUid) {
                            ARdouble trans_m_c[3][4];
                            ARdouble err = arGetTransMatSquare(ar3DHandle, &(markerInfoCopy[i]), m_markerWidth, trans_m_c);
                            if (err < m_maxErr) {
                                ARdouble trans_m_M[3][4];
                                arUtilMatMul(trans_c_M, trans_m_c, trans_m_M);
                                
                                int multi_marker_count_prev = m_MultiConfig->marker_num;
                                arMultiAddOrUpdateSubmarker(m_MultiConfig, markerInfoCopy[i].idMatrix, AR_MULTI_PATTERN_TYPE_MATRIX, m_markerWidth, trans_m_M, 0);
                                if (m_MultiConfig->marker_num > multi_marker_count_prev) {
                                    ARLOGi("Added marker %d to map (now %d markers in map) with pose:\n", markerInfoCopy[i].idMatrix, m_MultiConfig->marker_num);
                                    arUtilPrintTransMat(trans_m_M);
                                }
                            }
                        }
                    }
#else
                    // Construct map using GTSAM's iSAM2 (incremental smoothing and mapping v2).
                    // This results in pose error values which are minimised over the entire map.
                    
                    // Add a pose estimate to the graph.
                    m_pm->m_mapper.AddPose(m_MultiConfig->trans);
                    
                    // Now add factors for our marker observations to the graph.
                    std::vector<arx_mapper::Marker> markers;
                    for (int i = 0; i < markerNum; i++) {
                        if (markerInfoCopy[i].idMatrix != -1) {
                            arx_mapper::Marker marker;
                            ARdouble err = arGetTransMatSquare(ar3DHandle, &(markerInfoCopy[i]), m_markerWidth, marker.trans);
                            if (err < m_maxErr) {
                                marker.uid = markerInfoCopy[i].idMatrix;
                                markers.push_back(marker);
                            }
                        }
                    }
                    m_pm->m_mapper.AddFactors(markers);
                    
                    // Do mapping.
                    if (!m_pm->m_mapper.inited()) {
                        // Add a landmark for the origin marker.
                        // We fix this in the map at the origin and thus fix the scale for first pose and first landmark.
                        m_pm->m_mapper.Initialize(m_OriginMarkerUid, m_markerWidth);
                    } else {
                        // This will add new landmarks for each marker not previously seen, with the
                        // initial pose estimate calculated from the marker pose in the camera frame
                        // composed with the camera pose in the map frame.
                        m_pm->m_mapper.AddLandmarks(markers);
                        m_pm->m_mapper.Optimize();
                        // Get latest estimates from mapper and put into map.
                        m_pm->m_mapper.Update(m_MultiConfig);
                        // Prepare for next iteration.
                        m_pm->m_mapper.Clear();
                    }
#endif // USE_GTSAM
                    
                } // m_MultiConfig->prevF != 0
            } // m_MultiConfig->marker_num > 0

        } // goodMarkers > 0
        
        free(markerInfoCopy);
    } // markerInfo && markerNum > 0
    
    return (ARTrackable::update()); // Parent class will finish update.
}

bool ARTrackableMultiSquareAuto::updateWithDetectedMarkersStereo(ARMarkerInfo* markerInfoL, int markerNumL, int videoWidthL, int videoHeightL, ARMarkerInfo* markerInfoR, int markerNumR, int videoWidthR, int videoHeightR, AR3DStereoHandle *handle, ARdouble transL2R[3][4]) {
    
    ARLOGd("ARTrackableMultiSquareAuto::updateWithDetectedMarkersStereo(...)\n");
    
    visiblePrev = visible;
    visible = false;
    
    if (markerInfoL && markerNumL > 0 && markerInfoR && markerNumR > 0) {
        // TODO: stereo implementation.
    } // markerInfoL && markerNumL > 0 && markerInfoR && markerNumR > 0
    
    return (ARTrackable::update(transL2R)); // Parent class will finish update.
}

ARMultiMarkerInfoT *ARTrackableMultiSquareAuto::copyMultiConfig()
{
    return arMultiCopyConfig(m_MultiConfig);
}

int ARTrackableMultiSquareAuto::getPatternCount()
{
    if (!m_MultiConfig) return 0;
    return m_MultiConfig->marker_num;
}

std::pair<float, float> ARTrackableMultiSquareAuto::getPatternSize(int patternIndex)
{
    return std::pair<float, float>(m_markerWidth, m_markerWidth);
}

std::pair<int, int> ARTrackableMultiSquareAuto::getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    return std::pair<int, int>(matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK);
}

bool ARTrackableMultiSquareAuto::getPatternTransform(int patternIndex, ARdouble T[16])
{
    if (!m_MultiConfig || patternIndex < 0 || patternIndex >= m_MultiConfig->marker_num) return false;

    T[ 0] = m_MultiConfig->marker[patternIndex].trans[0][0];
    T[ 1] = m_MultiConfig->marker[patternIndex].trans[1][0];
    T[ 2] = m_MultiConfig->marker[patternIndex].trans[2][0];
    T[ 3] = _0_0;
    T[ 4] = m_MultiConfig->marker[patternIndex].trans[0][1];
    T[ 5] = m_MultiConfig->marker[patternIndex].trans[1][1];
    T[ 6] = m_MultiConfig->marker[patternIndex].trans[2][1];
    T[ 7] = _0_0;
    T[ 8] = m_MultiConfig->marker[patternIndex].trans[0][2];
    T[ 9] = m_MultiConfig->marker[patternIndex].trans[1][2];
    T[10] = m_MultiConfig->marker[patternIndex].trans[2][2];
    T[11] = _0_0;
    T[12] = m_MultiConfig->marker[patternIndex].trans[0][3];
    T[13] = m_MultiConfig->marker[patternIndex].trans[1][3];
    T[14] = m_MultiConfig->marker[patternIndex].trans[2][3];
    T[15] = _1_0;
    return true;
}

bool ARTrackableMultiSquareAuto::getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    if (!m_MultiConfig || patternIndex < 0 || patternIndex >= m_MultiConfig->marker_num) return false;

    uint8_t *code;
    encodeMatrixCode(matrixCodeType, m_MultiConfig->marker[patternIndex].patt_id, &code);
    int barcode_dimensions = matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK;
    int bit = 0;
#ifdef AR_LITTLE_ENDIAN
    const uint32_t colour_black = 0xff000000;
#else
    const uint32_t colour_black = 0x000000ff;
#endif
    const uint32_t colour_white = 0xffffffff;
    for (int row = barcode_dimensions - 1; row >= 0; row--) {
        for (int col = barcode_dimensions - 1; col >= 0; col--) {
            uint32_t pixel_colour;
            if ((row == 0 || row == (barcode_dimensions - 1)) && col == 0) {
                pixel_colour = colour_black;
            } else if (row == (barcode_dimensions - 1) && col == (barcode_dimensions - 1)) {
                pixel_colour = colour_white;
            } else {
                if (code[bit]) pixel_colour = colour_black;
                else pixel_colour = colour_white;
                bit++;
            }
            pattImageBuffer[barcode_dimensions * (barcode_dimensions - 1 - row) + col] = pixel_colour; // Flip pattern in Y, because output texture has origin at lower-left.
        }
    }
    free(code);
    return true;
}
