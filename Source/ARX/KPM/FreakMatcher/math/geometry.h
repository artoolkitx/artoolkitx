//
//  geometry.h
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

#include "linear_algebra.h"
#include "indexing.h"

namespace vision {

    /**
	 * Find which side of a line a point is on (+,-).
	 *
	 * @param[in] A First point on line
	 * @param[in] B Second point on line
	 * @param[in] C Arbitrary third point
	 */
	template<typename T>
	inline T LinePointSide(const T A[2], const T B[], const T C[2]) {
		return ((B[0]-A[0])*(C[1]-A[1])-(B[1]-A[1])*(C[0]-A[0]));
	}
    
    /**
     * Check the geometric consistency between three correspondences.
     */
    template<typename T>
    inline bool Homography3PointsGeometricallyConsistent(const T x1[2], const T x2[2], const T x3[2],
                                                         const T x1p[2], const T x2p[2], const T x3p[2]) {
        if(((LinePointSide(x1, x2, x3) > 0) ^ (LinePointSide(x1p, x2p, x3p) > 0)) == true) {
            return false;
        }
        return true;
    }
    
    /**
     * Check the geometric consistency between four correspondences.
     */
    template<typename T>
    inline bool Homography4PointsGeometricallyConsistent(const T x1[2], const T x2[2], const T x3[2], const T x4[2],
                                                         const T x1p[2], const T x2p[2], const T x3p[2], const T x4p[2]) {
        if(((LinePointSide(x1, x2, x3) > 0) ^ (LinePointSide(x1p, x2p, x3p) > 0)) == true)
            return false;
        if(((LinePointSide(x2, x3, x4) > 0) ^ (LinePointSide(x2p, x3p, x4p) > 0)) == true)
            return false;
        if(((LinePointSide(x3, x4, x1) > 0) ^ (LinePointSide(x3p, x4p, x1p) > 0)) == true)
            return false;
        if(((LinePointSide(x4, x1, x2) > 0) ^ (LinePointSide(x4p, x1p, x2p) > 0)) == true)
            return false;
        return true;
    }
    
    /**
     * Check if four points form a convex quadrilaternal.
     */
    template<typename T>
    bool QuadrilateralConvex(const T x1[2], const T x2[2], const T x3[2], const T x4[2]) {
        int s;
        
        s  = LinePointSide(x1, x2, x3) > 0 ? 1 : -1;
        s += LinePointSide(x2, x3, x4) > 0 ? 1 : -1;
        s += LinePointSide(x3, x4, x1) > 0 ? 1 : -1;
        s += LinePointSide(x4, x1, x2) > 0 ? 1 : -1;
        
        return (std::abs(s) == 4);
    }
    
    /**
     * Compute the area of a triangle.
     */
    template<typename T>
	inline T AreaOfTriangle(const T u[2], const T v[2]) {
		T a = u[0]*v[1] - u[1]*v[0];
		return std::abs(a)*0.5;
	}
    
    /**
     * Find the smallest area for each triangle formed by 4 points.
     */
    template<typename T>
    T SmallestTriangleArea(const T x1[2], const T x2[2], const T x3[2], const T x4[2]) {
		T v12[2];
		T v13[2];
		T v14[2];
		T v32[2];
		T v34[2];
        
		SubVector2(v12, x2, x1);
		SubVector2(v13, x3, x1);
		SubVector2(v14, x4, x1);
		SubVector2(v32, x2, x3);
		SubVector2(v34, x4, x3);
		
        T a1 = AreaOfTriangle(v12, v13);
		T a2 = AreaOfTriangle(v13, v14);
		T a3 = AreaOfTriangle(v12, v14);
		T a4 = AreaOfTriangle(v32, v34);
		
		return min4(a1, a2, a3, a4);
	}
    
} // vision