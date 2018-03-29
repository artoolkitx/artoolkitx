//
//  partial_sort.h
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
     * Perform a partial sort of an array. This algorithm is based on
     * Niklaus Wirth's k-smallest.
     *
     * @param[in/out] a array of elements
     * @param[in] n size of a
     * @param[in] k kth element starting from 1, i.e. 1st smallest, 2nd smallest, etc.
     */
    template<typename T>
	inline T PartialSort(T a[], int n, int k) {
		int i, j, l, m, k_minus_1;
		T x;
        
        ASSERT(n > 0, "n must be positive");
        ASSERT(k > 0, "k must be positive");
        
        k_minus_1 = k-1;
		
		l=0 ; m=n-1;
		while(l<m) {
			x=a[k_minus_1];
			i=l;
			j=m;
			do {
				while(a[i]<x) i++;
				while(x<a[j]) j--;
				if(i<=j) {
                    //std::swap<T>(a[i],a[j]); // FIXME: 
					std::swap(a[i], a[j]);
					i++; j--;
				}
			} while (i<=j);
			if(j<k_minus_1) l=i;
			if(k_minus_1<i) m=j;
		}
		return a[k_minus_1];
	}
    template<typename T1, typename T2>
	inline std::pair<T1, T2> PartialSort(std::pair<T1, T2> a[], int n, int k) {
		int i, j, l, m, k_minus_1;
		std::pair<T1, T2> x;
        
        ASSERT(n > 0, "n must be positive");
        ASSERT(k > 0, "k must be positive");
        
        k_minus_1 = k-1;
		
		l=0 ; m=n-1;
		while(l<m) {
			x=a[k_minus_1];
			i=l;
			j=m;
			do {
				while(a[i]<x) i++;
				while(x<a[j]) j--;
				if(i<=j) {
                    //std::swap<std::pair<T1, T2> >(a[i],a[j]); // FIXME: 
					std::swap(a[i], a[j]); // FIXME: 
					i++; j--;
				}
			} while (i<=j);
			if(j<k_minus_1) l=i;
			if(k_minus_1<i) m=j;
		}
		return a[k_minus_1];
	}
    
    /**
     * Find the median of an array.
     */
    template<typename T>
	inline T FastMedian(T a[], int n) {
		return PartialSort(a, n, (((n)&1)?((n)/2):(((n)/2)-1)));
	}
    template<typename T1, typename T2>
	inline std::pair<T1, T2> FastMedian(std::pair<T1, T2> a[], int n) {
		return PartialSort(a, n, (((n)&1)?((n)/2):(((n)/2)-1)));
	}
    
} // vision