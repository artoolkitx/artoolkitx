//
//  DoG_scale_invariant_detector.h
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

#include "gaussian_scale_space_pyramid.h"
#include "orientation_assignment.h"
#include "interpolate.h"
#include "utils/point.h"
#include <framework/error.h>
#include <math/math_utils.h>

namespace vision {
    
    /**
     * Computes a Difference-of-Gaussian Pyramid from a Gaussian Pyramid.
     */
    class DoGPyramid {
    public:
        
        DoGPyramid();
        ~DoGPyramid() {}
        
        /**
         * Allocate the pyramid.
         */
        void alloc(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * Compute the Difference-of-Gaussian from a Gaussian Pyramid.
         */
        void compute(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * Get a Laplacian image at a level in the pyramid.
         */
        inline Image& get(size_t octave, size_t scale) { return mImages[octave*mNumScalesPerOctave+scale]; }
        inline const Image& get(size_t octave, size_t scale) const { return mImages[octave*mNumScalesPerOctave+scale]; }
        
        /**
         * Get vector of images.
         */
        inline const std::vector<Image>& images() const { return mImages; }
        
        /**
         * Get a Laplacian image at an index.
         */
        inline Image& get(size_t index) { return mImages[index]; }
        inline const Image& get(size_t index) const { return mImages[index]; }
        
        /**
         * Get the number of octaves and scales.
         */
        inline int numOctaves() const { return mNumOctaves; }
        inline int numScalePerOctave() const { return mNumScalesPerOctave; }
        inline size_t size() const { return mImages.size(); }
        
        /**
         * Get the octave from the Laplacian image index.
         */
        inline int octaveFromIndex(int index) const {
            ASSERT(index < mImages.size(), "Index is out of range");
            return vision::round(vision::log2((float)(mImages[0].width()/mImages[index].width())));
        }
        
        /**
         * Get the scale from the Laplacian image index.
         */
        inline int scaleFromIndex(int index) const { return index%mNumScalesPerOctave; }
        
    private:
        
        // DoG images
        std::vector<Image> mImages;
        
        // Number of octaves and scales
        int mNumOctaves;
        int mNumScalesPerOctave;
        
        /**
         * Compute the difference image.
         *
         * d = im1 - im2
         */
        void difference_image_binomial(Image& d, const Image& im1, const Image& im2);
    };
    
    class DoGScaleInvariantDetector {
    public:
        
        static const size_t kMaxNumFeaturePoints = 5000;
        static const int kMaxNumOrientations = 36;
        
        struct FeaturePoint {
            float x, y;
            float angle;
            int octave;
            int scale;
            float sp_scale;
            float score;
            float sigma;
            float edge_score;
        }; // FeaturePoint
        
        DoGScaleInvariantDetector();
        ~DoGScaleInvariantDetector();
        
        /**
         * Allocate memory.
         */
        void alloc(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * @return Width/Height of configured image
         */
        inline size_t width() const { return mWidth; }
        inline size_t height() const { return mHeight; }
        
        /**
         * Detect scale-invariant feature points given a pyramid.
         */
        void detect(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * Get/Set the Laplacian absolute threshold.
         */
        float laplacianThreshold() const {
            return mLaplacianThreshold;
        }
        void setLaplacianThreshold(float tr) {
            mLaplacianThreshold = tr;
        }
        
        /**
         * Get/Set the maximum number of feature point.
         */
        size_t maxNumFeaturePoints() const {
            return mMaxNumFeaturePoints;
        }
        void setMaxNumFeaturePoints(size_t n) {
            mMaxNumFeaturePoints = n;
            mFeaturePoints.reserve(n);
        }
        
        /**
         * Get/Set the edge threshold.
         */
        float edgeThreshold() const {
            return mEdgeThreshold;
        }
        void setEdgeThreshold(float tr) {
            mEdgeThreshold = tr;
        }
        
        /**
         * Set/Get find orientations.
         */
        void setFindOrientation(bool b) {
            mFindOrientation = b;
        }
        bool findOrientation() const {
            return mFindOrientation;
        }
        
        /**
         * @return Feature points
         */
        inline const std::vector<FeaturePoint>& features() const { return mFeaturePoints; }
        
        /**
         * @return DoG Pyramid
         */
        inline const DoGPyramid& dogPyramid() const { return mLaplacianPyramid; }
        
    private:
        
        // Width/Height of configured image
        size_t mWidth;
        size_t mHeight;
        
        // Number of buckets in X/Y
        size_t mNumBucketsX;
        size_t mNumBucketsY;
        
        // Buckets for pruning points
        std::vector<std::vector<std::vector<std::pair<float, size_t> > > > mBuckets;
        
        // True if the orientation should be assigned
        bool mFindOrientation;
        
        // DoG pyramid
        DoGPyramid mLaplacianPyramid;
        
        // Laplacian score threshold
        float mLaplacianThreshold;
        
        // Edge threshold 
        float mEdgeThreshold;
        
        // Vector of extracted feature points
        std::vector<FeaturePoint> mFeaturePoints;
        
        // Tmp vector of extracted feature points that have orientation values
        std::vector<FeaturePoint> mTmpOrientatedFeaturePoints;
        
        // Maximum number of feature points
        size_t mMaxNumFeaturePoints;
        
        // Maximum update allowed for sub-pixel refinement
        float mMaxSubpixelDistanceSqr;
        
        // Orientation assignment
        OrientationAssignment mOrientationAssignment;
        
        // Vector of orientations. Pre-allocated to the maximum
        // number of orientations per feature point.
        std::vector<float> mOrientations;
        
        /**
         * Extract the minima/maxima.
         */
        void extractFeatures(const GaussianScaleSpacePyramid* pyramid,
                             const DoGPyramid* laplacian);
        
        /**
         * Sub-pixel refinement.
         */
        void findSubpixelLocations(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * Prune the number of features.
         */
        void pruneFeatures();
        
        /**
         * Find feature orientations.
         */
        void findFeatureOrientations(const GaussianScaleSpacePyramid* pyramid);
        
    }; // DoGScaleInvariantDetector
    
    inline void ComputeSubpixelDerivatives(float& Dx,
                                           float& Dy,
                                           float& Dxx,
                                           float& Dyy,
                                           float& Dxy,
                                           const Image& im,
                                           int x,
                                           int y)
    {
        // Sanity checks
        ASSERT((x-1) >= 0 && (x+1) < im.width(), "x out of bounds");
        ASSERT((y-1) >= 0 && (y+1) < im.height(), "y out of bounds");
        
        const float* pm1 = &im.get<float>(y-1)[x];
        const float* p   = &im.get<float>(y)[x];
        const float* pp1 = &im.get<float>(y+1)[x];
        
        Dx  = 0.5f*(p[1]-p[-1]);
        Dy  = 0.5f*(pp1[0]-pm1[0]);
        Dxx = p[-1] + (-2.f*p[0]) + p[1];
        Dyy = pm1[0] + (-2.f*p[0]) + pp1[0];
        Dxy = 0.25f*((pm1[-1] + pp1[1]) - (pm1[1] + pp1[-1]));
    }
    
    inline void ComputeSubpixelHessianSameOctave(float H[9],
                                                 float b[3],
                                                 const Image& lap0,
                                                 const Image& lap1,
                                                 const Image& lap2,
                                                 int x,
                                                 int y) {
        float Dx, Dy, Ds;
        float Dxx, Dyy, Dxy;
        float Dss, Dxs, Dys;
        
        ASSERT((x-1) >= 0 && (x+1) < lap1.width(), "x out of bounds");
        ASSERT((y-1) >= 0 && (y+1) < lap1.height(), "y out of bounds");
        ASSERT(lap0.width() == lap1.width(), "Image dimensions inconsistent");
        ASSERT(lap0.width() == lap2.width(), "Image dimensions inconsistent");
        ASSERT(lap0.height() == lap1.height(), "Image dimensions inconsistent");
        ASSERT(lap0.height() == lap2.height(), "Image dimensions inconsistent");
        
        const float* lap0_pm1 = &lap0.get<float>(y-1)[x];
        const float* lap0_p   = &lap0.get<float>(y)[x];
        const float* lap0_pp1 = &lap0.get<float>(y+1)[x];
        
        const float* lap1_p   = &lap1.get<float>(y)[x];
        
        const float* lap2_pm1 = &lap2.get<float>(y-1)[x];
        const float* lap2_p   = &lap2.get<float>(y)[x];
        const float* lap2_pp1 = &lap2.get<float>(y+1)[x];
        
        // Compute spatial derivatives
        ComputeSubpixelDerivatives(Dx, Dy, Dxx, Dyy, Dxy, lap1, x, y);
        
        // Compute scale derivates
        Ds  = 0.5f*(lap2_p[0] - lap0_p[0]);
        Dss = lap0_p[0] + (-2.f*lap1_p[0]) + lap2_p[0];
        Dxs = 0.25f*((lap0_p[-1] - lap0_p[1]) + (-lap2_p[-1] + lap2_p[1]));
        Dys = 0.25f*((lap0_pm1[0] - lap0_pp1[0]) + (-lap2_pm1[0] + lap2_pp1[0]));
        
        // H
        H[0] = Dxx; H[1] = Dxy; H[2] = Dxs;
        H[3] = Dxy; H[4] = Dyy; H[5] = Dys;
        H[6] = Dxs; H[7] = Dys; H[8] = Dss;
        
        // b
        b[0] = -Dx;
        b[1] = -Dy;
        b[2] = -Ds;
    }
    
    inline void ComputeSubpixelHessianCoarseOctavePair(float H[9],
                                                       float b[3],
                                                       const Image& lap0,
                                                       const Image& lap1,
                                                       const Image& lap2,
                                                       int x,
                                                       int y) {
        float val;
        float x_mul_2, y_mul_2;
        float Dx, Dy, Ds;
        float Dxx, Dyy, Dxy;
        float Dss, Dxs, Dys;
        
        ASSERT((x-1) >= 0 && (x+1) < lap1.width(), "x out of bounds");
        ASSERT((y-1) >= 0 && (y+1) < lap1.height(), "y out of bounds");
        ASSERT((lap0.width()>>1) == lap1.width(), "Image dimensions inconsistent");
        ASSERT((lap0.width()>>1) == lap2.width(), "Image dimensions inconsistent");
        ASSERT((lap0.height()>>1) == lap1.height(), "Image dimensions inconsistent");
        ASSERT((lap0.height()>>1) == lap2.height(), "Image dimensions inconsistent");

        const float* lap1_p   = &lap1.get<float>(y)[x];;
        
        const float* lap2_pm1 = &lap2.get<float>(y-1)[x];
        const float* lap2_p   = &lap2.get<float>(y)[x];
        const float* lap2_pp1 = &lap2.get<float>(y+1)[x];
        
        // Upsample the point to the higher octave
        bilinear_upsample_point(x_mul_2, y_mul_2, x, y, 1);
        
        // Compute spatial derivatives
        ComputeSubpixelDerivatives(Dx, Dy, Dxx, Dyy, Dxy, lap1, x, y);
        
        // Interpolate the VALUE at the finer octave
        val = bilinear_interpolation<float>(lap0, x_mul_2, y_mul_2);
        
        Ds  = 0.5f*(lap2_p[0] - val);
        Dss = val + (-2.f*lap1_p[0]) + lap2_p[0];
        Dxs = 0.25f*((bilinear_interpolation<float>(lap0, x_mul_2-2, y_mul_2) + lap2_p[1]) -
                     (bilinear_interpolation<float>(lap0, x_mul_2+2, y_mul_2) + lap2_p[-1]));
        Dys = 0.25f*((bilinear_interpolation<float>(lap0, x_mul_2, y_mul_2-2) + lap2_pp1[0]) -
                     (bilinear_interpolation<float>(lap0, x_mul_2, y_mul_2+2) + lap2_pm1[0]));
        
        // H
        H[0] = Dxx; H[1] = Dxy; H[2] = Dxs;
        H[3] = Dxy; H[4] = Dyy; H[5] = Dys;
        H[6] = Dxs; H[7] = Dys; H[8] = Dss;
        
        // h
        b[0] = -Dx;
        b[1] = -Dy;
        b[2] = -Ds;
    }
    
    inline void ComputeSubpixelHessianFineOctavePair(float H[9],
                                                     float b[3],
                                                     const Image& lap0,
                                                     const Image& lap1,
                                                     const Image& lap2,
                                                     int x,
                                                     int y)
    {
        float x_div_2, y_div_2;
        float val;
        float Dx, Dy, Ds;
        float Dxx, Dyy, Dxy;
        float Dss, Dxs, Dys;
        
        ASSERT((x-1) >= 0 && (x+1) < lap1.width(), "x out of bounds");
        ASSERT((y-1) >= 0 && (y+1) < lap1.height(), "y out of bounds");
        ASSERT(lap0.width() == lap1.width(), "Image dimensions inconsistent");
        ASSERT((lap0.width()>>1) == lap2.width(), "Image dimensions inconsistent");
        ASSERT(lap0.height() == lap1.height(), "Image dimensions inconsistent");
        ASSERT((lap0.height()>>1) == lap2.height(), "Image dimensions inconsistent");
        
        const float* lap0_pm1 = &lap0.get<float>(y-1)[x];
        const float* lap0_p   = &lap0.get<float>(y)[x];
        const float* lap0_pp1 = &lap0.get<float>(y+1)[x];
        
        const float* lap1_p   = &lap1.get<float>(y)[x];
        
        bilinear_downsample_point(x_div_2, y_div_2, x, y, 1);
        
        ASSERT(x_div_2-0.5f >= 0, "x_div_2 out of bounds out of bounds for interpolation");
        ASSERT(y_div_2-0.5f >= 0, "y_div_2 out of bounds out of bounds for interpolation");
        ASSERT(x_div_2+0.5f < lap2.width(), "x_div_2 out of bounds out of bounds for interpolation");
        ASSERT(y_div_2+0.5f < lap2.height(), "y_div_2 out of bounds out of bounds for interpolation");
        
        // Compute spatial derivatives
        ComputeSubpixelDerivatives(Dx, Dy, Dxx, Dyy, Dxy, lap1, x, y);
        
        // Interpolate the VALUE at the coarser octave
        val = bilinear_interpolation<float>(lap2, x_div_2, y_div_2);
        
        Ds = 0.5f*(val - lap0_p[0]);
        Dss = lap0_p[0] + (-2.f*lap1_p[0]) + val;
        Dxs = 0.25f*((lap0_p[-1]  + bilinear_interpolation<float>(lap2, x_div_2+.5f, y_div_2)) -
                     (lap0_p[ 1]  + bilinear_interpolation<float>(lap2, x_div_2-.5f, y_div_2)));
        Dys = 0.25f*((lap0_pm1[0] + bilinear_interpolation<float>(lap2, x_div_2,     y_div_2+.5f)) -
                     (lap0_pp1[0] + bilinear_interpolation<float>(lap2, x_div_2,     y_div_2-.5f)));
        
        // H
        H[0] = Dxx; H[1] = Dxy; H[2] = Dxs;
        H[3] = Dxy; H[4] = Dyy; H[5] = Dys;
        H[6] = Dxs; H[7] = Dys; H[8] = Dss;
        
        // b
        b[0] = -Dx;
        b[1] = -Dy;
        b[2] = -Ds;
    }
    
    inline bool ComputeSubpixelHessian(float H[9],
                                       float b[3],
                                       const Image& lap0,
                                       const Image& lap1,
                                       const Image& lap2,
                                       int x,
                                       int y) {

        if(lap0.width() == lap1.width() == lap2.width()) {
            ASSERT(lap0.height() == lap1.height() == lap2.height(), "Width/height are not consistent");
            ComputeSubpixelHessianSameOctave(H, b, lap0, lap1, lap2, x, y);
        } else if((lap0.width() == lap1.width()) && ((lap1.width()>>1) == lap2.width())) {
            ASSERT((lap0.height() == lap1.height()) && ((lap1.height()>>1) == lap2.height()), "Width/height are not consistent");
            ComputeSubpixelHessianFineOctavePair(H, b, lap0, lap1, lap2, x, y);
        } else if(((lap0.width()>>1) == lap1.width()) && (lap1.width() == lap2.width())) {
            ASSERT(((lap0.width()>>1) == lap1.width()) && (lap1.width() == lap2.width()), "Width/height are not consistent");
            ComputeSubpixelHessianCoarseOctavePair(H, b, lap0, lap1, lap2, x, y);
        } else {
            ASSERT(0, "Image sizes are inconsistent");
            return false;
        }
        
        return true;
    }
    
    inline bool ComputeEdgeScore(float& score, const float H[9]) {
        float det;
        
        const float& Dxx = H[0];
        const float& Dyy = H[4];
        const float& Dxy = H[1];
        
        det = (Dxx*Dyy)-sqr(Dxy);
        
        // The determinant cannot be zero
        if(det == 0) {
            return false;
        }
        
        // Compute a score based on the local curvature
        score = sqr(Dxx+Dyy)/det;
        
        return true;
    }
    
    void PruneDoGFeatures(std::vector<std::vector<std::vector<std::pair<float, size_t> > > >& buckets,
                          std::vector<DoGScaleInvariantDetector::FeaturePoint>& outPoints,
                          const std::vector<DoGScaleInvariantDetector::FeaturePoint>& inPoints,
                          int num_buckets_X,
                          int num_buckets_Y,
                          int width,
                          int height,
                          int max_points);
    
} // vision