//
//  kmedoids.h
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

#include <limits>
#include <vector>
#include <math/rand.h>
#include <math/indexing.h>
#include <math/hamming.h>
#include <framework/error.h>

namespace vision {

    /**
     * Implements k-medoids for binary features.
     */
    template<int NUM_BYTES_PER_FEATURE>
    class BinarykMedoids {
    public:
        
        BinarykMedoids(int& rand_seed);
        BinarykMedoids(int& rand_seed, int k, int num_hypotheses);
        ~BinarykMedoids() {}
        
        /**
         * Set/Get number of clusters
         */
        inline void setk(int k) {
            mK = k;
            mCenters.resize(k);
        }
        inline int k() const { return mK; }
        
        /**
         * Set/Get number of hypotheses
         */
        inline void setNumHypotheses(int n) { mNumHypotheses = n; }
        inline int numHypotheses() const { return mNumHypotheses; }
        
        /**
         * Assign featurs to a cluster center
         */
        void assign(const unsigned char* features,
                    int num_features);
        void assign(const unsigned char* features,
                    int num_features,
                    const int* indices,
                    int num_indices);
        
        /**
         * @return Assignment vector
         */
        inline const std::vector<int>& assignment() const { return mAssignment; }

        /**
         * @return Centers
         */
        inline const std::vector<int>& centers() const { return mCenters; }
        
    private:
        
        // Seed for random numbers
        int& mRandSeed;
        
        // Number of cluster centers
        int mK;
        
        // Number of hypotheses to evaulate
        int mNumHypotheses;
        
        // Index of each cluster center
        std::vector<int> mCenters;
        
        // Assignment of each feature to a cluster center
        std::vector<int> mAssignment;
        std::vector<int> mHypAssignment;
        
        // Vector to store random indices
        std::vector<int> mRandIndices;

        /**
         * Assign features to the centers.
         */
        unsigned int assign(std::vector<int>& assignment,
                            const unsigned char* features,
                            int num_features,
                            const int* indices,
                            int num_indices,
                            const int* centers,
                            int num_centers);

    }; // BinarykMedoids

    template<int NUM_BYTES_PER_FEATURE>
    BinarykMedoids<NUM_BYTES_PER_FEATURE>::BinarykMedoids(int& rand_seed)
    : mRandSeed(rand_seed)
    , mK(0)
    , mNumHypotheses(0) { }
    
    template<int NUM_BYTES_PER_FEATURE>
    BinarykMedoids<NUM_BYTES_PER_FEATURE>::BinarykMedoids(int& rand_seed, int k, int num_hypotheses)
    : mRandSeed(rand_seed)
    , mNumHypotheses(num_hypotheses) {
        setk(k);
    }

    template<int NUM_BYTES_PER_FEATURE>
    void BinarykMedoids<NUM_BYTES_PER_FEATURE>::assign(const unsigned char* features,
                                                       int num_features) {
        std::vector<int> indices(num_features);
        for(size_t i = 0; i < indices.size(); i++) {
            indices[i] = (int)i;
        }
        assign(features, num_features, &indices[0], (int)indices.size());
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    void BinarykMedoids<NUM_BYTES_PER_FEATURE>::assign(const unsigned char* features,
                                                       int num_features,
                                                       const int* indices,
                                                       int num_indices) {
        ASSERT(mK == mCenters.size(), "k should match the number of cluster centers");
        ASSERT(num_features > 0, "Number of features must be positive");
        ASSERT(num_indices <= num_features, "More indices than features");
        ASSERT(num_indices >= mK, "Not enough features");
        
        mAssignment.resize(num_indices, -1);
        mHypAssignment.resize(num_indices, -1);
        mRandIndices.resize(num_indices);
        
        SequentialVector(&mRandIndices[0], (int)mRandIndices.size(), 0);
        
        unsigned int best_dist = std::numeric_limits<unsigned int>::max();
        
        for(int i = 0; i < mNumHypotheses; i++) {
            // Shuffle the first "k" indices
            ArrayShuffle(&mRandIndices[0], (int)mRandIndices.size(), mK, mRandSeed);
            
            // Assign features to the centers
            unsigned int dist = assign(mHypAssignment,
                                       features,
                                       num_features,
                                       indices,
                                       num_indices,
                                       &mRandIndices[0],
                                       mK);
            
            if(dist < best_dist) {
                // Move the best assignment
                mAssignment.swap(mHypAssignment);
                CopyVector(&mCenters[0], &mRandIndices[0], mK);
                best_dist = dist;
            }
        }
        ASSERT(mK == mCenters.size(), "k should match the number of cluster centers");
    }

    template<int NUM_BYTES_PER_FEATURE>
    unsigned int BinarykMedoids<NUM_BYTES_PER_FEATURE>::assign(std::vector<int>& assignment,
                                                               const unsigned char* features,
                                                               int num_features,
                                                               const int* indices,
                                                               int num_indices,
                                                               const int* centers,
                                                               int num_centers) {
        ASSERT(assignment.size() == num_indices, "Assignment size is incorrect");
        ASSERT(num_features > 0, "Number of features must be positive");
        ASSERT(num_indices <= num_features, "More indices than features");
        ASSERT(num_centers > 0, "There must be at least 1 center");
        
        unsigned int sum_dist = 0;
        
        for(int i = 0; i < num_indices; i++) {
            unsigned int best_dist = std::numeric_limits<unsigned int>::max();
            // Find the closest center
            for(int j = 0; j < num_centers; j++) {
                // Compute the distance from the center
                unsigned int dist = HammingDistance<NUM_BYTES_PER_FEATURE>(&features[NUM_BYTES_PER_FEATURE*indices[i]],
                                                                           &features[NUM_BYTES_PER_FEATURE*indices[centers[j]]]);
                if(dist < best_dist) {
                    assignment[i] = centers[j];
                    best_dist = dist;
                }
            }
            // Sum the BEST_DIST measures
            sum_dist += best_dist;
        }
        
        return sum_dist;
    }

} // vision