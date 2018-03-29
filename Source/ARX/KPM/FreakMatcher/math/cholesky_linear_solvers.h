//
//  cholesky_linear_solvers.h
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

#include "cholesky.h"
#include "linear_algebra.h"

namespace vision {

    /**
	 * Solve the system A*x=b where A is a square right triangular matrix
	 *
	 * @param[out] x Solution to the system of equations
	 * @param[in] A n x n square matrix
	 * @param[in] b n x 1 vector
	 * @param[in] n Dimension of A
	 */
	template<typename T>
    bool SolveRightTriangularSystem(T* x, const T* A, const T* b, int n, T threshold = 0) {
        x[n-1] = b[n-1]/A[n*n-1];
		for(int i = n-2; i >= 0; i--) {
			const T* Ai = &A[i*n];
            if(std::abs(Ai[i]) <= threshold) {
                return false;
            }
            x[i] = (b[i] - DotProduct(Ai+i+1, x+i+1, n-i-1)) / Ai[i];
		}
        return true;
	}
	
	/**
	 * Solve the system A*x=b where A is a square left triangular matrix
	 *
	 * @param[out] x Solution to the system of equations
	 * @param[in] A n x n square matrix
	 * @param[in] b n x 1 vector
	 * @param[in] n Dimension of A
	 */
	template<typename T>
    bool SolveLeftTriangularSystem(T* x, const T* A, const T* b, int n, T threshold = 0) {
		for(int i = 0; i < n; i++) {
			const T* Ai = &A[i*n];
            if(std::abs(Ai[i]) <= threshold) {
                return false;
            }
            x[i] = (b[i] - DotProduct(Ai, x, i)) / Ai[i];
		}
        return true;
	}

    /**
     * Solve the system A*x=b where A is a symmetric positive-definite matrix.
     */
    template<typename T, int N>
    inline bool SolvePositiveDefiniteSystem(T* x, const T* A, const T* b, T threshold) {
        T L[N*N];
        T y[N];
        
        // A=L*L'
        if(!Cholesky<T>(L, N, A, N, N, 0)) {
            return false;
        }
        SymmetricExtendLowerToUpper(L, N);
        
        if(!SolveLeftTriangularSystem(y, L, b, N, threshold)) {
            return false;
        }
        
        if(!SolveRightTriangularSystem(x, L, y, N, threshold)) {
            return false;
        }
        
        return true;
    }
    
} // vision