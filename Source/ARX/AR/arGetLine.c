/*
 *  arGetLine.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.0
 * Date: 03/08/13
 *
 *******************************************************/

#include <stdio.h>
#include <ARX/AR/ar.h>

#ifdef ARDOUBLE_IS_FLOAT
#  define _0_5 0.5f
#  define _0_05 0.05f
#  define _0_0 0.0f
#  define EPSILON 0.0001f
#  define FABS(x) fabsf(x)
#else
#  define _0_5 0.5
#  define _0_05 0.05
#  define _0_0 0.0
#  define EPSILON 0.0001
#  define FABS(x) fabs(x)
#endif

int arGetLine(int x_coord[], int y_coord[], int coord_num, int vertex[], ARParamLTf *paramLTf,
              ARdouble line[4][3], ARdouble v[4][2])
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    ARdouble   w1;
    int      st, ed, n;
    int      i, j;

    ev     = arVecAlloc( 2 );
    mean   = arVecAlloc( 2 );
    evec   = arMatrixAlloc( 2, 2 );
    for( i = 0; i < 4; i++ ) {
        w1 = (ARdouble)(vertex[i+1]-vertex[i]+1) * _0_05 + _0_5;
        st = (int)(vertex[i]   + w1);
        ed = (int)(vertex[i+1] - w1);
        n = ed - st + 1;
        input  = arMatrixAlloc( n, 2 );
        for( j = 0; j < n; j++ ) {
#ifdef ARDOUBLE_IS_FLOAT
            if (arParamObserv2IdealLTf( paramLTf, (float)x_coord[st+j], (float)y_coord[st+j],
                                       &(input->m[j*2+0]), &(input->m[j*2+1]) ) < 0) goto bail;
#else
            float m0, m1;
            if (arParamObserv2IdealLTf( paramLTf, (float)x_coord[st+j], (float)y_coord[st+j], &m0, &m1 ) < 0) goto bail;
            input->m[j*2+0] = (ARdouble)m0;
            input->m[j*2+1] = (ARdouble)m1;
#endif
            //arParamObserv2Ideal( dist_factor, (ARdouble)x_coord[st+j], (ARdouble)y_coord[st+j],
            //                     &(input->m[j*2+0]), &(input->m[j*2+1]), dist_function_version );
        }
        if( arMatrixPCA(input, evec, ev, mean) < 0 ) goto bail;
        line[i][0] =  evec->m[1];
        line[i][1] = -evec->m[0];
        line[i][2] = -(line[i][0]*mean->v[0] + line[i][1]*mean->v[1]);
        arMatrixFree( input );
    }
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );

    for( i = 0; i < 4; i++ ) {
        w1 = line[(i+3)%4][0] * line[i][1] - line[i][0] * line[(i+3)%4][1];
        //if( w1 == _0_0 ) return(-1); // lines are parallel.
        if( FABS(w1) < EPSILON ) return(-1); // lines are close to parallel.
        v[i][0] = (  line[(i+3)%4][1] * line[i][2]
                   - line[i][1] * line[(i+3)%4][2] ) / w1;
        v[i][1] = (  line[i][0] * line[(i+3)%4][2]
                   - line[(i+3)%4][0] * line[i][2] ) / w1;
    }

    return 0;
    
bail:
    arMatrixFree( input );
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );
    return -1;
}
