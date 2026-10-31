/*
 *  ARTrackable.h
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
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

#ifndef ARTRACKABLE_H
#define ARTRACKABLE_H

#include <ARX/AR/ar.h>
#include <ARX/AR/arFilterTransMat.h>

#include <vector>
#include <utility>
#include <atomic>

#ifdef ARDOUBLE_IS_FLOAT
#  define _0_0 0.0f
#  define _1_0 1.0f
#else
#  define _0_0 0.0
#  define _1_0 1.0
#endif

class ARController; // Forward declaration of owner.

/**
 * Base class for supported trackable types.
 */
class ARTrackable {
    
private:
    ARFilterTransMatInfo *m_ftmi;
    ARdouble   m_filterCutoffFrequency;
    ARdouble   m_filterSampleRate;

protected:
    ARdouble trans[3][4];                   ///< Transformation from camera to this trackable. If stereo, transform from left camera to this trackable.

    ARdouble m_positionScaleFactor;

    static std::atomic<int> nextUID;

public:

	enum TrackableType {
		SINGLE,								///< A standard single square marker.
		MULTI,								///< A composite marker made up of multiple square markers.
        NFT,                                ///< A rectangular textured marker backed by an NFT data set.
        TwoD,                               ///< A 2D textured marker backed by an image.
        MULTI_AUTO                          ///< An automatically mapped composite marker made up of multiple square matrix (2D barcode) markers.
	};

	int UID;								///< Internal unique ID (note: not the same as artoolkitX pattern ID)
    static const int NO_ID = -1;            ///< Value of UID that indicates no ID.
	TrackableType type;						///< Type of trackable: single, multi, ...
	
    // Inputs from subclasses.
    bool visiblePrev;                       ///< Whether or not the trackable was visible prior to last update.
	bool visible;							///< Whether or not the trackable is visible at current time.
    
    // Output.
	ARdouble transformationMatrix[16];		///< Transformation suitable for use in OpenGL
	ARdouble transformationMatrixR[16];		///< Transformation suitable for use in OpenGL
	
	/**
	 * Constructor takes the type of this trackable.
	 */
	ARTrackable(TrackableType type);
    
    ARTrackable(const ARTrackable&) = delete; ///< Copy construction is undefined.
    ARTrackable& operator=(const ARTrackable&) = delete; ///< Copy assignment is undefined.

	virtual ~ARTrackable();
	
    void setPositionScalefactor(ARdouble scale);
    ARdouble positionScalefactor();
    
	/**
	 * Completes an update begun in the parent class, performing filtering, generating
     * OpenGL view matrix and notifying listeners (just a log message at the moment).
     * Subclasses should first do their required updates, set visible, visiblePrev,
     * and trans[3][4] then call ARTrackable::update().
	 * @return true if successful, false if an error occurred
	 */
    virtual bool update(const ARdouble transL2R[3][4] = NULL);


    virtual int getPatternCount() = 0;
    virtual std::pair<float, float> getPatternSize(int patternIndex) = 0;
    virtual std::pair<int, int> getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType) = 0;
    /// Get the transform, relative to this trackable's origin, of this pattern.
    /// Fills T with the transform in column-major (OpenGL) order.
    virtual bool getPatternTransform(int patternIndex, ARdouble T[16]) = 0;
    virtual bool getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType) = 0;

    // Filter control.
    void setFiltered(bool flag);
    bool isFiltered();
    ARdouble filterSampleRate();
    void setFilterSampleRate(ARdouble rate);
    ARdouble filterCutoffFrequency();
    void setFilterCutoffFrequency(ARdouble freq);

};


#endif // !ARTRACKABLE_H
