//
//  pyramid.h
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
#include "gaussian_scale_space_pyramid.h"

namespace vision {
    
    /**
     * Allocate the temporary memory space for binomial decimation.
     */
    unsigned short* BinomialDecimateAllocTemp(int chunk_size);
    
    /**
     * Decimate the source image by a factor of 2 with a [1,4,6,4,1] kernel.
     */
    void BinomialDecimate8u(unsigned char* dst,
                            const unsigned char* src,
                            unsigned short* tmp,
                            int src_width,
                            int src_height,
                            int src_step,
                            int dst_width,
                            int dst_step,
                            int chunk_size);
    
    /**
     * Decimate the source image by a factor of 2 with a box filter.
     *
     * How to calculate size of destination image:
     *
     * dst_width = CEIL((src_width-1)/2.)
     * dst_height = CEIL((src_height-1)/2.);
     */
    void BoxFilterDecimate(unsigned char* dst,
                           const unsigned char* src,
                           int src_width,
                           int src_height,
                           int src_step,
                           int dst_step);
    
    /**
     * Implements a [1 4 6 4 1] Binomial filter pyramid for image reduction. Each octave has
     * one scale.
     */
    class BinomialPyramid8u : public GaussianScaleSpacePyramid {
    public:
        
        BinomialPyramid8u();
        ~BinomialPyramid8u();
        
        /**
         * Initialize memory for the pyramid.
         */
        void init(int width, int height, int num_octaves, bool copy_fine);
        
        /**
         * Build the pyramid.
         */
        void build(const Image& image);
        
    private:
        
        // Temporary memory for downsampling
        std::auto_ptr<unsigned short> mTmp;
        
        // True if the fine image should be copied
        bool mCopyFine;
        
        // Chunk size for columns
        int mChunkSize;
    };
    
    /**
     * Implements a box filter pyramid for image reduction. Each octave has
     * one scale.
     */
    class BoxFilterPyramid8u : public GaussianScaleSpacePyramid {
    public:
        
        BoxFilterPyramid8u();
        ~BoxFilterPyramid8u();
        
        /**
         * Initialize memory for the pyramid.
         */
        void init(int width, int height, int num_octaves, bool copy_fine);
        
        /**
         * Build the pyramid.
         */
        void build(const Image& image);
        
    private:
        
        // True if the fine image should be copied
        bool mCopyFine;
    };
    
} // vision