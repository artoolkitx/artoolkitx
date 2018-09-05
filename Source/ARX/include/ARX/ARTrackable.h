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

#include <ARX/ARPattern.h>

#include <vector>

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
	/**
	 * Allocates space for patterns within this trackable.
	 * @param count	The number of patterns to allocate
	 */
	void allocatePatterns(int count);

	/**
	 * Frees allocated patterns and resets the pattern count to zero.
	 */
	void freePatterns();

    ARdouble m_positionScaleFactor;

public:
    
	enum TrackableType {
		SINGLE,								///< A standard single square marker.
		MULTI,								///< A composite marker made up of multiple square markers.
        NFT,                                ///< A rectangular textured marker backed by an NFT data set.
        TwoD,                               ///< A 2D textured marker backed by an image.
        MULTI_AUTO                          ///< An automatically mapped composite marker made up of multiple square matrix (2D barcode) markers.
	};

	int UID;								///< Internal unique ID (note: not the same as artoolkitX pattern ID)
	TrackableType type;						///< Type of trackable: single, multi, ...
	
    // Inputs from subclasses.
    bool visiblePrev;                       ///< Whether or not the trackable was visible prior to last update.
	bool visible;							///< Whether or not the trackable is visible at current time.
    
    // Output.
	ARdouble transformationMatrix[16];		///< Transformation suitable for use in OpenGL
	ARdouble transformationMatrixR[16];		///< Transformation suitable for use in OpenGL
	
	int patternCount;						///< If this trackable has a surface appearance, the number of patterns that it has (1 for single).
	ARPattern** patterns;					///< Array of pointers to patterns

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

	/**
	 * Returns the specified pattern within this trackable.
	 * @param n		The pattern to retrieve
	 */
	ARPattern* getPattern(int n);
    
    // Filter control.
    void setFiltered(bool flag);
    bool isFiltered();
    ARdouble filterSampleRate();
    void setFilterSampleRate(ARdouble rate);
    ARdouble filterCutoffFrequency();
    void setFilterCutoffFrequency(ARdouble freq);

};


#endif // !ARTRACKABLE_H
