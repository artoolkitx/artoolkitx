//
//  polynomial.h
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
     * Fit a quatratic to 3 points. The system of equations is:
     *
     * y0 = A*x0^2 + B*x0 + C
     * y1 = A*x1^2 + B*x1 + C
     * y2 = A*x2^2 + B*x2 + C
     *
     * This system of equations is solved for A,B,C.
     *
     * @param[out] A
     * @param[out] B
     * @param[out] C
     * @param[in] p1 2D point 1
     * @param[in] p2 2D point 2
     * @param[in] p3 2D point 3
     * @return True if the quatratic could be fit, otherwise false.
     */
    template<typename T>
	inline bool Quadratic3Points(T& A,
                                 T& B,
                                 T& C,
                                 const T p1[2],
                                 const T p2[2],
                                 const T p3[2]) {
		T d1 = (p3[0]-p2[0])*(p3[0]-p1[0]);
        T d2 = (p1[0]-p2[0])*(p3[0]-p1[0]);
		T d3 = p1[0]-p2[0];
		
        // If any of the denominators are zero then return FALSE.
		if(d1 == 0 ||
           d2 == 0 ||
           d3 == 0) {
			A = 0;
			B = 0;
			C = 0;
			return false;
		}
		else {
			T a = p1[0]*p1[0];
			T b = p2[0]*p2[0];
			
            // Solve for the coefficients A,B,C
			A = ((p3[1]-p2[1])/d1)-((p1[1]-p2[1])/d2);
			B = ((p1[1]-p2[1])+(A*(b-a)))/d3;
			C = p1[1]-(A*a)-(B*p1[0]);
			return true;
		}
	}
    
    /**
     * Evaluate a quatratic function.
     */
    template<typename T>
    inline T QuadraticEval(T A, T B, T C, const T x) {
        return A*x*x + B*x + C;
    }
    
    /**
	 * Find the critical point of a quadratic.
     *
     * y = A*x^2 + B*x + C
     *
     * This function finds where "x" where dy/dx = 0.
	 *
     * @param[out] x Parameter of the critical point.
     * @param[in] A
     * @param[in] B
     * @param[in] C
	 * @return True on success.
	 */
	template<typename T>
	inline bool QuadraticCriticalPoint(T& x, T A, T B, T C) {
		if(A == 0) {
			return false;
		}
		x = -B/(2*A);
		return true;
	}
    
    /**
     * Find the derivate of the quadratic at a point.
     */
    template<typename T>
    inline T QuadraticDerivative(T x, T A, T B) {
        return 2*A*x + B;
    }
    
} // vision