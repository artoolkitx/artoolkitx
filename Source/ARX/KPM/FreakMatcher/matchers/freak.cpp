//
//  freak.cpp
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#include "freak.h"
#include <framework/error.h>
#include "freak84-inline.h"

using namespace vision;

FREAKExtractor::FREAKExtractor() {
    CopyVector(mPointRing0, freak84_points_ring0, 12);
    CopyVector(mPointRing1, freak84_points_ring1, 12);
    CopyVector(mPointRing2, freak84_points_ring2, 12);
    CopyVector(mPointRing3, freak84_points_ring3, 12);
    CopyVector(mPointRing4, freak84_points_ring4, 12);
    CopyVector(mPointRing5, freak84_points_ring5, 12);
    
    mSigmaCenter = freak84_sigma_center;
    mSigmaRing0  = freak84_sigma_ring0;
    mSigmaRing1  = freak84_sigma_ring1;
    mSigmaRing2  = freak84_sigma_ring2;
    mSigmaRing3  = freak84_sigma_ring3;
    mSigmaRing4  = freak84_sigma_ring4;
    mSigmaRing5  = freak84_sigma_ring5;
    
    mExpansionFactor = 7;
    
    ASSERT(sizeof(freak84_points_ring0) == 48, "Size should be 48 bytes");
    ASSERT(sizeof(freak84_points_ring1) == 48, "Size should be 48 bytes");
    ASSERT(sizeof(freak84_points_ring2) == 48, "Size should be 48 bytes");
    ASSERT(sizeof(freak84_points_ring3) == 48, "Size should be 48 bytes");
    ASSERT(sizeof(freak84_points_ring4) == 48, "Size should be 48 bytes");
    ASSERT(sizeof(freak84_points_ring5) == 48, "Size should be 48 bytes");
}

void FREAKExtractor::layout84(std::vector<receptor>& receptors,
                              std::vector<std::vector<int> >& tests) {
    const int ring_size = 6;
    const int num_rings = 6;

    const float radius_m = 4;
    const float radius_b = 2;
    
    const float sigma_m = 2;
    const float sigma_b = std::sqrt(2.0f);
    
    float max_radius = -1;
    float max_sigma = -1;
    
    float delta_theta = (2.f*PI)/ring_size;
    for(int i = 0; i < num_rings+1; i++) {
        float sigma = std::log(sigma_m*i+sigma_b);
        
        if(i == 0) {
            receptor r;
            r.x = 0;
            r.y = 0;
            r.s = sigma;
            
            receptors.push_back(r);
        } else {
            float radius = std::log(radius_m*i+radius_b);
            
            for(int j = 0; j < ring_size; j++) {
                
                float theta = j*delta_theta+i*PI/2.f;
                
                receptor r;
                r.x = radius*std::cos(theta);
                r.y = radius*std::sin(theta);
                r.s = sigma;
                
                receptors.push_back(r);
            }
            
            if(radius > max_radius) {
                max_radius = radius;
            }
        }
        
        if(sigma > max_sigma) {
            max_sigma = sigma;
        }
    }
    
    // Normalize
    for(size_t i = 0; i < receptors.size(); i++) {
        receptors[i].x /= max_radius;
        receptors[i].y /= max_radius;
        receptors[i].s /= max_sigma;
    }
    
    // Generate tests
    tests.resize(receptors.size());
    for(size_t i = 0; i < receptors.size(); i++) {
        for(size_t j = i+1; j < receptors.size(); j++) {
            tests[i].push_back((int)j);
        }
    }
}

void FREAKExtractor::extract(BinaryFeatureStore& store,
                             const GaussianScaleSpacePyramid* pyramid,
                             const std::vector<FeaturePoint>& points) {
#ifdef FREAK_DEBUG
    mMappedPoints0.clear();
    mMappedPoints1.clear();
    mMappedPoints2.clear();
    mMappedPoints3.clear();
    mMappedPoints4.clear();
    mMappedPoints5.clear();
    mMappedPointsC.clear();
    mMappedS0.clear();
    mMappedS1.clear();
    mMappedS2.clear();
    mMappedS3.clear();
    mMappedS4.clear();
    mMappedS5.clear();
    mMappedSC.clear();
#endif
    
    store.setNumBytesPerFeature(96);
    store.resize(points.size());
    ExtractFREAK84(store,
                   pyramid,
                   points,
                   mPointRing0,
                   mPointRing1,
                   mPointRing2,
                   mPointRing3,
                   mPointRing4,
                   mPointRing5,
                   mSigmaCenter,
                   mSigmaRing0,
                   mSigmaRing1,
                   mSigmaRing2,
                   mSigmaRing3,
                   mSigmaRing4,
                   mSigmaRing5,
                   mExpansionFactor
#ifdef FREAK_DEBUG
                   ,
                   mMappedPoints0,
                   mMappedPoints1,
                   mMappedPoints2,
                   mMappedPoints3,
                   mMappedPoints4,
                   mMappedPoints5,
                   mMappedPointsC,
                   mMappedS0,
                   mMappedS1,
                   mMappedS2,
                   mMappedS3,
                   mMappedS4,
                   mMappedS5,
                   mMappedSC
#endif
                   );
}