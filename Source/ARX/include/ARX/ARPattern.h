/*
 *  ARPattern.h
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

#ifndef ARPATTERN_H
#define ARPATTERN_H

#include <ARX/AR/ar.h>
#if HAVE_NFT
#  include <ARX/AR2/imageSet.h>
#endif
#include <memory>

/**
 * Encapsulates information about a sub-component of a marker useful for
 * presentation to a user. For a single square marker, there will be one
 * pattern per marker. For a multimarker (multiple square markers), there
 * will be one or more patterns per marker.
 *
 * Currently, the encapsulated information is the pose relative to the marker
 * origin, and (for template markers only) the visual appearance of the pattern.
 */
class ARPattern {

public:
    
    enum PATTERN_TYPE {
        PATTERN_TYPE_NONE = 0,
        PATTERN_TYPE_TEMPLATE,
        PATTERN_TYPE_MATRIX,
        PATTERN_TYPE_ISET
    };

	ARPattern();
	~ARPattern();

    bool loadTemplate(int patternID, const ARPattHandle *arPattHandle, float width);
    bool loadMatrix(int barcodeID, AR_MATRIX_CODE_TYPE type, float width);
#if HAVE_NFT
    bool loadISet(const AR2ImageSetT *imageSet, float nftScale);
#endif
    
#if HAVE_2D
    bool load2DTrackerImage(std::shared_ptr<unsigned char> arImage, float imageX, float imageY, float width, float height);
#endif
    
	ARdouble m_matrix[16];	///< Transform of the pattern from origin.
	float m_width;          ///< Width of the pattern in mm.
    float m_height;         ///< Height of the pattern in mm.
    int m_imageSizeX;
    int m_imageSizeY;
	uint32_t *m_image;      ///< RGBA32 image of the pattern.
    
private:

    PATTERN_TYPE m_pattType;

	bool loadImage(ARPattHandle *arPattHandle);
	void freeImage();
    
    // Template/matrix.
    int m_patternID;        ///< ID of the pattern (artoolkitX internal ID)

};

#endif // !ARPATTERN_H
