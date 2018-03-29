//
//  gradients.cpp
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

#include "gradients.h"
#include <math/math_utils.h>
#include <cmath>

using namespace vision;

namespace vision {

    void ComputePolarGradients(float* gradient,
                               const float* im,
                               size_t width,
                               size_t height) {
        
#define SET_GRADIENT(dx, dy)                \
*(gradient++) = std::atan2(dy, dx)+PI;      \
*(gradient++) = std::sqrt(dx*dx+dy*dy);     \
p_ptr++; pm1_ptr++; pp1_ptr++;              \

        size_t width_minus_1;
        size_t height_minus_1;
        
        float dx, dy;
        const float* p_ptr;
        const float* pm1_ptr;
        const float* pp1_ptr;
        
        width_minus_1 = width-1;
        height_minus_1 = height-1;
        
        // Top row
        pm1_ptr = im;
        p_ptr   = im;
        pp1_ptr = p_ptr+width;
        
        dx = p_ptr[1] - p_ptr[0];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        for(int col = 1; col < width_minus_1; col++) {
            dx = p_ptr[1] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        dx = p_ptr[0] - p_ptr[-1];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        // Non-border pixels
        pm1_ptr = im;
        p_ptr   = pm1_ptr+width;
        pp1_ptr = p_ptr+width;
        
        for(int row = 1; row < height_minus_1; row++) {
            dx = p_ptr[1] - p_ptr[0];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
            
            for(int col = 1; col < width_minus_1; col++) {
                dx = p_ptr[1] - p_ptr[-1];
                dy = pp1_ptr[0] - pm1_ptr[0];
                SET_GRADIENT(dx, dy)
            }
            
            dx = p_ptr[0] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        // Lower row
        p_ptr   = &im[height_minus_1*width];
        pm1_ptr = p_ptr-width;
        pp1_ptr = p_ptr;
        
        dx = p_ptr[1] - p_ptr[0];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        for(int col = 1; col < width_minus_1; col++) {
            dx = p_ptr[1] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        dx = p_ptr[0]   - p_ptr[-1];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
#undef SET_GRADIENT
    }
    
    void ComputeGradients(float* gradient,
                          const float* im,
                          size_t width,
                          size_t height) {
#define SET_GRADIENT(dx, dy)                \
*(gradient++) = dx;                         \
*(gradient++) = dy;                         \
p_ptr++; pm1_ptr++; pp1_ptr++;              \

        size_t width_minus_1;
        size_t height_minus_1;
        
        float dx, dy;
        const float* p_ptr;
        const float* pm1_ptr;
        const float* pp1_ptr;
        
        width_minus_1 = width-1;
        height_minus_1 = height-1;
        
        // Top row
        pm1_ptr = im;
        p_ptr   = im;
        pp1_ptr = p_ptr+width;
        
        dx = p_ptr[1] - p_ptr[0];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        for(int col = 1; col < width_minus_1; col++) {
            dx = p_ptr[1] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        dx = p_ptr[0] - p_ptr[-1];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        // Non-border pixels
        pm1_ptr = im;
        p_ptr   = pm1_ptr+width;
        pp1_ptr = p_ptr+width;
        
        for(int row = 1; row < height_minus_1; row++) {
            dx = p_ptr[1] - p_ptr[0];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
            
            for(int col = 1; col < width_minus_1; col++) {
                dx = p_ptr[1] - p_ptr[-1];
                dy = pp1_ptr[0] - pm1_ptr[0];
                SET_GRADIENT(dx, dy)
            }
            
            dx = p_ptr[0] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        // Lower row
        p_ptr   = &im[height_minus_1*width];
        pm1_ptr = p_ptr-width;
        pp1_ptr = p_ptr;
        
        dx = p_ptr[1] - p_ptr[0];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
        for(int col = 1; col < width_minus_1; col++) {
            dx = p_ptr[1] - p_ptr[-1];
            dy = pp1_ptr[0] - pm1_ptr[0];
            SET_GRADIENT(dx, dy)
        }
        
        dx = p_ptr[0]   - p_ptr[-1];
        dy = pp1_ptr[0] - pm1_ptr[0];
        SET_GRADIENT(dx, dy)
        
#undef SET_GRADIENT
    }
    
} // vision
