/*
 *  ARTrackableNFT.h
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
 *  Author(s): Philip Lamb
 *
 */

#ifndef ARMARKER2D_H
#define ARMARKER2D_H

#include <ARX/ARTrackable.h>
#if HAVE_2D
#include <memory>

/**
 * 2D marker type of ARTrackable.
 */
class ARTrackable2d : public ARTrackable {
    
    friend class ARTracker2d;
    
private:
    bool m_loaded;
    float m_twoDScale;
    bool robustFlag;                                    ///< Flag specifying which pose estimation approach to use
    int pageNo;                                         ///< "Page number" (first page is page 0), or -1 if 2D data not yet loaded into tracker. Not strictly necessary for 2D tracker, but useful when multiple 2D trackables are loaded.
    char *datasetPathname;
    int m_refImageX, m_refImageY;
    std::shared_ptr<unsigned char> m_refImage;

    bool unload();
public:
    
    ARTrackable2d();
    ~ARTrackable2d();

    /**
     Loads an image as a 2D dataset.
     The image can be any time loadble by ARUtil's ReadImageFromFile, which currently
     uses STBI internally and can therefore load JPEG, PNG, BMP, PSD, TGA, GIF, HDR,
     PIC and PNM files. Note that the 2D tracker uses only image luminance, therefore colour
     images will be down-converted to greyscale during loading.
     \param dataSetPathname_in Pathname to the image.
     */
    bool load(const char* dataSetPathname_in);
    /**
     Load a 2D dataset from user-supplied greyscale (luminance) image data.
     \param dataSetPathname_in Pathname to the image.
     \param refImage shared_ptr to tightly-packed greyscale image pixels.
     \param m_refImageX Dimension in X (i.e. width) of the image data.
     \param m_refImageY Dimension in X (i.e. height) of the image data.
     */
    bool load2DData(const char* dataSetPathname_in, std::shared_ptr<unsigned char> refImage, int m_refImageX, int m_refImageY);

    bool updateWithTwoDResults(float trackingTrans[3][4], ARdouble transL2R[3][4] = NULL);
    
    void setTwoDScale(const float scale);
    float TwoDScale();

    int getPatternCount() override;
    std::pair<float, float> getPatternSize(int patternIndex) override;
    std::pair<int, int> getPatternImageSize(int patternIndex, AR_MATRIX_CODE_TYPE matrixCodeType) override;
    bool getPatternTransform(int patternIndex, ARdouble T[16]) override;
    bool getPatternImage(int patternIndex, uint32_t *pattImageBuffer, AR_MATRIX_CODE_TYPE matrixCodeType) override;
};

#endif // HAVE_2D

#endif // !ARMARKERNFT_H

