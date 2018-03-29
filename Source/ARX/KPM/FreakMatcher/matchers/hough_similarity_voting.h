//
//  hough_similarity_voting.h
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

#include <math/math_utils.h>
#include <math/homography.h>
#include <framework/error.h>

#include <vector>
#include <unordered_map>

namespace vision {

    /**
     * Hough voting for a similarity transformation based on a set of correspondences. 
     */
    class HoughSimilarityVoting
    {
    public:
        
        typedef std::unordered_map<unsigned int, unsigned int> hash_t;
        typedef std::pair<int /*size*/, int /*index*/> vote_t;
        typedef std::vector<vote_t> vote_vector_t;
        
        HoughSimilarityVoting();
        ~HoughSimilarityVoting();
        
        /**
         *
         */
        void init(float minX,
                  float maxX,
                  float minY,
                  float maxY,
                  int numXBins,
                  int numYBins,
                  int numAngleBins,
                  int numScaleBins);
        
        /**
         * The location of the center of the object in the reference image.
         */
        inline void setObjectCenterInReference(float x, float y) {
            mCenterX = x;
            mCenterY = y;
        }
        
        /**
         * Set the dimensions fo the reference image
         */
        inline void setRefImageDimensions(int width, int height) {
            mRefImageWidth = width;
            mRefImageHeight = height;
        }
        
        /**
         * Set the min/max of (x,y) for voting. Since we vote for the center of the
         * object. Sometimes the object center may be off the inspection image.
         */
        inline void setMinMaxXY(float minX, float maxX, float minY, float maxY) {
            mMinX = minX;
            mMaxX = maxX;
            mMinY = minY;
            mMaxY = maxY;
            mVotes.clear();
        }
        
        /**
         * Get the distance of two bin locations for each parameter.
         */
        inline void getBinDistance(float& distBinX,
                                   float& distBinY,
                                   float& distBinAngle,
                                   float& distBinScale,
                                   float insBinX,
                                   float insBinY,
                                   float insBinAngle,
                                   float insBinScale,
                                   float refBinX,
                                   float refBinY,
                                   float refBinAngle,
                                   float refBinScale) const;
        
        /**
         * Vote for the similarity transformation that maps the reference center to the inspection center.
         *
         * ins_features = S*ref_features where
         *
         * S = [scale*cos(angle),   -scale*sin(angle),  x;
         *      scale*sin(angle),   scale*cos(angle),   y;
         *      0,                  0,                  1];
         *
         * @param[in] x translation in x
         * @param[in] y translation in y
         * @param[in] angle (-pi,pi]
         * @param[in] scale
         */
        inline bool vote(float x, float y, float angle, float scale);
        void vote(const float* ins, const float* ref, int size);
        
        /**
         * Get the bins that have at least THRESHOLD number of votes.
         */
        void getVotes(vote_vector_t& votes, int threshold) const;
        
        /**
         * @return Sub-bin locations for each correspondence
         */
        inline const std::vector<float>& getSubBinLocations() const { return mSubBinLocations; }
        
        /**
         * @return Sub-bin indices for each correspondence
         */
        const std::vector<int>& getSubBinLocationIndices() const { return mSubBinLocationIndices; }
        
        /**
         * Get the bin that has the maximum number of votes
         */
        void getMaximumNumberOfVotes(float& maxVotes, int& maxIndex) const;
        
        /**
         * Map the similarity index to a transformation.
         */
        void getSimilarityFromIndex(float& x, float& y, float& angle, float& scale, int index) const;
        
        /**
         * Get an index from the discretized bin locations.
         */
        inline int getBinIndex(int binX, int binY, int binAngle, int binScale) const {
            int index;
            
            ASSERT(binX        >=  0, "binX out of range");
            ASSERT(binX        <   mNumXBins, "binX out of range");
            ASSERT(binY        >=  0, "binY out of range");
            ASSERT(binY        <   mNumYBins, "binY out of range");
            ASSERT(binAngle    >=  0, "binAngle out of range");
            ASSERT(binAngle    <   mNumAngleBins, "binAngle out of range");
            ASSERT(binScale    >=  0, "binScale out of range");
            ASSERT(binScale    <   mNumScaleBins, "binScale out of range");
            
            index =  binX + (binY*mNumXBins) + (binAngle*mA) + (binScale*mB);
            
            ASSERT(index <= (binX + binY*mNumXBins + binAngle*mNumXBins*mNumYBins + binScale*mNumXBins*mNumYBins*mNumAngleBins), "index out of range");
            
            return index;
        }
        
        /**
         * Get the bins locations from an index.
         */
        inline void getBinsFromIndex(int& binX, int& binY, int& binAngle, int& binScale, int index) const {
            binX        = ((index%mB)%mA)%mNumXBins;
            binY        = (((index-binX)%mB)%mA)/mNumXBins;
            binAngle    = ((index-binX-(binY*mNumXBins))%mB)/mA;
            binScale    = (index-binX-(binY*mNumXBins)-(binAngle*mA))/mB;
            
            ASSERT(binX        >=  0, "binX out of range");
            ASSERT(binX        <   mNumXBins, "binX out of range");
            ASSERT(binY        >=  0, "binY out of range");
            ASSERT(binY        <   mNumYBins, "binY out of range");
            ASSERT(binAngle    >=  0, "binAngle out of range");
            ASSERT(binAngle    <   mNumAngleBins, "binAngle out of range");
            ASSERT(binScale    >=  0, "binScale out of range");
            ASSERT(binScale    <   mNumScaleBins, "binScale out of range");
            
            // index = binX + (binY*mNumXBins) + (binAngle*A) + (binScale*B)
        }
        
        /**
         * Get the sub-bin location from the voting parameters.
         */
        inline void mapVoteToBin(float& fBinX,
                                 float& fBinY,
                                 float& fBinAngle,
                                 float& fBinScale,
                                 float x,
                                 float y,
                                 float angle,
                                 float scale) const;
        
        /**
         * Compute the similarity vote from a correspondence.
         *
         * @param[out] x
         * @param[out] y
         * @param[out] angle range (-pi,pi]
         * @param[out] scale exponential of the scale such that scale = log(s)/log(k)
         * @param[in] ins_x
         * @param[in] ins_y
         * @param[in] ins_angle
         * @param[in] ins_scale
         * @param[in] ref_x
         * @param[in] ref_y
         * @param[in] ref_angle
         * @param[in] ref_scale
         * @see voteWithCorrespondences for description
         */
        inline void mapCorrespondence(float& x,
                                      float& y,
                                      float& angle,
                                      float& scale,
                                      float ins_x,
                                      float ins_y,
                                      float ins_angle,
                                      float ins_scale,
                                      float ref_x,
                                      float ref_y,
                                      float ref_angle,
                                      float ref_scale) const;
        
    private:
    
        // Dimensions of reference image
        int mRefImageWidth;
        int mRefImageHeight;
        
        // Center of object in reference image
        float mCenterX;
        float mCenterY;
        
        // Set to true if the XY number of bins should be adjusted
        bool mAutoAdjustXYNumBins;
        
        // Min/Max (x,y,scale). The angle includes all angles (-pi,pi).
        float mMinX;
        float mMaxX;
        float mMinY;
        float mMaxY;
        float mMinScale;
        float mMaxScale;
        
        float mScaleK;
        float mScaleOneOverLogK;
        
        int mNumXBins;
        int mNumYBins;
        int mNumAngleBins;
        int mNumScaleBins;
        
        float mfBinX;
        float mfBinY;
        float mfBinAngle;
        float mfBinScale;
        
        int mA; // mNumXBins*mNumYBins
        int mB; // mNumXBins*mNumYBins*mNumAngleBins
        
        mutable hash_t mVotes;
        
        std::vector<float> mSubBinLocations;
        std::vector<int> mSubBinLocationIndices;
        
        /**
         * Cast a vote to an similarity index
         */
        inline void voteAtIndex(int index, unsigned int weight) {
            ASSERT(index >= 0, "index out of range");
            const hash_t::iterator it = mVotes.find(index);
            if(it == mVotes.end()) {
                mVotes.insert(std::pair<unsigned int, unsigned int>(index, weight));
            } else {
                it->second += weight;
            }
        }
        
        /**
         * Set the number of bins for translation based on the correspondences.
         */
        void autoAdjustXYNumBins(const float* ins, const float* ref, int size);
    };
    
    inline void HoughSimilarityVoting::getBinDistance(float& distBinX,
                                                      float& distBinY,
                                                      float& distBinAngle,
                                                      float& distBinScale,
                                                      float insBinX,
                                                      float insBinY,
                                                      float insBinAngle,
                                                      float insBinScale,
                                                      float refBinX,
                                                      float refBinY,
                                                      float refBinAngle,
                                                      float refBinScale) const {
        //
        // (x,y,scale)
        //
        
        distBinX = std::abs(insBinX - refBinX);
        distBinY = std::abs(insBinY - refBinY);
        distBinScale = std::abs(insBinScale - refBinScale);
        
        //
        // Angle
        //
        
        float d1 = std::abs(insBinAngle - refBinAngle);
        float d2 = (float)mNumAngleBins-d1;
        distBinAngle = min2<float>(d1, d2);
        
        ASSERT(distBinAngle >= 0, "distBinAngle must not be negative");
    }
    
    inline bool HoughSimilarityVoting::vote(float x, float y, float angle, float scale) {
        int binX;
        int binY;
        int binAngle;
        int binScale;
        
        int binXPlus1;
        int binYPlus1;
        int binAnglePlus1;
        int binScalePlus1;
        
        // Check that the vote is within range
        if(x        <  mMinX        ||
           x        >= mMaxX        ||
           y        <  mMinY        ||
           y        >= mMaxY        ||
           angle    <=  -PI         ||
           angle    >  PI           ||
           scale    <  mMinScale    ||
           scale    >= mMaxScale)
        {
            return false;
        }
        
        ASSERT(x       >= mMinX, "x out of range");
        ASSERT(x       <  mMaxX, "x out of range");
        ASSERT(y       >= mMinY, "y out of range");
        ASSERT(y       <  mMaxY, "y out of range");
        ASSERT(angle   > -PI, "angle out of range");
        ASSERT(angle   <= PI, "angle out of range");
        ASSERT(scale   >= mMinScale, "scale out of range");
        ASSERT(scale   <  mMaxScale, "scale out of range");
        
        // Compute the bin location
        mapVoteToBin(mfBinX, mfBinY, mfBinAngle, mfBinScale, x, y, angle, scale);
        binX        = std::floor(mfBinX-0.5f);
        binY        = std::floor(mfBinY-0.5f);
        binAngle    = std::floor(mfBinAngle-0.5f);
        binScale    = std::floor(mfBinScale-0.5f);
        
        binAngle    = (binAngle+mNumAngleBins)%mNumAngleBins;
        
        // Check that we can voting to all 16 bin locations 
        if(binX         <  0              ||
           (binX+1)     >= mNumXBins      ||
           binY         <  0              ||
           (binY+1)     >= mNumYBins      ||
           binScale     <  0              ||
           (binScale+1) >= mNumScaleBins) {
            return false;
        }
        
        binXPlus1     = binX+1;
        binYPlus1     = binY+1;
        binScalePlus1 = binScale+1;
        binAnglePlus1 = (binAngle+1)%mNumAngleBins;
        
        //
        // Cast the 16 votes
        //
        
        // bin location
        voteAtIndex(getBinIndex(binX, binY, binAngle, binScale), 1);
        
        // binX+1
        voteAtIndex(getBinIndex(binXPlus1, binY,      binAngle,      binScale),      1);
        voteAtIndex(getBinIndex(binXPlus1, binYPlus1, binAngle,      binScale),      1);
        voteAtIndex(getBinIndex(binXPlus1, binYPlus1, binAnglePlus1, binScale),      1);
        voteAtIndex(getBinIndex(binXPlus1, binYPlus1, binAnglePlus1, binScalePlus1), 1);
        voteAtIndex(getBinIndex(binXPlus1, binYPlus1, binAngle,      binScalePlus1), 1);
        voteAtIndex(getBinIndex(binXPlus1, binY,      binAnglePlus1, binScale),      1);
        voteAtIndex(getBinIndex(binXPlus1, binY,      binAnglePlus1, binScalePlus1), 1);
        voteAtIndex(getBinIndex(binXPlus1, binY,      binAngle,      binScalePlus1), 1);
        
        // binY+1
        voteAtIndex(getBinIndex(binX, binYPlus1, binAngle,        binScale),      1);
        voteAtIndex(getBinIndex(binX, binYPlus1, binAnglePlus1,   binScale),      1);
        voteAtIndex(getBinIndex(binX, binYPlus1, binAnglePlus1,   binScalePlus1), 1);
        voteAtIndex(getBinIndex(binX, binYPlus1, binAngle,        binScalePlus1), 1);
        
        // binAngle+1
        voteAtIndex(getBinIndex(binX, binY, binAnglePlus1, binScale),      1);
        voteAtIndex(getBinIndex(binX, binY, binAnglePlus1, binScalePlus1), 1);
        
        // binScale+1
        voteAtIndex(getBinIndex(binX, binY, binAngle, binScalePlus1), 1);
        
        return true;
    }
    
    inline void HoughSimilarityVoting::mapVoteToBin(float& fBinX,
                                                    float& fBinY,
                                                    float& fBinAngle,
                                                    float& fBinScale,
                                                    float x,
                                                    float y,
                                                    float angle,
                                                    float scale) const {
        fBinX       = mNumXBins*SafeDivision<float>(x-mMinX, mMaxX-mMinX);
        fBinY       = mNumYBins*SafeDivision<float>(y-mMinY, mMaxY-mMinY);
        fBinAngle   = mNumAngleBins*((angle+PI)*(1/(2*PI)));
        fBinScale   = mNumScaleBins*SafeDivision<float>(scale-mMinScale, mMaxScale-mMinScale);
    }
    
    inline void HoughSimilarityVoting::mapCorrespondence(float& x,
                                                         float& y,
                                                         float& angle,
                                                         float& scale,
                                                         float ins_x,
                                                         float ins_y,
                                                         float ins_angle,
                                                         float ins_scale,
                                                         float ref_x,
                                                         float ref_y,
                                                         float ref_angle,
                                                         float ref_scale) const {
        float S[4];
        float tp[2];
        float tx, ty;
        
        //
        // Angle
        //
        
        angle = ins_angle - ref_angle;
        // Map angle to (-pi,pi]
        if(angle <= -PI) {
            angle += (2*PI);
        }
        else if(angle > PI) {
            angle -= (2*PI);
        }
        ASSERT(angle > -PI, "angle out of range");
        ASSERT(angle <= PI, "angle out of range");
        
        //
        // Scale
        //
        
        scale = SafeDivision(ins_scale, ref_scale);
        Similarity2x2(S, angle, scale);
        
        scale = std::log(scale)*mScaleOneOverLogK;
        
        //
        // Position
        //
        
        tp[0] = S[0]*ref_x + S[1]*ref_y;
        tp[1] = S[2]*ref_x + S[3]*ref_y;
        
        tx = ins_x - tp[0];
        ty = ins_y - tp[1];
        
        x = S[0]*mCenterX + S[1]*mCenterY + tx;
        y = S[2]*mCenterX + S[3]*mCenterY + ty;
    }

} // vision