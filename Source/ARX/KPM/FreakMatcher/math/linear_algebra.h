//
//  linear_algebra.h
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

#include "math_utils.h"
#include "matrix.h"
#include <limits>
namespace vision {
    
    /**
     * Create a 3x3 identity matrix.
     */
    template<typename T>
	inline void Identity3x3(T A[9]) {
		A[0] = 1;	A[1] = 0;	A[2] = 0;
		A[3] = 0;	A[4] = 1;	A[5] = 0;
		A[6] = 0;	A[7] = 0;	A[8] = 1;
	}
    
    /**
     * Create a 3x4 identity matrix.
     */
    template<typename T>
	inline void Identity3x4(T A[12]) {
		A[0] = 1;	A[1] = 0;	A[2] = 0;   A[3] = 0;
		A[4] = 0;	A[5] = 1;	A[6] = 0;   A[7] = 0;
		A[8] = 0;	A[9] = 0;	A[10] = 1;  A[11] = 0;
	}
    
    /**
	 * Computes the cofactor of a 2x2 matrix.
	 *
	 * A = [a	b;
	 *		c	d]
	 *
	 * |A| = (a * d) - (b * c)
	 */
	template<typename T>
	inline T Cofactor2x2(T a, T b, T c, T d) {
		return (a * d) - (b * c);
	}

    /**
	 * Computes the cofactor of a 2x2 matrix.
	 *
	 * A = [a	b;
	 *		c	d]
	 *
	 * |A| = (a * d) - (b * c)
	 */
    template<typename T>
	inline T Cofactor2x2(const T A[4]) {
		return Cofactor2x2(A[0], A[1], A[2], A[3]);
	}
    
    /**
     * Cofactor of a symmetrix 2x2 matrix.
     *
	 * A = [a	b;
	 *		b	c]
     */
    template<typename T>
	inline T Cofactor2x2(T a, T b, T c) {
		return (a * c) - (b * b);
	}
    
    /**
	 * Compute the determinant of a 2x2 matrix
	 */
	template<typename T>
	inline T Determinant2x2(const T A[4]) {
		return Cofactor2x2(A);
	}
    
    /**
     * Compute the determinant of a 3x3 matrix.
     */
    template<typename T>
	inline T Determinant3x3(const T A[9]) {
        T C1 = Cofactor2x2(A[4], A[5], A[7], A[8]);
        T C2 = Cofactor2x2(A[3], A[5], A[6], A[8]);
        T C3 = Cofactor2x2(A[3], A[4], A[6], A[7]);
		return (A[0] * C1) - (A[1] * C2) + (A[2] * C3);
	}
    
    /**
     * Compute the determinant of a 3x3 symmetric matrix.
     */
    template<typename T>
    inline T DeterminantSymmetric3x3(const T A[9]) {
        T a = -A[8]*sqr(A[1]);
        T b = 2*A[1]*A[2]*A[5];
        T c = A[4]*sqr(A[2]);
        T d = A[0]*sqr(A[5]);
        T e = A[0]*A[4]*A[8];
        return a + b - c - d + e;
    }
    
    /**
     * Compute the inverse of a 2x2 matrix.
     */
    template<typename T>
	bool MatrixInverse2x2(T B[4], const T A[4], T threshold) {
		const T det = Determinant2x2(A);
		
		if(std::abs(det) <= threshold) {
			return false;
		}
		
		T one_over_det = 1./det;
		
		B[0] = A[3] * one_over_det;
		B[1] = -A[1] * one_over_det;
		B[2] = -A[2] * one_over_det;
		B[3] = A[0] * one_over_det;
		
		return true;
	}
    
    /**
     * Compute the inverse of a 3x3 matrix.
     */
    template<typename T>
	bool MatrixInverse3x3(T B[9], const T A[9], T threshold) {
		const T det = Determinant3x3(A);
		
		if(std::abs(det) <= threshold) {
			return false;
		}
		
		T one_over_det = 1./det;
		
		B[0] = Cofactor2x2(A[4], A[5], A[7], A[8]) * one_over_det;
		B[1] = Cofactor2x2(A[2], A[1], A[8], A[7]) * one_over_det;
		B[2] = Cofactor2x2(A[1], A[2], A[4], A[5]) * one_over_det;
		B[3] = Cofactor2x2(A[5], A[3], A[8], A[6]) * one_over_det;
		B[4] = Cofactor2x2(A[0], A[2], A[6], A[8]) * one_over_det;
		B[5] = Cofactor2x2(A[2], A[0], A[5], A[3]) * one_over_det;
		B[6] = Cofactor2x2(A[3], A[4], A[6], A[7]) * one_over_det;
		B[7] = Cofactor2x2(A[1], A[0], A[7], A[6]) * one_over_det;
		B[8] = Cofactor2x2(A[0], A[1], A[3], A[4]) * one_over_det;
		
		return true;
	}
    
    /**
     * Compute the inverse of a 3x3 symmetric matrix.
     */
    template<typename T>
	bool MatrixInverseSymmetric3x3(T B[9], const T A[9], T threshold) {
        T det = DeterminantSymmetric3x3(A);
		
		if(std::abs(det) <= threshold) {
			return false;
		}
		
		T one_over_det = 1./det;
		
		B[0] = Cofactor2x2(A[4], A[5], A[8]) * one_over_det;
		B[1] = Cofactor2x2(A[2], A[1], A[8], A[7]) * one_over_det;
		B[2] = Cofactor2x2(A[1], A[2], A[4], A[5]) * one_over_det;
		B[4] = Cofactor2x2(A[0], A[2], A[8]) * one_over_det;
		B[5] = Cofactor2x2(A[2], A[0], A[5], A[3]) * one_over_det;
		B[8] = Cofactor2x2(A[0], A[1], A[4]) * one_over_det;
		
        // Symmetric terms
        B[3] = B[1];
        B[6] = B[2];
        B[7] = B[5];
        
		return true;
	}

    /**
     * Solve a 2x2 linear system.
     */
    template<typename T>
    inline bool SolveLinearSystem2x2(T x[2], const T A[4], const T b[2]) {
        T Ainv[4];
        
        // Inverse of A
        if(!MatrixInverse2x2(Ainv, A, std::numeric_limits<T>::epsilon())) {
            return false;
        }
        
        // x = inv(A)*b
        Multiply_2x2_2x1(x, Ainv, b);
        
        return true;
    }
    
    /**
     * Solve a 3x3 symmetric linear system.
     */
    template<typename T>
    inline bool SolveSymmetricLinearSystem3x3(T x[3], const T A[9], const T b[3]) {
        T Ainv[9];
        
        // Inverse of A
        if(!MatrixInverseSymmetric3x3(Ainv, A, std::numeric_limits<T>::epsilon())) {
            return false;
        }
        
        // x = inv(A)*b
        Multiply_3x3_3x1(x, Ainv, b);
        
        return true;
    }
    
    /**
     * Compute the dot product.
     */
    template<typename T>
    inline T DotProduct4(const T a[4], const T b[4]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
    }
    template<typename T>
    inline T DotProduct9(const T a[9], const T b[9]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3] + a[4]*b[4] + a[5]*b[5] + a[6]*b[6] + a[7]*b[7] + a[8]*b[8];
    }

    /**
     * Compute the dot product.
     */
    template<typename T>
	inline T DotProduct(const T* a, const T* b, int n) {
		T x = 0;
		for(int i = 0; i < n; i++) {
			x += (a[i]*b[i]);
		}
		return x;
	}
    
    /**
     * Sum sqaured.
     */
    template<typename T>
    inline T SumSquares9(const T x[9]) {
        return DotProduct9(x, x);
    }
    
    /**
     * Scale a vector.
     */
    template<typename T>
    inline void ScaleVector4(T dst[4], const T src[4], T s) {
        dst[0] = src[0]*s;
        dst[1] = src[1]*s;
        dst[2] = src[2]*s;
        dst[3] = src[3]*s;
    }
    template<typename T>
    inline void ScaleVector8(T dst[8], const T src[8], T s) {
        dst[0] = src[0]*s;
        dst[1] = src[1]*s;
        dst[2] = src[2]*s;
        dst[3] = src[3]*s;
        dst[4] = src[4]*s;
        dst[5] = src[5]*s;
        dst[6] = src[6]*s;
        dst[7] = src[7]*s;
    }
    template<typename T>
    inline void ScaleVector9(T dst[9], const T src[9], T s) {
        dst[0] = src[0]*s;
        dst[1] = src[1]*s;
        dst[2] = src[2]*s;
        dst[3] = src[3]*s;
        dst[4] = src[4]*s;
        dst[5] = src[5]*s;
        dst[6] = src[6]*s;
        dst[7] = src[7]*s;
        dst[8] = src[8]*s;
    }
    
    /**
     * Accumulate a scaled vector.
     *
     * dst += src*s
     */
    template<typename T>
    inline void AccumulateScaledVector9(T dst[9], const T src[9], T s) {
        dst[0] += src[0]*s;
        dst[1] += src[1]*s;
        dst[2] += src[2]*s;
        dst[3] += src[3]*s;
        dst[4] += src[4]*s;
        dst[5] += src[5]*s;
        dst[6] += src[6]*s;
        dst[7] += src[7]*s;
        dst[8] += src[8]*s;
    }
    
    /**
     * w = a*u + b*v
     */
    template<typename T>
    inline void AddScaledVectors4(T w[4], const T u[4], const T v[4], T a, T b) {
        w[0] = a*u[0] + b*v[0];
        w[1] = a*u[1] + b*v[1];
        w[2] = a*u[2] + b*v[2];
        w[3] = a*u[3] + b*v[3];
    }
    
    /**
     * Subtract two vectors.
     */
    template<typename T>
	inline void SubVector2(T c[2], const T a[2], const T b[2]) {
		c[0] = a[0] - b[0];
		c[1] = a[1] - b[1];
	}
    
    /**
     * Update the outer product such that A += x*x'.
     */
    template<typename T>
    inline void UpdateOuterProduct2x2(T A[4], const T x[2]) {
        A[0] += x[0]*x[0];
        A[1] += x[0]*x[1];
        A[3] += x[1]*x[1];
    }
    
    /**
     * Compute b += -J'r
     */
    template<typename T>
    inline void UpdateGaussNewtonOperations2x2(T b[2], const T J[2], T residual) {
        b[0] -= J[0]*residual;
        b[1] -= J[1]*residual;
    }
    
    /**
	 * Extend the lower left triangular matrix to the upper right
	 *
	 * @param[in,out] A n x n square matrix
	 * @param[in] n size of matrix
	 */
	template<typename T>
	inline void SymmetricExtendLowerToUpper(T* A, int n) {
		int i, j;
		for(i = 1; i < n; i++) {
			T* Ai = &A[i*n];
			for(j = 0; j < i; j++) {
				A[j*n+i] = Ai[j];
			}
		}
	}
    
    /**
	 * Extend the upper right triangular matrix to the lower left
	 *
	 * @param[in,out] A n x n square matrix
	 * @param[in] n size of matrix
	 */
	template<typename T>
	inline void SymmetricExtendUpperToLower(T* A, int n) {
		int i, j;
		for(i = 1; i < n; i++) {
			T* Ai = &A[i*n];
			for(j = 0; j < i; j++) {
				Ai[j] = A[j*n+i];
			}
		}
	}
    
} // vision
