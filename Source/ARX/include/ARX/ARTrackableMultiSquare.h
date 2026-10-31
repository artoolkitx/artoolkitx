/*
 *  ARTrackableMulti.h
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

#ifndef ARMARKERMULTI_H
#define ARMARKERMULTI_H

#include <ARX/ARTrackable.h>
#include <ARX/AR/arMulti.h>

/**
 * Multiple marker type of ARTrackable.
 */
class ARTrackableMultiSquare : public ARTrackable {

private:
    bool m_loaded;
    ARPattHandle *m_arPattHandle;
    
protected:
    bool unload();

public:

	ARMultiMarkerInfoT *config;							///< Structure holding information about the multimarker patterns
	bool robustFlag;									///< Flag specifying which pose estimation approach to use
	
	ARTrackableMultiSquare();
	~ARTrackableMultiSquare();

	bool load(const char *multiConfig, ARPattHandle *arPattHandle);

	/**
	 * Updates the marker with new tracking info.
     * Then calls ARTrackable::update()
     * @param markerInfo		Array containing detected marker information
     * @param markerNum			Number of items in the array
     * @param ar3DHandle        AR3DHandle used to extract marker pose.
     */
	bool updateWithDetectedMarkers(ARMarkerInfo *markerInfo, int markerNum, AR3DHandle *ar3DHandle);

    bool updateWithDetectedMarkersStereo(ARMarkerInfo* markerInfoL, int markerNumL, ARMarkerInfo* markerInfoR, int markerNumR, AR3DStereoHandle *handle, ARdouble transL2R[3][4]);

    int getPatternCount() override;
    std::pair<float, float> getPatternSize(int patternIndex) override;
    std::pair<int, int> getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType) override;
    bool getPatternTransform(int patternIndex, ARdouble T[16]) override;
    bool getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType) override;
};

#endif // !ARMARKERMULTI_H
