//
//  hough_similarity_voting.cpp
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

#include "hough_similarity_voting.h"
#include <utils/partial_sort.h>

using namespace vision;

HoughSimilarityVoting::HoughSimilarityVoting()
: mRefImageWidth(0)
, mRefImageHeight(0)
, mCenterX(0)
, mCenterY(0)
, mAutoAdjustXYNumBins(true)
, mMinX(0)
, mMaxX(0)
, mMinY(0)
, mMaxY(0)
, mMinScale(0)
, mMaxScale(0)
, mScaleK(0)
, mScaleOneOverLogK(0)
, mNumXBins(0)
, mNumYBins(0)
, mNumAngleBins(0)
, mNumScaleBins(0)
, mfBinX(0)
, mfBinY(0)
, mfBinAngle(0)
, mfBinScale(0)
, mA(0)
, mB(0) {
}

HoughSimilarityVoting::~HoughSimilarityVoting() {}

void HoughSimilarityVoting::init(float minX,
                                 float maxX,
                                 float minY,
                                 float maxY,
                                 int numXBins,
                                 int numYBins,
                                 int numAngleBins,
                                 int numScaleBins) {
    mMinX       = minX;
    mMaxX       = maxX;
    mMinY       = minY;
    mMaxY       = maxY;
    mMinScale   = -1;
    mMaxScale   = 1;
    
    mNumXBins = numXBins;
    mNumYBins = numYBins;
    mNumAngleBins = numAngleBins;
    mNumScaleBins = numScaleBins;
    
    mA = mNumXBins*mNumYBins;
    mB = mNumXBins*mNumYBins*mNumAngleBins;
    
    mScaleK = 10;
    mScaleOneOverLogK = 1.f/std::log(mScaleK);
    
    // If the number of bins for (x,y) are not set, then we adjust the number of bins automatically.
    if(numXBins == 0 && numYBins == 0)
        mAutoAdjustXYNumBins = true;
    else
        mAutoAdjustXYNumBins = false;
    
    mVotes.clear();
}

void HoughSimilarityVoting::vote(const float* ins, const float* ref, int size) {
    float x, y, angle, scale;
    int num_features_that_cast_vote;
    
    mVotes.clear();
    if(size == 0) {
        return;
    }
    
    mSubBinLocations.resize(size*4);
    mSubBinLocationIndices.resize(size);
    if(mAutoAdjustXYNumBins) {
        autoAdjustXYNumBins(ins, ref, size);
    }
    
    num_features_that_cast_vote = 0;
    for(int i = 0; i < size; i++) {
        const float* ins_ptr = &ins[i<<2];
        const float* ref_ptr = &ref[i<<2];
        
        // Map the correspondence to a vote
        mapCorrespondence(x,
                          y,
                          angle,
                          scale,
                          ins_ptr[0],
                          ins_ptr[1],
                          ins_ptr[2],
                          ins_ptr[3],
                          ref_ptr[0],
                          ref_ptr[1],
                          ref_ptr[2],
                          ref_ptr[3]);
        
        // Cast a vote
        if(vote(x, y, angle, scale)) {
            float* ptr_bin = &mSubBinLocations[num_features_that_cast_vote<<2];
            ptr_bin[0] = mfBinX;
            ptr_bin[1] = mfBinY;
            ptr_bin[2] = mfBinAngle;
            ptr_bin[3] = mfBinScale;
            
            mSubBinLocationIndices[num_features_that_cast_vote] = i;
            num_features_that_cast_vote++;
        }
    }
    
    mSubBinLocations.resize(num_features_that_cast_vote*4);
    mSubBinLocationIndices.resize(num_features_that_cast_vote);
}

void HoughSimilarityVoting::getVotes(vote_vector_t& votes, int threshold) const {
    votes.clear();
    votes.reserve(mVotes.size());
    
    for(hash_t::const_iterator it = mVotes.begin(); it != mVotes.end(); it++) {
        if(it->second >= threshold) {
            votes.push_back(std::make_pair(it->second, it->first));
        }
    }
}

void HoughSimilarityVoting::getMaximumNumberOfVotes(float& maxVotes, int& maxIndex) const {
    maxVotes = 0;
    maxIndex = -1;
    
    for(hash_t::const_iterator it = mVotes.begin(); it != mVotes.end(); it++) {
        if(it->second > maxVotes) {
            maxIndex = it->first;
            maxVotes = it->second;
        }
    }
}

void HoughSimilarityVoting::getSimilarityFromIndex(float& x, float& y, float& angle, float& scale, int index) const {
    int binX;
    int binY;
    int binScale;
    int binAngle;
    float s;
    
    getBinsFromIndex(binX, binY, binAngle, binScale, index);
    
    x       = ((binX+0.5f)/mNumXBins)*(mMaxX-mMinX)+mMinX;
    y       = ((binY+0.5f)/mNumYBins)*(mMaxY-mMinY)+mMinY;
    angle   = (((binAngle+0.5f)/mNumAngleBins)*(2*PI))-PI;
    s       = ((binScale+0.5f)/mNumScaleBins)*(mMaxScale-mMinScale)+mMinScale;
    
    ASSERT(x       >= mMinX, "x out of range");
    ASSERT(x       <  mMaxX, "x out of range");
    ASSERT(y       >= mMinY, "y out of range");
    ASSERT(y       <  mMaxY, "y out of range");
    ASSERT(angle   > -PI, "angle out of range");
    ASSERT(angle   <= PI, "angle out of range");
    ASSERT(scale   >= mMinScale, "scale out of range");
    ASSERT(scale   <  mMaxScale, "scale out of range");
    
    scale   = std::pow(mScaleK, s);
}

void HoughSimilarityVoting::autoAdjustXYNumBins(const float* ins, const float* ref, int size) {
    int max_dim = max2<int>(mRefImageWidth, mRefImageHeight);
    std::vector<float> projected_dim(size);
    
    ASSERT(size > 0, "size must be positive");
    ASSERT(mRefImageWidth > 0, "width must be positive");
    ASSERT(mRefImageHeight > 0, "height must be positive");
    
    for(int i = 0; i < size; i++) {
        const float* ins_ptr = &ins[i<<2];
        const float* ref_ptr = &ref[i<<2];
        
        // Scale is the 3rd component
        float ins_scale = ins_ptr[3];
        float ref_scale = ref_ptr[3];

        // Project the max_dim via the scale
        float scale = SafeDivision(ins_scale, ref_scale);
        projected_dim[i] = scale*max_dim;
    }
    
    // Find the median projected dim
    float median_proj_dim = FastMedian<float>(&projected_dim[0], (int)projected_dim.size());
    
    // Compute the bin size a fraction of the median projected dim
    float bin_size = 0.25f*median_proj_dim;
    
    mNumXBins = max2<int>(5, std::ceil((mMaxX-mMinX)/bin_size));
    mNumYBins = max2<int>(5, std::ceil((mMaxY-mMinY)/bin_size));
    
    mA = mNumXBins*mNumYBins;
    mB = mNumXBins*mNumYBins*mNumAngleBins;
}