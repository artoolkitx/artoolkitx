//
//  visual_database.h
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

#include <framework/image.h>
#include <framework/exception.h>
#include <detectors/DoG_scale_invariant_detector.h>
#include <matchers/keyframe.h>
#include <matchers/feature_matcher-inline.h>
#include <matchers/hough_similarity_voting.h>
#include <homography_estimation/robust_homography.h>
#include <utils/point.h>
#include <math/homography.h>
#include <math/indexing.h>

#include <vector>
#include <memory>
#include <unordered_map>

#include "feature_point.h"

#ifdef USE_OPENCV
#  include <opencv2/calib3d/calib3d.hpp>
#endif

namespace vision {

    /**
     * The visual database provides functionality to insert and query images.
     */
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    class VisualDatabase {
    public:
        
        typedef int id_t;
        
        typedef Keyframe<96> keyframe_t;
        typedef std::shared_ptr<keyframe_t> keyframe_ptr_t;
        typedef std::unordered_map<id_t, keyframe_ptr_t> keyframe_map_t;
        
        typedef BinomialPyramid32f pyramid_t;
        typedef DoGScaleInvariantDetector detector_t;
        
        VisualDatabase();
        ~VisualDatabase();
        
        /**
         * Add an image to the database with a specific ID.
         */
        void addImage(const Image& image, id_t id);
        
        /**
         * Add an image to the database with a specific ID.
         */
        void addImage(const GaussianScaleSpacePyramid* pyramid, id_t id);
        
        /**
         * Add a keyframe to the database.
         */
        void addKeyframe(keyframe_ptr_t keyframe , id_t id);
    
        /**
         * Query the visual database.
         */
        bool query(const Image& image);
        
        /**
         * Query the visual database.
         */
        bool query(const GaussianScaleSpacePyramid* pyramid);
        bool query(const keyframe_t* query_keyframe);
        
        /**
         * Erase an ID.
         */
        bool erase(id_t id);
        
        /**
         * @return Keyframe
         */
        const keyframe_ptr_t keyframe(id_t id) {
            typename keyframe_map_t::const_iterator it = mKeyframeMap.find(id);
            if(it != mKeyframeMap.end()) {
                return it->second;
            } else {
                return keyframe_ptr_t();
            }
        }
        
        /**
         * @return Query store
         */
        const keyframe_ptr_t queryKeyframe() const { return mQueryKeyframe; }
        
        const size_t databaseCount() const { return mKeyframeMap.size(); }
        
        /**
         * @return Matcher
         */
        const MATCHER& matcher() const { return mMatcher; }
        
        /**
         * @return Feature extractor
         */
        const FEATURE_EXTRACTOR& featureExtractor() const { return mFeatureExtractor; }
        
        /**
         * @return Inlier
         */
        const matches_t& inliers() const { return mMatchedInliers; }
        
        /**
         * Get the mathced id.
         */
        inline id_t matchedId() const { return mMatchedId; }
        
        /**
         * @return Matched geometry matrix
         */
        const float* matchedGeometry() const { return mMatchedGeometry; }
        
        /**
         * Get the detector.
         */
        inline detector_t& detector() { return mDetector; }
        inline const detector_t& detector() const { return mDetector; }
        
        /**
         * Set/Get minimum number of inliers.
         */
        inline void setMinNumInliers(size_t n) { mMinNumInliers = n; }
        inline size_t minNumInliers() const { return mMinNumInliers; }
        
    private:
        
        size_t mMinNumInliers;
        float mHomographyInlierThreshold;
        
        // Set to true if the feature index is enabled
        bool mUseFeatureIndex;
        
        matches_t mMatchedInliers;
        id_t mMatchedId;
        float mMatchedGeometry[9];
        
        keyframe_ptr_t mQueryKeyframe;
    
        // Map of keyframe
        keyframe_map_t mKeyframeMap;
        
        // Pyramid builder
        pyramid_t mPyramid;
        
        // Interest point detector (DoG, etc)
        detector_t mDetector;
        
        // Feature Extractor (FREAK, etc).
        FEATURE_EXTRACTOR mFeatureExtractor;
        
        // Feature matcher
        MATCHER mMatcher;
        
        // Similarity voter
        HoughSimilarityVoting mHoughSimilarityVoting;
        
        // Robust homography estimation
        RobustHomography<float> mRobustHomography;
        
    }; // VisualDatabase
    
    /**
     * Find feature points in an image.
     */
    template<typename FEATURE_EXTRACTOR, int NUM_BYTES_PER_FEATURE>
    void FindFeatures(Keyframe<NUM_BYTES_PER_FEATURE>* keyframe,
                      const GaussianScaleSpacePyramid* pyramid,
                      DoGScaleInvariantDetector* detector,
                      FEATURE_EXTRACTOR* extractor) {
        ASSERT(pyramid, "Pyramid is NULL");
        ASSERT(detector, "Detector is NULL");
        ASSERT(pyramid->images().size() > 0, "Pyramid is empty");
        ASSERT(pyramid->images()[0].width() == detector->width(), "Pyramid and detector size mismatch");
        ASSERT(pyramid->images()[0].height() == detector->height(), "Pyramid and detector size mismatch");
        
        //
        // Detect feature points
        //
        
        detector->detect(pyramid);
        
        //
        // Copy the points
        //
        
        std::vector<FeaturePoint> points(detector->features().size());
        for(size_t i = 0; i < detector->features().size(); i++) {
            const DoGScaleInvariantDetector::FeaturePoint& p = detector->features()[i];
            points[i] = FeaturePoint(p.x, p.y, p.angle, p.sigma, p.score > 0);
        }
        
        //
        // Extract features
        //
        
        extractor->extract(keyframe->store(), pyramid, points);
    }
    
    /**
     * Check if a homography is valid based on some heuristics.
     */
    inline bool CheckHomographyHeuristics(float H[9], int refWidth, int refHeight) {
        float p0p[2];
        float p1p[2];
        float p2p[2];
        float p3p[2];
        
        float Hinv[9];
        if(!MatrixInverse3x3<float>(Hinv, H, 1e-5)) {
            return false;
        }
        
        const float p0[] = {0, 0};
        const float p1[] = {(float)refWidth, 0};
        const float p2[] = {(float)refWidth, (float)refHeight};
        const float p3[] = {0, (float)refHeight};
        
        MultiplyPointHomographyInhomogenous(p0p, Hinv, p0);
        MultiplyPointHomographyInhomogenous(p1p, Hinv, p1);
        MultiplyPointHomographyInhomogenous(p2p, Hinv, p2);
        MultiplyPointHomographyInhomogenous(p3p, Hinv, p3);
        
        const float tr = refWidth*refHeight*0.0001;
        if(SmallestTriangleArea(p0p, p1p, p2p, p3p) < tr) {
            return false;
        }
        
        if(!QuadrilateralConvex(p0p, p1p, p2p, p3p)) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Vote for a similarity transformation.
     */
    inline int FindHoughSimilarity(HoughSimilarityVoting& hough,
                                   const std::vector<FeaturePoint>& p1,
                                   const std::vector<FeaturePoint>& p2,
                                   const matches_t& matches,
                                   int insWidth,
                                   int insHeigth,
                                   int refWidth,
                                   int refHeight) {
        std::vector<float> query(4*matches.size());
        std::vector<float> ref(4*matches.size());
        
        // Extract the data from the features
        for(size_t i = 0; i < matches.size(); i++) {
            const FeaturePoint& query_point = p1[matches[i].ins];
            const FeaturePoint& ref_point = p2[matches[i].ref];
            
            float* q = &query[i*4];
            q[0] = query_point.x;
            q[1] = query_point.y;
            q[2] = query_point.angle;
            q[3] = query_point.scale;
            
            float* r = &ref[i*4];
            r[0] = ref_point.x;
            r[1] = ref_point.y;
            r[2] = ref_point.angle;
            r[3] = ref_point.scale;
        }
        
        float dx = insWidth+(insWidth*0.2f);
        float dy = insHeigth+(insHeigth*0.2f);
        
        hough.init(-dx, dx, -dy, dy, 0, 0, 12, 10);        
        hough.setObjectCenterInReference(refWidth>>1, refHeight>>1);
        hough.setRefImageDimensions(refWidth, refHeight);
        hough.vote((float*)&query[0], (float*)&ref[0], (int)matches.size());

        float maxVotes;
        int maxIndex;
        hough.getMaximumNumberOfVotes(maxVotes, maxIndex);
        
        return (maxVotes < 3) ? -1 : maxIndex;
    }
    
    /**
     * Get only the matches that are consistent based on the hough votes.
     */
    inline void FindHoughMatches(matches_t& out_matches,
                                 const HoughSimilarityVoting& hough,
                                 const matches_t& in_matches,
                                 int binIndex,
                                 float binDelta) {
        
        float dx, dy, dangle, dscale;
        int bin_x, bin_y, bin_angle, bin_scale;
        hough.getBinsFromIndex(bin_x, bin_y, bin_angle, bin_scale, binIndex);
        
        out_matches.clear();
        
        int n = (int)hough.getSubBinLocationIndices().size();
        const float* vote_loc = hough.getSubBinLocations().data();
        
        ASSERT(n <= in_matches.size(), "Should be the same");
        
        for(int i = 0; i < n; i++, vote_loc+=4) {
            hough.getBinDistance(dx,            dy,            dangle,        dscale,
                                 vote_loc[0],   vote_loc[1],   vote_loc[2],   vote_loc[3],
                                 bin_x+.5,      bin_y+.5,      bin_angle+.5,  bin_scale+.5);
            
            if (dx < binDelta && dy < binDelta && dangle < binDelta && dscale < binDelta) {
                int idx = hough.getSubBinLocationIndices()[i];
                out_matches.push_back(in_matches[idx]);
            }
        }
    }
    
    /**
     * Estimate the homography between a set of correspondences.
     */
    inline bool EstimateHomography(float H[9],
                                   const std::vector<FeaturePoint>& p1,
                                   const std::vector<FeaturePoint>& p2,
                                   const matches_t& matches,
                                   RobustHomography<float>& estimator,
                                   int refWidth,
                                   int refHeight) {
        
        std::vector<vision::Point2d<float> > srcPoints(matches.size());
        std::vector<vision::Point2d<float> > dstPoints(matches.size());
        
        //
        // Copy correspondences
        //
        
        for(size_t i = 0; i < matches.size(); i++) {
            dstPoints[i].x = p1[matches[i].ins].x;
            dstPoints[i].y = p1[matches[i].ins].y;
            srcPoints[i].x = p2[matches[i].ref].x;
            srcPoints[i].y = p2[matches[i].ref].y;
        }
        
        //
        // Create test points for geometric verification
        //
        
        float test_points[8];
        test_points[0] = 0;
        test_points[1] = 0;
        test_points[2] = refWidth;
        test_points[3] = 0;
        test_points[4] = refWidth;
        test_points[5] = refHeight;
        test_points[6] = 0;
        test_points[7] = refHeight;
        
        //
        // Compute the homography
        //
        
        if(!estimator.find(H, (float*)&srcPoints[0], (float*)&dstPoints[0], (int)matches.size(), test_points, 4)) {
            return false;
        }
        
        //
        // Apply some heuristics to the homography
        //
        
        if(!CheckHomographyHeuristics(H, refWidth, refHeight)) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Find the inliers given a homography and a set of correspondences.
     */
    inline void FindInliers(matches_t& inliers,
                            const float H[9],
                            const std::vector<FeaturePoint>& p1,
                            const std::vector<FeaturePoint>& p2,
                            const matches_t& matches,
                            float threshold) {
        float threshold2 = sqr(threshold);
        inliers.reserve(matches.size());
        for(size_t i = 0; i < matches.size(); i++) {
            float xp[2];
            MultiplyPointHomographyInhomogenous(xp[0],
                                                xp[1],
                                                H,
                                                p2[matches[i].ref].x,
                                                p2[matches[i].ref].y);
            float d2 = sqr(xp[0]-p1[matches[i].ins].x) + sqr(xp[1]-p1[matches[i].ins].y);
            if(d2 <= threshold2) {
                inliers.push_back(matches[i]);
            }
        }
    }
    
} // vision
