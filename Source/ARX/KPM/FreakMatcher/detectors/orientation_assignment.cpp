//
//  orientation_assignment.cpp
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

#include "orientation_assignment.h"
#include "gradients.h"

#include <framework/error.h>
#include <math/indexing.h>
#include <math/polynomial.h>
#include <math/math_utils.h>
#include <math/math_io.h>

using namespace vision;

OrientationAssignment::OrientationAssignment()
: mNumOctaves(0)
, mNumScalesPerOctave(0)
, mGaussianExpansionFactor(0)
, mSupportRegionExpansionFactor(0)
, mNumSmoothingIterations(0)
, mPeakThreshold(0) {
}

OrientationAssignment::~OrientationAssignment() {}

void OrientationAssignment::alloc(size_t fine_width,
                                  size_t fine_height,
                                  int num_octaves,
                                  int num_scales_per_octave,
                                  int num_bins,
                                  float gaussian_expansion_factor,
                                  float support_region_expansion_factor,
                                  int num_smoothing_iterations,
                                  float peak_threshold) {
    mNumOctaves = num_octaves;
    mNumScalesPerOctave = num_scales_per_octave;
    mNumBins = num_bins;
    mGaussianExpansionFactor = gaussian_expansion_factor;
    mSupportRegionExpansionFactor = support_region_expansion_factor;
    mNumSmoothingIterations = num_smoothing_iterations;
    mPeakThreshold = peak_threshold;
    
    mHistogram.resize(num_bins);
    
    // Allocate gradient images
    mGradients.resize(mNumOctaves*mNumScalesPerOctave);
    for(size_t i = 0; i < num_octaves; i++) {
        for(size_t j = 0; j < num_scales_per_octave; j++) {
            mGradients[i*num_scales_per_octave+j].alloc(IMAGE_F32,
                                                        fine_width>>i,
                                                        fine_height>>i,
                                                        AUTO_STEP,
                                                        2);
        }
    }
}

void OrientationAssignment::computeGradients(const GaussianScaleSpacePyramid* pyramid) {
    // Loop over each pyramid image and compute the gradients
    for(size_t i = 0; i < pyramid->images().size(); i++) {
        const Image& im = pyramid->images()[i];

        // Compute gradient image
        ASSERT(im.width() == im.step()/sizeof(float), "Step size must be equal to width for now");
        ComputePolarGradients(mGradients[i].get<float>(),
                              im.get<float>(),
                              im.width(),
                              im.height());
    }
}

void OrientationAssignment::compute(float* angles,
                                    int& num_angles,
                                    int octave,
                                    int scale,
                                    float x,
                                    float y,
                                    float sigma) {  
    int xi, yi;
    float radius;
    float radius2;
    int x0, y0;
    int x1, y1;
    float max_height;
    float gw_sigma, gw_scale;
    
    ASSERT(x >= 0, "x must be positive");
    ASSERT(x < mGradients[octave*mNumScalesPerOctave+scale].width(), "x must be less than the image width");
    ASSERT(y >= 0, "y must be positive");
    ASSERT(y < mGradients[octave*mNumScalesPerOctave+scale].height(), "y must be less than the image height");
    
    int level = octave*mNumScalesPerOctave+scale;
    const Image& g = mGradients[level];
    ASSERT(g.channels() == 2, "Number of channels should be 2");
    
    max_height = 0;
    num_angles = 0;
    
    xi = (int)(x+0.5f);
    yi = (int)(y+0.5f);
    
    // Check that the position is with the image bounds
    if(xi < 0 ||
       xi >= g.width() ||
       yi < 0 ||
       yi >= g.height())
    {
        return;
    }
    
    gw_sigma = max2<float>(1.f, mGaussianExpansionFactor*sigma);
    gw_scale = -1.f/(2*sqr(gw_sigma));
    
    // Radius of the support region
    radius  = mSupportRegionExpansionFactor*gw_sigma;
    radius2 = std::ceil(sqr(radius));
    
    // Box around feature point
    x0 = xi-(int)(radius+0.5f);
    x1 = xi+(int)(radius+0.5f);
    y0 = yi-(int)(radius+0.5f);
    y1 = yi+(int)(radius+0.5f);
    
    // Clip the box to be within the bounds of the image
    x0 = max2<int>(0, x0);
    x1 = min2<int>(x1, (int)g.width()-1);
    y0 = max2<int>(0, y0);
    y1 = min2<int>(y1, (int)g.height()-1);
    
    // Zero out the orientation histogram
    ZeroVector(&mHistogram[0], mHistogram.size());
    
    // Build up the orientation histogram
    for(int yp = y0; yp <= y1; yp++) {
        float dy = yp-y;
        float dy2 = sqr(dy);
        
        const float* y_ptr = g.get<float>(yp);
        
        for(int xp = x0; xp <= x1; xp++) {
            float dx = xp-x;
            float r2 = sqr(dx)+dy2;
            
            // Only use the gradients within the circular window
            if(r2 > radius2) {
                continue;
            }
            
            const float* g = &y_ptr[xp<<1];
            const float& angle = g[0];
            const float& mag   = g[1];
            
            // Compute the gaussian weight based on distance from center of keypoint
            float w = fastexp6(r2*gw_scale);
            
            // Compute the sub-bin location
            float fbin  = mNumBins*angle*ONE_OVER_2PI;
            
            // Vote to the orientation histogram with a bilinear update
            bilinear_histogram_update(&mHistogram[0], fbin, w*mag, mNumBins);
        }
    }
    
    // The orientation histogram is smoothed with a Gaussian
    for(int iter = 0; iter < mNumSmoothingIterations; iter++) {
        // sigma=1
        const float kernel[] = {
            0.274068619061197f,
            0.451862761877606f,
            0.274068619061197f};
        SmoothOrientationHistogram(&mHistogram[0], &mHistogram[0], mNumBins, kernel);
    }
    
    // Find the peak of the histogram.
    for(int i = 0; i < mNumBins; i++) {
        if(mHistogram[i] > max_height) {
            max_height = mHistogram[i];
        }
    }
    
    // The max height should be positive.
    if(max_height == 0) {
        return;
    }
    
    ASSERT(max_height > 0, "Maximum bin should be positive");
    
    // Find all the peaks.
    for(int i = 0; i < mNumBins; i++) {
        const float p0[]  = {(float)i, mHistogram[i]};
        const float pm1[] = {(float)(i-1), mHistogram[(i-1+mNumBins)%mNumBins]};
        const float pp1[] = {(float)(i+1), mHistogram[(i+1+mNumBins)%mNumBins]};
        
        // Ensure that "p0" is a relative peak w.r.t. the two neighbors
        if((mHistogram[i] > mPeakThreshold*max_height) && (p0[1] > pm1[1]) && (p0[1] > pp1[1])) {
            float A, B, C, fbin;
            
            // The default sub-pixel bin location is the discrete location if the quadratic
            // fitting fails.
            fbin = i;
            
            // Fit a quatratic to the three bins
            if(Quadratic3Points(A, B, C, pm1, p0, pp1)) {
                // If "QuadraticCriticalPoint" fails, then "fbin" is not updated.
                QuadraticCriticalPoint(fbin, A, B, C);
            }
            
            // The sub-pixel angle needs to be in the range [0,2*pi)
            angles[num_angles] = std::fmod((2.f*PI)*((fbin+0.5f+(float)mNumBins)/(float)mNumBins), 2.f*PI);
            
            // Increment the number of angles
            num_angles++;
        }
    }
}