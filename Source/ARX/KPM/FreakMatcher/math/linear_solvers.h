//
//  linear_solvers.h
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
#include <math/linear_algebra.h>
#include <math/indexing.h>

namespace vision {
    
    /**
     * Project a vector "a" onto a normalized basis vector "e".
     *
     * x = x - dot(a,e)*e
     */
    template<typename T>
    inline void AccumulateProjection9(T x[9], const T e[9], const T a[9]) {
        T d = DotProduct9(a, e);
        x[0] -= d*e[0];
        x[1] -= d*e[1];
        x[2] -= d*e[2];
        x[3] -= d*e[3];
        x[4] -= d*e[4];
        x[5] -= d*e[5];
        x[6] -= d*e[6];
        x[7] -= d*e[7];
        x[8] -= d*e[8];
    }
    
    /**
     * \defgroup Project the rows of A onto the current basis set to identity a new orthogonal vector.
     * @{
     */
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis0(T Q[8*9], T A[8*9]) {
        T ss[8];
        ss[0] = SumSquares9(A);
        ss[1] = SumSquares9(A+9);
        ss[2] = SumSquares9(A+18);
        ss[3] = SumSquares9(A+27);
        ss[4] = SumSquares9(A+36);
        ss[5] = SumSquares9(A+45);
        ss[6] = SumSquares9(A+54);
        ss[7] = SumSquares9(A+63);
        
        int index = MaxIndex8(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(A, A+index*9);
        ScaleVector9(Q, A, 1.f/std::sqrt(ss[index]));
        CopyVector(Q+9, A+9, 63);
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis1(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+9,  Q, A+9);
        AccumulateProjection9(Q+18, Q, A+18);
        AccumulateProjection9(Q+27, Q, A+27);
        AccumulateProjection9(Q+36, Q, A+36);
        AccumulateProjection9(Q+45, Q, A+45);
        AccumulateProjection9(Q+54, Q, A+54);
        AccumulateProjection9(Q+63, Q, A+63);

        T ss[7];
        ss[0] = SumSquares9(Q+9);
        ss[1] = SumSquares9(Q+18);
        ss[2] = SumSquares9(Q+27);
        ss[3] = SumSquares9(Q+36);
        ss[4] = SumSquares9(Q+45);
        ss[5] = SumSquares9(Q+54);
        ss[6] = SumSquares9(Q+63);
        
        int index = MaxIndex7(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+9, Q+9+index*9);
        Swap9(A+9, A+9+index*9);
        ScaleVector9(Q+9, Q+9, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis2(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+18, Q+9, A+18);
        AccumulateProjection9(Q+27, Q+9, A+27);
        AccumulateProjection9(Q+36, Q+9, A+36);
        AccumulateProjection9(Q+45, Q+9, A+45);
        AccumulateProjection9(Q+54, Q+9, A+54);
        AccumulateProjection9(Q+63, Q+9, A+63);
        
        T ss[6];
        ss[0] = SumSquares9(Q+18);
        ss[1] = SumSquares9(Q+27);
        ss[2] = SumSquares9(Q+36);
        ss[3] = SumSquares9(Q+45);
        ss[4] = SumSquares9(Q+54);
        ss[5] = SumSquares9(Q+63);
        
        int index = MaxIndex6(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+18, Q+18+index*9);
        Swap9(A+18, A+18+index*9);
        ScaleVector9(Q+18, Q+18, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis3(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+27, Q+18, A+27);
        AccumulateProjection9(Q+36, Q+18, A+36);
        AccumulateProjection9(Q+45, Q+18, A+45);
        AccumulateProjection9(Q+54, Q+18, A+54);
        AccumulateProjection9(Q+63, Q+18, A+63);
        
        T ss[5];
        ss[0] = SumSquares9(Q+27);
        ss[1] = SumSquares9(Q+36);
        ss[2] = SumSquares9(Q+45);
        ss[3] = SumSquares9(Q+54);
        ss[4] = SumSquares9(Q+63);
        
        int index = MaxIndex5(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+27, Q+27+index*9);
        Swap9(A+27, A+27+index*9);
        ScaleVector9(Q+27, Q+27, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis4(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+36, Q+27, A+36);
        AccumulateProjection9(Q+45, Q+27, A+45);
        AccumulateProjection9(Q+54, Q+27, A+54);
        AccumulateProjection9(Q+63, Q+27, A+63);
        
        T ss[4];
        ss[0] = SumSquares9(Q+36);
        ss[1] = SumSquares9(Q+45);
        ss[2] = SumSquares9(Q+54);
        ss[3] = SumSquares9(Q+63);
        
        int index = MaxIndex4(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+36, Q+36+index*9);
        Swap9(A+36, A+36+index*9);
        ScaleVector9(Q+36, Q+36, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis5(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+45, Q+36, A+45);
        AccumulateProjection9(Q+54, Q+36, A+54);
        AccumulateProjection9(Q+63, Q+36, A+63);
        
        T ss[3];
        ss[0] = SumSquares9(Q+45);
        ss[1] = SumSquares9(Q+54);
        ss[2] = SumSquares9(Q+63);
        
        int index = MaxIndex3(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+45, Q+45+index*9);
        Swap9(A+45, A+45+index*9);
        ScaleVector9(Q+45, Q+45, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis6(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+54, Q+45, A+54);
        AccumulateProjection9(Q+63, Q+45, A+63);
        
        T ss[2];
        ss[0] = SumSquares9(Q+54);
        ss[1] = SumSquares9(Q+63);
        
        int index = MaxIndex2(ss);
        if(ss[index] == 0) {
            return false;
        }
        
        Swap9(Q+54, Q+54+index*9);
        Swap9(A+54, A+54+index*9);
        ScaleVector9(Q+54, Q+54, 1.f/std::sqrt(ss[index]));
        
        return true;
    }
    
    template<typename T>
    inline bool OrthogonalizePivot8x9Basis7(T Q[8*9], T A[8*9]) {
        AccumulateProjection9(Q+63, Q+54, A+63);
        
        T ss = SumSquares9(Q+63);
        if(ss == 0) {
            return false;
        }
        
        ScaleVector9(Q+63, Q+63, 1.f/std::sqrt(ss));
        
        return true;
    }
    
    /**@}*/
    
    /**
     * \defgroup Project the rows of the identity matrix to identify the remaining basis vector.
     * @{
     */
    
    template<typename T>
    inline T OrthogonalizeIdentityRow0(T x[9], const T Q[72]) {
        ScaleVector9(x, Q, -Q[0]);
        x[0] = 1+x[0];
        
        AccumulateScaledVector9(x, Q+9,  -Q[9]);
        AccumulateScaledVector9(x, Q+18, -Q[18]);
        AccumulateScaledVector9(x, Q+27, -Q[27]);
        AccumulateScaledVector9(x, Q+36, -Q[36]);
        AccumulateScaledVector9(x, Q+45, -Q[45]);
        AccumulateScaledVector9(x, Q+54, -Q[54]);
        AccumulateScaledVector9(x, Q+63, -Q[63]);
        
        T ss = SumSquares9(x);
        if(ss == 0) {
            return 0;
        }
        
        T w = std::sqrt(ss);
        ScaleVector9(x, x, 1.f/w);
        
        return w;
    }
    
    template<typename T>
    inline T OrthogonalizeIdentity8x9(T x[9], const T Q[72], int i) {
        ScaleVector9(x, Q, -Q[i]);
        x[i] = 1+x[i];
        
        AccumulateScaledVector9(x, Q+9,  -Q[9 +i]);
        AccumulateScaledVector9(x, Q+18, -Q[18+i]);
        AccumulateScaledVector9(x, Q+27, -Q[27+i]);
        AccumulateScaledVector9(x, Q+36, -Q[36+i]);
        AccumulateScaledVector9(x, Q+45, -Q[45+i]);
        AccumulateScaledVector9(x, Q+54, -Q[54+i]);
        AccumulateScaledVector9(x, Q+63, -Q[63+i]);
        
        T ss = SumSquares9(x);
        if(ss == 0) {
            return 0;
        }
        
        T w = std::sqrt(ss);
        ScaleVector9(x, x, 1.f/w);
        
        return w;
    }
    
    template<typename T>
    inline bool OrthogonalizeIdentity8x9(T x[9], const T Q[72]) {
        T w[9];
        T X[9*9];
        
        w[0] = OrthogonalizeIdentity8x9(X,    Q, 0);
        w[1] = OrthogonalizeIdentity8x9(X+9,  Q, 1);
        w[2] = OrthogonalizeIdentity8x9(X+18, Q, 2);
        w[3] = OrthogonalizeIdentity8x9(X+27, Q, 3);
        w[4] = OrthogonalizeIdentity8x9(X+36, Q, 4);
        w[5] = OrthogonalizeIdentity8x9(X+45, Q, 5);
        w[6] = OrthogonalizeIdentity8x9(X+54, Q, 6);
        w[7] = OrthogonalizeIdentity8x9(X+63, Q, 7);
        w[8] = OrthogonalizeIdentity8x9(X+72, Q, 8);
        
        int index = MaxIndex9(w);
        if(w[index] == 0) {
            return false;
        }
        
        CopyVector9(x, X+index*9);
        
        return true;
    }
    
    /**@}*/
    
    /**
     * Solve for the null vector x of an 8x9 matrix A such A*x=0. The matrix
     * A is destroyed in the process. This system is solved using QR 
     * decomposition with Gram-Schmidt.
     */
    template<typename T>
    inline bool SolveNullVector8x9Destructive(T x[9], T A[8*9]) {
        T Q[72];
        
        if(!OrthogonalizePivot8x9Basis0(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis1(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis2(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis3(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis4(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis5(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis6(Q, A)) return false;
        if(!OrthogonalizePivot8x9Basis7(Q, A)) return false;
        
        return OrthogonalizeIdentity8x9(x, Q);
    }
    
    /**
     * Solve the tridiagonal system A*x=v
     *
     * A is of the form:
     *
     *      A = [b0 c0  0  0 0 ...
     *           a1 b1 c1  0 0 ...
     *           0  a2 b2 c2 0 ...
     *           .
     *           .
     *           .
     *
     * @param[in/out] x input v, output x
     * @param[in] n Length of the diagonal
     * @param[in] a Below diagonal elements
     * @param[in] b Diagonal elements
     * @param[in] c Above diagonal elements over allocated by 1
     * @return true on successs
     */
    template<typename T>
    inline bool SolveTridiagonalDestructive(T* x, int n, const T* a, const T* b, T* c) {
        if(b[0] == 0) {
            return false;
        }
        c[0] = c[0]/b[0];
        x[0] = x[0]/b[0];
        
        // Forward elimination
        for(int i = 1; i < n; i++) {
            T d = b[i]-a[i-1]*c[i-1];
            if(d == 0) {
                return false;
            }
            T m = 1/d;
            c[i] *= m;
            x[i] = (x[i]-a[i-1]*x[i-1])*m;
        }
        
        // Back substitution
        for(int i = n-1; i-- > 0;) {
            x[i] = x[i] - c[i]*x[i+1];
        }
        
        return true;
    }
    
} // vision
