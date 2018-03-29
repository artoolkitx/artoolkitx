/*
 *  icpPoint.c
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
#include <ARX/AR/matrix.h>
#include <ARX/AR/icp.h>


static void   icpGetXw2XcCleanup( char *message, ARdouble *J_U_S, ARdouble *dU );

int icpPoint( ICPHandleT   *handle,
              ICPDataT     *data,
              ARdouble        initMatXw2Xc[3][4],
              ARdouble        matXw2Xc[3][4],
              ARdouble       *err )
{
    ICP2DCoordT   U;
    ARdouble        *J_U_S;
    ARdouble        *dU, dx, dy;
    ARdouble         matXw2U[3][4];
    ARdouble         dS[6];
    ARdouble         err0, err1;
    int           i, j;

    if( data->num < 3 ) return -1;

    if( (J_U_S = (ARdouble *)malloc( sizeof(ARdouble)*12*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        return -1;
    }
    if( (dU = (ARdouble *)malloc( sizeof(ARdouble)*2*(data->num) )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_S);
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

        err1 = 0.0;
        for( j = 0; j < data->num; j++ ) {
            if( icpGetU_from_X_by_MatX2U( &U, matXw2U, &(data->worldCoord[j]) ) < 0 ) {
                icpGetXw2XcCleanup("icpGetU_from_X_by_MatX2U", J_U_S, dU);
                return -1;
            }
            dx = data->screenCoord[j].x - U.x;
            dy = data->screenCoord[j].y - U.y;
            err1 += dx*dx + dy*dy;
            dU[j*2+0] = dx;
            dU[j*2+1] = dy;
        }
        err1 /= data->num;
#if ICP_DEBUG
        ARLOGd("Loop[%d]: err = %15.10f\n", i, err1);
#endif
        if( err1 < handle->breakLoopErrorThresh ) break;
        if( i > 0 && err1 < handle->breakLoopErrorThresh2 && err1/err0 > handle->breakLoopErrorRatioThresh ) break;
        if( i == handle->maxLoop ) break;
        err0 = err1;

        for( j = 0; j < data->num; j++ ) {
            if( icpGetJ_U_S( (ARdouble (*)[6])(&J_U_S[12*j]), handle->matXc2U, matXw2Xc, &(data->worldCoord[j]) ) < 0 ) {
                icpGetXw2XcCleanup("icpGetJ_U_S", J_U_S, dU);
                return -1;
            }
#if ICP_DEBUG
            icpDispMat( "J_U_S", (ARdouble *)(&J_U_S[j*12]), 2, 6 );
#endif
        }
        if( icpGetDeltaS( dS, dU, (ARdouble (*)[6])J_U_S, (data->num)*2 ) < 0 ) {
            icpGetXw2XcCleanup("icpGetDeltaS", J_U_S, dU);
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

    return 0;
}

static void icpGetXw2XcCleanup( char *message, ARdouble *J_U_S, ARdouble *dU )
{
    ARLOGd("Error: %s\n", message);
    free(J_U_S);
    free(dU);
}
