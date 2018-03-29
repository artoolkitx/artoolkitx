//
//  feature_matcher-inline.h
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

#include "feature_matcher.h"
#include "feature_store.h"
#include <math/hamming.h>
#include <math/indexing.h>
#include <math/homography.h>
#include <math/linear_algebra.h>
#include <framework/error.h>
#include <utility>
namespace vision {

    /************************************************************************************************************************
     *
     * BinaryFeatureMatcher
     *
     ***********************************************************************************************************************/
    
    template<int FEATURE_SIZE>
    BinaryFeatureMatcher<FEATURE_SIZE>::BinaryFeatureMatcher()
    : mThreshold(0.7) {}

    template<int FEATURE_SIZE>
    BinaryFeatureMatcher<FEATURE_SIZE>::~BinaryFeatureMatcher() {}

    template<int FEATURE_SIZE>
    size_t BinaryFeatureMatcher<FEATURE_SIZE>::match(const BinaryFeatureStore* features1,
                                                     const BinaryFeatureStore* features2) {
        
        mMatches.clear();
        
        if(features1->size() == 0 ||
           features2->size() == 0) {
            return 0;
        }
        
        mMatches.reserve(features1->size());
        for(size_t i = 0; i < features1->size(); i++) {
            unsigned int first_best = std::numeric_limits<unsigned int>::max();
            unsigned int second_best = std::numeric_limits<unsigned int>::max();
            int best_index = std::numeric_limits<int>::max();
            
            // Search for 1st and 2nd best match
            const unsigned char* f1 = features1->feature(i);
            const FeaturePoint& p1 = features1->point(i);
            for(size_t j = 0; j < features2->size(); j++) {
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != features2->point(j).maxima) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance<FEATURE_SIZE>(f1,
                                                               (unsigned char*)features2->feature(j));
                if(d < first_best) {
                    second_best = first_best;
                    first_best = d;
                    best_index = (int)j;
                } else if(d < second_best) {
                    second_best = d;
                }
            }
            
            // Check if FIRST_BEST has been set
            if(first_best != std::numeric_limits<unsigned int>::max()) {
                // If there isn't a SECOND_BEST, then always choose the FIRST_BEST.
                // Otherwise, do a ratio test.
                if(second_best == std::numeric_limits<unsigned int>::max()) {
                    mMatches.push_back(match_t((int)i, best_index));
                } else {
                    // Ratio test
                    float r = (float)first_best / (float)second_best;
                    if(r < mThreshold) {
                        mMatches.push_back(match_t((int)i, best_index));
                    }
                }
            }
        }
        ASSERT(mMatches.size() <= features1->size(), "Number of matches should be lower");
        return mMatches.size();
    }
    
    template<int FEATURE_SIZE>
    size_t BinaryFeatureMatcher<FEATURE_SIZE>::match(const BinaryFeatureStore* features1,
                                                     const BinaryFeatureStore* features2,
                                                     const index_t& index2) {
        mMatches.clear();
        
        if(features1->size() == 0 ||
           features2->size() == 0) {
            return 0;
        }
        
        mMatches.reserve(features1->size());
        for(size_t i = 0; i < features1->size(); i++) {
            unsigned int first_best = std::numeric_limits<unsigned int>::max();
            unsigned int second_best = std::numeric_limits<unsigned int>::max();
            int best_index = std::numeric_limits<int>::max();
            
            // Perform an indexed nearest neighbor lookup
            const unsigned char* f1 = features1->feature(i);
            index2.query(f1);
            
            const FeaturePoint& p1 = features1->point(i);
            
            // Search for 1st and 2nd best match
            const std::vector<int>& v = index2.reverseIndex();
            for(size_t j = 0; j < v.size(); j++) {
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != features2->point(v[j]).maxima) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance<FEATURE_SIZE>(f1, (unsigned char*)features2->feature(v[j]));
                if(d < first_best) {
                    second_best = first_best;
                    first_best = d;
                    best_index = v[j];
                } else if(d < second_best) {
                    second_best = d;
                }
            }
            
            // Check if FIRST_BEST has been set
            if(first_best != std::numeric_limits<unsigned int>::max()) {
                ASSERT(best_index != std::numeric_limits<size_t>::max(), "Something strange");
                
                // If there isn't a SECOND_BEST, then always choose the FIRST_BEST.
                // Otherwise, do a ratio test.
                if(second_best == std::numeric_limits<unsigned int>::max()) {
                    mMatches.push_back(match_t((int)i, best_index));
                } else {
                    // Ratio test
                    float r = (float)first_best / (float)second_best;
                    if(r < mThreshold) {
                        mMatches.push_back(match_t((int)i, best_index));
                    }
                }
            }
        }
        ASSERT(mMatches.size() <= features1->size(), "Number of matches should be lower");
        return mMatches.size();
    }
    
    template<int FEATURE_SIZE>
    size_t BinaryFeatureMatcher<FEATURE_SIZE>::match(const BinaryFeatureStore* features1,
                                                     const BinaryFeatureStore* features2,
                                                     const float H[9],
                                                     float tr) {

        mMatches.clear();
        
        if(features1->size() == 0 ||
           features2->size() == 0) {
            return 0;
        }
        
        float tr_sqr = sqr(tr);
        
        float Hinv[9];
        if(!MatrixInverse3x3(Hinv, H, 0.f)) {
            ASSERT(0, "Failed to compute matrix inverse");
            return 0;
        }
        
        mMatches.reserve(features1->size());
        for(size_t i = 0; i < features1->size(); i++) {
            unsigned int first_best = std::numeric_limits<unsigned int>::max();
            unsigned int second_best = std::numeric_limits<unsigned int>::max();
            int best_index = std::numeric_limits<int>::max();
            
            const unsigned char* f1 = features1->feature(i);
            const FeaturePoint& p1 = features1->point(i);
            
            // Map p1 to p2 space through H
            float xp1, yp1;
            MultiplyPointHomographyInhomogenous(xp1, yp1, Hinv, p1.x, p1.y);
            
            // Search for 1st and 2nd best match
            for(size_t j = 0; j < features2->size(); j++) {
                const FeaturePoint& p2 = features2->point(j);
                
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != p2.maxima) {
                    continue;
                }
                
                // Check spatial constraint
                if(sqr(xp1-p2.x) + sqr(yp1-p2.y) > tr_sqr) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance768((unsigned int*)f1,
                                                    (unsigned int*)features2->feature(j));
                if(d < first_best) {
                    second_best = first_best;
                    first_best = d;
                    best_index = (int)j;
                } else if(d < second_best) {
                    second_best = d;
                }
            }
            
            // Check if FIRST_BEST has been set
            if(first_best != std::numeric_limits<unsigned int>::max()) {
                ASSERT(best_index != std::numeric_limits<size_t>::max(), "Something strange");
                
                // If there isn't a SECOND_BEST, then always choose the FIRST_BEST.
                // Otherwise, do a ratio test.
                if(second_best == std::numeric_limits<unsigned int>::max()) {
                    mMatches.push_back(match_t((int)i, best_index));
                } else {
                    // Ratio test
                    float r = (float)first_best / (float)second_best;
                    if(r < mThreshold) {
                        mMatches.push_back(match_t((int)i, best_index));
                    }
                }
            }
        }
        ASSERT(mMatches.size() <= features1->size(), "Number of matches should be lower");
        return mMatches.size();
    }
    
    /************************************************************************************************************************
     *
     * MutualCorrespondenceBinaryFeatureMatcher
     *
     ***********************************************************************************************************************/
    
    template<int FEATURE_SIZE>
    size_t MutualCorrespondenceBinaryFeatureMatcher<FEATURE_SIZE>::match(const BinaryFeatureStore* features1,
                                                                         const BinaryFeatureStore* features2,
                                                                         float tr) {
        mMatches.clear();
        
        if(features1->size() == 0 ||
           features2->size() == 0) {
            return 0;
        }
        
        float tr2 = sqr(tr);
        
        mMatches.reserve(features2->size());
        mIndices.resize(features1->size(), -1);
        
        // Assign features in "1" to best feature from "2"
        for(size_t i = 0; i < features1->size(); i++) {
            int best_index = -1;
            unsigned int best_d = std::numeric_limits<unsigned int>::max();
            const unsigned int* f1 = (unsigned int*)features1->feature(i);
            
            const FeaturePoint& p1 = features1->point(i);
            for(size_t j = 0; j < features2->size(); j++) {
                const FeaturePoint& p2 = features2->point(j);
                
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != p2.maxima) {
                    continue;
                }
                
                // Check spatial constraint
                if(sqr(p1.x-p2.x) + sqr(p1.y-p2.y) > tr2) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance768(f1,
                                                    (unsigned int*)features2->feature(j));
                if(d < best_d) {
                    best_d = d;
                    best_index = (int)j;
                }
            }
            mIndices[i] = best_index;
        }
        
        // Assign features in "2" to best feature from "1"
        for(size_t i = 0; i < features2->size(); i++) {
            int best_index = -1;
            unsigned int best_d = std::numeric_limits<unsigned int>::max();
            const unsigned int* f2 = (unsigned int*)features2->feature(i);
            
            const FeaturePoint& p2 = features2->point(i);
            for(size_t j = 0; j < features1->size(); j++) {
                const FeaturePoint& p1 = features1->point(j);
                
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != p2.maxima) {
                    continue;
                }
                
                // Check spatial constraint
                if(sqr(p1.x-p2.x) + sqr(p1.y-p2.y) > tr2) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance768((unsigned int*)features1->feature(j),
                                                    f2);
                if(d < best_d) {
                    best_d = d;
                    best_index = (int)j;
                }
            }
            
            if(best_index >= 0 && mIndices[best_index] == (int)i) {
                mMatches.push_back(match_t(best_index, (int)i));
            }
        }
        return mMatches.size();
    }

    template<int FEATURE_SIZE>
    size_t MutualCorrespondenceBinaryFeatureMatcher<FEATURE_SIZE>::match(const BinaryFeatureStore* features1,
                                                                         const BinaryFeatureStore* features2,
                                                                         const float H[9],
                                                                         float tr1,
                                                                         float tr2) {
        mMatches.clear();
        
        if(features1->size() == 0 ||
           features2->size() == 0) {
            return 0;
        }
        
        float Hinv[9];
        if(!MatrixInverse3x3(Hinv, H, 0.f)) {
            ASSERT(0, "Failed to compute matrix inverse");
            return 0;
        }
        
        float tr1_sqr = sqr(tr1);
        float tr2_sqr = sqr(tr2);
        
        mMatches.reserve(features2->size());
        mIndices.resize(features1->size(), -1);
        
        // Assign features in "1" to best feature from "2" within the spatial threshold
        for(size_t i = 0; i < features1->size(); i++) {
            int best_index = -1;
            unsigned int best_d = std::numeric_limits<unsigned int>::max();
            
            const FeaturePoint& p1 = features1->point(i);
            
            // Map p1 to p2 space through H
            float xp1, yp1;
            MultiplyPointHomographyInhomogenous(xp1, yp1, H, p1.x, p1.y);
            
            for(size_t j = 0; j < features2->size(); j++) {
                const FeaturePoint& p2 = features1->point(j);
                
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != p2.maxima) {
                    continue;
                }
                
                // Check spatial constraint
                if(sqr(xp1-p2.x) + sqr(yp1-p2.y) > tr2_sqr) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance768((unsigned int*)features1->feature(i),
                                                    (unsigned int*)features2->feature(j));
                if(d < best_d) {
                    best_d = d;
                    best_index = (int)j;
                }
            }
            mIndices[i] = best_index;
        }
        
        // Assign features in "2" to best feature from "1"
        for(size_t i = 0; i < features2->size(); i++) {
            int best_index = -1;
            unsigned int best_d = std::numeric_limits<unsigned int>::max();
            
            const FeaturePoint& p2 = features2->point(i);
            
            // Map p2 to p1 space through inv(H)
            float xp2, yp2;
            MultiplyPointHomographyInhomogenous(xp2, yp2, Hinv, p2.x, p2.y);
            
            for(size_t j = 0; j < features1->size(); j++) {
                const FeaturePoint& p1 = features1->point(j);
                
                // Both points should be a MINIMA or MAXIMA
                if(p1.maxima != p2.maxima) {
                    continue;
                }
                
                // Check spatial constraint
                if(sqr(xp2-p1.x) + sqr(yp2-p1.y) > tr1_sqr) {
                    continue;
                }
                
                ASSERT(FEATURE_SIZE == 96, "Only 96 bytes supported now");
                unsigned int d = HammingDistance768((unsigned int*)features1->feature(j),
                                                    (unsigned int*)features2->feature(i));
                if(d < best_d) {
                    best_d = d;
                    best_index = (int)j;
                }
            }
            
            if(best_index >= 0 && mIndices[best_index] == (int)i) {
                ASSERT(mIndices[best_index] >= 0, "Index not set correctly");
                mMatches.push_back(match_t(best_index, i));
            }
        }
        return mMatches.size();
    }
    
} // vision
