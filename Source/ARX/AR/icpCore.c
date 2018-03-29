/*
 *  icpCore.c
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
#include <ARX/AR/icpCore.h>

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#  define COS cosf
#  define SIN sinf
#  define ONE 1.0f
#else
#  define SQRT sqrt
#  define COS cos
#  define SIN sin
#  define ONE 1.0
#endif


static int icpGetJ_U_Xc( ARdouble J_U_Xc[2][3], ARdouble matXc2U[3][4], ICP3DCoordT *cameraCoord );
static int icpGetJ_Xc_S( ARdouble J_Xc_S[3][6], ICP3DCoordT *cameraCoord, ARdouble T0[3][4], ICP3DCoordT *worldCoord );
static int icpGetJ_T_S( ARdouble J_T_S[12][6] );
static int icpGetQ_from_S( ARdouble q[7], ARdouble s[6] );
static int icpGetMat_from_Q( ARdouble mat[3][4], ARdouble q[7] );

int icpGetXc_from_Xw_by_MatXw2Xc( ICP3DCoordT *Xc, ARdouble matXw2Xc[3][4], ICP3DCoordT *Xw )
{
    Xc->x = matXw2Xc[0][0] * Xw->x + matXw2Xc[0][1] * Xw->y + matXw2Xc[0][2] * Xw->z + matXw2Xc[0][3];
    Xc->y = matXw2Xc[1][0] * Xw->x + matXw2Xc[1][1] * Xw->y + matXw2Xc[1][2] * Xw->z + matXw2Xc[1][3];
    Xc->z = matXw2Xc[2][0] * Xw->x + matXw2Xc[2][1] * Xw->y + matXw2Xc[2][2] * Xw->z + matXw2Xc[2][3];

    return 0;
}

int icpGetU_from_X_by_MatX2U( ICP2DCoordT *u, ARdouble matX2U[3][4], ICP3DCoordT *coord3d )
{
    ARdouble    hx, hy, h;

    hx = matX2U[0][0] * coord3d->x + matX2U[0][1] * coord3d->y
       + matX2U[0][2] * coord3d->z + matX2U[0][3];
    hy = matX2U[1][0] * coord3d->x + matX2U[1][1] * coord3d->y
       + matX2U[1][2] * coord3d->z + matX2U[1][3];
    h  = matX2U[2][0] * coord3d->x + matX2U[2][1] * coord3d->y
       + matX2U[2][2] * coord3d->z + matX2U[2][3];

    if( h == 0.0 ) return -1;

    u->x = hx / h;
    u->y = hy / h;

    return 0;
}

int icpGetJ_U_S( ARdouble J_U_S[2][6], ARdouble matXc2U[3][4], ARdouble matXw2Xc[3][4], ICP3DCoordT *worldCoord )
{
    ARdouble        J_Xc_S[3][6];
    ARdouble        J_U_Xc[2][3];
    ICP3DCoordT   Xc;
    int           i, j, k;

    if( icpGetJ_Xc_S( J_Xc_S, &Xc, matXw2Xc, worldCoord ) < 0 ) {
        ARLOGe("Error: icpGetJ_Xc_S\n");
        return -1;
    }
#if ICP_DEBUG
    icpDispMat( "J_Xc_S", (ARdouble *)J_Xc_S, 3, 6 );
#endif

    if( icpGetJ_U_Xc( J_U_Xc, matXc2U, &Xc ) < 0 ) {
        ARLOGe("Error: icpGetJ_U_Xc\n");
        return -1;
    }
#if ICP_DEBUG
    icpDispMat( "J_U_Xc", (ARdouble *)J_U_Xc, 2, 3 );
#endif

    for( j = 0; j < 2; j++ ) {
        for( i = 0; i < 6; i++ ) {
            J_U_S[j][i] = 0.0;
            for( k = 0; k < 3; k++ ) {
                J_U_S[j][i] += J_U_Xc[j][k] * J_Xc_S[k][i];
            }
        }
    }
#if ICP_DEBUG
    icpDispMat( "J_U_S", (ARdouble *)J_U_S, 2, 6 );
#endif

    return 0;
}

int icpGetDeltaS( ARdouble S[6], ARdouble dU[], ARdouble J_U_S[][6], int n )
{
    ARMat   matS, matU, matJ;
    ARMat  *matJt, *matJtJ, *matJtU;

    matS.row = 6;
    matS.clm = 1;
    matS.m   = S;

    matU.row = n;
    matU.clm = 1;
    matU.m   = dU;

    matJ.row = n;
    matJ.clm = 6;
    matJ.m   = &J_U_S[0][0];

#if ICP_DEBUG
    ARLOGd("cc1 - matU\n");
    arMatrixDisp( &matU );
    ARLOGd("cc1 - matJ\n");
    arMatrixDisp( &matJ );
#endif
    matJt = arMatrixAllocTrans( &matJ );
    if( matJt == NULL ) return -1;
#if ICP_DEBUG
    ARLOGd("cc1 - matJt\n");
    arMatrixDisp( matJt );
#endif
    matJtJ = arMatrixAllocMul( matJt, &matJ );
    if( matJtJ == NULL ) {
        arMatrixFree( matJt );
        return -1;
    }
#if ICP_DEBUG
    ARLOGd("cc2 - matJtJ\n");
    arMatrixDisp( matJtJ );
#endif
    matJtU = arMatrixAllocMul( matJt, &matU );
    if( matJtU == NULL ) {
        arMatrixFree( matJt );
        arMatrixFree( matJtJ );
        return -1;
    }
#if ICP_DEBUG
    ARLOGd("cc3 -- matJtU\n");
    arMatrixDisp( matJtU );
#endif
    if( arMatrixSelfInv(matJtJ) < 0 ) {
        arMatrixFree( matJt );
        arMatrixFree( matJtJ );
        arMatrixFree( matJtU );
        return -1;
    }

#if ICP_DEBUG
    ARLOGd("cc4 -- matJtJ_Inv\n");
    arMatrixDisp( matJtJ );
#endif
    arMatrixMul( &matS, matJtJ, matJtU );
    arMatrixFree( matJt );
    arMatrixFree( matJtJ );
    arMatrixFree( matJtU );
#if ICP_DEBUG
    ARLOGd("cc5 -- matS\n");
    arMatrixDisp( &matS );
#endif

    return 0;
}

int icpUpdateMat( ARdouble matXw2Xc[3][4], ARdouble dS[6] )
{
    ARdouble   q[7];
    ARdouble   mat[3][4], mat2[3][4];
    int      i, j;

    if( icpGetQ_from_S(q, dS) < 0 ) return -1;
    if( icpGetMat_from_Q( mat, q ) < 0 ) return -1;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat2[j][i] = matXw2Xc[j][0] * mat[0][i]
                       + matXw2Xc[j][1] * mat[1][i]
                       + matXw2Xc[j][2] * mat[2][i];
        }
        mat2[j][3] += matXw2Xc[j][3];
    }

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matXw2Xc[j][i] = mat2[j][i];
    }

    return 0;
}

#if ICP_DEBUG
void icpDispMat( char *title, ARdouble *mat, int row, int clm )
{
    int    i, j;

    ARLOGd("====== %s ========\n", title);
    for( j = 0; j < row; j++ ) {
        for( i = 0; i < clm; i++ ) {
            ARLOGd("%7.5f ", mat[j*clm+i]);
        }
        ARLOGd("\n");
    }
    ARLOGd("-------------------------\n");

    return;
}
#endif

static int icpGetJ_U_Xc( ARdouble J_U_Xc[2][3], ARdouble matXc2U[3][4], ICP3DCoordT *cameraCoord )
{
    ARdouble   w1, w2, w3, w3_w3;

    w1 = matXc2U[0][0] * cameraCoord->x + matXc2U[0][1] * cameraCoord->y + matXc2U[0][2] * cameraCoord->z + matXc2U[0][3];
    w2 = matXc2U[1][0] * cameraCoord->x + matXc2U[1][1] * cameraCoord->y + matXc2U[1][2] * cameraCoord->z + matXc2U[1][3];
    w3 = matXc2U[2][0] * cameraCoord->x + matXc2U[2][1] * cameraCoord->y + matXc2U[2][2] * cameraCoord->z + matXc2U[2][3];

    if( w3 == 0.0 ) return -1;

    w3_w3 = w3 * w3;
    J_U_Xc[0][0] = (matXc2U[0][0] * w3 - matXc2U[2][0] * w1) / w3_w3;
    J_U_Xc[0][1] = (matXc2U[0][1] * w3 - matXc2U[2][1] * w1) / w3_w3;
    J_U_Xc[0][2] = (matXc2U[0][2] * w3 - matXc2U[2][2] * w1) / w3_w3;
    J_U_Xc[1][0] = (matXc2U[1][0] * w3 - matXc2U[2][0] * w2) / w3_w3;
    J_U_Xc[1][1] = (matXc2U[1][1] * w3 - matXc2U[2][1] * w2) / w3_w3;
    J_U_Xc[1][2] = (matXc2U[1][2] * w3 - matXc2U[2][2] * w2) / w3_w3;

    return 0;
}

static int icpGetJ_Xc_S( ARdouble J_Xc_S[3][6], ICP3DCoordT *cameraCoord, ARdouble T0[3][4], ICP3DCoordT *worldCoord )
{
    ARdouble   J_Xc_T[3][12];
    ARdouble   J_T_S[12][6];
    int      i, j, k;

    cameraCoord->x = T0[0][0]*worldCoord->x + T0[0][1]*worldCoord->y + T0[0][2]*worldCoord->z + T0[0][3];
    cameraCoord->y = T0[1][0]*worldCoord->x + T0[1][1]*worldCoord->y + T0[1][2]*worldCoord->z + T0[1][3];
    cameraCoord->z = T0[2][0]*worldCoord->x + T0[2][1]*worldCoord->y + T0[2][2]*worldCoord->z + T0[2][3];

    J_Xc_T[0][0] = T0[0][0] * worldCoord->x;
    J_Xc_T[0][1] = T0[0][0] * worldCoord->y;
    J_Xc_T[0][2] = T0[0][0] * worldCoord->z;
    J_Xc_T[0][3] = T0[0][1] * worldCoord->x;
    J_Xc_T[0][4] = T0[0][1] * worldCoord->y;
    J_Xc_T[0][5] = T0[0][1] * worldCoord->z;
    J_Xc_T[0][6] = T0[0][2] * worldCoord->x;
    J_Xc_T[0][7] = T0[0][2] * worldCoord->y;
    J_Xc_T[0][8] = T0[0][2] * worldCoord->z;
    J_Xc_T[0][9] = T0[0][0];
    J_Xc_T[0][10] = T0[0][1];
    J_Xc_T[0][11] = T0[0][2];

    J_Xc_T[1][0] = T0[1][0] * worldCoord->x;
    J_Xc_T[1][1] = T0[1][0] * worldCoord->y;
    J_Xc_T[1][2] = T0[1][0] * worldCoord->z;
    J_Xc_T[1][3] = T0[1][1] * worldCoord->x;
    J_Xc_T[1][4] = T0[1][1] * worldCoord->y;
    J_Xc_T[1][5] = T0[1][1] * worldCoord->z;
    J_Xc_T[1][6] = T0[1][2] * worldCoord->x;
    J_Xc_T[1][7] = T0[1][2] * worldCoord->y;
    J_Xc_T[1][8] = T0[1][2] * worldCoord->z;
    J_Xc_T[1][9] = T0[1][0];
    J_Xc_T[1][10] = T0[1][1];
    J_Xc_T[1][11] = T0[1][2];

    J_Xc_T[2][0] = T0[2][0] * worldCoord->x;
    J_Xc_T[2][1] = T0[2][0] * worldCoord->y;
    J_Xc_T[2][2] = T0[2][0] * worldCoord->z;
    J_Xc_T[2][3] = T0[2][1] * worldCoord->x;
    J_Xc_T[2][4] = T0[2][1] * worldCoord->y;
    J_Xc_T[2][5] = T0[2][1] * worldCoord->z;
    J_Xc_T[2][6] = T0[2][2] * worldCoord->x;
    J_Xc_T[2][7] = T0[2][2] * worldCoord->y;
    J_Xc_T[2][8] = T0[2][2] * worldCoord->z;
    J_Xc_T[2][9] = T0[2][0];
    J_Xc_T[2][10] = T0[2][1];
    J_Xc_T[2][11] = T0[2][2];

    if( icpGetJ_T_S( J_T_S ) < 0 ) return -1;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 6; i++ ) {
            J_Xc_S[j][i] = 0.0;
            for( k = 0; k < 12; k++ ) {
                J_Xc_S[j][i] += J_Xc_T[j][k] * J_T_S[k][i];
            }
        }
    }

    return 0;
}

static int icpGetJ_T_S( ARdouble J_T_S[12][6] )
{
    J_T_S[0][0] = 0.0;
    J_T_S[0][1] = 0.0;
    J_T_S[0][2] = 0.0;
    J_T_S[0][3] = 0.0;
    J_T_S[0][4] = 0.0;
    J_T_S[0][5] = 0.0;

    J_T_S[1][0] = 0.0;
    J_T_S[1][1] = 0.0;
    J_T_S[1][2] = -1.0;
    J_T_S[1][3] = 0.0;
    J_T_S[1][4] = 0.0;
    J_T_S[1][5] = 0.0;

    J_T_S[2][0] = 0.0;
    J_T_S[2][1] = 1.0;
    J_T_S[2][2] = 0.0;
    J_T_S[2][3] = 0.0;
    J_T_S[2][4] = 0.0;
    J_T_S[2][5] = 0.0;

    J_T_S[3][0] = 0.0;
    J_T_S[3][1] = 0.0;
    J_T_S[3][2] = 1.0;
    J_T_S[3][3] = 0.0;
    J_T_S[3][4] = 0.0;
    J_T_S[3][5] = 0.0;

    J_T_S[4][0] = 0.0;
    J_T_S[4][1] = 0.0;
    J_T_S[4][2] = 0.0;
    J_T_S[4][3] = 0.0;
    J_T_S[4][4] = 0.0;
    J_T_S[4][5] = 0.0;

    J_T_S[5][0] = -1.0;
    J_T_S[5][1] = 0.0;
    J_T_S[5][2] = 0.0;
    J_T_S[5][3] = 0.0;
    J_T_S[5][4] = 0.0;
    J_T_S[5][5] = 0.0;

    J_T_S[6][0] = 0.0;
    J_T_S[6][1] = -1.0;
    J_T_S[6][2] = 0.0;
    J_T_S[6][3] = 0.0;
    J_T_S[6][4] = 0.0;
    J_T_S[6][5] = 0.0;

    J_T_S[7][0] = 1.0;
    J_T_S[7][1] = 0.0;
    J_T_S[7][2] = 0.0;
    J_T_S[7][3] = 0.0;
    J_T_S[7][4] = 0.0;
    J_T_S[7][5] = 0.0;

    J_T_S[8][0] = 0.0;
    J_T_S[8][1] = 0.0;
    J_T_S[8][2] = 0.0;
    J_T_S[8][3] = 0.0;
    J_T_S[8][4] = 0.0;
    J_T_S[8][5] = 0.0;

    J_T_S[9][0] = 0.0;
    J_T_S[9][1] = 0.0;
    J_T_S[9][2] = 0.0;
    J_T_S[9][3] = 1.0;
    J_T_S[9][4] = 0.0;
    J_T_S[9][5] = 0.0;

    J_T_S[10][0] = 0.0;
    J_T_S[10][1] = 0.0;
    J_T_S[10][2] = 0.0;
    J_T_S[10][3] = 0.0;
    J_T_S[10][4] = 1.0;
    J_T_S[10][5] = 0.0;

    J_T_S[11][0] = 0.0;
    J_T_S[11][1] = 0.0;
    J_T_S[11][2] = 0.0;
    J_T_S[11][3] = 0.0;
    J_T_S[11][4] = 0.0;
    J_T_S[11][5] = 1.0;

    return 0;
}

static int icpGetQ_from_S( ARdouble q[7], ARdouble s[6] )
{
    ARdouble    ra;

    ra = s[0]*s[0] + s[1]*s[1] + s[2]*s[2];
    if( ra == 0.0 ) {
        q[0] = 1.0;
        q[1] = 0.0;
        q[2] = 0.0;
        q[3] = 0.0;
    }
    else {
        ra = SQRT(ra);
        q[0] = s[0] / ra;
        q[1] = s[1] / ra;
        q[2] = s[2] / ra;
        q[3] = ra;
    }
    q[4] = s[3];
    q[5] = s[4];
    q[6] = s[5];

    return 0;
}

static int icpGetMat_from_Q( ARdouble mat[3][4], ARdouble q[7] )
{
    ARdouble    cra, one_cra, sra;

    cra = COS(q[3]);
    one_cra = ONE - cra;
    sra = SIN(q[3]);

    mat[0][0] = q[0]*q[0]*one_cra + cra;
    mat[0][1] = q[0]*q[1]*one_cra - q[2]*sra;
    mat[0][2] = q[0]*q[2]*one_cra + q[1]*sra;
    mat[0][3] = q[4];
    mat[1][0] = q[1]*q[0]*one_cra + q[2]*sra;
    mat[1][1] = q[1]*q[1]*one_cra + cra;
    mat[1][2] = q[1]*q[2]*one_cra - q[0]*sra;
    mat[1][3] = q[5];
    mat[2][0] = q[2]*q[0]*one_cra - q[1]*sra;
    mat[2][1] = q[2]*q[1]*one_cra + q[0]*sra;
    mat[2][2] = q[2]*q[2]*one_cra + cra;
    mat[2][3] = q[6];

    return 0;
}
