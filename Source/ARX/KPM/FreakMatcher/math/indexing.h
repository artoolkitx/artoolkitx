//
//  indexing.h
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

#include <memory>
#include <cstddef>
#include <string.h>
namespace vision {

    template<typename T>
    inline void ZeroVector3(T* x) {
        memset(x, 0, sizeof(T)*3);
    }
    
    template<typename T>
    inline void ZeroVector(T* x, size_t num_elements) {
        memset(x, 0, sizeof(T)*num_elements);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // min/max
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    template<typename T>
    inline T max2(T a, T b) {
        return a > b ? a : b;
    }
    
    template<typename T>
    inline T min2(T a, T b) {
        return a < b ? a : b;
    }
    
	template<typename T>
	inline T min3(T x, T y, T z) {
		return min2(min2(x, y), z);
	}
	
	template<typename T>
	inline T min4(T a1, T a2, T a3, T a4) {
		return min2(min3(a1, a2, a3), a4);
	}
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MaxIndex
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    template<typename T>
    inline int MaxIndex2(const T x[2]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex3(const T x[3]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex4(const T x[4]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex5(const T x[5]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        if(x[4] > x[index]) index = 4;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex6(const T x[6]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        if(x[4] > x[index]) index = 4;
        if(x[5] > x[index]) index = 5;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex7(const T x[7]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        if(x[4] > x[index]) index = 4;
        if(x[5] > x[index]) index = 5;
        if(x[6] > x[index]) index = 6;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex8(const T x[8]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        if(x[4] > x[index]) index = 4;
        if(x[5] > x[index]) index = 5;
        if(x[6] > x[index]) index = 6;
        if(x[7] > x[index]) index = 7;
        return index;
    }
    
    template<typename T>
    inline int MaxIndex9(const T x[9]) {
        int index = 0;
        if(x[1] > x[index]) index = 1;
        if(x[2] > x[index]) index = 2;
        if(x[3] > x[index]) index = 3;
        if(x[4] > x[index]) index = 4;
        if(x[5] > x[index]) index = 5;
        if(x[6] > x[index]) index = 6;
        if(x[7] > x[index]) index = 7;
        if(x[8] > x[index]) index = 8;
        return index;
    }
    
    /**
     * Vector copy functions.
     */
    
    template<typename T>
    inline void CopyVector2(T dst[2], const T src[2]) {
        memcpy(dst, src, sizeof(T)*2);
    }
    template<typename T>
    inline void CopyVector3(T dst[3], const T src[3]) {
        memcpy(dst, src, sizeof(T)*3);
    }
    template<typename T>
    inline void CopyVector4(T dst[4], const T src[4]) {
        memcpy(dst, src, sizeof(T)*4);
    }
    template<typename T>
    inline void CopyVector5(T dst[5], const T src[5]) {
        memcpy(dst, src, sizeof(T)*5);
    }
    template<typename T>
    inline void CopyVector6(T dst[6], const T src[6]) {
        memcpy(dst, src, sizeof(T)*6);
    }
    template<typename T>
    inline void CopyVector7(T dst[7], const T src[7]) {
        memcpy(dst, src, sizeof(T)*7);
    }
    template<typename T>
    inline void CopyVector8(T dst[8], const T src[8]) {
        memcpy(dst, src, sizeof(T)*8);
    }
    template<typename T>
    inline void CopyVector9(T dst[9], const T src[9]) {
        memcpy(dst, src, sizeof(T)*9);
    }
    
    template<typename T>
    inline void CopyVector(T* dst, const T* src, size_t size) {
        memcpy(dst, src, sizeof(T)*size);
    }
    
    /**
     * Swap the contents of two vectors.
     */
    template<typename T>
    inline void Swap9(T a[9], T b[9]) {
        T tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
        tmp = *a; *a++ = *b; *b++ = tmp;
    }
    
    /**
     * Set a bit in a bit string represented as an array of UNSIGNED CHAR bytes. The ordering
     * on the bits is as follows:
     *
     * [7 6 5 4 3 2 1 0, 15 14 13 12 11 10 9 8, 23 22 21 20 19 18 17 15, ...] 
     */
    inline void bitstring_set_bit(unsigned char* bitstring, int pos, unsigned char bit) {
        bitstring[pos/8] |= (bit << (pos%8));
    }
    inline unsigned char bitstring_get_bit(const unsigned char* bitstring, int pos) {
        return (bitstring[pos/8] >> (pos%8)) & 1;
    }
    
    /**
     * Create a sequential vector {x0+0, x0+1, x0+2, ...}
     */
    template<typename T>
    inline void SequentialVector(T* x, int n, T x0) {
        if(n < 1) {
            return;
        }
        x[0] = x0;
        for(int i = 1; i < n; i++) {
            x[i] = x[i-1]+1;
        }
    }
    
} // vision
