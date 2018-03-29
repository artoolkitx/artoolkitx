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

#include <math/indexing.h>

#include <framework/timers.h>
#include <framework/logger.h>
#include <framework/image_utils.h>
#include <math/math_io.h>
#include <matchers/visual_database.h>


namespace vision {
    
    static const float kLaplacianThreshold = 3;
    static const float kEdgeThreshold = 4;
    static const size_t kMaxNumFeatures = 500;
    static const int kMinCoarseSize = 8;
    
    static const int kHomographyInlierThreshold = 3;
    static const int kMinNumInliers = 8;
    
    static const float kHoughBinDelta = 1;
    
    static const int kBytesPerFeature = 96;
    
    static const bool kUseFeatureIndex = true;
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::VisualDatabase() {
        mDetector.setLaplacianThreshold(kLaplacianThreshold);
        mDetector.setEdgeThreshold(kEdgeThreshold);
        mDetector.setMaxNumFeaturePoints(kMaxNumFeatures);
        
        mHomographyInlierThreshold = kHomographyInlierThreshold;
        mMinNumInliers = kMinNumInliers;
        
        mUseFeatureIndex = kUseFeatureIndex;
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::~VisualDatabase() {}
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    void VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::addImage(const vision::Image& image, id_t id) throw(Exception) {
        if(mKeyframeMap.find(id) != mKeyframeMap.end()) {
            throw EXCEPTION("ID already exists");
        }
        
        // Allocate pyramid
        if(mPyramid.images().size() == 0 ||
           mPyramid.images()[0].width() != image.width() ||
           mPyramid.images()[0].height() != image.height()) {
            int num_octaves = numOctaves((int)image.width(), (int)image.height(), kMinCoarseSize);
            mPyramid.alloc(image.width(), image.height(), num_octaves);
        }
        
        // Build the pyramid
        TIMED("Build Pyramid") {
            mPyramid.build(image);
        }
        
        // Add the image with a pyramid
        addImage(&mPyramid, id);
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    void VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::addImage(const GaussianScaleSpacePyramid* pyramid, id_t id) throw(Exception) {
        if(mKeyframeMap.find(id) != mKeyframeMap.end()) {
            throw EXCEPTION("ID already exists");
        }
        
        // Allocate detector
        if(mDetector.width() != pyramid->images()[0].width() ||
           mDetector.height() != pyramid->images()[0].height()) {
            mDetector.alloc(pyramid);
        }
        
        // Find the features on the image
        keyframe_ptr_t keyframe(new keyframe_t());
        keyframe->setWidth((int)pyramid->images()[0].width());
        keyframe->setHeight((int)pyramid->images()[0].height());
        TIMED("Extract Features") {
            FindFeatures<FEATURE_EXTRACTOR, kBytesPerFeature>(keyframe.get(), pyramid, &mDetector, &mFeatureExtractor);
        }
        LOG_INFO("Found %d features", keyframe->store().size());
        
        // Build the feature index
        TIMED("Build Index") {
            keyframe->buildIndex();
        }
        
        // Store the keyframe
        mKeyframeMap[id] = keyframe;
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    void VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::addKeyframe(keyframe_ptr_t keyframe , id_t id) throw(Exception) {
        typename keyframe_map_t::iterator it = mKeyframeMap.find(id);
        if(it != mKeyframeMap.end()) {
            throw EXCEPTION("ID already exists");
        }
        
        mKeyframeMap[id] = keyframe;
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    bool VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::query(const vision::Image& image) throw(Exception) {
        // Allocate pyramid
        if(mPyramid.images().size() == 0 ||
           mPyramid.images()[0].width() != image.width() ||
           mPyramid.images()[0].height() != image.height()) {
            int num_octaves = numOctaves((int)image.width(), (int)image.height(), kMinCoarseSize);
            mPyramid.alloc(image.width(), image.height(), num_octaves);
        }
        
        // Build the pyramid
        TIMED("Build Pyramid") {
            mPyramid.build(image);
        }
        
        return query(&mPyramid);
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    bool VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::query(const GaussianScaleSpacePyramid* pyramid) throw(Exception) {
        // Allocate detector
        if(mDetector.width() != pyramid->images()[0].width() ||
           mDetector.height() != pyramid->images()[0].height()) {
            mDetector.alloc(pyramid);
        }
        
        // Find the features on the image
        mQueryKeyframe.reset(new keyframe_t());
        mQueryKeyframe->setWidth((int)pyramid->images()[0].width());
        mQueryKeyframe->setHeight((int)pyramid->images()[0].height());
        TIMED("Extract Features") {
            FindFeatures<FEATURE_EXTRACTOR, kBytesPerFeature>(mQueryKeyframe.get(), pyramid, &mDetector, &mFeatureExtractor);
        }
        LOG_INFO("Found %d features in query", mQueryKeyframe->store().size());
        
        return query(mQueryKeyframe.get());
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    bool VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::query(const keyframe_t* query_keyframe) throw(Exception) {
        mMatchedInliers.clear();
        mMatchedId = -1;
        
        const std::vector<FeaturePoint>& query_points = query_keyframe->store().points();
        
        // Loop over all the images in the database
        typename keyframe_map_t::const_iterator it = mKeyframeMap.begin();
        for(; it != mKeyframeMap.end(); it++) {
            TIMED("Find Matches (1)") {
                if(mUseFeatureIndex) {
                    if(mMatcher.match(&query_keyframe->store(), &it->second->store(), it->second->index()) < mMinNumInliers) {
                        continue;
                    }
                } else {
                    if(mMatcher.match(&query_keyframe->store(), &it->second->store()) < mMinNumInliers) {
                        continue;
                    }
                }
            }
            
            const std::vector<FeaturePoint>& ref_points = it->second->store().points();
            //std::cout<<"ref_points-"<<ref_points.size()<<std::endl;
            //std::cout<<"query_points-"<<query_points.size()<<std::endl;
            
            //
            // Vote for a transformation based on the correspondences
            //
            
            int max_hough_index = -1;
            TIMED("Hough Voting (1)") {
                max_hough_index = FindHoughSimilarity(mHoughSimilarityVoting,
                                                      query_points,
                                                      ref_points,
                                                      mMatcher.matches(),
                                                      query_keyframe->width(),
                                                      query_keyframe->height(),
                                                      it->second->width(),
                                                      it->second->height());
                if(max_hough_index < 0) {
                    continue;
                }
            }
            
            matches_t hough_matches;
            TIMED("Find Hough Matches (1)") {
                FindHoughMatches(hough_matches,
                                 mHoughSimilarityVoting,
                                 mMatcher.matches(),
                                 max_hough_index,
                                 kHoughBinDelta);
            }
            
            //
            // Estimate the transformation between the two images
            //
            
            float H[9];
            TIMED("Estimate Homography (1)") {
                if(!EstimateHomography(H,
                                       query_points,
                                       ref_points,
                                       hough_matches,
                                       mRobustHomography,
                                       it->second->width(),
                                       it->second->height())) {
                    continue;
                }
            }
            
            //
            // Find the inliers
            //
            
            matches_t inliers;
            TIMED("Find Inliers (1)") {
                FindInliers(inliers, H, query_points, ref_points, hough_matches, mHomographyInlierThreshold);
                if(inliers.size() < mMinNumInliers) {
                    continue;
                }
            }
            
            //
            // Use the estimated homography to find more inliers
            //
            
            TIMED("Find Matches (2)") {
                if(mMatcher.match(&query_keyframe->store(),
                                  &it->second->store(),
                                  H,
                                  10) < mMinNumInliers) {
                    continue;
                }
            }
            
            //
            // Vote for a similarity with new matches
            //
            
            TIMED("Hough Voting (2)") {
                max_hough_index = FindHoughSimilarity(mHoughSimilarityVoting,
                                                      query_points,
                                                      ref_points,
                                                      mMatcher.matches(),
                                                      query_keyframe->width(),
                                                      query_keyframe->height(),
                                                      it->second->width(),
                                                      it->second->height());
                if(max_hough_index < 0) {
                    continue;
                }
            }
            
            TIMED("Find Hough Matches (2)") {
                FindHoughMatches(hough_matches,
                                 mHoughSimilarityVoting,
                                 mMatcher.matches(),
                                 max_hough_index,
                                 kHoughBinDelta);
            }
            
            //
            // Re-estimate the homography
            //
            
            TIMED("Estimate Homography (2)") {
                if(!EstimateHomography(H,
                                       query_points,
                                       ref_points,
                                       hough_matches,
                                       mRobustHomography,
                                       it->second->width(),
                                       it->second->height())) {
                    continue;
                }
            }
            
            //
            // Check if this is the best match based on number of inliers
            //
            
            inliers.clear();
            TIMED("Find Inliers (2)") {
                FindInliers(inliers, H, query_points, ref_points, hough_matches, mHomographyInlierThreshold);
            }
            
            //std::cout<<"inliers-"<<inliers.size()<<std::endl;
            if(inliers.size() >= mMinNumInliers && inliers.size() > mMatchedInliers.size()) {
                CopyVector9(mMatchedGeometry, H);
                mMatchedInliers.swap(inliers);
                mMatchedId = it->first;
            }
        }
        
        return mMatchedId >= 0;
    }
    
    template<typename FEATURE_EXTRACTOR, typename STORE, typename MATCHER>
    bool VisualDatabase<FEATURE_EXTRACTOR, STORE, MATCHER>::erase(id_t id) {
        typename keyframe_map_t::iterator it = mKeyframeMap.find(id);
        if(it == mKeyframeMap.end()) {
            return false;
        }
        mKeyframeMap.erase(it);
        return true;
    }
    
} // vision
