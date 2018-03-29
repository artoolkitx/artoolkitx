//
//  pyramid-inline.h
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

namespace vision {
    
    /**
     * Apply horizontal binomial.
     */
    inline unsigned short HorizontalBinomial(unsigned char xm2,
                                             unsigned char xm1,
                                             unsigned char x,
                                             unsigned char xp1,
                                             unsigned char xp2) {
        return xm2 + (xm1<<2) + ((x<<1) + (x<<2)) + (xp1<<2) + xp2;
    }
    
    /**
     * Apply vertical binomial.
     */
    inline unsigned char VerticalBinomial(unsigned short xm2,
                                          unsigned short xm1,
                                          unsigned short x,
                                          unsigned short xp1,
                                          unsigned short xp2) {
        int v = xm2 + (xm1<<2) + ((x<<1) + (x<<2)) + (xp1<<2) + xp2 + 128; // add 128 for rounding
        return v>>8;
    }
    
    /**
     * Binomial decimate (n*2)+4 bytes into n bytes. This function is centered around the nth pixel, so
     * that there must be 2 valid pixels to the left and right.
     */
    inline void HorizontalBinomialDecimate(unsigned short* dst, const unsigned char* ptr, int n) {
        for(int i = 0; i < n; i+=2, dst++, ptr+=2) {
            *dst = HorizontalBinomial(ptr[-2], ptr[-1], ptr[0], ptr[1], ptr[2]);
        }
    }
    
    /**
     * Vertical binomial blur of 62 shorts into 62 bytes.
     */
    inline void VerticalBinomial62(unsigned char dst[62],
                                   const unsigned short pm2[62],
                                   const unsigned short pm1[62],
                                   const unsigned short p[62],
                                   const unsigned short pp1[62],
                                   const unsigned short pp2[62]) {
        for(int i = 0; i < 62; i++) {
            dst[i] = VerticalBinomial(pm2[i], pm1[i], p[i], pp1[i], pp2[i]);
        }
    }
    inline void VerticalBinomial(unsigned char* dst,
                                   const unsigned short* pm2,
                                   const unsigned short* pm1,
                                   const unsigned short* p,
                                   const unsigned short* pp1,
                                   const unsigned short* pp2,
                                   int n) {
        for(int i = 0; i < n; i++) {
            dst[i] = VerticalBinomial(pm2[i], pm1[i], p[i], pp1[i], pp2[i]);
        }
    }
    
    /**
     * Horizontal box filter decimate.
     */
    inline void HorizontalBoxFilterDecimate128(unsigned short dst[64], const unsigned char ptr[128]) {
        for(int i = 0; i < 128; i+=2, dst++, ptr+=2) {
            *dst = ptr[0]+ptr[1];
        }
    }
    inline void HorizontalBoxFilterDecimate(unsigned short* dst, const unsigned char* ptr, int n) {
        for(int i = 0; i < n; i+=2, dst++, ptr+=2) {
            *dst = ptr[0]+ptr[1];
        }
    }
    
    /**
     * Vertical box filter.
     */
    inline void VerticalBoxFilter64(unsigned char dst[64],
                                    const unsigned short p1[64],
                                    const unsigned short p2[64]) {
        for(int i = 0; i < 64; i++) {
            /*register*/ int v = p1[i]+p2[i]+2; // add 2 for rounding
            dst[i] = v>>2;
        }
    }
    inline void VerticalBoxFilter(unsigned char* dst,
                                  const unsigned short* p1,
                                  const unsigned short* p2,
                                  int n) {
        for(int i = 0; i < n; i++) {
            /*register*/ int v = p1[i]+p2[i]+2; // add 2 for rounding
            dst[i] = v>>2;
        }
    }
    
} // vision