//
//  DoG_scale_invariant_detector.cpp
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

#include "DoG_scale_invariant_detector.h"
#include <framework/error.h>
#include <framework/timers.h>
#include <math/math_utils.h>
#include <math/linear_algebra.h>
#include <algorithm>
#include <functional>
#include "interpolate.h"

using namespace vision;

DoGPyramid::DoGPyramid()
: mNumOctaves(0)
, mNumScalesPerOctave(0)
{}

void DoGPyramid::alloc(const GaussianScaleSpacePyramid* pyramid) {
    ASSERT(pyramid->size() > 0, "Pyramid is not allocated");
   
    ImageType type = pyramid->get(0, 0).type();
    size_t width = pyramid->get(0, 0).width();
    size_t height = pyramid->get(0, 0).height();
    
    mNumOctaves = pyramid->numOctaves();;
    mNumScalesPerOctave = pyramid->numScalesPerOctave()-1;
    
    // Allocate DoG images
    mImages.resize(mNumOctaves*mNumScalesPerOctave);
    for(size_t i = 0; i < mNumOctaves; i++) {
        for(size_t j = 0; j < mNumScalesPerOctave; j++) {
            mImages[i*mNumScalesPerOctave+j].alloc(type, width>>i, height>>i, AUTO_STEP, 1);
        }
    }
}

void DoGPyramid::compute(const GaussianScaleSpacePyramid* pyramid) {
    ASSERT(mImages.size() > 0, "Laplacian pyramid has not been allocated");
    ASSERT(pyramid->numOctaves() > 0, "Pyramid does not contain any levels");
    ASSERT(dynamic_cast<const BinomialPyramid32f*>(pyramid), "Only binomial pyramid is supported");
    
    for(size_t i = 0; i < mNumOctaves; i++) {
        for(size_t j = 0; j < mNumScalesPerOctave; j++) {
            difference_image_binomial(get(i, j),
                                      pyramid->get(i, j),
                                      pyramid->get(i, j+1));
        }
    }
}

void DoGPyramid::difference_image_binomial(Image& d, const Image& im1, const Image& im2) {
    ASSERT(d.type() == IMAGE_F32, "Only F32 images supported");
    ASSERT(im1.type() == IMAGE_F32, "Only F32 images supported");
    ASSERT(im2.type() == IMAGE_F32, "Only F32 images supported");
    ASSERT(d.channels() == 1, "Only single channel images supported");
    ASSERT(im1.channels() == 1, "Only single channel images supported");
    ASSERT(im2.channels() == 1, "Only single channel images supported");
    ASSERT(d.width() == im2.width(), "Images must have the same width");
    ASSERT(d.height() == im2.height(), "Images must have the same height");
    ASSERT(im1.width() == im2.width(), "Images must have the same width");
    ASSERT(im1.height() == im2.height(), "Images must have the same height");
    
    // Compute diff
    for(size_t i = 0; i < im1.height(); i++) {
        float* p0 = d.get<float>(i);
        const float* p1 = im1.get<float>(i);
        const float* p2 = im2.get<float>(i);
        for(size_t j = 0; j < im1.width(); j++) {
            p0[j] = p1[j]-p2[j];
        }
    }
}

DoGScaleInvariantDetector::DoGScaleInvariantDetector()
: mWidth(0)
, mHeight(0)
, mNumBucketsX(10)
, mNumBucketsY(10)
, mFindOrientation(true)
, mLaplacianThreshold(0)
, mEdgeThreshold(10)
, mMaxSubpixelDistanceSqr(3*3) {
    setMaxNumFeaturePoints(kMaxNumFeaturePoints);
    mOrientations.resize(kMaxNumOrientations);
}

DoGScaleInvariantDetector::~DoGScaleInvariantDetector() {}

void DoGScaleInvariantDetector::alloc(const GaussianScaleSpacePyramid* pyramid) {
    mLaplacianPyramid.alloc(pyramid);
    
    mOrientationAssignment.alloc(pyramid->images()[0].width(),
                                 pyramid->images()[0].height(),
                                 pyramid->numOctaves(),
                                 pyramid->numScalesPerOctave(),
                                 kMaxNumOrientations,
                                 3,
                                 1.5,
                                 5,
                                 0.8);
    
    mWidth = pyramid->images()[0].width();
    mHeight = pyramid->images()[0].height();
    
    // Allocate bucket container
    mBuckets.resize(mNumBucketsX);
    for(size_t i = 0; i < mBuckets.size(); i++) {
        mBuckets[i].resize(mNumBucketsY);
    }
}

void DoGScaleInvariantDetector::detect(const GaussianScaleSpacePyramid* pyramid) {
    ASSERT(pyramid->numOctaves() > 0, "Pyramid does not contain any levels");
    
    // Compute Laplacian images (DoG)
    TIMED("DoG Pyramid") {
        mLaplacianPyramid.compute(pyramid);
    }
    
    // Detect minima and maximum in Laplacian images
    TIMED("Non-max suppression") {
        extractFeatures(pyramid, &mLaplacianPyramid);
    }
    
    // Sub-pixel refinement
    TIMED("Subpixel") {
        findSubpixelLocations(pyramid);
    }
    
    // Prune features
    TIMED("pruneFeatures") {
        pruneFeatures();
    }
    
    // Compute dominant angles
    TIMED("Find Orientations") {
        findFeatureOrientations(pyramid);
    }
}

void DoGScaleInvariantDetector::extractFeatures(const GaussianScaleSpacePyramid* pyramid,
                                                const DoGPyramid* laplacian) {
    
    // Clear old features
    mFeaturePoints.clear();
    
    float laplacianSqrThreshold = sqr(mLaplacianThreshold);
    
    for(size_t i = 1; i < mLaplacianPyramid.size()-1; i++) {
        const Image& im0 = laplacian->get(i-1);
        const Image& im1 = laplacian->get(i);
        const Image& im2 = laplacian->get(i+1);
        
        int octave = laplacian->octaveFromIndex((int)i);
        int scale = laplacian->scaleFromIndex((int)i);
        
        if(im0.width() == im1.width() && im0.width() == im2.width()) { // All images are the same size
            ASSERT(im0.height() == im1.height(), "Height is inconsistent");
            ASSERT(im0.height() == im2.height(), "Height is inconsistent");
            
            size_t width_minus_1 = im1.width() - 1;
            size_t heigh_minus_1 = im1.height() - 1;
            
            for(size_t row = 1; row < heigh_minus_1; row++) {
                const float* im0_ym1 = im0.get<float>(row-1);
                const float* im0_y   = im0.get<float>(row);
                const float* im0_yp1 = im0.get<float>(row+1);
                
                const float* im1_ym1 = im1.get<float>(row-1);
                const float* im1_y   = im1.get<float>(row);
                const float* im1_yp1 = im1.get<float>(row+1);
                
                const float* im2_ym1 = im2.get<float>(row-1);
                const float* im2_y   = im2.get<float>(row);
                const float* im2_yp1 = im2.get<float>(row+1);
                
                for(size_t col = 1; col < width_minus_1; col++) {
                    const float& value = im1_y[col];
                    FeaturePoint fp;
                    
                    // Check laplacian score
                    if(sqr(value) < laplacianSqrThreshold) {
                        continue;
                    }
                    
#define NONMAX_CHECK(OPERATOR, VALUE)                  \
                    /* im0 - 9 evaluations */          \
                    VALUE OPERATOR im0_ym1[col-1]   && \
                    VALUE OPERATOR im0_ym1[col]     && \
                    VALUE OPERATOR im0_ym1[col+1]   && \
                    VALUE OPERATOR im0_y[col-1]     && \
                    VALUE OPERATOR im0_y[col]       && \
                    VALUE OPERATOR im0_y[col+1]     && \
                    VALUE OPERATOR im0_yp1[col-1]   && \
                    VALUE OPERATOR im0_yp1[col]     && \
                    VALUE OPERATOR im0_yp1[col+1]   && \
                    /* im1 - 8 evaluations */          \
                    VALUE OPERATOR im1_ym1[col-1]   && \
                    VALUE OPERATOR im1_ym1[col]     && \
                    VALUE OPERATOR im1_ym1[col+1]   && \
                    VALUE OPERATOR im1_y[col-1]     && \
                    VALUE OPERATOR im1_y[col+1]     && \
                    VALUE OPERATOR im1_yp1[col-1]   && \
                    VALUE OPERATOR im1_yp1[col]     && \
                    VALUE OPERATOR im1_yp1[col+1]   && \
                    /* im2 - 9 evaluations */          \
                    VALUE OPERATOR im2_ym1[col-1]   && \
                    VALUE OPERATOR im2_ym1[col]     && \
                    VALUE OPERATOR im2_ym1[col+1]   && \
                    VALUE OPERATOR im2_y[col-1]     && \
                    VALUE OPERATOR im2_y[col]       && \
                    VALUE OPERATOR im2_y[col+1]     && \
                    VALUE OPERATOR im2_yp1[col-1]   && \
                    VALUE OPERATOR im2_yp1[col]     && \
                    VALUE OPERATOR im2_yp1[col+1]
                    
                    bool extrema = false;
                    if(NONMAX_CHECK(>, value)) { // strictly greater than
                        extrema = true;
                    } else if(NONMAX_CHECK(<, value)) { // strictly less than
                        extrema = true;
                    }
                    
                    if(extrema) {
                        fp.octave = octave;
                        fp.scale  = scale;
                        fp.score  = value;
                        fp.sigma  = pyramid->effectiveSigma(octave, scale);
                        
                        bilinear_upsample_point(fp.x,
                                                fp.y,
                                                col,
                                                row,
                                                octave);
                        
                        mFeaturePoints.push_back(fp);
                    }
                    
#undef NONMAX_CHECK
                }
            }
        } else if(im0.width() == im1.width() && (im1.width()>>1) == im2.width()) { // 0,1 are the same size, 2 is half size
            ASSERT(im0.height() == im1.height(), "Height is inconsistent");
            ASSERT((im1.height()>>1) == im2.height(), "Height is inconsistent");
    
            size_t end_x = std::floor(((im2.width()-1)-0.5f)*2.f+0.5f);
            size_t end_y = std::floor(((im2.height()-1)-0.5f)*2.f+0.5f);

            for(size_t row = 2; row < end_y; row++) {
                const float* im0_ym1 = im0.get<float>(row-1);
                const float* im0_y   = im0.get<float>(row);
                const float* im0_yp1 = im0.get<float>(row+1);
                
                const float* im1_ym1 = im1.get<float>(row-1);
                const float* im1_y   = im1.get<float>(row);
                const float* im1_yp1 = im1.get<float>(row+1);

                for(size_t col = 2; col < end_x; col++) {
                    const float& value = im1_y[col];
                    FeaturePoint fp;
                    
                    // Check laplacian score
                    if(sqr(value) < laplacianSqrThreshold) {
                        continue;
                    }
                    
                    // Compute downsampled point location
                    float ds_x = col*0.5f-0.25f;
                    float ds_y = row*0.5f-0.25f;
                                        
#define NONMAX_CHECK(OPERATOR, VALUE)                  \
                    /* im0 - 9 evaluations */          \
                    VALUE OPERATOR im0_ym1[col-1]   && \
                    VALUE OPERATOR im0_ym1[col]     && \
                    VALUE OPERATOR im0_ym1[col+1]   && \
                    VALUE OPERATOR im0_y[col-1]     && \
                    VALUE OPERATOR im0_y[col]       && \
                    VALUE OPERATOR im0_y[col+1]     && \
                    VALUE OPERATOR im0_yp1[col-1]   && \
                    VALUE OPERATOR im0_yp1[col]     && \
                    VALUE OPERATOR im0_yp1[col+1]   && \
                    /* im1 - 8 evaluations */          \
                    VALUE OPERATOR im1_ym1[col-1]   && \
                    VALUE OPERATOR im1_ym1[col]     && \
                    VALUE OPERATOR im1_ym1[col+1]   && \
                    VALUE OPERATOR im1_y[col-1]     && \
                    VALUE OPERATOR im1_y[col+1]     && \
                    VALUE OPERATOR im1_yp1[col-1]   && \
                    VALUE OPERATOR im1_yp1[col]     && \
                    VALUE OPERATOR im1_yp1[col+1]   && \
                    /* im2 - 9 evaluations */          \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x-0.5f, ds_y-0.5f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x,      ds_y-0.5f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x+0.5f, ds_y-0.5f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x-0.5f, ds_y)        && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x,      ds_y)        && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x+0.5f, ds_y)        && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x-0.5f, ds_y+0.5f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x,      ds_y+0.5f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im2, ds_x+0.5f, ds_y+0.5f)

                    bool extrema = false;
                    if(NONMAX_CHECK(>, value)) { // strictly greater than
                        extrema = true;
                    } else if(NONMAX_CHECK(<, value)) { // strictly less than
                        extrema = true;
                    }
                    
                    if(extrema) {
                        fp.octave = octave;
                        fp.scale  = scale;
                        fp.score  = value;
                        fp.sigma  = pyramid->effectiveSigma(octave, scale);
                        
                        bilinear_upsample_point(fp.x,
                                                fp.y,
                                                col,
                                                row,
                                                octave);
                        
                        mFeaturePoints.push_back(fp);
                    }
                    
#undef NONMAX_CHECK
                }
            }
        } else if((im0.width()>>1) == im1.width() && (im0.width()>>1) == im2.width()) { // 0 is twice the size of 1 and 2
            ASSERT((im0.height()>>1) == im1.height(), "Height is inconsistent");
            ASSERT((im0.height()>>1) == im2.height(), "Height is inconsistent");
            
            size_t width_minus_1 = im1.width() - 1;
            size_t height_minus_1 = im1.height() - 1;
            
            for(size_t row = 1; row < height_minus_1; row++) {
                const float* im1_ym1 = im1.get<float>(row-1);
                const float* im1_y   = im1.get<float>(row);
                const float* im1_yp1 = im1.get<float>(row+1);
                
                const float* im2_ym1 = im2.get<float>(row-1);
                const float* im2_y   = im2.get<float>(row);
                const float* im2_yp1 = im2.get<float>(row+1);
                
                for(size_t col = 1; col < width_minus_1; col++) {
                    const float& value = im1_y[col];
                    FeaturePoint fp;
                    
                    // Check laplacian score
                    if(sqr(value) < laplacianSqrThreshold) {
                        continue;
                    }
                    
                    float us_x = (col<<1)+0.5f;
                    float us_y = (row<<1)+0.5f;
                    
#define NONMAX_CHECK(OPERATOR, VALUE)                  \
                    /* im1 - 8 evaluations */          \
                    VALUE OPERATOR im1_ym1[col-1]   && \
                    VALUE OPERATOR im1_ym1[col]     && \
                    VALUE OPERATOR im1_ym1[col+1]   && \
                    VALUE OPERATOR im1_y[col-1]     && \
                    VALUE OPERATOR im1_y[col+1]     && \
                    VALUE OPERATOR im1_yp1[col-1]   && \
                    VALUE OPERATOR im1_yp1[col]     && \
                    VALUE OPERATOR im1_yp1[col+1]   && \
                    /* im2 - 9 evaluations */          \
                    VALUE OPERATOR im2_ym1[col-1]   && \
                    VALUE OPERATOR im2_ym1[col]     && \
                    VALUE OPERATOR im2_ym1[col+1]   && \
                    VALUE OPERATOR im2_y[col-1]     && \
                    VALUE OPERATOR im2_y[col]       && \
                    VALUE OPERATOR im2_y[col+1]     && \
                    VALUE OPERATOR im2_yp1[col-1]   && \
                    VALUE OPERATOR im2_yp1[col]     && \
                    VALUE OPERATOR im2_yp1[col+1]   && \
                    /* im2 - 9 evaluations */          \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x-2.f, us_y-2.f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x,     us_y-2.f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x+2.f, us_y-2.f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x-2.f, us_y)       && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x,     us_y)       && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x+2.f, us_y)       && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x-2.f, us_y+2.f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x,     us_y+2.f)   && \
                    VALUE OPERATOR bilinear_interpolation<float>(im0, us_x+2.f, us_y+2.f)
                    
                    bool extrema = false;
                    if(NONMAX_CHECK(>, value)) { // strictly greater than
                        extrema = true;
                    } else if(NONMAX_CHECK(<, value)) { // strictly less than
                        extrema = true;
                    }
                    
                    if(extrema) {
                        fp.octave = octave;
                        fp.scale  = scale;
                        fp.score  = value;
                        fp.sigma  = pyramid->effectiveSigma(octave, scale);
                        
                        bilinear_upsample_point(fp.x,
                                                fp.y,
                                                col,
                                                row,
                                                octave);
                        
                        mFeaturePoints.push_back(fp);
                    }
                    
#undef NONMAX_CHECK
                }
            }
        }
    }
}

void DoGScaleInvariantDetector::pruneFeatures() {
    if(mFeaturePoints.size() <= mMaxNumFeaturePoints) {
        return;
    }
    
    ASSERT(mBuckets.size() == mNumBucketsX, "Buckets are not allocated");
    ASSERT(mBuckets[0].size() == mNumBucketsY, "Buckets are not allocated");
    
    std::vector<FeaturePoint> points;
    PruneDoGFeatures(mBuckets,
                     points,
                     mFeaturePoints,
                     (int)mNumBucketsX,
                     (int)mNumBucketsY,
                     (int)mWidth,
                     (int)mHeight,
                     (int)mMaxNumFeaturePoints);
    
    mFeaturePoints.swap(points);
    
    ASSERT(mFeaturePoints.size() <= mMaxNumFeaturePoints, "Too many feature points");
}

void DoGScaleInvariantDetector::findSubpixelLocations(const GaussianScaleSpacePyramid* pyramid) {
    float A[9];
    float b[3];
    float u[3];
    int x, y;
    float xp, yp;
    int num_points;
    float laplacianSqrThreshold;
    float hessianThreshold;
    
    num_points = 0;
    laplacianSqrThreshold = sqr(mLaplacianThreshold);
    hessianThreshold = (sqr(mEdgeThreshold+1)/mEdgeThreshold);
    
    for(size_t i = 0; i < mFeaturePoints.size(); i++) {
        FeaturePoint& kp = mFeaturePoints[i];
        
        ASSERT(kp.scale < mLaplacianPyramid.numScalePerOctave(), "Feature point scale is out of bounds");
        int lap_index = kp.octave*mLaplacianPyramid.numScalePerOctave()+kp.scale;
        
        // Downsample the feature point to the detection octave
        bilinear_downsample_point(xp, yp, kp.x, kp.y, kp.octave);
        
        // Compute the discrete pixel location
        x = (int)(xp+0.5f);
        y = (int)(yp+0.5f);
        
        // Get Laplacian images
        const Image& lap0 = mLaplacianPyramid.images()[lap_index-1];
        const Image& lap1 = mLaplacianPyramid.images()[lap_index];
        const Image& lap2 = mLaplacianPyramid.images()[lap_index+1];
        
        // Compute the Hessian
        if(!ComputeSubpixelHessian(A, b, lap0, lap1, lap2, x, y)) {
            continue;
        }
        
        // A*u=b
        if(!SolveSymmetricLinearSystem3x3(u, A, b)) {
            continue;
        }
        
        // If points move too much in the sub-pixel update, then the point probably
        // unstable.
        if(sqr(u[0])+sqr(u[1]) > mMaxSubpixelDistanceSqr) {
            continue;
        }
        
        // Compute the edge score
        if(!ComputeEdgeScore(kp.edge_score, A)) {
            continue;
        }
        
        // Compute a linear estimate of the intensity
        ASSERT(kp.score == lap1.get<float>(y)[x], "Score is not consistent with the DoG image");
        kp.score = lap1.get<float>(y)[x] - (b[0]*u[0] + b[1]*u[1] + b[2]*u[2]);
        
        // Update the location:
        // Apply the update on the downsampled location and then upsample the result.
        bilinear_upsample_point(kp.x, kp.y, xp+u[0], yp+u[1], kp.octave);
        
        // Update the scale
        kp.sp_scale = kp.scale + u[2];
        kp.sp_scale = ClipScalar<float>(kp.sp_scale, 0, mLaplacianPyramid.numScalePerOctave());
        
        if(std::abs(kp.edge_score)  < hessianThreshold &&
           sqr(kp.score)            >= laplacianSqrThreshold &&
           kp.x                     >= 0 &&
           kp.x                     < mLaplacianPyramid.images()[0].width() &&
           kp.y                     >= 0 &&
           kp.y                     < mLaplacianPyramid.images()[0].height()) {
            // Update the sigma
            kp.sigma = pyramid->effectiveSigma(kp.octave, kp.sp_scale);
            mFeaturePoints[num_points++] = kp;
        }
    }
    
    mFeaturePoints.resize(num_points);
}

void DoGScaleInvariantDetector::findFeatureOrientations(const GaussianScaleSpacePyramid* pyramid) {
    if(!mFindOrientation) {
        for(size_t i = 0; i < mFeaturePoints.size(); i++) {
            mFeaturePoints[i].angle = 0;
        }
        return;
    }

    int num_angles;
    mTmpOrientatedFeaturePoints.clear();
    mTmpOrientatedFeaturePoints.reserve(mFeaturePoints.size()*kMaxNumOrientations);
    
    // Compute the gradient pyramid
    mOrientationAssignment.computeGradients(pyramid);
    
    // Compute an orientation for each feature point
    for(size_t i = 0; i < mFeaturePoints.size(); i++) {
        float x, y, s;
        
        // Down sample the point to the detected octave
        bilinear_downsample_point(x,
                                  y,
                                  s, 
                                  mFeaturePoints[i].x,
                                  mFeaturePoints[i].y,
                                  mFeaturePoints[i].sigma,
                                  mFeaturePoints[i].octave);
        
        // Downsampling the point can cause (x,y) to leave the image bounds by
        // a tiny amount. Here we just clip it to be within the image bounds.
        x = ClipScalar<float>(x, 0, pyramid->get(mFeaturePoints[i].octave, 0).width()-1);
        y = ClipScalar<float>(y, 0, pyramid->get(mFeaturePoints[i].octave, 0).height()-1);
        
        // Compute dominant orientations
        mOrientationAssignment.compute(&mOrientations[0],
                                       num_angles,
                                       mFeaturePoints[i].octave,
                                       mFeaturePoints[i].scale,
                                       x,
                                       y,
                                       s);
        
        // Create a feature point for each angle
        for(int j = 0; j < num_angles; j++) {
            // Copy the feature point
            FeaturePoint fp = mFeaturePoints[i];
            // Update the orientation
            fp.angle = mOrientations[j];
            // Store oriented feature point
            mTmpOrientatedFeaturePoints.push_back(fp);
        }
    }
    
    mFeaturePoints.swap(mTmpOrientatedFeaturePoints);
}

namespace vision {
    
    void PruneDoGFeatures(std::vector<std::vector<std::vector<std::pair<float, size_t> > > >& buckets,
                          std::vector<DoGScaleInvariantDetector::FeaturePoint>& outPoints,
                          const std::vector<DoGScaleInvariantDetector::FeaturePoint>& inPoints,
                          int num_buckets_X,
                          int num_buckets_Y,
                          int width,
                          int height,
                          int max_points) {
        
        int num_buckets = num_buckets_X*num_buckets_Y;
        int num_points_per_bucket = max_points/num_buckets;
        int dx = (int)std::ceil((float)width/num_buckets_X);
        int dy = (int)std::ceil((float)height/num_buckets_Y);
        
        //
        // Clear the previous state
        //
        outPoints.clear();
        outPoints.reserve(max_points);
        for(size_t i = 0; i < buckets.size(); i++) {
            for(size_t j = 0; j < buckets[i].size(); j++) {
                buckets[i][j].clear();
            }
        }
        
        //
        // Insert each features into a bucket
        //
        for(size_t i = 0; i < inPoints.size(); i++) {
            const DoGScaleInvariantDetector::FeaturePoint& p = inPoints[i];
            int binX = p.x/dx;
            int binY = p.y/dy;
            buckets[binX][binY].push_back(std::make_pair(std::abs(p.score), i));
        }
        
        //
        // Do a partial sort on the first N points of each bucket
        //
        for(size_t i = 0; i < buckets.size(); i++) {
            for(size_t j = 0; j < buckets[i].size(); j++) {
                std::vector<std::pair<float, size_t> >& bucket = buckets[i][j];
                size_t n = std::min<size_t>(bucket.size(), num_points_per_bucket);
                if(n == 0) {
                    continue;
                }
                std::nth_element(bucket.begin(),
                                 bucket.begin()+n,
                                 bucket.end(), std::greater<std::pair<float, size_t> >());
                
                DEBUG_BLOCK(
                            if(n > bucket.size()) {
                                ASSERT(bucket[0].first >= bucket[n].first, "nth_element failed");
                            }
                            )
                
                for(size_t k = 0; k < n; k++) {
                    outPoints.push_back(inPoints[bucket[k].second]);
                }
            }
        }
    }
    
}