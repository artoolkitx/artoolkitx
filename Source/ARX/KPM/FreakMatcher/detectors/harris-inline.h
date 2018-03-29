//
//  harris-inline.h
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

#include "math/math_utils.h"
#include "framework/error.h"

#if __ARM_NEON__
#include <arm_neon.h>
#endif

namespace vision {
    
    /**
     * Apply horizontal binomial.
     */
    inline int HorizontalBinomial(short xm2,
                                  short xm1,
                                  short x,
                                  short xp1,
                                  short xp2) {
        return xm2 + (xm1<<2) + ((x<<1) + (x<<2)) + (xp1<<2) + xp2;
    }
    
    /**
     * Compute the binomial response an input vector. This function assumes there is a 
     * two pixel padding on the left and right sides.
     */
    inline void HorizontalBinomial(int* dst, const short* src, int n) {
        for(int i = 0; i < n; i++, src++) {
            dst[i] = HorizontalBinomial(src[-2], src[-1], src[0], src[1], src[2]);
            ASSERT(dst[i] >= -258064, "Out of range");
            ASSERT(dst[i] <= 258064, "Out of range");
        }
    }
    
    /**
     * Apply vertical binomial on a pixel.
     */
    inline int VerticalBinomial(int xm2,
                                int xm1,
                                int x,
                                int xp1,
                                int xp2) {
        return xm2 + (xm1<<2) + ((x<<1) + (x<<2)) + (xp1<<2) + xp2;
    }
    
    /**
     * Apply vertical binomial.
     */
    inline void VerticalBinomial(int* dst,
                                 const int* pm2,
                                 const int* pm1,
                                 const int* p,
                                 const int* pp1,
                                 const int* pp2,
                                 int n) {
        for(int i = 0; i < n; i++) {
            dst[i] = VerticalBinomial(pm2[i], pm1[i], p[i], pp1[i], pp2[i]);
            ASSERT(dst[i] <= 4129024, "Out of range");
            ASSERT(dst[i] >= -4129024, "Out of range");
        }
    }
    
    /**
     * Compute (Ixx, Iyy, Ixy) given 3 rows.
     */
    inline void ComputeDerivatives(short* Ixx,
                                   short* Iyy,
                                   short* Ixy,
                                   const unsigned char* pm1,
                                   const unsigned char* p,
                                   const unsigned char* pp1,
                                   int n) {
        for(int i = 0; i < n; i++, pm1++, p++, pp1++) {
            // [-1,0,1]
            short Ix = (-p[-1] + p[1])/2;
            short Iy = (-pm1[0] + pp1[0])/2;
            
            ASSERT(Ix <= 127, "Out of range");
            ASSERT(Ix >= -127, "Out of range");
            ASSERT(Iy <= 127, "Out of range");
            ASSERT(Iy >= -127, "Out of range");
            
            Ixx[i] = sqr(Ix);
            Iyy[i] = sqr(Iy);
            Ixy[i] = Ix*Iy;
        }
    }
    
    /**
     * Compute the horizontal Harris stengths.
     */
    template<typename T>
    inline void HorizontalHarrisStengths(T* S,
                                         const int* Gxx,
                                         const int* Gyy,
                                         const int* Gxy,
                                         int n,
                                         T k) {
        for(int i = 0; i < n; i++) {
            T gxx = (T)Gxx[i];
            T gyy = (T)Gyy[i];
            T gxy = (T)Gxy[i];
            
            ASSERT(gxx <= 4129024, "Out of range");
            ASSERT(gxx >= 0, "Out of range");
            ASSERT(gyy <= 4129024, "Out of range");
            ASSERT(gyy >= 0, "Out of range");
            ASSERT(gxy <= 4129024, "Out of range");
            ASSERT(gxy >= -4129024, "Out of range");
            
            // NOTE: These numbers tend to be very large. Some rounding
            // error will occur.
            S[i] = (gxx*gyy-sqr(gxy)) - k*sqr(gxx+gyy);
        }
    }
    
} // vision

#if __ARM_NEON__

extern "C" {
    
void HorizontalBinomialNeon(int * pDst, short * pSrc, unsigned int count);

void VerticalBinomialNeon(int* __restrict dst,
                          int* __restrict pm2,
                          int* __restrict pm1,
                          int* __restrict p,
                          int* __restrict pp1,
                          int* __restrict pp2,
                          unsigned int count);
    
void ComputeDerivativesNeon(short* __restrict Ixx,
                            short* __restrict Iyy,
                            short* __restrict Ixy,
                            const unsigned char* __restrict pm1,
                            const unsigned char* __restrict p,
                            const unsigned char* __restrict pp1,
                            int count);

void HorizontalHarrisStengthsNeon(float* S,
                                  const int* Gxx,
                                  const int* Gyy,
                                  const int* Gxy,
                                  int count,
                                  float k);
}

#endif
