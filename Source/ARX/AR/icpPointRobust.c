/*
 *  icpPointRobust.c
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>

#ifndef ARDOUBLE_IS_FLOAT
#define     K2_FACTOR     4.0
#else
#define     K2_FACTOR     4.0f
#endif

static void   icpGetXw2XcCleanup( char *message, ARdouble *J_U_S, ARdouble *dU, ARdouble *E, ARdouble *E2 );
static int    compE(const void *a, const void *b );

int icpPointRobust( ICPHandleT   *handle,
                    ICPDataT     *data,
                    ARdouble        initMatXw2Xc[3][4],
                    ARdouble        matXw2Xc[3][4],
                    ARdouble       *err )
{
    ICP2DCoordT   U;
    ARdouble       *J_U_S;
    ARdouble       *dU, dx, dy;
    ARdouble       *E, *E2, K2, W;
    ARdouble        matXw2U[3][4];
    ARdouble        dS[6];
    ARdouble        err0, err1;
    int           inlierNum;
    int           i, j, k;

    if( data->num < 4 ) return -1;

    inlierNum = (int)(data->num * handle->inlierProb) - 1;
    if( inlierNum < 3 ) inlierNum = 3;

    if( (J_U_S = (ARdouble *)malloc( sizeof(ARdouble)*12*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        return -1;
    }
    if( (dU = (ARdouble *)malloc( sizeof(ARdouble)*2*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_S);
        return -1;
    }
    if( (E = (ARdouble *)malloc( sizeof(ARdouble)*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_S);
        free(dU);
        return -1;
    }
    if( (E2 = (ARdouble *)malloc( sizeof(ARdouble)*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_S);
        free(dU);
        free(E);
        return -1;
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matXw2Xc[j][i] = initMatXw2Xc[j][i];
    }


    for( i = 0;; i++ ) {
#if ICP_DEBUG
        icpDispMat( "matXw2Xc", &(matXw2Xc[0][0]), 3, 4 );
#endif
        arUtilMatMul( (const ARdouble (*)[4])handle->matXc2U, (const ARdouble (*)[4])matXw2Xc, matXw2U );

        for( j = 0; j < data->num; j++ ) {
            if( icpGetU_from_X_by_MatX2U( &U, matXw2U, &(data->worldCoord[j]) ) < 0 ) {
                icpGetXw2XcCleanup("icpGetU_from_X_by_MatX2U",J_U_S,dU,E,E2);
                return -1;
            }
            dx = data->screenCoord[j].x - U.x;
            dy = data->screenCoord[j].y - U.y;
            dU[j*2+0] = dx;
            dU[j*2+1] = dy;
            E[j] = E2[j] = dx*dx + dy*dy;
        }
        qsort(E2, data->num, sizeof(ARdouble), compE);
        K2 = E2[inlierNum] * K2_FACTOR;
        if( K2 < 16.0 ) K2 = 16.0;

        err1 = 0.0;
        for( j = 0; j < data->num; j++ ) {
            if( E2[j] > K2 ) err1 += K2/6.0;
            else err1 += K2/6.0 * (1.0 - (1.0-E2[j]/K2)*(1.0-E2[j]/K2)*(1.0-E2[j]/K2));
        }
        err1 /= data->num;
#if ICP_DEBUG
        ARLOGd("Loop[%d]: k^2 = %f, err = %15.10f\n", i, K2, err1);
#endif
        //ARLOGd("  %f %f %f %f\n", E2[0], E2[1], E2[2], E2[3]);
        //ARLOGd("  %f %f %f %f\n", E[0], E[1], E[2], E[3]);
        if( err1 < handle->breakLoopErrorThresh ) break;
        if( i > 0 && err1 < handle->breakLoopErrorThresh2 && err1/err0 > handle->breakLoopErrorRatioThresh ) break;
        if( i == handle->maxLoop ) break;
        err0 = err1;

        k = 0;
        for( j = 0; j < data->num; j++ ) {
            if( E[j] <= K2 ) {
                if( icpGetJ_U_S( (ARdouble (*)[6])(&J_U_S[6*k]), handle->matXc2U, matXw2Xc, &(data->worldCoord[j]) ) < 0 ) {
                    icpGetXw2XcCleanup("icpGetJ_U_S", J_U_S, dU, E, E2);
                    return -1;
                }
#if ICP_DEBUG
                icpDispMat( "J_U_S", (ARdouble *)(&J_U_S[6*k]), 2, 6 );
#endif
                W = (1.0 - E[j]/K2)*(1.0 - E[j]/K2);
                J_U_S[k*6+0] *= W;
                J_U_S[k*6+1] *= W;
                J_U_S[k*6+2] *= W;
                J_U_S[k*6+3] *= W;
                J_U_S[k*6+4] *= W;
                J_U_S[k*6+5] *= W;
                J_U_S[k*6+6] *= W;
                J_U_S[k*6+7] *= W;
                J_U_S[k*6+8] *= W;
                J_U_S[k*6+9] *= W;
                J_U_S[k*6+10] *= W;
                J_U_S[k*6+11] *= W;
                dU[k+0] = dU[j*2+0] * W;
                dU[k+1] = dU[j*2+1] * W;
                k+=2;
            }
        }

        if( k < 6 ) {
            icpGetXw2XcCleanup("icpPointRobust: k < 6",J_U_S,dU,E,E2);
            return -1;
        }

        if( icpGetDeltaS( dS, dU, (ARdouble (*)[6])J_U_S, k ) < 0 ) {
            icpGetXw2XcCleanup("icpGetDeltaS",J_U_S,dU,E,E2);
            return -1;
        }

        icpUpdateMat( matXw2Xc, dS );
    }

#if ICP_DEBUG
    ARLOGd("*********** %f\n", err1);
    ARLOGd("Loop = %d\n", i);
#endif

    *err = err1;
    free(J_U_S);
    free(dU);
    free(E);
    free(E2);

    return 0;
}

static void icpGetXw2XcCleanup( char *message, ARdouble *J_U_S, ARdouble *dU, ARdouble *E, ARdouble *E2 )
{
    ARLOGd("Error: %s\n", message);
    free(J_U_S);
    free(dU);
    free(E);
    free(E2);
}

static int compE( const void *a, const void *b )
{
    ARdouble  c;
    c = *(ARdouble *)a - *(ARdouble *)b;
    if( c < 0.0 ) return -1;
    if( c > 0.0 ) return  1;
    return 0;
}
