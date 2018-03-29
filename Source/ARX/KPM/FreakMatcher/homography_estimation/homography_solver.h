//
//  homography_solver.h
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

#include <math/indexing.h>
#include <math/linear_solvers.h>
#include <math/math_utils.h>

namespace vision {
    
    /**
	 * Condition four 2D points such that the mean is zero and the standard deviation is sqrt(2).
	 */
	template<typename T>
	inline bool Condition4Points2d(T xp1[2],
                                   T xp2[2],
                                   T xp3[2],
                                   T xp4[2],
                                   T& s,
                                   T mu[2],
                                   const T x1[2],
                                   const T x2[2],
                                   const T x3[2],
                                   const T x4[2]) {
		T d1[2], d2[2], d3[2], d4[2];
		
		mu[0] = (x1[0]+x2[0]+x3[0]+x4[0])/4;
		mu[1] = (x1[1]+x2[1]+x3[1]+x4[1])/4;
		
		d1[0] = x1[0]-mu[0];
		d1[1] = x1[1]-mu[1];
		d2[0] = x2[0]-mu[0];
		d2[1] = x2[1]-mu[1];
		d3[0] = x3[0]-mu[0];
		d3[1] = x3[1]-mu[1];
		d4[0] = x4[0]-mu[0];
		d4[1] = x4[1]-mu[1];
		
		T ds1 = std::sqrt(d1[0]*d1[0]+d1[1]*d1[1]);
		T ds2 = std::sqrt(d2[0]*d2[0]+d2[1]*d2[1]);
		T ds3 = std::sqrt(d3[0]*d3[0]+d3[1]*d3[1]);
		T ds4 = std::sqrt(d4[0]*d4[0]+d4[1]*d4[1]);
		T d = (ds1+ds2+ds3+ds4)/4;
		
        if(d == 0) {
            return false;
        }
        
		s = (1/d)*SQRT2;
        
        xp1[0] = d1[0]*s;
		xp1[1] = d1[1]*s;
		xp2[0] = d2[0]*s;
		xp2[1] = d2[1]*s;
		xp3[0] = d3[0]*s;
		xp3[1] = d3[1]*s;
		xp4[0] = d4[0]*s;
		xp4[1] = d4[1]*s;
        
        return true;
	}
    
    /**
     * Denomalize the homograhy H.
     *
     * Hp = inv(Tp)*H*T 
     *
     * where T and Tp are the noramalizing transformations for the points that were used 
     * to compute H.
     */
    template<typename T>
	inline void DenormalizeHomography(T Hp[9],
                                      const T H[9],
                                      T s,
                                      const T t[2],
                                      T sp,
                                      const T tp[2]) {
		T a = H[6]*tp[0];
		T b = H[7]*tp[0];
		T c = H[0]/sp;
		T d = H[1]/sp;
		T apc = a+c;
		T bpd = b+d;
		
		T e = H[6]*tp[1];
		T f = H[7]*tp[1];
		T g = H[3]/sp;
		T h = H[4]/sp;
		T epg = e+g;
		T fph = f+h;
		
		T stx = s*t[0];
		T sty = s*t[1];
		
		Hp[0] = s*apc;
		Hp[1] = s*bpd;
		Hp[2] = H[8]*tp[0] + H[2]/sp - stx*apc - sty*bpd;
		
		Hp[3] = s*epg;
		Hp[4] = s*fph;
		Hp[5] = H[8]*tp[1] + H[5]/sp - stx*epg - sty*fph;
		
		Hp[6] = H[6]*s;
		Hp[7] = H[7]*s;
		Hp[8] = H[8] - Hp[6]*t[0] - Hp[7]*t[1];
	}
    
    /**
     * Add a point to the homography constraint matrix.
     */
    template<typename T>
    inline void AddHomographyPointContraint(T A[18], const T x[2], const T xp[2]) {
        A[0] = -x[0];
        A[1] = -x[1];
        A[2] = -1;
        ZeroVector3(A+3);
        A[6] = xp[0]*x[0];
        A[7] = xp[0]*x[1];
        A[8] = xp[0];
        
        ZeroVector3(A+9);
        A[12] = -x[0];
        A[13] = -x[1];
        A[14] = -1;
        A[15] = xp[1]*x[0];
        A[16] = xp[1]*x[1];
        A[17] = xp[1];
    }
    
    /**
     * Construct the homography constraint matrix from 4 point correspondences.
     */
    template<typename T>
    inline void Homography4PointsInhomogeneousConstraint(T A[72],
                                                         const T x1[2],
                                                         const T x2[2],
                                                         const T x3[2],
                                                         const T x4[2],
                                                         const T xp1[2],
                                                         const T xp2[2],
                                                         const T xp3[2],
                                                         const T xp4[2]) {
        AddHomographyPointContraint(A, x1, xp1);
        AddHomographyPointContraint(A+18, x2, xp2);
        AddHomographyPointContraint(A+36, x3, xp3);
        AddHomographyPointContraint(A+54, x4, xp4);
    }
    
    /**
     * Solve for the homography given four 2D point correspondences.
     */
    template<typename T>
	inline bool SolveHomography4PointsInhomogenous(T H[9],
                                                   const T x1[2],
                                                   const T x2[2],
                                                   const T x3[2],
                                                   const T x4[2],
                                                   const T xp1[2],
                                                   const T xp2[2],
                                                   const T xp3[2],
                                                   const T xp4[2]) {
        T A[72];
        Homography4PointsInhomogeneousConstraint(A, x1, x2, x3, x4, xp1, xp2, xp3, xp4);
        if(!SolveNullVector8x9Destructive(H, A)) {
            return false;
        }
        if(std::abs(Determinant3x3(H)) < 1e-5) {
            return false;
        }
        return true;
    }
    
    /**
     * Solve for the homography given 4 point correspondences.
     */
    template<typename T>
    inline bool SolveHomography4Points(T H[9],
                                       const T x1[2],
                                       const T x2[2],
                                       const T x3[2],
                                       const T x4[2],
                                       const T xp1[2],
                                       const T xp2[2],
                                       const T xp3[2],
                                       const T xp4[2]) {
        T Hn[9];
		
		T s, sp;
		T t[2], tp[2];
		
		T x1p[2], x2p[2], x3p[2], x4p[2];
		T xp1p[2], xp2p[2], xp3p[2], xp4p[2];
		
        //
		// Condition the points
        //
        
		if(!Condition4Points2d(x1p,
                               x2p,
                               x3p,
                               x4p,
                               s,
                               t,
                               x1,
                               x2,
                               x3,
                               x4)) {
            return false;
        }
		if(!Condition4Points2d(xp1p,
                               xp2p,
                               xp3p,
                               xp4p,
                               sp,
                               tp,
                               xp1,
                               xp2,
                               xp3,
                               xp4)) {
            return false;
        }
        
        //
        // Solve for the homography
        //
        
        if(!SolveHomography4PointsInhomogenous(Hn,
                                               x1p,
                                               x2p,
                                               x3p,
                                               x4p,
                                               xp1p,
                                               xp2p,
                                               xp3p,
                                               xp4p)) {
            return false;
        }
        
        //
        // Denomalize the computed homography
        //
        
		DenormalizeHomography(H, Hn, s, t, sp, tp);
        
        return true;
    }
    
} // vision