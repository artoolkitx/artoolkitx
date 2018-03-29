//
//  orientation_assignment.h
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
#include <framework/error.h>
#include <vector>

#include "gaussian_scale_space_pyramid.h"

namespace vision {
    
    /**
     * Implements orientation assignment to feature points based on dominant gradients. 
     */
    class OrientationAssignment {
    public:
        
        OrientationAssignment();
        ~OrientationAssignment();
        
        /**
         * Allocate memory.
         */
        void alloc(size_t fine_width,
                   size_t fine_height,
                   int num_octaves,
                   int num_scales_per_octave,
                   int num_bins,
                   float gaussian_expansion_factor,
                   float support_region_expansion_factor,
                   int num_smoothing_iterations,
                   float peak_threshold);
        
        /**
         * Compute the gradients given a pyramid.
         */
        void computeGradients(const GaussianScaleSpacePyramid* pyramid);
        
        /**
         * Compute orientations for a keypont.
         */
        void compute(float* angles,
                     int& num_angles,
                     int octave,
                     int scale,
                     float x,
                     float y,
                     float sigma);
        
        /**
         * @return Vector of images.
         */
        inline const std::vector<Image>& images() const { return mGradients; }
        
        /**
         * Get a gradient image at an index.
         */
        inline const Image& get(size_t i) const { return mGradients[i]; }
        
    private:
        
        int mNumOctaves;
        int mNumScalesPerOctave;
        
        // Number of bins in the histogram
        int mNumBins;
        
        // Factor to expand the Gaussian weighting function. The Gaussian sigma is computed
        // by expanding the feature point scale. The feature point scale represents the isometric
        // size of the feature. 
        float mGaussianExpansionFactor;
        
        // Factor to expand the support region. This factor is multipled by the expanded
        // Gaussian sigma. It essentially acts at the "window" to collect gradients in.
        float mSupportRegionExpansionFactor;
        
        // Number of binomial smoothing iterations of the orientation histogram. The histogram
        // is smoothed before find the peaks.
        int mNumSmoothingIterations;
        
        // All the supporting peaks which are X percent of the absolute peak are considered
        // dominant orientations. 
        float mPeakThreshold;
        
        // Orientation histogram
        std::vector<float> mHistogram;
        
        // Vector of gradient images
        std::vector<Image> mGradients;
        
    }; // OrientationAssignment
    
    /**
     * Update a histogram with bilinear interpolation.
     *
     * @param[in/out] hist Histogram
     * @param[in] fbin Decimal bin position to vote
     * @param[in] magnitude Magnitude of the vote
     * @param[in] num_bin Number of bins in the histogram
     */
    inline void bilinear_histogram_update(float* hist,
                                          float fbin,
                                          float magnitude,
                                          int num_bins) {
        ASSERT(hist != NULL, "Histogram pointer is NULL");
        ASSERT((fbin+0.5f) > 0 && (fbin-0.5f) < num_bins, "Decimal bin position index out of range");
        ASSERT(magnitude >= 0, "Magnitude cannot be negative");
        ASSERT(num_bins >= 0, "Number bins must be positive");
        
        int bin = (int)std::floor(fbin-0.5f);
        float w2 = fbin-(float)bin-0.5f;
        float w1 = (1.f-w2);
        int b1 = (bin+num_bins)%num_bins;
        int b2 = (bin+1)%num_bins;
        
        ASSERT(w1 >= 0, "w1 must be positive");
        ASSERT(w2 >= 0, "w2 must be positive");        
        ASSERT(b1 >= 0 && b1 < num_bins, "b1 bin index out of range");
        ASSERT(b2 >= 0 && b2 < num_bins, "b2 bin index out of range");
        
        // Vote to 2 weighted bins
        hist[b1] += w1*magnitude;
        hist[b2] += w2*magnitude;
    }
    
    /**
     * Smooth the orientation histogram with a kernel.
     *
     * @param[out] y Destination histogram (in-place processing supported) 
     * @param[in] x Source histogram
     * @param[in] kernel
     */
    template<typename T>
    inline void SmoothOrientationHistogram(T* y, const T* x, size_t n, const T kernel[3]) {
        T first = x[0];
        T prev = x[n-1];
        for(size_t i = 0; i < n-1; i++) {
            T cur = x[i];
            y[i] = kernel[0]*prev + kernel[1]*cur + kernel[2]*x[i+1];
            prev = cur;
        }
        y[n-1] = kernel[0]*prev + kernel[1]*x[n-1] + kernel[2]*first;
    }
    
} // vision
