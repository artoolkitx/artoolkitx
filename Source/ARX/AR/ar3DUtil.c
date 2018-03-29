/*
 *  ar3DUtil.c
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
 * Revision: 1.0
 * Date: 00/08/31
 *
 *******************************************************/

#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>

ARdouble arGetStereoMatchingErrorSquare( AR3DStereoHandle *handle,
                                       ARMarkerInfo *marker_infoL, ARMarkerInfo *marker_infoR )
{
    ARdouble   pos2dL[4][2], pos2dR[4][2];
    ARdouble   err;
    int      dir;

    dir = marker_infoL->dir;
    pos2dL[0][0] = marker_infoL->vertex[(4-dir)%4][0];
    pos2dL[0][1] = marker_infoL->vertex[(4-dir)%4][1];
    pos2dL[1][0] = marker_infoL->vertex[(5-dir)%4][0];
    pos2dL[1][1] = marker_infoL->vertex[(5-dir)%4][1];
    pos2dL[2][0] = marker_infoL->vertex[(6-dir)%4][0];
    pos2dL[2][1] = marker_infoL->vertex[(6-dir)%4][1];
    pos2dL[3][0] = marker_infoL->vertex[(7-dir)%4][0];
    pos2dL[3][1] = marker_infoL->vertex[(7-dir)%4][1];
    dir = marker_infoR->dir;
    pos2dR[0][0] = marker_infoR->vertex[(4-dir)%4][0];
    pos2dR[0][1] = marker_infoR->vertex[(4-dir)%4][1];
    pos2dR[1][0] = marker_infoR->vertex[(5-dir)%4][0];
    pos2dR[1][1] = marker_infoR->vertex[(5-dir)%4][1];
    pos2dR[2][0] = marker_infoR->vertex[(6-dir)%4][0];
    pos2dR[2][1] = marker_infoR->vertex[(6-dir)%4][1];
    pos2dR[3][0] = marker_infoR->vertex[(7-dir)%4][0];
    pos2dR[3][1] = marker_infoR->vertex[(7-dir)%4][1];

    err = 0.0;
    err += arGetStereoMatchingError(handle, pos2dL[0], pos2dR[0]);
    err += arGetStereoMatchingError(handle, pos2dL[1], pos2dR[1]);
    err += arGetStereoMatchingError(handle, pos2dL[2], pos2dR[2]);
    err += arGetStereoMatchingError(handle, pos2dL[3], pos2dR[3]);
    err /= 4.0;

    return err;
}

ARdouble arGetStereoMatchingError( AR3DStereoHandle *handle, ARdouble pos2dL[2], ARdouble pos2dR[2] )
{
    ARMat   *cL, *cR, *cLi, *cRi;
    ARMat   *tL, *tR, *tLi, *tRi;
    ARMat   *w1, *w2, *w3;
    ARdouble  q1, q2, q3, t1, t2, t3;
    ARdouble  a1, b1, c1, a2, b2, c2;
    ARdouble  lL2, lR2;
    int     i, j;
                                                                                                   
    cL = arMatrixAlloc( 4, 4 );
    cR = arMatrixAlloc( 4, 4 );
    tL = arMatrixAlloc( 4, 4 );
    tR = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            cL->m[j*4+i] = handle->icpStereoHandle->matXcl2Ul[j][i];
        }
    }
    cL->m[12] = cL->m[13] = cL->m[14] = 0.0; cL->m[15] = 1.0;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            cR->m[j*4+i] = handle->icpStereoHandle->matXcr2Ur[j][i];
        }
    }
    cR->m[12] = cR->m[13] = cR->m[14] = 0.0; cR->m[15] = 1.0;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            tL->m[j*4+i] = handle->icpStereoHandle->matC2L[j][i];
        }
    }
    tL->m[12] = tL->m[13] = tL->m[14] = 0.0; tL->m[15] = 1.0;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            tR->m[j*4+i] = handle->icpStereoHandle->matC2R[j][i];
        }
    }
    tR->m[12] = tR->m[13] = tR->m[14] = 0.0; tR->m[15] = 1.0;

    cLi = arMatrixAllocInv( cL );
    cRi = arMatrixAllocInv( cR );
    tLi = arMatrixAllocInv( tL );
    tRi = arMatrixAllocInv( tR );

    w1 = arMatrixAllocMul( cR, tR );
    w2 = arMatrixAllocMul( w1, tLi );
    w3 = arMatrixAllocMul( w2, cLi );
    q1 = w3->m[0*4+0]*pos2dL[0] + w3->m[0*4+1]*pos2dL[1] + w3->m[0*4+2];
    q2 = w3->m[1*4+0]*pos2dL[0] + w3->m[1*4+1]*pos2dL[1] + w3->m[1*4+2];
    q3 = w3->m[2*4+0]*pos2dL[0] + w3->m[2*4+1]*pos2dL[1] + w3->m[2*4+2];
    t1 = w3->m[0*4+3];
    t2 = w3->m[1*4+3];
    t3 = w3->m[2*4+3];
    a1 = q3*t2 - q2*t3;
    b1 = q1*t3 - q3*t1;
    c1 = q2*t1 - q1*t2;

    arMatrixMul( w1, cL, tL );
    arMatrixMul( w2, w1, tRi );
    arMatrixMul( w3, w2, cRi );
    q1 = w3->m[0*4+0]*pos2dR[0] + w3->m[0*4+1]*pos2dR[1] + w3->m[0*4+2];
    q2 = w3->m[1*4+0]*pos2dR[0] + w3->m[1*4+1]*pos2dR[1] + w3->m[1*4+2];
    q3 = w3->m[2*4+0]*pos2dR[0] + w3->m[2*4+1]*pos2dR[1] + w3->m[2*4+2];
    t1 = w3->m[0*4+3];
    t2 = w3->m[1*4+3];
    t3 = w3->m[2*4+3];
    a2 = q3*t2 - q2*t3;
    b2 = q1*t3 - q3*t1;
    c2 = q2*t1 - q1*t2;

    arMatrixFree( w3 );
    arMatrixFree( w2 );
    arMatrixFree( w1 );
    arMatrixFree( tL );
    arMatrixFree( tR );
    arMatrixFree( tLi );
    arMatrixFree( tRi );
    arMatrixFree( cR );
    arMatrixFree( cL );
    arMatrixFree( cRi );
    arMatrixFree( cLi );

    lL2 = (a1*pos2dR[0]+b1*pos2dR[1]+c1)*(a1*pos2dR[0]+b1*pos2dR[1]+c1) / (a1*a1+b1*b1);
    lR2 = (a2*pos2dL[0]+b2*pos2dL[1]+c2)*(a2*pos2dL[0]+b2*pos2dL[1]+c2) / (a2*a2+b2*b2);

    return (lL2 + lR2)/2.0;
}

int arGetStereoMatching(AR3DStereoHandle *handle, ARdouble pos2dL[2], ARdouble pos2dR[2], ARdouble pos3d[3])
{
    ARMat   *matP,  *matQ;
    ARMat   *matPt, *matR, *matS, *matT;
    ARdouble   wL[3][4], wR[3][4];

    arUtilMatMul( (const ARdouble (*)[4])(handle->icpStereoHandle->matXcl2Ul), (const ARdouble (*)[4])(handle->icpStereoHandle->matC2L), wL );
    arUtilMatMul( (const ARdouble (*)[4])(handle->icpStereoHandle->matXcr2Ur), (const ARdouble (*)[4])(handle->icpStereoHandle->matC2R), wR );

    matP  = arMatrixAlloc(4,3);
    matPt = arMatrixAlloc(3,4);
    matQ  = arMatrixAlloc(4,1);
    matR  = arMatrixAlloc(3,3);
    matS  = arMatrixAlloc(3,1);
    matT  = arMatrixAlloc(3,1);

    matP->m[0*3+0] = matPt->m[0*4+0] = wL[0][0] - wL[2][0] * pos2dL[0];
    matP->m[0*3+1] = matPt->m[1*4+0] = wL[0][1] - wL[2][1] * pos2dL[0];
    matP->m[0*3+2] = matPt->m[2*4+0] = wL[0][2] - wL[2][2] * pos2dL[0];
    matP->m[1*3+0] = matPt->m[0*4+1] = wL[1][0] - wL[2][0] * pos2dL[1];
    matP->m[1*3+1] = matPt->m[1*4+1] = wL[1][1] - wL[2][1] * pos2dL[1];
    matP->m[1*3+2] = matPt->m[2*4+1] = wL[1][2] - wL[2][2] * pos2dL[1];

    matP->m[2*3+0] = matPt->m[0*4+2] = wR[0][0] - wR[2][0] * pos2dR[0];
    matP->m[2*3+1] = matPt->m[1*4+2] = wR[0][1] - wR[2][1] * pos2dR[0];
    matP->m[2*3+2] = matPt->m[2*4+2] = wR[0][2] - wR[2][2] * pos2dR[0];
    matP->m[3*3+0] = matPt->m[0*4+3] = wR[1][0] - wR[2][0] * pos2dR[1];
    matP->m[3*3+1] = matPt->m[1*4+3] = wR[1][1] - wR[2][1] * pos2dR[1];
    matP->m[3*3+2] = matPt->m[2*4+3] = wR[1][2] - wR[2][2] * pos2dR[1];
                                                                                                   
    matQ->m[0] = wL[2][3] * pos2dL[0] - wL[0][3];
    matQ->m[1] = wL[2][3] * pos2dL[1] - wL[1][3];
    matQ->m[2] = wR[2][3] * pos2dR[0] - wR[0][3];
    matQ->m[3] = wR[2][3] * pos2dR[1] - wR[1][3];

    arMatrixMul( matR, matPt, matP );
    arMatrixMul( matS, matPt, matQ );
    if( arMatrixSelfInv( matR ) < 0 ) {
        arMatrixFree( matP );
        arMatrixFree( matPt);
        arMatrixFree( matQ );
        arMatrixFree( matR );
        arMatrixFree( matS );
        arMatrixFree( matT );
        return -1;
    }
    arMatrixMul( matT, matR, matS );

    pos3d[0] = matT->m[0];
    pos3d[1] = matT->m[1];
    pos3d[2] = matT->m[2];
                                                                                                   
    arMatrixFree( matP );
    arMatrixFree( matPt);
    arMatrixFree( matQ );
    arMatrixFree( matR );
    arMatrixFree( matS );
    arMatrixFree( matT );
                                                                                                   
    return 0;
}
