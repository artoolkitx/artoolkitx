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

#ifdef ARDOUBLE_IS_FLOAT
#  define _0_0 0.0f
#  define _1_0 1.0f
#else
#  define _0_0 0.0
#  define _1_0 1.0
#endif

ARTrackableMultiSquare::ARTrackableMultiSquare() : ARTrackable(MULTI),
    m_loaded(false),
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
    
    config = arMultiReadConfigFile(multiConfig, arPattHandle);
	
	if (!config) {
		ARLOGe("Error loading multimarker config %s\n", multiConfig);
		return false;
	}
	
	visible = visiblePrev = false;
	
    // ARPatterns to hold images and positions of the patterns for display to the user.
	allocatePatterns(config->marker_num);
	for (int i = 0; i < patternCount; i++) {
        if (config->marker[i].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE) {
            patterns[i]->loadTemplate(config->marker[i].patt_id, arPattHandle, (float)config->marker[i].width);
        } else {
			patterns[i]->loadMatrix(config->marker[i].patt_id, AR_MATRIX_CODE_3x3, (float)config->marker[i].width); // TODO: Determine actual matrix code type in use.
        }
        patterns[i]->m_matrix[ 0] = config->marker[i].trans[0][0];
        patterns[i]->m_matrix[ 1] = config->marker[i].trans[1][0];
        patterns[i]->m_matrix[ 2] = config->marker[i].trans[2][0];
        patterns[i]->m_matrix[ 3] = _0_0;
        patterns[i]->m_matrix[ 4] = config->marker[i].trans[0][1];
        patterns[i]->m_matrix[ 5] = config->marker[i].trans[1][1];
        patterns[i]->m_matrix[ 6] = config->marker[i].trans[2][1];
        patterns[i]->m_matrix[ 7] = _0_0;
        patterns[i]->m_matrix[ 8] = config->marker[i].trans[0][2];
        patterns[i]->m_matrix[ 9] = config->marker[i].trans[1][2];
        patterns[i]->m_matrix[10] = config->marker[i].trans[2][2];
        patterns[i]->m_matrix[11] = _0_0;
        patterns[i]->m_matrix[12] = config->marker[i].trans[0][3];
        patterns[i]->m_matrix[13] = config->marker[i].trans[1][3];
        patterns[i]->m_matrix[14] = config->marker[i].trans[2][3];
        patterns[i]->m_matrix[15] = _1_0;
	}
	config->min_submarker = 0;
    m_loaded = true;
	return true;
}

bool ARTrackableMultiSquare::unload()
{
    if (m_loaded) {
        freePatterns();
        if (config) {
            arMultiFreeConfig(config);
            config = NULL;
        }
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

