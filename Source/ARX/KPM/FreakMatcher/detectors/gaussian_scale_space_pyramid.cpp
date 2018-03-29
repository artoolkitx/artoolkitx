//
//  gaussian_scale_space_pyramid.cpp
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

#include "gaussian_scale_space_pyramid.h"
#include <framework/error.h>
//#include <framework/logger.h>

using namespace vision;

namespace vision {

    void binomial_4th_order(float* dst,
                            unsigned short* tmp,
                            const unsigned char* src,
                            size_t width,
                            size_t height) {
        unsigned short* tmp_ptr;
        float* dst_ptr;
        
        size_t width_minus_1, width_minus_2;
        size_t height_minus_2;
        
        ASSERT(width >= 5, "Image is too small");
        ASSERT(height >= 5, "Image is too small");
        
        width_minus_1 = width-1;
        width_minus_2 = width-2;
        height_minus_2 = height-2;
        
        tmp_ptr = tmp;
        
        // Apply horizontal filter
        for(size_t row = 0; row < height; row++) {
            const unsigned char* src_ptr = &src[row*width];
            
            // Left border is computed by extending the border pixel beyond the image
            *(tmp_ptr++) = ((src_ptr[0]<<1)+(src_ptr[0]<<2)) + ((src_ptr[0]+src_ptr[1])<<2) + (src_ptr[0]+src_ptr[2]);
            *(tmp_ptr++) = ((src_ptr[1]<<1)+(src_ptr[1]<<2)) + ((src_ptr[0]+src_ptr[2])<<2) + (src_ptr[0]+src_ptr[3]);
            
            // Compute non-border pixels
            for(size_t col = 2; col < width_minus_2; col++, tmp_ptr++) {
                *tmp_ptr = ((src_ptr[col]<<1)+(src_ptr[col]<<2)) + ((src_ptr[col-1]+src_ptr[col+1])<<2) + (src_ptr[col-2]+src_ptr[col+2]);
            }
            
            // Right border. Computed similarily as the left border.
            *(tmp_ptr++) = ((src_ptr[width_minus_2]<<1)+(src_ptr[width_minus_2]<<2)) + ((src_ptr[width_minus_2-1]+src_ptr[width_minus_2+1])<<2) + (src_ptr[width_minus_2-2]+src_ptr[width_minus_2+1]);
            *(tmp_ptr++) = ((src_ptr[width_minus_1]<<1)+(src_ptr[width_minus_1]<<2)) + ((src_ptr[width_minus_1-1]+src_ptr[width_minus_1])<<2)   + (src_ptr[width_minus_1-2]+src_ptr[width_minus_1]);
        }
        
        const unsigned short* pm2;
        const unsigned short* pm1;
        const unsigned short* p;
        const unsigned short* pp1;
        const unsigned short* pp2;
        
        // Apply vertical filter along top border. This is applied twice as there are two
        // border pixels.
        pm2     = tmp;
        pm1     = tmp;
        p       = tmp;
        pp1     = p+width;
        pp2     = pp1+width;
        dst_ptr = dst;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (((*p<<1)+(*p<<2)) + ((*pm1+*pp1)<<2) + (*pm2+*pp2))*(1.f/256.f);
        }
        
        pm2     = tmp;
        pm1     = tmp;
        p       = tmp+width;
        pp1     = p+width;
        pp2     = pp1+width;
        dst_ptr = dst+width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (((*p<<1)+(*p<<2)) + ((*pm1+*pp1)<<2) + (*pm2+*pp2))*(1.f/256.f);
        }
        
        // Apply vertical filter for non-border pixels.
        pm2 = tmp;
        pm1 = pm2+width;
        p   = pm1+width;
        pp1 = p+width;
        pp2 = pp1+width;
        
        for(size_t row = 2; row < height_minus_2; row++) {
            pm2 = &tmp[(row-2)*width];
            pm1 = pm2+width;
            p   = pm1+width;
            pp1 = p+width;
            pp2 = pp1+width;
            
            dst_ptr = &dst[row*width];
            
            for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
                *dst_ptr = (((*p<<1)+(*p<<2)) + ((*pm1+*pp1)<<2) + (*pm2+*pp2))*(1.f/256.f);
            }
        }
        
        // Apply vertical filter for bottom border. Similar to top border.
        pm2     = tmp+(height-4)*width;
        pm1     = pm2+width;
        p       = pm1+width;
        pp1     = p+width;
        pp2     = pp1;
        dst_ptr = dst+(height-2)*width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (((*p<<1)+(*p<<2)) + ((*pm1+*pp1)<<2) + (*pm2+*pp2))*(1.f/256.f);
        }
        
        pm2     = tmp+(height-3)*width;
        pm1     = pm2+width;
        p       = pm1+width;
        pp1     = p;
        pp2     = p;
        dst_ptr = dst+(height-1)*width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (((*p<<1)+(*p<<2)) + ((*pm1+*pp1)<<2) + (*pm2+*pp2))*(1.f/256.f);
        }
    }
    
    void binomial_4th_order(float* dst,
                            float* tmp,
                            const float* src,
                            size_t width,
                            size_t height) {
        float* tmp_ptr;
        float* dst_ptr;
        
        size_t width_minus_1, width_minus_2;
        size_t height_minus_2;
        
        ASSERT(width >= 5, "Image is too small");
        ASSERT(height >= 5, "Image is too small");
        
        width_minus_1 = width-1;
        width_minus_2 = width-2;
        height_minus_2 = height-2;
        
        tmp_ptr = tmp;
        
        // Apply horizontal filter
        for(size_t row = 0; row < height; row++) {
            const float* src_ptr = &src[row*width];
            
            // Left border is computed by extending the border pixel beyond the image
            *(tmp_ptr++) = 6.f*src_ptr[0] + 4.f*(src_ptr[0]+src_ptr[1]) + src_ptr[0] + src_ptr[2];
            *(tmp_ptr++) = 6.f*src_ptr[1] + 4.f*(src_ptr[0]+src_ptr[2]) + src_ptr[0] + src_ptr[3];
            
            // Compute non-border pixels
            for(size_t col = 2; col < width_minus_2; col++, tmp_ptr++) {
                *tmp_ptr = (6.f*src_ptr[col] + 4.f*(src_ptr[col-1]+src_ptr[col+1]) + src_ptr[col-2] + src_ptr[col+2]);
            }
            
            // Right border. Computed similarily as the left border.
            *(tmp_ptr++) = 6.f*src_ptr[width_minus_2] + 4.f*(src_ptr[width_minus_2-1]+src_ptr[width_minus_2+1]) + src_ptr[width_minus_2-2] + src_ptr[width_minus_2+1];
            *(tmp_ptr++) = 6.f*src_ptr[width_minus_1] + 4.f*(src_ptr[width_minus_1-1]+src_ptr[width_minus_1])   + src_ptr[width_minus_1-2] + src_ptr[width_minus_1];
        }
        
        const float* pm2;
        const float* pm1;
        const float* p;
        const float* pp1;
        const float* pp2;
        
        // Apply vertical filter along top border. This is applied twice as there are two
        // border pixels.
        pm2     = tmp;
        pm1     = tmp;
        p       = tmp;
        pp1     = p+width;
        pp2     = pp1+width;
        dst_ptr = dst;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (6.f*(*p) + 4.f*(*pm1+*pp1) + (*pm2) + (*pp2))*(1.f/256.f);;
        }
        
        pm2     = tmp;
        pm1     = tmp;
        p       = tmp+width;
        pp1     = p+width;
        pp2     = pp1+width;
        dst_ptr = dst+width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (6.f*(*p) + 4.f*(*pm1+*pp1) + (*pm2) + (*pp2))*(1.f/256.f);;
        }
        
        // Apply vertical filter for non-border pixels.
        for(size_t row = 2; row < height_minus_2; row++) {
            pm2 = &tmp[(row-2)*width];
            pm1 = pm2+width;
            p   = pm1+width;
            pp1 = p+width;
            pp2 = pp1+width;
            
            dst_ptr = &dst[row*width];
            
            for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
                *dst_ptr = (6.f*(*p) + 4.f*(*pm1+*pp1) + (*pm2) + (*pp2))*(1.f/256.f);;
            }
        }
        
        // Apply vertical filter for bottom border. Similar to top border.
        pm2     = tmp+(height-4)*width;
        pm1     = pm2+width;
        p       = pm1+width;
        pp1     = p+width;
        pp2     = pp1;
        dst_ptr = dst+(height-2)*width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (6.f*(*p) + 4.f*(*pm1+*pp1) + (*pm2) + (*pp2))*(1.f/256.f);;
        }
        
        pm2     = tmp+(height-3)*width;
        pm1     = pm2+width;
        p       = pm1+width;
        pp1     = p;
        pp2     = p;
        dst_ptr = dst+(height-1)*width;
        
        for(size_t col = 0; col < width; col++, dst_ptr++, pm2++, pm1++, p++, pp1++, pp2++) {
            *dst_ptr = (6.f*(*p) + 4.f*(*pm1+*pp1) + (*pm2) + (*pp2))*(1.f/256.f);;
        }
    }
    
    void downsample_bilinear(float* dst, const float* src, size_t src_width, size_t src_height) {
        size_t dst_width;
        size_t dst_height;
        const float* src_ptr1;
        const float* src_ptr2;
        
        dst_width = src_width>>1;
        dst_height = src_height>>1;
        
        for(size_t row = 0; row < dst_height; row++) {
            src_ptr1 = &src[(row<<1)*src_width];
            src_ptr2 = src_ptr1 + src_width;
            for(size_t col = 0; col < dst_width; col++, src_ptr1+=2, src_ptr2+=2) {
                *(dst++) = (src_ptr1[0]+src_ptr1[1]+src_ptr2[0]+src_ptr2[1])*0.25f;
            }
        }
    }
    
}

GaussianScaleSpacePyramid::GaussianScaleSpacePyramid()
: mNumOctaves(0)
, mNumScalesPerOctave(0)
, mK(0)
, mOneOverLogK(0)
{}

void GaussianScaleSpacePyramid::configure(int num_octaves, int num_scales_per_octaves) {
    mNumOctaves = num_octaves;
    mNumScalesPerOctave = num_scales_per_octaves;
    mK = std::pow(2.f, 1.f/(mNumScalesPerOctave-1));
    mOneOverLogK = 1.f/std::log(mK);
}

BinomialPyramid32f::BinomialPyramid32f() {
}

BinomialPyramid32f::~BinomialPyramid32f()
{}

void BinomialPyramid32f::alloc(size_t width,
                               size_t height,
                               int num_octaves) {
    //LOG_DEBUG("Binomial pyramid allocating memory for: w = %u, h = %u, levels = %u",              width, height, num_octaves);
    
    GaussianScaleSpacePyramid::configure(num_octaves, 3);
    
    // Allocate the pyramid memory
    mPyramid.resize(num_octaves*mNumScalesPerOctave);
    for(int i = 0; i < num_octaves; i++) {
        for(size_t j = 0; j < mNumScalesPerOctave; j++) {
            mPyramid[i*mNumScalesPerOctave+j].alloc(IMAGE_F32, width>>i, height>>i, AUTO_STEP, 1);
        }
    }
    
    mTemp_us16.resize(width*height);
    mTemp_f32_1.resize(width*height);
    mTemp_f32_2.resize(width*height);
    
}

void BinomialPyramid32f::release() {
    mPyramid.clear();
}

void BinomialPyramid32f::build(const Image& image) {
    ASSERT(image.type() == IMAGE_UINT8, "Image must be grayscale");
    ASSERT(image.channels() == 1, "Image must have 1 channel");
    
    ASSERT(mPyramid.size() == mNumOctaves*mNumScalesPerOctave, "Pyramid has not been allocated yet");
    ASSERT(image.width() == mPyramid[0].width(), "Image of wrong size for pyramid");
    ASSERT(image.height() == mPyramid[0].height(), "Image of wrong size for pyramid");
    
    // First octave
    apply_filter(mPyramid[0], image);
    apply_filter(mPyramid[1], mPyramid[0]);
    apply_filter_twice(mPyramid[2], mPyramid[1]);
    
    // Remaining octaves
    for(size_t i = 1; i < mNumOctaves; i++) {
        // Downsample
        downsample_bilinear((float*)mPyramid[i*mNumScalesPerOctave].get(),
                            (const float*)mPyramid[i*mNumScalesPerOctave-1].get(),
                            mPyramid[i*mNumScalesPerOctave-1].width(),
                            mPyramid[i*mNumScalesPerOctave-1].height());
        
        // Apply binomial filters
        apply_filter(mPyramid[i*mNumScalesPerOctave+1], mPyramid[i*mNumScalesPerOctave]);
        apply_filter_twice(mPyramid[i*mNumScalesPerOctave+2], mPyramid[i*mNumScalesPerOctave+1]);
    }
}

void BinomialPyramid32f::apply_filter(Image& dst, const Image& src) {
    ASSERT(dst.type() == IMAGE_F32, "Destination image should be a float");
    
    switch(src.type()) {
        case IMAGE_UINT8:
            binomial_4th_order((float*)dst.get(),
                               &mTemp_us16[0],
                               (const unsigned char*)src.get(),
                               src.width(),
                               src.height());
            break;
        case IMAGE_F32:
            binomial_4th_order((float*)dst.get(),
                               &mTemp_f32_1[0],
                               (const float*)src.get(),
                               src.width(),
                               src.height());
            break;
        case IMAGE_UNKNOWN:
            throw EXCEPTION("Unknown image type");
        default:
            throw EXCEPTION("Unsupported image type");
    }
}

void BinomialPyramid32f::apply_filter_twice(Image& dst, const Image& src) {
    Image tmp((unsigned char*)&mTemp_f32_2[0], src.type(), src.width(), src.height(), (int)src.step(), 1);
    apply_filter(tmp, src);
    apply_filter(dst, tmp);
}
