//
//  quaternion.h
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

#include <framework/error.h>

#include "linear_algebra.h"
#include "math_utils.h"
#include "indexing.h"

namespace vision {
    
    /**
     * Normalize a quaternion so that it has unit norm.
     */
    template<typename T>
    inline void NormalizeQuaternion(T q_out[4], const T q_in[4]) {
        T r2 = sqr(q_in[0]) + sqr(q_in[1]) + sqr(q_in[2]) + sqr(q_in[3]);
        ASSERT(r2 > 0, "Must be positive");
        T one_over_r = 1/std::sqrt(r2);
        ScaleVector4(q_out, q_in, one_over_r);
    }
    
    /**
     * Quaternion Spherical Linear Interpolation (SLERP).
     *
     * @param[out] q Interpolated quaternion
     * @param[in] q0 Initial quaternion
     * @param[in] q1 Final quaternion
     * @param[in] t Interpolation parameter in range [0,1]
     */
    template<typename T>
    inline void QuaternionSLERP(T q[4], const T q0[4], const T q1[4], T t) {
        T qq1[4];
        
        // "t" is the range [0,1]
        ASSERT(t >= 0, "Out of range");
        ASSERT(t <= 1, "Out of range");
        
        // Take the shortest path: (q and -q represent the same rotation)
        T d = DotProduct4(q0, q1);
        if(d < 0) {
            ScaleVector4<T>(qq1, q1, -1);
            d *= -1;
        } else {
            CopyVector4(qq1, q1);
        }
        
        // Make sure that "d" is in the range [-1,1].
        ASSERT(d <= 1+1e-5, "Out of range");
        ASSERT(d >= -1-1e-5, "Out of range");
        d = ClipScalar<T>(d, -1, 1);
        
        // Angle between quanternions
        T angle = std::acos(d);
        
        if(angle == 0) {
            // If the angle is 0, then the output quaternion is q0
            CopyVector4(q, q0);
        } else {
            T a = std::sin(angle*(1-t));
            T b = std::sin(angle*t);
            AddScaledVectors4(q, q0, qq1, a, b);
            NormalizeQuaternion(q, q);
        }
    }
    
} // vision