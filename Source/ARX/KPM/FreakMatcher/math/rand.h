//
//  rand.h
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

#include <algorithm>

namespace vision {
    
#define FAST_RAND_MAX 32767
    
    /**
     * Implements a fast random number generator. 
     *
     * http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
     */
    inline int FastRandom(int& seed) {
        seed = (214013*seed+2531011);
        return (seed>>16)&0x7FFF;
    }
    
    /**
     * Get a float between [0,1].
     */
    template<typename T>
    inline T FastRandomFloat(int& seed) {
        return FastRandom(seed)/(T)FAST_RAND_MAX;
    }
    
    /**
     * Shuffle the elements of an array.
     *
     * @param[in/out] v Array of elements
     * @param[in] pop_size Population size, or size of the array v
     * @param[in] sample_size The first SAMPLE_SIZE samples of v will be shuffled
     * @param[in] seed Seed for random number generator
     */
    template<typename T>
    inline void ArrayShuffle(T* v, int pop_size, int sample_size, int& seed) {
        for(int i = 0; i < sample_size; i++) {
            int k = FastRandom(seed)%pop_size;
            std::swap(v[i], v[k]);
        }
    }
    
} // vision