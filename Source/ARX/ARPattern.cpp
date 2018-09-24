/*
 *  ARPattern.cpp
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

#include <ARX/ARPattern.h>
#include <ARX/ARController.h>
#ifdef ARDOUBLE_IS_FLOAT
#  define _0_0 0.0f
#  define _1_0 1.0f
#else
#  define _0_0 0.0
#  define _1_0 1.0
#endif

ARPattern::ARPattern() :
	m_width(0.0f),
    m_height(0.0f),
    m_imageSizeX(0),
    m_imageSizeY(0),
	m_image(NULL),
    m_pattType(PATTERN_TYPE_NONE),
    m_patternID(-1)
{
    m_matrix[ 0] = _1_0;
    m_matrix[ 1] = _0_0;
    m_matrix[ 2] = _0_0;
    m_matrix[ 3] = _0_0;
    m_matrix[ 4] = _0_0;
    m_matrix[ 5] = _1_0;
    m_matrix[ 6] = _0_0;
    m_matrix[ 7] = _0_0;
    m_matrix[ 8] = _0_0;
    m_matrix[ 9] = _0_0;
    m_matrix[10] = _1_0;
    m_matrix[11] = _0_0;
    m_matrix[12] = _0_0;
    m_matrix[13] = _0_0;
    m_matrix[14] = _0_0;
    m_matrix[15] = _1_0;
}

ARPattern::~ARPattern()
{
	freeImage();
}

bool ARPattern::loadTemplate(int patternID, const ARPattHandle *arPattHandle, float width)
{
    if (!arPattHandle) return false;
    if (!arPattHandle->pattf[patternID]) return false;
    
    m_patternID = patternID;
    m_width = m_height = width;
    
    // Create the image.
    m_imageSizeX = m_imageSizeY = arPattHandle->pattSize;
    m_image = new uint32_t[m_imageSizeX * m_imageSizeY];
    
    const int *arr = arPattHandle->patt[m_patternID * 4];
    for (int y = 0; y < m_imageSizeY; y++) {
        for (int x = 0; x < m_imageSizeX; x++) {
            
            int pattIdx = (m_imageSizeY - 1 - y)*m_imageSizeX + x; // Flip pattern in Y.
            int buffIdx = y*m_imageSizeX + x;
            
            uint8_t *c = (uint8_t *)&m_image[buffIdx];
            *c++ = 255 - arr[pattIdx * 3 + 2];
            *c++ = 255 - arr[pattIdx * 3 + 1];
            *c++ = 255 - arr[pattIdx * 3 + 0];
            *c++ = 255;
        }
    }
    
	return true;
}

bool ARPattern::loadMatrix(int barcodeID, AR_MATRIX_CODE_TYPE type, float width)
{
    m_patternID = barcodeID;
    m_width = m_height = width;

    // TODO: implement matrix code to image.
    
    return true;
}

#if HAVE_NFT
bool ARPattern::loadISet(const AR2ImageSetT *imageSet, float nftScale)
{
    // TODO: implement imageset to image.
    if (imageSet && imageSet->scale) {
        AR2ImageT *image = imageSet->scale[0]; // Assume best scale (largest image) is first entry in array scale[index] (index is in range [0, imageSet->num - 1]).
        m_width = image->xsize * 25.4f / image->dpi * nftScale;
        m_height = image->ysize * 25.4f / image->dpi * nftScale;
    }
    return true;
}
#endif

#if HAVE_2D

bool ARPattern::load2DTrackerImage(std::shared_ptr<unsigned char> arImage, float imageX, float imageY, float width, float height)
{
    if (!arImage) return false;
    
    m_width = width;
    m_height = height;
    
    // Create the image.
    // Assuming greyscale only.
    // ifdef'ed out because it's not used at the moment, and it uses heaps of memory. TODO: just copy the shared_ptr, and generate the pattern on demand.
#if 0
    freeImage();
    m_imageSizeX = (int)imageX;
    m_imageSizeY = (int)imageY;
    m_image = new uint32_t[m_imageSizeX * m_imageSizeY];
    
    unsigned char* inData = arImage.get();
    for (int y = 0; y < m_imageSizeY; y++) {
        for (int x = 0; x < m_imageSizeX; x++) {
            int buffIdx = y*m_imageSizeX + x;
            uint8_t *c = (uint8_t *)&m_image[buffIdx];
            uint8_t pixVal = inData[y * m_imageSizeX + x];
            *c++ = pixVal;
            *c++ = pixVal;
            *c++ = pixVal;
            *c++ = 255;
            
        }
    }
#endif
    return true;
}
#endif

void ARPattern::freeImage()
{
	if (m_image) {
		delete[] m_image;
		m_image = NULL;
        m_imageSizeX = m_imageSizeY = 0;
	}
}
