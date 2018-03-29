//
//  feature_matcher.h
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

#pragma once

#include <vector>
#include <matchers/binary_hierarchical_clustering.h>
#include <matchers/matcher_types.h>

namespace vision {

    class BinaryFeatureStore;
    
    /**
     * Implements a matcher between two feature stores.
     */
    template<int FEATURE_SIZE>
    class BinaryFeatureMatcher {
    public:
        
        typedef BinaryHierarchicalClustering<FEATURE_SIZE> index_t;
        
        BinaryFeatureMatcher();
        ~BinaryFeatureMatcher();
        
        /**
         * Set the ratio threshold between the 1st and 2nd best matches.
         */
        void setThreshold(float tr) {
            mThreshold = tr;
        }
        
        /**
         * @return Get the threshold
         */
        float threshold() const {
            return mThreshold;
        }
        
        /**
         * Match two feature stores.
         * @return Number of matches
         */
        size_t match(const BinaryFeatureStore* features1,
                     const BinaryFeatureStore* features2);

        /**
         * Match two feature stores with an index on features2.
         * @return Number of matches
         */
        size_t match(const BinaryFeatureStore* features1,
                     const BinaryFeatureStore* features2,
                     const index_t& index2);
        
        /**
         * Match two feature stores given a homography from the features in store 1 to
         * store 2. The THRESHOLD is a spatial threshold in pixels to restrict the number
         * of feature comparisons.
         * @return Number of matches
         */
        size_t match(const BinaryFeatureStore* features1,
                     const BinaryFeatureStore* features2,
                     const float H[9],
                     float tr);
        
        /**
         * @return Vector of matches after a call to MATCH.
         */
        inline const matches_t& matches() const { return mMatches; }
        
    private:
        
        // Vector of indices that represent matches
        matches_t mMatches;
        
        // Threshold on the 1st and 2nd best matches
        float mThreshold;
        
    }; // BinaryFeatureMatcher
    
    /**
     * Implements a matcher between two feature stores based on mutual correspondence.
     */
    template<int FEATURE_SIZE>
    class MutualCorrespondenceBinaryFeatureMatcher {
    public:
        
        MutualCorrespondenceBinaryFeatureMatcher() {}
        ~MutualCorrespondenceBinaryFeatureMatcher() {}
        
        /**
         * Match two feature stores.
         * @return Number of matches
         */
        size_t match(const BinaryFeatureStore* features1,
                     const BinaryFeatureStore* features2,
                     float tr);
        
        /**
         * Match two feature stores given a homography from the features in store 1 to
         * store 2. The THRESHOLD is a spatial threshold in pixels to restrict the number
         * of feature comparisons.
         * @return Number of matches
         */
        size_t match(const BinaryFeatureStore* features1,
                     const BinaryFeatureStore* features2,
                     const float H[9],
                     float tr1,
                     float tr2);
        
        /**
         * @return Vector of matches after a call to MATCH.
         */
        inline const matches_t& matches() const { return mMatches; }
        
    private:
        
        // Vector of indices that represent matches
        matches_t mMatches;
        
        // Vector to hold indices
        std::vector<int> mIndices;
        
    }; // MutualCorrespondenceBinaryFeatureMatcher
    
} // vision
