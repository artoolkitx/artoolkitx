//
//  hamming.h
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
     * Hamming distance for 32 bits.
     */
    inline unsigned int HammingDistance32(unsigned int a, unsigned int b) {
        const unsigned int m1  = 0x55555555; // 0101...
        const unsigned int m2  = 0x33333333; // 00110011..
        const unsigned int m4  = 0x0f0f0f0f; // 4 zeros,  4 ones
        const unsigned int h01 = 0x01010101; // the sum of 256 to the power of 0,1,2,...
        
        unsigned int x;
        
        x = a^b;
        x -= (x >> 1) & m1;             // put count of each 2 bits into those 2 bits
        x = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
        x = (x + (x >> 4)) & m4;        // put count of each 8 bits into those 8 bits
        
        return (x * h01) >> 24;         // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
    }
    
    /**
     * Hamming distance for 768 bits (96 bytes)
     */
    inline unsigned int HammingDistance768(const unsigned int a[24], const unsigned int b[24]) {
        return  HammingDistance32(a[0],  b[0]) +
                HammingDistance32(a[1],  b[1]) +
                HammingDistance32(a[2],  b[2]) +
                HammingDistance32(a[3],  b[3]) +
                HammingDistance32(a[4],  b[4]) +
                HammingDistance32(a[5],  b[5]) +
                HammingDistance32(a[6],  b[6]) +
                HammingDistance32(a[7],  b[7]) +
                HammingDistance32(a[8],  b[8]) +
                HammingDistance32(a[9],  b[9]) +
                HammingDistance32(a[10], b[10]) +
                HammingDistance32(a[11], b[11]) +
                HammingDistance32(a[12], b[12]) +
                HammingDistance32(a[13], b[13]) +
                HammingDistance32(a[14], b[14]) +
                HammingDistance32(a[15], b[15]) +
                HammingDistance32(a[16], b[16]) +
                HammingDistance32(a[17], b[17]) +
                HammingDistance32(a[18], b[18]) +
                HammingDistance32(a[19], b[19]) +
                HammingDistance32(a[20], b[20]) +
                HammingDistance32(a[21], b[21]) +
                HammingDistance32(a[22], b[22]) +
                HammingDistance32(a[23], b[23]);
    }
    
    template<int NUM_BYTES>
    inline unsigned int HammingDistance(const unsigned char a[NUM_BYTES], const unsigned char b[NUM_BYTES]) {
        switch(NUM_BYTES) {
            case 96:
                return HammingDistance768((unsigned int*)a, (unsigned int*)b);
                break;
        };
        return std::numeric_limits<unsigned int>::max();
    }

} // vision
