/*
 *  ARTrackableNFT.cpp
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

#include <ARX/ARTrackableNFT.h>


#if HAVE_NFT

ARTrackableNFT::ARTrackableNFT() : ARTrackable(NFT),
    m_loaded(false),
    m_nftScale(1.0f),
    pageNo(-1),
    datasetPathname(NULL)
{
}

ARTrackableNFT::~ARTrackableNFT()
{
	if (m_loaded) unload();
}

bool ARTrackableNFT::load(const char* dataSetPathname_in)
{
    if (m_loaded) unload();
    
	visible = visiblePrev = false;
	
    // Load AR2 data.
    ARLOGi("Loading '%s.fset'.\n", dataSetPathname_in);
    if ((surfaceSet = ar2ReadSurfaceSet(dataSetPathname_in, "fset", NULL)) == NULL) {
        ARLOGe("Error reading data from '%s.fset'.\n", dataSetPathname_in);
        return (false);
    }
 	datasetPathname = strdup(dataSetPathname_in);

    m_loaded = true;
    
	return true;
}

bool ARTrackableNFT::unload()
{
    if (m_loaded) {
        pageNo = -1;
        if (surfaceSet) {
            ARLOGi("Unloading '%s.fset'.\n", datasetPathname);
            ar2FreeSurfaceSet(&surfaceSet); // Sets surfaceSet to NULL.
        }
        if (datasetPathname) {
            free(datasetPathname);
            datasetPathname = NULL;
        }
        
        m_loaded = false;
    }
	return true;
}

bool ARTrackableNFT::updateWithNFTResults(int detectedPage, float trackingTrans[3][4], ARdouble transL2R[3][4])
{
    if (!m_loaded) return false;
    
	visiblePrev = visible;
    
    // The marker will only have a pageNo if the data has actually been loaded by a call to ARTrackerNFT::loadNFTData().
	if (pageNo >= 0 && pageNo == detectedPage) {
        visible = true;
        for (int j = 0; j < 3; j++) {
            trans[j][0] = (ARdouble)trackingTrans[j][0];
            trans[j][1] = (ARdouble)trackingTrans[j][1];
            trans[j][2] = (ARdouble)trackingTrans[j][2];
            trans[j][3] = (ARdouble)(trackingTrans[j][3] * m_nftScale);
        }
	} else visible = false;

	return (ARTrackable::update(transL2R)); // Parent class will finish update.
}

void ARTrackableNFT::setNFTScale(const float scale)
{
    m_nftScale = scale;
}

float ARTrackableNFT::NFTScale()
{
    return (m_nftScale);
}

int ARTrackableNFT::getPatternCount()
{
    if (!surfaceSet) return 0;
    return surfaceSet->num;
}

AR2ImageT *ARTrackableNFT::getBestImage(int patternIndex)
{
    if (!surfaceSet
        || patternIndex < 0 || patternIndex >= surfaceSet->num
        || !surfaceSet->surface
        || !surfaceSet->surface[patternIndex].imageSet
        || surfaceSet->surface[patternIndex].imageSet->num < 1
        || !surfaceSet->surface[patternIndex].imageSet->scale) {
        return nullptr;
    }
    return surfaceSet->surface[patternIndex].imageSet->scale[0]; // Assume best scale (largest image) is first entry in array scale[index] (index is in range [0, imageSet->num - 1]).
}

std::pair<float, float> ARTrackableNFT::getPatternSize(int patternIndex)
{
    AR2ImageT *image = getBestImage(patternIndex);
    if (!image) return std::pair<float, float>();

    float w = image->xsize * 25.4f / image->dpi * m_nftScale;
    float h = image->ysize * 25.4f / image->dpi * m_nftScale;
    return std::pair<float, float>(w, h);
}

std::pair<int, int> ARTrackableNFT::getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    AR2ImageT *image = getBestImage(patternIndex);
    if (!image) return std::pair<int, int>();

    return std::pair<int, int>(image->xsize, image->ysize);
}

bool ARTrackableNFT::getPatternTransform(int patternIndex, ARdouble T[16])
{
    if (!surfaceSet
        || patternIndex < 0 || patternIndex >= surfaceSet->num
        || !surfaceSet->surface) {
        return false;
    }
    T[ 0] = surfaceSet->surface[patternIndex].trans[0][0];
    T[ 1] = surfaceSet->surface[patternIndex].trans[1][0];
    T[ 2] = surfaceSet->surface[patternIndex].trans[2][0];
    T[ 3] = _0_0;
    T[ 4] = surfaceSet->surface[patternIndex].trans[0][1];
    T[ 5] = surfaceSet->surface[patternIndex].trans[1][1];
    T[ 6] = surfaceSet->surface[patternIndex].trans[2][1];
    T[ 7] = _0_0;
    T[ 8] = surfaceSet->surface[patternIndex].trans[0][2];
    T[ 9] = surfaceSet->surface[patternIndex].trans[1][2];
    T[10] = surfaceSet->surface[patternIndex].trans[2][2];
    T[11] = _0_0;
    T[12] = surfaceSet->surface[patternIndex].trans[0][3] * m_nftScale;
    T[13] = surfaceSet->surface[patternIndex].trans[1][3] * m_nftScale;
    T[14] = surfaceSet->surface[patternIndex].trans[2][3] * m_nftScale;
    T[15] = _1_0;
    return true;
}

bool ARTrackableNFT::getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    AR2ImageT *image = getBestImage(patternIndex);
    if (!image) return false;

    for (int y = 0; y < image->ysize; y++) {
        for (int x = 0; x < image->xsize; x++) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
            ARUint8 c = image->imgBWBlur[0][(image->ysize - 1 - y) * image->xsize + x]; // flip in y as output image has origin at lower-left.
#else
            ARUint8 c = image->imgBW[(image->ysize - 1 - y) * image->xsize + x]; // flip in y as output image has origin at lower-left.
#endif
#ifdef AR_LITTLE_ENDIAN
            *pattImageBuffer++ = 0xff000000 | c << 16 | c << 8 | c;
#else
            *pattImageBuffer++ = c << 24 | c << 16 | c << 8 | 0xff;
#endif
        }
    }
    return true;
}

#endif // HAVE_NFT
