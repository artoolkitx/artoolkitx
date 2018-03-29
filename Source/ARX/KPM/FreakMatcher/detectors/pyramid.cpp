//
//  pyramid.cpp
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

#include "pyramid.h"
#include "pyramid-inline.h"
#include <framework/error.h>
//#include <framework/logger.h>
#include <math/indexing.h>
#include <math/math_io.h>

using namespace vision;

namespace vision {
    
    /**
     * Swap the pointers in a "rolling shutter" style to decimate the image.
     */
    inline void SwapPointers(unsigned short** pm2,
                             unsigned short** pm1,
                             unsigned short** p,
                             unsigned short** pp1,
                             unsigned short** pp2) {
        unsigned short* tmp_pm2 = *pm2;
        unsigned short* tmp_pm1 = *pm1;
        *pm2 = *p;
        *pm1 = *pp1;
        *p   = *pp2;
        *pp1 = tmp_pm2;
        *pp2 = tmp_pm1;
    }
    
    /**
     * Reset the 5 row pointers.
     */
    inline void ResetPointers(unsigned short** pm2,
                              unsigned short** pm1,
                              unsigned short** p,
                              unsigned short** pp1,
                              unsigned short** pp2,
                              unsigned short* tmp,
                              int step) {
        *pm2 = &tmp[0];
        *pm1 = &tmp[step];
        *p   = &tmp[2*step];
        *pp1 = &tmp[3*step];
        *pp2 = &tmp[4*step];
    }
    
    /**
     * Decimate the left/right border pixels.
     */
    void BinomialVerticalBorderDecimateEvenWidth8u(unsigned char* dst,
                                                   const unsigned char* src,
                                                   int src_width,
                                                   int src_height,
                                                   int src_step,
                                                   int dst_width,
                                                   int dst_step) {
        
        ASSERT((src_width%2) == 0, "Source width must be even");
        ASSERT(dst != 0, "Destination is NULL");
        ASSERT(src != 0, "Source is NULL");
        ASSERT(src_width > 0, "Width must be positive");
        ASSERT(src_height > 0, "Height must be positive");
        ASSERT(src_step > 0, "Step must be positive");
        ASSERT(dst_width > 0, "Width must be positive");
        ASSERT(dst_step > 0, "Step must be positive");
        
        unsigned short* lpm2;
        unsigned short* lpm1;
        unsigned short* lp;
        unsigned short* lpp1;
        unsigned short* lpp2;
        unsigned short* rpm2;
        unsigned short* rpm1;
        unsigned short* rp;
        unsigned short* rpp1;
        unsigned short* rpp2;
        unsigned short tmp_left[5];
        unsigned short tmp_right[5];
        
        const unsigned char* src_left_ptr;
        const unsigned char* src_right_ptr;
        unsigned char* dst_left_ptr;
        unsigned char* dst_right_ptr;
        
        int height_minus_border = src_height-4;
        
        src_left_ptr  = src;
        src_right_ptr = src+(src_width-2);
        dst_left_ptr  = dst;
        dst_right_ptr = dst+(dst_width-1);
        
        ResetPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2, tmp_left, 1);
        ResetPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2, tmp_right, 1);
        
        *lp   = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *lpp2 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *dst_left_ptr = VerticalBinomial(*lp, *lp, *lp, *lpp1, *lpp2); dst_left_ptr += dst_step;
        
        *rp   = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]); src_right_ptr += src_step;
        *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]); src_right_ptr += src_step;
        *rpp2 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]); src_right_ptr += src_step;
        *dst_right_ptr = VerticalBinomial(*rp, *rp, *rp, *rpp1, *rpp2); dst_right_ptr += dst_step;
        
        for(int i = 0; i < height_minus_border; i+=2) {
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
            *lpp2 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lpp1, *lpp2); dst_left_ptr += dst_step;
            
            *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]); src_right_ptr += src_step;
            *rpp2 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]); src_right_ptr += src_step;
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rpp1, *rpp2); dst_right_ptr += dst_step;
        }
        
        if(src_height%2 == 0) { // EVEN height
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]);
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lpp1, *lpp1);
            
            *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[1], src_right_ptr[1]);
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rpp1, *rpp1);
        } else { // ODD height
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lp, *lp);
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rp, *rp);
        }
    }
    
    /**
     * Decimate the left/right border pixels.
     */
    void BinomialVerticalBorderDecimateOddWidth8u(unsigned char* dst,
                                                  const unsigned char* src,
                                                  int src_width,
                                                  int src_height,
                                                  int src_step,
                                                  int dst_width,
                                                  int dst_step) {
        
        ASSERT((src_width%2) == 1, "Source width must be odd");
        ASSERT(dst != 0, "Destination is NULL");
        ASSERT(src != 0, "Source is NULL");
        ASSERT(src_width > 0, "Width must be positive");
        ASSERT(src_height > 0, "Height must be positive");
        ASSERT(src_step > 0, "Step must be positive");
        ASSERT(dst_width > 0, "Width must be positive");
        ASSERT(dst_step > 0, "Step must be positive");
        
        unsigned short* lpm2;
        unsigned short* lpm1;
        unsigned short* lp;
        unsigned short* lpp1;
        unsigned short* lpp2;
        unsigned short* rpm2;
        unsigned short* rpm1;
        unsigned short* rp;
        unsigned short* rpp1;
        unsigned short* rpp2;
        unsigned short tmp_left[5];
        unsigned short tmp_right[5];
        
        const unsigned char* src_left_ptr;
        const unsigned char* src_right_ptr;
        unsigned char* dst_left_ptr;
        unsigned char* dst_right_ptr;
        
        int height_minus_border = src_height-4;
        
        src_left_ptr  = src;
        src_right_ptr = src+(src_width-1);
        dst_left_ptr  = dst;
        dst_right_ptr = dst+(dst_width-1);
        
        ResetPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2, tmp_left, 1);
        ResetPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2, tmp_right, 1);
        
        *lp   = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *lpp2 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
        *dst_left_ptr = VerticalBinomial(*lp, *lp, *lp, *lpp1, *lpp2); dst_left_ptr += dst_step;
        
        *rp   = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]); src_right_ptr += src_step;
        *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]); src_right_ptr += src_step;
        *rpp2 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]); src_right_ptr += src_step;
        *dst_right_ptr = VerticalBinomial(*rp, *rp, *rp, *rpp1, *rpp2); dst_right_ptr += dst_step;
        
        for(int i = 0; i < height_minus_border; i+=2) {
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
            *lpp2 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]); src_left_ptr += src_step;
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lpp1, *lpp2); dst_left_ptr += dst_step;
            
            *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]); src_right_ptr += src_step;
            *rpp2 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]); src_right_ptr += src_step;
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rpp1, *rpp2); dst_right_ptr += dst_step;
        }
        
        if(src_height%2 == 0) { // EVEN height
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *lpp1 = HorizontalBinomial(src_left_ptr[0], src_left_ptr[0], src_left_ptr[0], src_left_ptr[1], src_left_ptr[2]);
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lpp1, *lpp1);
            
            *rpp1 = HorizontalBinomial(src_right_ptr[-2], src_right_ptr[-1], src_right_ptr[0], src_right_ptr[0], src_right_ptr[0]);
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rpp1, *rpp1);
        } else { // ODD height
            SwapPointers(&lpm2, &lpm1, &lp, &lpp1, &lpp2);
            SwapPointers(&rpm2, &rpm1, &rp, &rpp1, &rpp2);
            
            *dst_left_ptr = VerticalBinomial(*lpm2, *lpm1, *lp, *lp, *lp);
            *dst_right_ptr = VerticalBinomial(*rpm2, *rpm1, *rp, *rp, *rp);
        }
    }
    
    unsigned short* BinomialDecimateAllocTemp(int chunk_size) {
        ASSERT((chunk_size%2)==0, "Untested behavior for odd chunk size");
        return new unsigned short[(chunk_size>>1)*5];
    }
    
    void BinomialDecimate8u(unsigned char* dst,
                            const unsigned char* src,
                            unsigned short* tmp,
                            int src_width,
                            int src_height,
                            int src_step,
                            int dst_width,
                            int dst_step,
                            int chunk_size) {
        
        ASSERT(dst != 0, "Destination is NULL");
        ASSERT(src != 0, "Source is NULL");
        ASSERT(src_width > 0, "Width must be positive");
        ASSERT(src_height > 0, "Height must be positive");
        ASSERT(src_step > 0, "Step must be positive");
        ASSERT(dst_width > 0, "Width must be positive");
        ASSERT(dst_step > 0, "Step must be positive");
        
        unsigned short* pm2;
        unsigned short* pm1;
        unsigned short* p;
        unsigned short* pp1;
        unsigned short* pp2;
        
        const unsigned char* src_ptr;
        unsigned char* dst_ptr;
        
        // The border of the image is processed seperately. Here we want the width/height of the image
        // minus the border. 
        int width_minus_border = src_width-4;
        int height_minus_border = src_height-4;

        int num_chunks = std::floor((float)width_minus_border/(float)chunk_size);
        int remaining_size = width_minus_border%chunk_size;
        int decimated_chunk_size = chunk_size>>1;
        int decimated_remaining_size = std::ceil(remaining_size/2.f);
        
        for(int i = 0; i < num_chunks; i++) {
            src_ptr = src+(i*chunk_size)+2;
            dst_ptr = dst+(i*decimated_chunk_size)+1;
            
            ResetPointers(&pm2, &pm1, &p, &pp1, &pp2, tmp, decimated_chunk_size);
            
            // Process the first 3 rows
            HorizontalBinomialDecimate(p,   src_ptr, chunk_size); src_ptr +=src_step;
            HorizontalBinomialDecimate(pp1, src_ptr, chunk_size); src_ptr +=src_step;
            HorizontalBinomialDecimate(pp2, src_ptr, chunk_size); src_ptr +=src_step;
            VerticalBinomial(dst_ptr, p, p, p, pp1, pp2, decimated_chunk_size); dst_ptr += dst_step;
            
            // Non-border rows
            for(int j = 0; j < height_minus_border; j+=2) {
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                HorizontalBinomialDecimate(pp1, src_ptr, chunk_size); src_ptr +=src_step;
                HorizontalBinomialDecimate(pp2, src_ptr, chunk_size); src_ptr +=src_step;
                VerticalBinomial(dst_ptr, pm2, pm1, p, pp1, pp2, decimated_chunk_size); dst_ptr += dst_step;
            }
            
            if(src_height%2 == 0) { // EVEN height
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                HorizontalBinomialDecimate(pp1, src_ptr, chunk_size);
                VerticalBinomial(dst_ptr, pm2, pm1, p, pp1, pp1, decimated_chunk_size);
            } else { // ODD height
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                VerticalBinomial(dst_ptr, pm2, pm1, p, p, p, decimated_chunk_size);
            }
        }

        // Remaining bytes along column
        if(remaining_size > 0) {
            src_ptr = src+(num_chunks*chunk_size)+2;
            dst_ptr = dst+(num_chunks*decimated_chunk_size)+1;
            
            ResetPointers(&pm2, &pm1, &p, &pp1, &pp2, tmp, decimated_chunk_size);
            
            // Process the first 3 rows
            HorizontalBinomialDecimate(p,   src_ptr, remaining_size); src_ptr +=src_step;
            HorizontalBinomialDecimate(pp1, src_ptr, remaining_size); src_ptr +=src_step;
            HorizontalBinomialDecimate(pp2, src_ptr, remaining_size); src_ptr +=src_step;
            VerticalBinomial(dst_ptr, p, p, p, pp1, pp2, decimated_remaining_size); dst_ptr += dst_step;
            
            // Non-border rows
            for(int i = 0; i < height_minus_border; i+=2) {
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                HorizontalBinomialDecimate(pp1, src_ptr, remaining_size); src_ptr +=src_step;
                HorizontalBinomialDecimate(pp2, src_ptr, remaining_size); src_ptr +=src_step;
                VerticalBinomial(dst_ptr, pm2, pm1, p, pp1, pp2, decimated_remaining_size); dst_ptr += dst_step;
            }
            
            if(src_height%2 == 0) {
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                HorizontalBinomialDecimate(pp1, src_ptr, remaining_size);
                VerticalBinomial(dst_ptr, pm2, pm1, p, pp1, pp1, decimated_remaining_size);
            } else { // ODD height
                SwapPointers(&pm2, &pm1, &p, &pp1, &pp2);
                VerticalBinomial(dst_ptr, pm2, pm1, p, p, p, decimated_remaining_size);
            }
        }
        
        if(src_width%2 == 0) {
            BinomialVerticalBorderDecimateEvenWidth8u(dst, src, src_width, src_height, src_step, dst_width, dst_step);
        } else {
            BinomialVerticalBorderDecimateOddWidth8u(dst, src, src_width, src_height, src_step, dst_width, dst_step);
        }
    }
    
    void BoxFilterDecimate(unsigned char* dst,
                           const unsigned char* src,
                           int src_width,
                           int src_height,
                           int src_step,
                           int dst_step) {
        
        ASSERT(dst != 0, "Destination is NULL");
        ASSERT(src != 0, "Source is NULL");
        ASSERT(src_width > 0, "Width is zero");
        ASSERT(src_height > 0, "Height is zero");
        ASSERT(src_step > 0, "Step is zero");
        ASSERT(dst_step > 0, "Step is zero");
        
        unsigned short tmp1[64];
        unsigned short tmp2[64];
        
        // The image is split into columns of 128 bytes. Here we calculate the number
        // of columns and the remaining bytes. Images that are smaller than 128 bytes will
        // be processed in 1 column equal to the width of the image.
        int num_c = src_width/128;
        int num_r = src_width%128;
        
        int src_height_minus_1 = src_height-1;
        for(int i = 0; i < src_height_minus_1; i+=2) {
            const unsigned char* src_ptr = src+src_step*i;
            ASSERT((i>>1) < std::ceil((src_height-1)/2.f), "Index is out of bounds");
            unsigned char* dst_ptr = dst+dst_step*(i>>1);
            
            // 128 byte column
            for(int j = 0; j < num_c; j++, src_ptr+=128, dst_ptr+=64) {
                HorizontalBoxFilterDecimate128(tmp1, src_ptr);
                HorizontalBoxFilterDecimate128(tmp2, src_ptr+src_step);
                VerticalBoxFilter64(dst_ptr, tmp1, tmp2);
            }
            // Remaining bytes
            HorizontalBoxFilterDecimate(tmp1, src_ptr, num_r);
            HorizontalBoxFilterDecimate(tmp2, src_ptr+src_step, num_r);
            VerticalBoxFilter(dst_ptr, tmp1, tmp2, num_r>>1);
        }
    }
    
}

BinomialPyramid8u::BinomialPyramid8u()
: mCopyFine(true)
, mChunkSize(128) {
    mTmp.reset(BinomialDecimateAllocTemp(mChunkSize));
}

BinomialPyramid8u::~BinomialPyramid8u() {}

void BinomialPyramid8u::init(int width, int height, int num_octaves, bool copy_fine) {
    //LOG_DEBUG("Binomial pyramid allocating memory for: w = %u, h = %u, levels = %u",              width, height, num_octaves);
    
    GaussianScaleSpacePyramid::configure(num_octaves, 1);
    
    // Allocate the pyramid memory
    mPyramid.resize(num_octaves);
    mPyramid[0].alloc(IMAGE_UINT8, width, height, AUTO_STEP, 1);
    for(int i = 1; i < num_octaves; i++) {
        mPyramid[i].alloc(IMAGE_UINT8,
                          std::ceil(mPyramid[i-1].width()/2.),
                          std::ceil(mPyramid[i-1].height()/2.),
                          AUTO_STEP,
                          1);
    }
    
    mCopyFine = copy_fine;
}

void BinomialPyramid8u::build(const Image& image) {
    ASSERT(image.type() == IMAGE_UINT8, "Only gray scale images are supported");
    
    Image& octave0 = get(0, 0);
    if(mCopyFine) {
        octave0.deepCopy(image);
    } else {
        octave0 = Image((unsigned char*)image.get(),
                        image.type(),
                        image.width(),
                        image.height(),
                        (int)image.step(),
                        image.channels());
    }
    
    for(int i = 1; i < mNumOctaves; i++) {
        Image& dst = get(i,0);
        const Image& src = get(i-1,0);
        BinomialDecimate8u(dst.get(),
                           src.get(),
                           mTmp.get(),
                           (int)src.width(),
                           (int)src.height(),
                           (int)src.step(),
                           (int)dst.width(),
                           (int)dst.step(),
                           mChunkSize);
    }
}

BoxFilterPyramid8u::BoxFilterPyramid8u() : mCopyFine(true) {}

BoxFilterPyramid8u::~BoxFilterPyramid8u() {}

void BoxFilterPyramid8u::init(int width, int height, int num_octaves, bool copy_fine) {
    //LOG_DEBUG("Box pyramid allocating memory for: w = %u, h = %u, levels = %u",              width, height, num_octaves);
    
    GaussianScaleSpacePyramid::configure(num_octaves, 1);
    
    // Allocate the pyramid memory
    mPyramid.resize(num_octaves);
    mPyramid[0].alloc(IMAGE_UINT8, width, height, AUTO_STEP, 1);
    for(int i = 1; i < num_octaves; i++) {
        mPyramid[i].alloc(IMAGE_UINT8,
                          std::ceil((mPyramid[i-1].width()-1)/2.),
                          std::ceil((mPyramid[i-1].height()-1)/2.),
                          AUTO_STEP,
                          1);
    }
    
    mCopyFine = copy_fine;
}

void BoxFilterPyramid8u::build(const Image& image) {
    ASSERT(image.type() == IMAGE_UINT8, "Only gray scale images are supported");

    Image& octave0 = get(0, 0);
    if(mCopyFine) {
        octave0.deepCopy(image);
    } else {
        octave0 = Image((unsigned char*)image.get(),
                        image.type(),
                        image.width(),
                        image.height(),
                        (int)image.step(),
                        image.channels());
    }
    
    for(int i = 1; i < mNumOctaves; i++) {
        Image& dst = get(i,0);
        const Image& src = get(i-1,0);
        BoxFilterDecimate(dst.get(),
                          src.get(),
                          (int)src.width(),
                          (int)src.height(),
                          (int)src.step(),
                          (int)dst.step());
    }
}
