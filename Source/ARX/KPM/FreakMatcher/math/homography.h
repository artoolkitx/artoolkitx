//
//  homography.h
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
#include "linear_algebra.h"
#include "indexing.h"
#include "geometry.h"

namespace vision {
    
    /**
     * Create a similarity matrix.
     */
    template<typename T>
    inline void Similarity(T H[9], T x, T y, T angle, T scale) {
        T c = scale*std::cos(angle);
        T s = scale*std::sin(angle);
        H[0] = c;	H[1] = -s;	H[2] = x;
		H[3] = s;	H[4] = c;	H[5] = y;
		H[6] = 0;	H[7] = 0;	H[8] = 1;
    }
    
    /**
     * Create a similarity matrix.
     */
    template<typename T>
    inline void Similarity2x2(T S[4], T angle, T scale) {
        T c = scale*std::cos(angle);
        T s = scale*std::sin(angle);
        S[0] = c; S[1] = -s;
        S[2] = s; S[3] = c;
    }
    
    template<typename T>
    inline void CreateSimilarityTransformation2d(T H[9], T cx, T cy, T angle, T scale) {
        T c = scale*std::cos(angle);
        T s = scale*std::sin(angle);
        
        H[0] = c;	H[1] = -s;
        H[3] = s;	H[4] = c;
        
        H[2] = -(H[0]*cx + H[1]*cy) + cx;
        H[5] = -(H[3]*cx + H[4]*cy) + cy;
        
        H[6] = 0;   H[7] = 0;   H[8] = 1;
    }
    
    /**
     * Multiply an in-homogenous point by a similarity.
     */
    template<typename T>
    inline void MultiplyPointSimilarityInhomogenous(T xp[2], const T H[9], const T x[2]) {
        xp[0] = H[0]*x[0] + H[1]*x[1] + H[2];
        xp[1] = H[3]*x[0] + H[4]*x[1] + H[5];
    }

    /**
     * Multiply an in-homogenous point by a similarity.
     */
    template<typename T>
    inline void MultiplyPointHomographyInhomogenous(T& xp, T& yp, const T H[9], T x, T y) {
        float w = H[6]*x + H[7]*y + H[8];
        xp = (H[0]*x + H[1]*y + H[2])/w;
        yp = (H[3]*x + H[4]*y + H[5])/w;
    }
    
    /**
     * Multiply an in-homogenous point by a similarity.
     */
    template<typename T>
    inline void MultiplyPointHomographyInhomogenous(T xp[2], const T H[9], const T x[2]) {
        float w = H[6]*x[0] + H[7]*x[1] + H[8];
        xp[0] = (H[0]*x[0] + H[1]*x[1] + H[2])/w;
        xp[1] = (H[3]*x[0] + H[4]*x[1] + H[5])/w;
    }
    
    template<typename T>
    inline bool HomographyPointsGeometricallyConsistent(const T H[9], const T* x, int size) {
        T xp1[2];
        T xp2[2];
        T xp3[2];
        T first_xp1[2];
        T first_xp2[2];
        
        // We need at least 3 points
        if(size < 2) {
            return true;
        }
        
        const T* x1_ptr = x;
        const T* x2_ptr = x+2;
        const T* x3_ptr = x+4;
        
        T* xp1_ptr = xp1;
        T* xp2_ptr = xp2;
        T* xp3_ptr = xp3;
        
        //
        // Check the first 3 points
        //
        
        MultiplyPointHomographyInhomogenous(xp1, H, x1_ptr);
        MultiplyPointHomographyInhomogenous(xp2, H, x2_ptr);
        MultiplyPointHomographyInhomogenous(xp3, H, x3_ptr);
        
        CopyVector2(first_xp1, xp1);
        CopyVector2(first_xp2, xp2);
        
        if(!Homography3PointsGeometricallyConsistent(x1_ptr, x2_ptr, x3_ptr, xp1_ptr, xp2_ptr, xp3_ptr)) {
            return false;
        }
        
        //
        // Check the remaining points
        //
        
        for(int i = 3; i < size; i++) {
            x1_ptr += 2;
            x2_ptr += 2;
            x3_ptr += 2;
            
            MultiplyPointHomographyInhomogenous(xp1_ptr, H, x3_ptr);
            
            T* tmp_ptr = xp1_ptr;
            xp1_ptr = xp2_ptr;
            xp2_ptr = xp3_ptr;
            xp3_ptr = tmp_ptr;
            
            if(!Homography3PointsGeometricallyConsistent(x1_ptr, x2_ptr, x3_ptr, xp1_ptr, xp2_ptr, xp3_ptr)) {
                return false;
            }
        }
        
        //
        // Check the last 3 points
        //
        
        if(!Homography3PointsGeometricallyConsistent(x2_ptr, x3_ptr, x, xp2_ptr, xp3_ptr, first_xp1)) {
            return false;
        }
        if(!Homography3PointsGeometricallyConsistent(x3_ptr, x, x+2, xp3_ptr, first_xp1, first_xp2)) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Normalize a homography such that H(3,3) = 1.
     */
    template<typename T>
    inline void NormalizeHomography(T H[9]) {
        T one_over = 1./H[8];
        H[0] *= one_over;  H[1] *= one_over;  H[2] *= one_over;
        H[3] *= one_over;  H[4] *= one_over;  H[5] *= one_over;
        H[6] *= one_over;  H[7] *= one_over;  H[8] =1.0f;
    }
    
    /**
     * Update a homography with an incremental translation update.
     *
     * H*(I+T) where T = [1 0 tx; 0 1 ty; 0 0 1].
     */
    template<typename T>
	inline void UpdateHomographyTranslation(T H[9], T tx, T ty) {
        H[2] += H[0]*tx + H[1]*ty;
        H[5] += H[3]*tx + H[4]*ty;
        H[8] += H[6]*tx + H[7]*ty;
    }
    
    /**
	 * Symmetrically scale a homography.
	 *
	 * y = Hp*x where Hp = inv(S)*H*S
	 */
	template<typename T>
	inline void ScaleHomography(T H[9], T s) {
		H[2] /= s;
		H[5] /= s;
		H[6] *= s;
		H[7] *= s;
	}
    
} // vision
