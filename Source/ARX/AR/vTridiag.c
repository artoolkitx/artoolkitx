/*
 *  vTridiag.c
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Shinsaku Hiura, Hirokazu Kato
 *
 */
/*******************************************************
 *
 * Author: Shinsaku Hiura, Hirokazu Kato
 *
 *         shinsaku@sys.es.osaka-u.ac.jp
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 2.1
 * Date: 99/07/16
 *
 *******************************************************/

#include <stdio.h>
#include <math.h>
#include <ARX/AR/ar.h>

int arVecTridiagonalize( ARMat *a, ARVec *d, ARVec *e )
{
    ARVec     wv1, wv2;
    ARdouble  *v;
    ARdouble  s, t, p, q;
    int     dim;
    int     i, j, k;

    if( a->clm != a->row )   return(-1);
    if( a->clm != d->clm )   return(-1);
    if( a->clm != e->clm+1 ) return(-1);
    dim = a->clm;

    for( k = 0; k < dim-2; k++ ) {
        v = &(a->m[k*dim]);
        d->v[k] = v[k];

        wv1.clm = dim-k-1;
        wv1.v = &(v[k+1]);
        e->v[k] = arVecHousehold(&wv1);
        if( e->v[k] == 0.0 ) continue;

        for( i = k+1; i < dim; i++ ) {
            s = 0.0;
            for( j = k+1; j < i; j++ ) {
                s += a->m[j*dim+i] * v[j];
            }
            for( j = i; j < dim; j++ ) {
                s += a->m[i*dim+j] * v[j];
            }
            d->v[i] = s;
        }

        wv1.clm = wv2.clm = dim-k-1;
        wv1.v = &(v[k+1]);
        wv2.v = &(d->v[k+1]);
        t = arVecInnerproduct( &wv1, &wv2 ) / 2;
        for( i = dim-1; i > k; i-- ) {
            p = v[i];
            q = d->v[i] -= t*p;
            for( j = i; j < dim; j++ ) {
                a->m[i*dim+j] -= p*(d->v[j]) + q*v[j];
            }
        }
    }

    if( dim >= 2) {
        d->v[dim-2] = a->m[(dim-2)*dim+(dim-2)];
        e->v[dim-2] = a->m[(dim-2)*dim+(dim-1)];
    }

    if( dim >= 1 ) d->v[dim-1] = a->m[(dim-1)*dim+(dim-1)];

    for( k = dim-1; k >= 0; k-- ) {
        v = &(a->m[k*dim]);
        if( k < dim-2 ) {
            for( i = k+1; i < dim; i++ ) {
                wv1.clm = wv2.clm = dim-k-1;
                wv1.v = &(v[k+1]);
                wv2.v = &(a->m[i*dim+k+1]);
                t = arVecInnerproduct( &wv1, &wv2 );
                for( j = k+1; j < dim; j++ ) a->m[i*dim+j] -= t * v[j];
            }
        }
        for( i = 0; i < dim; i++ ) v[i] = 0.0;
        v[k] = 1;
    }

    return(0);
}
