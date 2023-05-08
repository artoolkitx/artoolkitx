/*
 *  ARTrackableSquare.h
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

#ifndef ARMARKERSQUARE_H
#define ARMARKERSQUARE_H

#include <ARX/ARTrackable.h>

#define    AR_PATTERN_TYPE_TEMPLATE    0
#define    AR_PATTERN_TYPE_MATRIX      1

/**
 * Single marker type of ARTrackable.
 */
class ARTrackableSquare : public ARTrackable {

private:
    bool m_loaded;
    
protected:
    ARPattHandle *m_arPattHandle;
    ARdouble m_width;
    ARdouble m_cf;
    ARdouble m_cfMin;
    
    bool unload();
    
public:
	
	int patt_id;							///< Pattern ID provided by libAR.
    int patt_type;
    uint64_t globalID;

	ARTrackableSquare();
	~ARTrackableSquare();
    
    bool useContPoseEstimation;
    ARdouble getConfidence();
    ARdouble getConfidenceCutoff();
    void setConfidenceCutoff(ARdouble value);
    ARdouble width() { return m_width; }
    void setWidth(ARdouble width) { m_width = width; }
    
	bool initWithPatternFile(const char* path, ARdouble width, ARPattHandle *arPattHandle);
	bool initWithPatternFromBuffer(const char* buffer, ARdouble width, ARPattHandle *arPattHandle);

    /**
     * Init the trackable as a barcode.
     * @param barcodeID The barcode (matrix code) ID in the range alloweable by the current matrix code type, or 0 if the barcode is a globalD code,
     * @param width The width of the marker between the outer edge of the square borders, in calibrated camera units (usually millimetres).
     * @param globalID_ If barcodeID is 0 and this is non-zero, the globalID code to be used. Otherwise, ignored.
     */
    bool initWithBarcode(int barcodeID, ARdouble width, uint64_t globalID_ = 0);

	/**
	 * Updates the marker with new tracking info.
     * Then calls ARTrackable::update()
     * @param markerInfo		Array containing detected marker information
     * @param markerNum			Number of items in the array
     * @param ar3DHandle        AR3DHandle used to extract marker pose.
     */
	bool updateWithDetectedMarkers(ARMarkerInfo* markerInfo, int markerNum, AR3DHandle *ar3DHandle);

    bool updateWithDetectedMarkersStereo(ARMarkerInfo* markerInfoL, int markerNumL, ARMarkerInfo* markerInfoR, int markerNumR, AR3DStereoHandle *handle, ARdouble transL2R[3][4]);

    int getPatternCount() override;
    std::pair<float, float> getPatternSize(int patternIndex) override;
    std::pair<int, int> getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType) override;
    bool getPatternTransform(int patternIndex, ARdouble T[16]) override;
    bool getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType) override;

};


#endif // !ARMARKERSQUARE_H
