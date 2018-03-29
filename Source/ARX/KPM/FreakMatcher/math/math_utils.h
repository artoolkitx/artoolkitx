//
//  math_utils.h
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

#include <cmath>

namespace vision {
    
#define PI                  3.1415926535897932384626433832795
#define ONE_OVER_2PI        0.159154943091895
#define SQRT2               1.41421356237309504880
    
    /**
     * @return x*x
     */
    template<typename T>
    inline T sqr(T x) { return x*x; }
    
    /**
     * @return nearest rounding of x
     */
    template<typename T>
    inline T round(T x) { return std::floor((x+(T)0.5)); }
    
    /**
     * Base 2 log
     */
    template<typename T>
    inline T log2(T x) {
        return std::log(x) / std::log((T)2);
    }
    
    /**
     * Base "b" log
     */
    template<typename T>
    inline T logb(T x, T b) {
        return std::log(x) / std::log(b);
    }
    
    /**
     * Safe reciprical (1/x).
     */
    template<typename T>
    inline T SafeReciprical(T x) {
        return 1/(x == 0 ? 1 : x);
    }
    
    /**
     * Safe division (x/y).
     */
    template<typename T>
    inline T SafeDivision(T x, T y) {
        return x/(y == 0 ? 1 : y);
    }
    
    /**
     * Fast atan2 [-pi,pi]
     */
    inline float fastatan2(float y, float x) {
        float angle, r;
        float abs_y = std::abs(y)+1e-7;
        
        if(x == 0 && y == 0) {
            return 0;
        }
        else if(x > 0) {
            r = (x - abs_y) / (x + abs_y);
            angle = PI/4;
        }
        else {
            r = (x + abs_y) / (abs_y - x);
            angle = 3*PI/4;
        }
        
        angle += (0.1821f*r*r - 0.9675f)*r;
        return (y < 0) ? - angle : angle;
    }
    
    /**
     * Fast atan2 in range [0,360]
     */
    inline float fastatan2_360(float y, float x) {
        float angle, r;
        float abs_y = std::abs(y)+1e-7;
        
        if(x == 0 && y == 0) {
            return 0;
        }
        else if(x > 0) {
            r = (x - abs_y) / (x + abs_y);
            angle = PI/4;
        }
        else {
            r = (x + abs_y) / (abs_y - x);
            angle = 3*PI/4;
        }
        
        angle += (0.1821f*r*r - 0.9675f)*r;
        
        return angle;
    }
    
    /**
     * Fast square-root. 
     *
     * http://en.wikipedia.org/wiki/Fast_inverse_square_root
     */
    inline float fastsqrt1(float x) {
        union {
            float x;
            int  i;
        } u;
        
        float xhalf = 0.5f*x;
        
        // Cast FLOAT to INT
        u.x = (int)x;
        
        // Initial guess
        u.i = 0x5f3759df - (u.i >> 1);
        
        // Newton iteration
        u.x = u.x * (1.5f - xhalf*u.x*u.x);
        
        return x*u.x;
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    // Fast exponential functions.
    //
    //////////////////////////////////////////////////////////////////////////////////////////
    
    /**
     * 0.01% error at 1.030
     * 0.10% error at 1.520
     * 1.00% error at 2.330
     * 5.00% error at 3.285
     */
    template<typename T>
    inline T fastexp6(T x) {
        return (720+x*(720+x*(360+x*(120+x*(30+x*(6+x))))))*0.0013888888;
    }

    /**
     * Clip a scalar to be within a range.
     */
    template<typename T>
    inline T ClipScalar(T x, T min, T max) {
        if(x < min) {
            x = min;
        } else if(x > max) {
            x = max;
        }
        return x;
    }
    
    template<typename T>
    inline T deg2rad(T deg) {
        return deg*(PI/180);
    }

} // vision