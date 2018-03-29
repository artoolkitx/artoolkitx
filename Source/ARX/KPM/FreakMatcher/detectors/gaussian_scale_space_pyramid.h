//
//  gaussian_scale_space_pyramid.h
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
#include <framework/image.h>
#include <math/math_utils.h>
#include <cmath>

namespace vision {
    
    /**
     * Use this function to upsample a point that has been found from a
     * bilinear downsample pyramid.
     *
     * xp = x*2^n + 2^(n-1) - 0.5
     * yp = y*2^n + 2^(n-1) - 0.5
     *
     * n: Octave that the point was detected on.
     
     * @param[out] xp Upsampled x location
     * @param[out] yp Upsampled y location
     * @param[in] x X location on detected image
     * @param[in] y Y location on detected image
     * @param[in] octave The octave of the detected image
     */
    inline void bilinear_upsample_point(float& xp,
                                        float& yp,
                                        float x,
                                        float y,
                                        int octave) {
        float a, b;
        a = std::pow(2.f, octave-1)-0.5f;
        b = (1<<octave);
        xp = (x*b)+a;
        yp = (y*b)+a;
    }
    
    /**
     * Use this function to upsample a point that has been found from a
     * bilinear downsample pyramid.
     *
     * xp = x*2^n + 2^(n-1) - 0.5
     * yp = y*2^n + 2^(n-1) - 0.5
     *
     * n: Octave that the point was detected on.
     
     * @param[out] xp Upsampled x location
     * @param[out] yp Upsampled y location
     * @param[in] x X location on detected image
     * @param[in] y Y location on detected image
     * @param[in] octave The octave of the detected image
     */
    inline void bilinear_upsample_point(float& xp,
                                        float& yp,
                                        float& sp,
                                        float x,
                                        float y,
                                        float s,
                                        int octave) {
        float a, b;
        a = std::pow(2.f, octave-1)-0.5f;
        b = (1<<octave);
        xp = (x*b)+a;
        yp = (y*b)+a;
        sp = s*b;
    }
    
    /**
     * Use this function to downsample a point to an octave that was found from
     * a bilinear downsampled pyramid.
     *
     * @param[out] xp Downsampled x location
     * @param[out] yp Downsampled y location
     * @param[in] x X location on fine image
     * @param[in] y Y location on fine image
     * @param[in] octave The octave to downsample (x,y) to
     */
    inline void bilinear_downsample_point(float& xp,
                                          float& yp,
                                          float x,
                                          float y,
                                          int octave) {
        float a, b;
        a = 1.f/(1<<octave);
        b = 0.5f*a-0.5f;
        xp = x*a+b;
        yp = y*a+b;
    }
    
    /**
     * Use this function to downsample a point to an octave that was found from
     * a bilinear downsampled pyramid.
     *
     * @param[out] xp Downsampled x location
     * @param[out] yp Downsampled y location
     * @param[out] sp Downsampled sigma
     * @param[in] x X location on fine image
     * @param[in] y Y location on fine image
     * @param[in] s Sigma on fine image
     * @param[in] octave The octave to downsample (x,y) to
     */
    inline void bilinear_downsample_point(float& xp,
                                          float& yp,
                                          float& sp,
                                          float x,
                                          float y,
                                          float s,
                                          int octave) {
        float a, b;
        a = 1.f/(1<<octave);
        b = 0.5f*a-0.5f;
        xp = x*a+b;
        yp = y*a+b;
        sp = s*a;
    }
    
    /**
     * Apply a 2D binomial filter to a source image.
     *
     * @param[out] dst Destination image
     * @param[in] tmp Temporary memory of same size as source
     * @param[in] src Source image
     * @param[in] width Width of image
     * @param[in] height Height of image
     */
    void binomial_4th_order(float* dst,
                            unsigned short* tmp,
                            const unsigned char* src,
                            size_t width,
                            size_t height);
    void binomial_4th_order(float* dst,
                            float* tmp,
                            const float* src,
                            size_t width,
                            size_t height);
    
    /**
     * The mean of the first pixel quad, and then every other pixel quad afterwards.
     *
     * @param[out] dst Destination image
     * @param[in] src Source image
     * @param[in] src_width Source width
     * @param[in] src_height Source height
     */
    void downsample_bilinear(float* dst, const float* src, size_t src_width, size_t src_height);
    
    class GaussianScaleSpacePyramid {
    public:
        
        GaussianScaleSpacePyramid();
        virtual ~GaussianScaleSpacePyramid() {}
        
        /**
         * Configure the pyramid.
         */
        void configure(int num_octaves, int num_scales_per_octaves);
        
        /**
         * Get the number of octaves and scales.
         */
        inline int numOctaves() const { return mNumOctaves; }
        inline int numScalesPerOctave() const { return mNumScalesPerOctave; }
        
        /**
         * @return Get the number of images in the pyramid.
         */
        inline size_t size() const { return mPyramid.size(); }
        
        /**
         * @return Get the vector of images.
         */
        inline std::vector<Image>& images() { return mPyramid; }
        inline const std::vector<Image>& images() const { return mPyramid; }
        
        /**
         * Get a pyramid image.
         */
        inline Image& get(size_t octave, size_t scale) {
            ASSERT(octave < mNumOctaves, "Octave out of range");
            ASSERT(scale < mNumScalesPerOctave, "Scale out of range");
            return mPyramid[octave*mNumScalesPerOctave+scale];
        }
        inline const Image& get(size_t octave, size_t scale) const {
            ASSERT(octave < mNumOctaves, "Octave out of range");
            ASSERT(scale < mNumScalesPerOctave, "Scale out of range");
            return mPyramid[octave*mNumScalesPerOctave+scale];
        }
        
        /**
         * Get the constant k-factor.
         */
        inline float kfactor() const { return mK; }
        
        /**
         * Get the effective sigma given the octave and sub-pixel scale.
         */
        inline float effectiveSigma(size_t octave, float scale) const {
            ASSERT(scale >= 0, "Scale must be positive");
            ASSERT(scale < mNumScalesPerOctave, "Scale must be less than number of scale per octave");
            return std::pow(mK, scale)*(1<<octave);
        }
        
        /**
         * Locate a SIGMA on the pyramid.
         */
        inline void locate(int& octave,
                           int& scale,
                           float sigma) const {
            // octave = floor(log2(s))
            octave = (int)std::floor(vision::log2<float>(sigma));
            // scale = logk(s/2^octave)
            // Here we ROUND the scale to an integer value
            float fscale = std::log(sigma/(float)(1<<octave))*mOneOverLogK;
            scale = (int)vision::round(fscale);
            
            // The last scale in an octave has the same sigma as the first scale
            // of the next octave. We prefer coarser octaves for efficiency.
            if(scale == mNumScalesPerOctave-1) {
                // If this octave is out of range, then it will be clipped to
                // be in range on the next step below.
                octave = octave+1;
                scale = 0;
            }
            
            // Clip the octave/scale to be in range of the pyramid
            if(octave < 0) {
                octave = 0;
                scale = 0;
            } else if(octave >= mNumOctaves) {
                octave = mNumOctaves-1;
                scale = mNumScalesPerOctave-1;
            }
            
            ASSERT(octave >= 0, "Octave must be positive");
            ASSERT(octave < mNumOctaves, "Octave must be less than number of octaves");
            ASSERT(scale >= 0, "Scale must be positive");
            ASSERT(scale < mNumScalesPerOctave, "Scale must be less than number of scale per octave");
        }
        
        /**
         * Locate an (x,y,sigma) specified at the highest resolution on the pyramid.
         */
        inline void locate(float& xp,
                           float& yp,
                           float& sigmap,
                           int& octave,
                           int& scale,
                           float x,
                           float y,
                           float sigma) const {
            // Locate the SIGMA on the pyramid
            locate(octave, scale, sigma);
            // Downsample the point
            bilinear_downsample_point(xp, yp, sigmap, x, y, sigma, octave);
        }
        
    protected:
        
        // Vector of pyramid images
        std::vector<Image> mPyramid;
        
        // Number of octaves
        int mNumOctaves;
        int mNumScalesPerOctave;
        
        float mK; // = 2^(1/(mNumScalesPerOctave-1))
        float mOneOverLogK; // 1/log(k) precomputed for efficiency
    };
    
    /**
     * Build a Binomial Gaussian Scale Space Pyramid represented represented as 32-bit floats.
     */
    class BinomialPyramid32f : public GaussianScaleSpacePyramid {
    public:
        
        BinomialPyramid32f();
        ~BinomialPyramid32f();
        
        /**
         * Allocate memory for the pyramid.
         */
        void alloc(size_t width,
                   size_t height,
                   int num_octaves);
        
        /**
         * Release the pyramid memory.
         */
        void release();
        
        /**
         * Build the pyramid.
         */
        void build(const Image& image);
        
    private:
        
        // Temporary space for binomial filter
        std::vector<unsigned short> mTemp_us16;
        std::vector<float> mTemp_f32_1;
        std::vector<float> mTemp_f32_2;
        
        void apply_filter(Image& dst, const Image& src);
        void apply_filter_twice(Image& dst, const Image& src);
    };
    
    /**
     * Compute the number of octaves for a width/height from the minimum size
     * at the coarsest octave.
     */
    inline int numOctaves(int width, int height, int min_size) {
        int num_octaves = 0;
        while(width >= min_size &&
              height >= min_size) {
            width >>= 1;
            height >>= 1;
            num_octaves++;
        }
        return num_octaves;
    }
    
} // vision