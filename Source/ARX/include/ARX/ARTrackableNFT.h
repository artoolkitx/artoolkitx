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

#ifndef ARMARKERNFT_H
#define ARMARKERNFT_H

#include <ARX/ARTrackable.h>
#if HAVE_NFT
#include <ARX/AR2/tracking.h>

/**
 * NFT marker type of ARTrackable.
 */
class ARTrackableNFT : public ARTrackable {

    friend class ARTrackerNFT;

private:
    bool m_loaded;
    float m_nftScale;
    bool robustFlag;                                    ///< Flag specifying which pose estimation approach to use
    int pageNo;
    char *datasetPathname;
    AR2SurfaceSetT *surfaceSet;
    bool unload();

public:
    
	ARTrackableNFT();
	~ARTrackableNFT();

	bool load(const char* dataSetPathname_in);

	bool updateWithNFTResults(int detectedPage, float trackingTrans[3][4], ARdouble transL2R[3][4] = NULL);

    void setNFTScale(const float scale);
    float NFTScale();
};

#endif // HAVE_NFT

#endif // !ARMARKERNFT_H
