//
//  matrix.h
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

#include "framework/error.h"

namespace vision {
    
    /**
     * C = A*B where A,B and C are 2x2 matrices.
     */
    template<typename T>
    inline void Multiply2x2_2x2(T C[4], const T A[4], const T B[4]) {
        C[0] = A[0]*B[0] + A[1]*B[2];
        C[1] = A[0]*B[1] + A[1]*B[3];
        C[2] = A[2]*B[0] + A[3]*B[2];
        C[3] = A[2]*B[1] + A[3]*B[3];
    }
    
    /**
     * C = A*B where A,B and C are 3x3 matrices.
     */
    template<typename T>
	inline void Multiply3x3_3x3(T C[9], const T A[9], const T B[9]) {
		C[0] = A[0]*B[0] + A[1]*B[3] + A[2]*B[6];
		C[1] = A[0]*B[1] + A[1]*B[4] + A[2]*B[7];
		C[2] = A[0]*B[2] + A[1]*B[5] + A[2]*B[8];
		C[3] = A[3]*B[0] + A[4]*B[3] + A[5]*B[6];
		C[4] = A[3]*B[1] + A[4]*B[4] + A[5]*B[7];
		C[5] = A[3]*B[2] + A[4]*B[5] + A[5]*B[8];
		C[6] = A[6]*B[0] + A[7]*B[3] + A[8]*B[6];
		C[7] = A[6]*B[1] + A[7]*B[4] + A[8]*B[7];
		C[8] = A[6]*B[2] + A[7]*B[5] + A[8]*B[8];
	}
    
    /**
     * y = A*x where A is a 2x2 and x is a 2x1.
     */
    template<typename T>
	inline void Multiply_2x2_2x1(T y[2], const T A[4], const T x[2]) {
		y[0] = A[0]*x[0] + A[1]*x[1];
		y[1] = A[2]*x[0] + A[3]*x[1];
	}
    
    /**
     * y = A*x where A is a 3x3 and x is a 3x1.
     */
    template<typename T>
	inline void Multiply_3x3_3x1(T y[3], const T A[9], const T x[3]) {
		y[0] = A[0]*x[0] + A[1]*x[1] + A[2]*x[2];
		y[1] = A[3]*x[0] + A[4]*x[1] + A[5]*x[2];
		y[2] = A[6]*x[0] + A[7]*x[1] + A[8]*x[2];
	}
    
    /**
     * General matrix multiplication.
     *
     * C = A*B
     */
    template<typename T>
    inline void Multiply(T* C,
                         const T* A,
                         int Arows,
                         int Acols,
                         const T* B,
                         int Brows,
                         int Bcols) {
        ASSERT(Acols == Brows, "Number of columns of A must be the same as the number of rows of B");
		T sum;
		for(int i = 0; i < Arows; ++i) {
			const int A_offset = i*Acols;
			const int B_offset = i*Bcols;
			const T* Aptr = &A[A_offset];
			for(int j = 0; j < Bcols; ++j) {
				sum = *Aptr * B[j];
				for(int k = 1; k < Acols; ++k) {
					sum += Aptr[k] * B[k*Bcols+j];
				}
				C[B_offset+j] = sum;
			}
		}
	}
    
    /**
     * C += A'*A
     */
    template<typename T>
    inline void MultiplyAndAccumulateAtA(T* C, const T* A, int Arows, int Acols) {
        int Brows = Acols;
		for(int i = 0; i < Brows; ++i) {
            const int shift = i*Acols;
			for(int j = i; j < Acols; ++j) {
				T sum = A[i] * A[j];
				for(int k = 1; k < Arows; ++k) {
                    sum += A[k*Acols+i] * A[k*Acols+j];
				}
				C[shift+j] += sum;
			}
		}
    }
    
    /**
     *  y += A'*x
     */
    template<class T>
    inline void MultiplyAndAccumulateAtx(T* y, const T* A, int rows, int cols, const T* x) {
		T sum;
		for(int i = 0; i < cols; i++) {
			sum = A[i]*x[0];
			for(int j = 1; j < rows; j++) {
				sum += A[j*cols+i] * x[j];
			}
			y[i] += sum;
		}
	}
    
} // vision