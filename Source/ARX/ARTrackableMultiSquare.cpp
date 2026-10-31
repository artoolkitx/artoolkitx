/*
 *  ARTrackableMultiSquare.cpp
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb.
 *
 */

#include <ARX/ARTrackableMultiSquare.h>
#include <ARX/ARController.h>
#include "AR/matrixCode.h"

#ifdef ARDOUBLE_IS_FLOAT
#  define _0_0 0.0f
#  define _1_0 1.0f
#else
#  define _0_0 0.0
#  define _1_0 1.0
#endif

ARTrackableMultiSquare::ARTrackableMultiSquare() : ARTrackable(MULTI),
    m_loaded(false),
    m_arPattHandle(NULL),
    config(NULL),
    robustFlag(true)
{
}

ARTrackableMultiSquare::~ARTrackableMultiSquare()
{
	if (m_loaded) unload();
}

bool ARTrackableMultiSquare::load(const char *multiConfig, ARPattHandle *arPattHandle)
{
	if (m_loaded) unload();
    m_arPattHandle = arPattHandle;
    config = arMultiReadConfigFile(multiConfig, m_arPattHandle);
	
	if (!config) {
		ARLOGe("Error loading multimarker config %s\n", multiConfig);
		return false;
	}
	
	visible = visiblePrev = false;
    config->min_submarker = 0;
    m_loaded = true;
	return true;
}

bool ARTrackableMultiSquare::unload()
{
    if (m_loaded) {
        if (config) {
            arMultiFreeConfig(config);
            config = NULL;
        }
        m_arPattHandle = NULL;
        m_loaded = false;
    }
	
	return true;
}

bool ARTrackableMultiSquare::updateWithDetectedMarkers(ARMarkerInfo* markerInfo, int markerNum, AR3DHandle *ar3DHandle)
{
	if (!m_loaded || !config) return false;			// Can't update without multimarker config

    visiblePrev = visible;

	if (markerInfo) {
	
		ARdouble err;

		if (robustFlag) {
			err = arGetTransMatMultiSquareRobust(ar3DHandle, markerInfo, markerNum, config);		
		} else {
			err = arGetTransMatMultiSquare(ar3DHandle, markerInfo, markerNum, config);
		}
		
		// Marker is visible if a match was found.
        if (config->prevF != 0) {
            visible = true;
            for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) trans[j][k] = config->trans[j][k];
        } else visible = false;

	} else visible = false;

	return (ARTrackable::update()); // Parent class will finish update.
}

bool ARTrackableMultiSquare::updateWithDetectedMarkersStereo(ARMarkerInfo* markerInfoL, int markerNumL, ARMarkerInfo* markerInfoR, int markerNumR, AR3DStereoHandle *handle, ARdouble transL2R[3][4])
{
	if (!m_loaded || !config) return false;			// Can't update without multimarker config
    
    visiblePrev = visible;
    
	if (markerInfoL && markerInfoR) {
        
		ARdouble err;
        
		if (robustFlag) {
			err = arGetTransMatMultiSquareStereoRobust(handle, markerInfoL, markerNumL, markerInfoR, markerNumR, config);
		} else {
			err = arGetTransMatMultiSquareStereo(handle, markerInfoL, markerNumL, markerInfoR, markerNumR, config);
		}
		
		// Marker is visible if a match was found.
        if (config->prevF != 0) {
            visible = true;
            for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) trans[j][k] = config->trans[j][k];
        } else visible = false;
        
	} else visible = false;
    
	return (ARTrackable::update(transL2R)); // Parent class will finish update.
}

int ARTrackableMultiSquare::getPatternCount()
{
    if (!config) return 0;
    return config->marker_num;
}

std::pair<float, float> ARTrackableMultiSquare::getPatternSize(int patternIndex)
{
    if (!config || patternIndex < 0 || patternIndex >= config->marker_num) return std::pair<float, float>();
    return std::pair<float, float>((float)config->marker[patternIndex].width, (float)config->marker[patternIndex].width);
}

std::pair<int, int> ARTrackableMultiSquare::getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    if (!config || patternIndex < 0 || patternIndex >= config->marker_num) return std::pair<int, int>();
    if (config->marker[patternIndex].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE) {
        if (!m_arPattHandle) return std::pair<int, int>();
        return std::pair<int, int>(m_arPattHandle->pattSize, m_arPattHandle->pattSize);
    } else  /* config->marker[patternIndex].patt_type == AR_PATTERN_TYPE_MATRIX */ {
        return std::pair<int, int>(matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK);
    }
}

bool ARTrackableMultiSquare::getPatternTransform(int patternIndex, ARdouble T[16])
{
    if (!config || patternIndex < 0 || patternIndex >= config->marker_num) return false;

    T[ 0] = config->marker[patternIndex].trans[0][0];
    T[ 1] = config->marker[patternIndex].trans[1][0];
    T[ 2] = config->marker[patternIndex].trans[2][0];
    T[ 3] = _0_0;
    T[ 4] = config->marker[patternIndex].trans[0][1];
    T[ 5] = config->marker[patternIndex].trans[1][1];
    T[ 6] = config->marker[patternIndex].trans[2][1];
    T[ 7] = _0_0;
    T[ 8] = config->marker[patternIndex].trans[0][2];
    T[ 9] = config->marker[patternIndex].trans[1][2];
    T[10] = config->marker[patternIndex].trans[2][2];
    T[11] = _0_0;
    T[12] = config->marker[patternIndex].trans[0][3];
    T[13] = config->marker[patternIndex].trans[1][3];
    T[14] = config->marker[patternIndex].trans[2][3];
    T[15] = _1_0;
    return true;
}

bool ARTrackableMultiSquare::getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    if (!config || patternIndex < 0 || patternIndex >= config->marker_num) return false;

    if (config->marker[patternIndex].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE) {
        if (!m_arPattHandle || !m_arPattHandle->pattf[config->marker[patternIndex].patt_id]) return false;
        const int *arr = m_arPattHandle->patt[config->marker[patternIndex].patt_id * 4];
        for (int y = 0; y < m_arPattHandle->pattSize; y++) {
            for (int x = 0; x < m_arPattHandle->pattSize; x++) {

                int pattIdx = (m_arPattHandle->pattSize - 1 - y)*m_arPattHandle->pattSize + x; // Flip pattern in Y.
                int buffIdx = y*m_arPattHandle->pattSize + x;

#ifdef AR_LITTLE_ENDIAN
                pattImageBuffer[buffIdx] = 0xff000000 | (arr[pattIdx*3] & 0xff) << 16 | (arr[pattIdx*3 + 1] & 0xff) << 8 | (arr[pattIdx*3 + 2] & 0xff); // In-memory ordering will be R->G->B->A.
#else
                pattImageBuffer[buffIdx] = (arr[pattIdx*3 + 2] & 0xff) << 24 | (arr[pattIdx*3 + 1] & 0xff) << 16 | (arr[pattIdx*3] & 0xff) << 8 | 0xff;
#endif
            }
        }
        return true;
    } else  /* config->marker[patternIndex].patt_type == AR_PATTERN_TYPE_MATRIX */ {
        uint8_t *code;
        encodeMatrixCode(matrixCodeType, config->marker[patternIndex].patt_id, &code);
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
}

