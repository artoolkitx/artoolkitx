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
#include <ARX/ARUtil/image_utils.h>


#if HAVE_2D

ARTrackable2d::ARTrackable2d() : ARTrackable(TwoD),
m_loaded(false),
m_twoDScale(1.0f),
pageNo(-1),
datasetPathname(NULL),
m_height(1.0f)
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
    
    allocatePatterns(1);
    patterns[0]->load2DTrackerImage(m_refImage, m_refImageX, m_refImageY, m_height*((float)m_refImageX)/((float)m_refImageY), m_height);
    
    allocatePatterns(1);
    m_loaded = true;
    
    return true;
}


bool ARTrackable2d::unload()
{
    if (m_loaded) {
        freePatterns();
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

bool ARTrackable2d::updateWithTwoDResults(int detectedPage, float trackingTrans[3][4], ARdouble transL2R[3][4])
{
    if (!m_loaded) return false;
    
    visiblePrev = visible;
    
    // The marker will only have a pageNo if the data has actually been loaded by a call to ARController::loadNFTData().
    if (pageNo >= 0 && pageNo == detectedPage) {
        visible = true;
        for (int j = 0; j < 3; j++) {
            trans[j][0] = (ARdouble)trackingTrans[j][0];
            trans[j][1] = (ARdouble)trackingTrans[j][1];
            trans[j][2] = (ARdouble)trackingTrans[j][2];
            trans[j][3] = (ARdouble)(trackingTrans[j][3]) * m_twoDScale;
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

#endif // HAVE_2D

