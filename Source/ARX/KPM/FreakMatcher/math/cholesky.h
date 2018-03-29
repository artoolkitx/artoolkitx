//
//  cholesky.h
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
     * Compute the Cholesky decomposition of the square matrix A.
     */
    template<typename T>
    bool Cholesky(T* L, int strideL, const T* A, int strideA, int n, T threshold = 0) {
        int i, j, k;
        T s;
        
        for(i = 0; i < n; i++) {
            T* Li = &L[i*strideL];
            const T* Ai = &A[i*strideA];
            for(j = 0; j <= i; j++) {
                T* Lj = &L[j*strideL];
                for(s = Ai[j], k = 0; k < j; k++) {
                    s -= Li[k]*Lj[k];
                }
                if(i == j) {
                    if(s < 0) {
                        return false;
                    }
                    Li[j] = std::sqrt(s);
                }
                else if(std::abs(Lj[j]) > threshold) {
                    Li[j] = s/Lj[j];
                }
                else {
                    return false;
                }
            }
        }
        
        return true;
    }
    
} // vision