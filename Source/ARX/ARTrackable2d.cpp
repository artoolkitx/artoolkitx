/*
 *  ARTrackable2d.cpp
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

#include <ARX/ARTrackable2d.h>
#if HAVE_2D
#include <ARX/ARUtil/image_utils.h>
#include <stdexcept>

ARTrackable2d::ARTrackable2d() : ARTrackable(TwoD),
m_loaded(false),
m_twoDScale(1000.0f),
pageNo(-1),
datasetPathname(NULL)
{
    ARTrackable::setFiltered(true);
}

ARTrackable2d::~ARTrackable2d()
{
    if (m_loaded) unload();
}

bool ARTrackable2d::load(const char* dataSetPathname_in)
{
    // Load data.
    ARLOGi("Loading image data %s.\n", dataSetPathname_in);
    std::shared_ptr<unsigned char> refImage;
    int refImageX, refImageY;
    try {
        int nc;
        if (!ReadImageFromFile(dataSetPathname_in, refImage, &refImageX, &refImageY, &nc, true)) {
            ARLOGi("Unable to load image '%s'.\n", dataSetPathname_in);
            return(false);
        }
    } catch (std::runtime_error) { // File not found.
        return (false);
    }
    return load2DData(dataSetPathname_in, refImage, refImageX, refImageY);
}

bool ARTrackable2d::load2DData(const char* dataSetPathname_in, std::shared_ptr<unsigned char> refImage, int refImageX, int refImageY)
{
    if (m_loaded) unload();
    
    visible = visiblePrev = false;
    
    m_refImage = refImage;
    m_refImageX = refImageX;
    m_refImageY = refImageY;

    datasetPathname = strdup(dataSetPathname_in);
    
    m_loaded = true;
    
    return true;
}


bool ARTrackable2d::unload()
{
    if (m_loaded) {
        pageNo = -1;
        m_refImage.reset();
        if (datasetPathname) {
            free(datasetPathname);
            datasetPathname = NULL;
        }
        
        m_loaded = false;
    }
    return true;
}

bool ARTrackable2d::updateWithTwoDResults(float trackingTrans[3][4], ARdouble transL2R[3][4])
{
    if (!m_loaded) return false;
    
    visiblePrev = visible;
    
    if (trackingTrans) {
        visible = true;
        for (int j = 0; j < 3; j++) {
            trans[j][0] =  (ARdouble)trackingTrans[j][0];
            trans[j][1] = -(ARdouble)trackingTrans[j][1];
            trans[j][2] = -(ARdouble)trackingTrans[j][2];
            trans[j][3] =  (ARdouble)(trackingTrans[j][3] * m_twoDScale * 0.001f * 1.64f );
        }
    } else visible = false;
    
    return (ARTrackable::update(transL2R)); // Parent class will finish update.
}

void ARTrackable2d::setTwoDScale(const float scale)
{
    m_twoDScale = scale;
}

float ARTrackable2d::TwoDScale()
{
    return (m_twoDScale);
}

int ARTrackable2d::getPatternCount()
{
    return 1;
}

std::pair<float, float> ARTrackable2d::getPatternSize(int patternIndex)
{
    if (patternIndex != 0) return std::pair<float, float>();
    return std::pair<float, float>(m_twoDScale, m_twoDScale/((float)m_refImageX / (float)m_refImageY));
}

std::pair<int, int> ARTrackable2d::getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    if (patternIndex != 0) return std::pair<int, int>();
    return std::pair<int, int>(m_refImageX, m_refImageY);
}

bool ARTrackable2d::getPatternTransform(int patternIndex, ARdouble T[16])
{
    if (patternIndex != 0) return false;
    T[ 0] = _1_0; T[ 1] = _0_0; T[ 2] = _0_0; T[ 3] = _0_0;
    T[ 4] = _0_0; T[ 5] = _1_0; T[ 6] = _0_0; T[ 7] = _0_0;
    T[ 8] = _0_0; T[ 9] = _0_0; T[10] = _1_0; T[11] = _0_0;
    T[12] = _0_0; T[13] = _0_0; T[14] = _0_0; T[15] = _1_0;
    return true;
}

bool ARTrackable2d::getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType)
{
    if (patternIndex != 0) return false;
    if (m_refImage == nullptr) return false;
    unsigned char *buff = m_refImage.get();
    for (int y = 0; y < m_refImageY; y++) {
        unsigned char *buffRow = buff + (m_refImageY - 1 - y)*m_refImageX; // Flip in y as output image has origin at lower-left.
        for (int x = 0; x < m_refImageX; x++) {
            unsigned char c = buffRow[x];
#ifdef AR_LITTLE_ENDIAN
            *pattImageBuffer++ = 0xff000000 | c << 16 | c << 8 | c;
#else
            *pattImageBuffer++ = c << 24 | c << 16 | c << 8 | 0xff;
#endif
        }
    }
    return true;
}

#endif // HAVE_2D

