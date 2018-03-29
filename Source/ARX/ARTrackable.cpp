/*
 *  ARTrackable.cpp
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
 *  Author(s): Julian Looser, Philip Lamb.
 *
 */

#include <ARX/ARTrackable.h>
#include <ARX/ARTrackableSquare.h>
#include <ARX/ARTrackableMultiSquare.h>
#if HAVE_NFT
#  include <ARX/ARTrackableNFT.h>
#endif
#if HAVE_2D
#  include <ARX/ARTrackable2d.h>
#endif
#include <ARX/ARController.h>
#include <ARX/AR/paramGL.h>

#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h>
#endif

ARTrackable::ARTrackable(TrackableType type) :
    m_ftmi(NULL),
    m_filterCutoffFrequency(AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT),
    m_filterSampleRate(AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT),
#ifdef ARDOUBLE_IS_FLOAT
    m_positionScaleFactor(1.0f),
#else
    m_positionScaleFactor(1.0),
#endif
    type(type),
    visiblePrev(false),
	visible(false),
    patternCount(0),
    patterns(NULL)
{
	static int nextUID = 0;
	UID = nextUID++;
}

ARTrackable::~ARTrackable()
{
    freePatterns();

    if (m_ftmi) arFilterTransMatFinal(m_ftmi);
}

void ARTrackable::allocatePatterns(int count)
{
	freePatterns();

    if (count) {
        patternCount = count;
        ARLOGd("Allocating %d patterns on trackable %d.\n", patternCount, UID);
        patterns = new ARPattern*[patternCount];
        for (int i = 0; i < patternCount; i++) {
            patterns[i] = new ARPattern();
        }        
    }
}

void ARTrackable::freePatterns()
{
	if (patternCount) ARLOGd("Freeing %d patterns on trackable %d.\n", patternCount, UID);

	for (int i = 0; i < patternCount; i++) {
        if (patterns[i]) {
            delete patterns[i];
            patterns[i] = NULL;
        }
	}
	if (patterns) {
        delete[] patterns;
        patterns = NULL;
    }

	patternCount = 0;
}

ARPattern* ARTrackable::getPattern(int n)
{
	// Check n is in acceptable range
	if (!patterns || n < 0 || n >= patternCount) return NULL;

	return patterns[n];
}

void ARTrackable::setPositionScalefactor(ARdouble scale)
{
    m_positionScaleFactor = scale;
}

ARdouble ARTrackable::positionScalefactor()
{
    return m_positionScaleFactor;
}

bool ARTrackable::update(const ARdouble transL2R[3][4])
{
    // Subclasses will have already determined visibility and set/cleared 'visible' and 'visiblePrev',
    // as well as setting 'trans'.
    if (visible) {
        
        // Filter the pose estimate.
        if (m_ftmi) {
            if (arFilterTransMat(m_ftmi, trans, !visiblePrev) < 0) {
                ARLOGe("arFilterTransMat error with trackable %d.\n", UID);
            }
        }
        
        if (!visiblePrev) {
            ARLOGi("trackable %d now visible.\n", UID);
        }
        
        // Convert to GL matrix.
#ifdef ARDOUBLE_IS_FLOAT
        arglCameraViewRHf(trans, transformationMatrix, m_positionScaleFactor);
#else
        arglCameraViewRH(trans, transformationMatrix, m_positionScaleFactor);
#endif

        // Do stereo if required.
        if (transL2R) {
            ARdouble transR[3][4];
            
            arUtilMatMul(transL2R, trans, transR);
#ifdef ARDOUBLE_IS_FLOAT
            arglCameraViewRHf(transR, transformationMatrixR, m_positionScaleFactor);
#else
            arglCameraViewRH(transR, transformationMatrixR, m_positionScaleFactor);
#endif
            
        }
    } else {
        
        if (visiblePrev) {
            ARLOGi("Trackable %d no longer visible.\n", UID);
        }
        
    }
    
    return true;
}

void ARTrackable::setFiltered(bool flag)
{
    if (flag && !m_ftmi) {
        m_ftmi = arFilterTransMatInit(m_filterSampleRate, m_filterCutoffFrequency);
    } else if (!flag && m_ftmi) {
        arFilterTransMatFinal(m_ftmi);
        m_ftmi = NULL;
    }
}

bool ARTrackable::isFiltered()
{
    return (m_ftmi != NULL);
}

ARdouble ARTrackable::filterSampleRate()
{
    return m_filterSampleRate;
}

void ARTrackable::setFilterSampleRate(ARdouble rate)
{
    m_filterSampleRate = rate;
    if (m_ftmi) arFilterTransMatSetParams(m_ftmi, m_filterSampleRate, m_filterCutoffFrequency);
}

ARdouble ARTrackable::filterCutoffFrequency()
{
    return m_filterCutoffFrequency;
}

void ARTrackable::setFilterCutoffFrequency(ARdouble freq)
{
    m_filterCutoffFrequency = freq;
    if (m_ftmi) arFilterTransMatSetParams(m_ftmi, m_filterSampleRate, m_filterCutoffFrequency);
}

