/*
 *  icpUtil.c
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

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#  define _0_0 0.0f
#  define _0_5 0.5f
#  define _1_0 1.0f
#  define _2_0 2.0f
#else
#  define SQRT sqrt
#  define _0_0 0.0
#  define _0_5 0.5
#  define _1_0 1.0
#  define _2_0 2.0
#endif

static int check_rotation( ARdouble rot[2][3] );

#if 0
static void icpGetInitXw2XcSub( ARdouble       rot[3][4],
                                ICP2DCoordT  pos2d[],
                                ICP3DCoordT  ppos3d[],
                                int          num,
                                ARdouble       cpara[3][4],
                                ARdouble       conv[3][4] );
static int check_dir( ARdouble dir[3], ARdouble st[2], ARdouble ed[2],
                      ARdouble cpara[3][4] );
#endif

int icpGetInitXw2Xc_from_PlanarData( ARdouble       matXc2U[3][4],
                                     ICP2DCoordT  screenCoord[],
                                     ICP3DCoordT  worldCoord[],
                                     int          num,
                                     ARdouble       initMatXw2Xc[3][4] )
{
    ARMat   *matA, *matB;
    ARMat   *matAt, *matAtA, *matAtB, *matC;
    ARdouble   v[3][3], t[3];
    ARdouble   l1, l2;
    int      i;

    if( num < 4 ) return -1;
    for( i = 0; i < num; i++ ) {
        if( worldCoord[i].z != 0.0 ) return -1;
    }
    if( matXc2U[0][0] == 0.0 ) return -1;
    if( matXc2U[1][0] != 0.0 ) return -1;
    if( matXc2U[1][1] == 0.0 ) return -1;
    if( matXc2U[2][0] != 0.0 ) return -1;
    if( matXc2U[2][1] != 0.0 ) return -1;
    if( matXc2U[2][2] != 1.0 ) return -1;
    if( matXc2U[0][3] != 0.0 ) return -1;
    if( matXc2U[1][3] != 0.0 ) return -1;
    if( matXc2U[2][3] != 0.0 ) return -1;

    matA = arMatrixAlloc( num*2, 8 );
    if( matA == NULL ) {
        ARLOGe("Error 1: icpGetInitXw2Xc\n");
        return -1;
    }
    matB = arMatrixAlloc( num*2, 1 );
    if( matB == NULL ) {
        arMatrixFree(matA);
        ARLOGe("Error 2: icpGetInitXw2Xc\n");
        return -1;
    }

    for( i = 0; i < num; i++ ) {
        matA->m[i*16+0]  = worldCoord[i].x;
        matA->m[i*16+1]  = worldCoord[i].y;
        matA->m[i*16+2]  = 1.0;
        matA->m[i*16+3]  = 0.0;
        matA->m[i*16+4]  = 0.0;
        matA->m[i*16+5]  = 0.0;
        matA->m[i*16+6]  = -(worldCoord[i].x)*(screenCoord[i].x);
        matA->m[i*16+7]  = -(worldCoord[i].y)*(screenCoord[i].x);
        matA->m[i*16+8]  = 0.0;
        matA->m[i*16+9]  = 0.0;
        matA->m[i*16+10] = 0.0;
        matA->m[i*16+11] = worldCoord[i].x;
        matA->m[i*16+12] = worldCoord[i].y;
        matA->m[i*16+13] = 1.0;
        matA->m[i*16+14] = -(worldCoord[i].x)*(screenCoord[i].y);
        matA->m[i*16+15] = -(worldCoord[i].y)*(screenCoord[i].y);

        matB->m[i*2+0] = screenCoord[i].x;
        matB->m[i*2+1] = screenCoord[i].y;
    }

    matAt = arMatrixAllocTrans( matA );
    if( matAt == NULL ) {
        arMatrixFree(matA);
        arMatrixFree(matB);
        ARLOGe("Error 3: icpGetInitXw2Xc\n");
        return -1;
    }
    matAtA = arMatrixAllocMul( matAt, matA );
    if( matAtA == NULL ) {
        arMatrixFree(matA);
        arMatrixFree(matB);
        arMatrixFree(matAt);
        ARLOGe("Error 4: icpGetInitXw2Xc\n");
        return -1;
    }
    matAtB = arMatrixAllocMul( matAt, matB );
    if( matAtB == NULL ) {
        arMatrixFree(matA);
        arMatrixFree(matB);
        arMatrixFree(matAt);
        arMatrixFree(matAtA);
        ARLOGe("Error 5: icpGetInitXw2Xc\n");
        return -1;
    }
    if( arMatrixSelfInv(matAtA) < 0 ) {
        arMatrixFree(matA);
        arMatrixFree(matB);
        arMatrixFree(matAt);
        arMatrixFree(matAtA);
        arMatrixFree(matAtB);
        ARLOGe("Error 6: icpGetInitXw2Xc\n");
        return -1;
    }
    matC = arMatrixAllocMul( matAtA, matAtB );
    if( matC == NULL ) {
        arMatrixFree(matA);
        arMatrixFree(matB);
        arMatrixFree(matAt);
        arMatrixFree(matAtA);
        arMatrixFree(matAtB);
        ARLOGe("Error 7: icpGetInitXw2Xc\n");
        return -1;
    }

    v[0][2] =  matC->m[6];
    v[0][1] = (matC->m[3] - matXc2U[1][2] * v[0][2]) / matXc2U[1][1];
    v[0][0] = (matC->m[0] - matXc2U[0][2] * v[0][2] - matXc2U[0][1] * v[0][1]) / matXc2U[0][0];
    v[1][2] =  matC->m[7];
    v[1][1] = (matC->m[4] - matXc2U[1][2] * v[1][2]) / matXc2U[1][1];
    v[1][0] = (matC->m[1] - matXc2U[0][2] * v[1][2] - matXc2U[0][1] * v[1][1]) / matXc2U[0][0];
    t[2]  =  1.0;
    t[1]  = (matC->m[5] - matXc2U[1][2] * t[2]) / matXc2U[1][1];
    t[0]  = (matC->m[2] - matXc2U[0][2] * t[2] - matXc2U[0][1] * t[1]) / matXc2U[0][0];

    arMatrixFree(matA);
    arMatrixFree(matB);
    arMatrixFree(matAt);
    arMatrixFree(matAtA);
    arMatrixFree(matAtB);
    arMatrixFree(matC);

    l1 = SQRT( v[0][0]*v[0][0] + v[0][1]*v[0][1] + v[0][2]*v[0][2] );
    l2 = SQRT( v[1][0]*v[1][0] + v[1][1]*v[1][1] + v[1][2]*v[1][2] );
    v[0][0] /= l1;
    v[0][1] /= l1;
    v[0][2] /= l1;
    v[1][0] /= l2;
    v[1][1] /= l2;
    v[1][2] /= l2;
    t[0] /= (l1+l2)/_2_0;
    t[1] /= (l1+l2)/_2_0;
    t[2] /= (l1+l2)/_2_0;
    if( t[2] < 0.0 ) {
        v[0][0] = -v[0][0];
        v[0][1] = -v[0][1];
        v[0][2] = -v[0][2];
        v[1][0] = -v[1][0];
        v[1][1] = -v[1][1];
        v[1][2] = -v[1][2];
        t[0] = -t[0];
        t[1] = -t[1];
        t[2] = -t[2];
    }

    check_rotation( v );
    v[2][0] = v[0][1]*v[1][2] - v[0][2]*v[1][1];
    v[2][1] = v[0][2]*v[1][0] - v[0][0]*v[1][2];
    v[2][2] = v[0][0]*v[1][1] - v[0][1]*v[1][0];
    l1 = SQRT( v[2][0]*v[2][0] + v[2][1]*v[2][1] + v[2][2]*v[2][2] );
    v[2][0] /= l1;
    v[2][1] /= l1;
    v[2][2] /= l1;

    initMatXw2Xc[0][0] = v[0][0];
    initMatXw2Xc[1][0] = v[0][1];
    initMatXw2Xc[2][0] = v[0][2];
    initMatXw2Xc[0][1] = v[1][0];
    initMatXw2Xc[1][1] = v[1][1];
    initMatXw2Xc[2][1] = v[1][2];
    initMatXw2Xc[0][2] = v[2][0];
    initMatXw2Xc[1][2] = v[2][1];
    initMatXw2Xc[2][2] = v[2][2];
    initMatXw2Xc[0][3] = t[0];
    initMatXw2Xc[1][3] = t[1];
    initMatXw2Xc[2][3] = t[2];

#if 0
//ARLOGe("   %f %f %f --->", t[0], t[1], t[2]);
    icpGetInitXw2XcSub( initMatXw2Xc, screenCoord, worldCoord, num, matXc2U, initMatXw2Xc );
//ARLOGe("%f %f %f\n", initMatXw2Xc[0][3], initMatXw2Xc[1][3], initMatXw2Xc[2][3]);
#endif

    return 0;
}



static int check_rotation( ARdouble rot[2][3] )
{
    ARdouble  v1[3], v2[3], v3[3];
    ARdouble  ca, cb, k1, k2, k3, k4;
    ARdouble  a, b, c, d;
    ARdouble  p1, q1, r1;
    ARdouble  p2, q2, r2;
    ARdouble  p3, q3, r3;
    ARdouble  p4, q4, r4;
    ARdouble  w;
    ARdouble  e1, e2, e3, e4;
    int       rotFlag;

    v1[0] = rot[0][0];
    v1[1] = rot[0][1];
    v1[2] = rot[0][2];
    v2[0] = rot[1][0];
    v2[1] = rot[1][1];
    v2[2] = rot[1][2];
    v3[0] = v1[1]*v2[2] - v1[2]*v2[1];
    v3[1] = v1[2]*v2[0] - v1[0]*v2[2];
    v3[2] = v1[0]*v2[1] - v1[1]*v2[0];
    w = SQRT( v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2] );
    if( w == _0_0 ) return -1;
    v3[0] /= w;
    v3[1] /= w;
    v3[2] /= w;

    cb = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    if( cb < 0 ) cb *= -_1_0;
    ca = (SQRT(cb+_1_0) + SQRT(_1_0-cb)) * _0_5;

    if( v3[1]*v1[0] - v1[1]*v3[0] != 0.0 ) {
        rotFlag = 0;
    }
    else {
        if( v3[2]*v1[0] - v1[2]*v3[0] != _0_0 ) {
            w = v1[1]; v1[1] = v1[2]; v1[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            rotFlag = 1;
        }
        else {
            w = v1[0]; v1[0] = v1[2]; v1[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            rotFlag = 2;
        }
    }
    if( v3[1]*v1[0] - v1[1]*v3[0] == _0_0 ) return -1;
    k1 = (v1[1]*v3[2] - v3[1]*v1[2]) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k3 = (v1[0]*v3[2] - v3[0]*v1[2]) / (v3[0]*v1[1] - v1[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v1[1] - v1[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r1 = (-b + SQRT(d))/a;
    p1 = k1*r1 + k2;
    q1 = k3*r1 + k4;
    r2 = (-b - SQRT(d))/a;
    p2 = k1*r2 + k2;
    q2 = k3*r2 + k4;
    if ( rotFlag == 1 ) {
        w = q1; q1 = r1; r1 = w;
        w = q2; q2 = r2; r2 = w;
        w = v1[1]; v1[1] = v1[2]; v1[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
    } else if ( rotFlag == 2 ) {
        w = p1; p1 = r1; r1 = w;
        w = p2; p2 = r2; r2 = w;
        w = v1[0]; v1[0] = v1[2]; v1[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
    }

    if( v3[1]*v2[0] - v2[1]*v3[0] != 0.0 ) {
        rotFlag = 0;
    }
    else {
        if( v3[2]*v2[0] - v2[2]*v3[0] != 0.0 ) {
            w = v2[1]; v2[1] = v2[2]; v2[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            rotFlag = 1;
        }
        else {
            w = v2[0]; v2[0] = v2[2]; v2[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            rotFlag = 2;
        }
    }
    if( v3[1]*v2[0] - v2[1]*v3[0] == _0_0 ) return -1;
    k1 = (v2[1]*v3[2] - v3[1]*v2[2]) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k3 = (v2[0]*v3[2] - v3[0]*v2[2]) / (v3[0]*v2[1] - v2[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v2[1] - v2[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r3 = (-b + SQRT(d))/a;
    p3 = k1*r3 + k2;
    q3 = k3*r3 + k4;
    r4 = (-b - SQRT(d))/a;
    p4 = k1*r4 + k2;
    q4 = k3*r4 + k4;
    if ( rotFlag == 1 ) {
        w = q3; q3 = r3; r3 = w;
        w = q4; q4 = r4; r4 = w;
        w = v2[1]; v2[1] = v2[2]; v2[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
    } else if ( rotFlag == 2 ) {
        w = p3; p3 = r3; r3 = w;
        w = p4; p4 = r4; r4 = w;
        w = v2[0]; v2[0] = v2[2]; v2[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
    }

    e1 = p1*p3+q1*q3+r1*r3; if( e1 < 0 ) e1 = -e1;
    e2 = p1*p4+q1*q4+r1*r4; if( e2 < 0 ) e2 = -e2;
    e3 = p2*p3+q2*q3+r2*r3; if( e3 < 0 ) e3 = -e3;
    e4 = p2*p4+q2*q4+r2*r4; if( e4 < 0 ) e4 = -e4;
    if( e1 < e2 ) {
        if( e1 < e3 ) {
            if( e1 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }
    else {
        if( e2 < e3 ) {
            if( e2 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }

    return 0;
}

#if 0
static void icpGetInitXw2XcSub( ARdouble       rot[3][4],
                                ICP2DCoordT  pos2d[],
                                ICP3DCoordT  ppos3d[],
                                int          num,
                                ARdouble       cpara[3][4],
                                ARdouble       conv[3][4] )
{
    ICP3DCoordT  *pos3d;
    ARMat        *mat_a, *mat_b, *mat_c, *mat_d, *mat_e, *mat_f;
    ARdouble        trans[3];
    ARdouble        wx, wy, wz;
    ARdouble        off[3], pmax[3], pmin[3];
    int           i, j;
    
    arMalloc(pos3d, ICP3DCoordT, num);
    mat_a = arMatrixAlloc( num*2, 3 );
    mat_b = arMatrixAlloc( 3, num*2 );
    mat_c = arMatrixAlloc( num*2, 1 );
    mat_d = arMatrixAlloc( 3, 3 );
    mat_e = arMatrixAlloc( 3, 1 );
    mat_f = arMatrixAlloc( 3, 1 );

    pmax[0]=pmax[1]=pmax[2] = -10000000000.0;
    pmin[0]=pmin[1]=pmin[2] =  10000000000.0;
    for( i = 0; i < num; i++ ) {
        if( ppos3d[i].x > pmax[0] ) pmax[0] = ppos3d[i].x;
        if( ppos3d[i].x < pmin[0] ) pmin[0] = ppos3d[i].x;
        if( ppos3d[i].y > pmax[1] ) pmax[1] = ppos3d[i].y;
        if( ppos3d[i].y < pmin[1] ) pmin[1] = ppos3d[i].y;
        if( ppos3d[i].z > pmax[2] ) pmax[2] = ppos3d[i].z;
        if( ppos3d[i].z < pmin[2] ) pmin[2] = ppos3d[i].z;
    }
    off[0] = -(pmax[0] + pmin[0]) / _2_0;
    off[1] = -(pmax[1] + pmin[1]) / _2_0;
    off[2] = -(pmax[2] + pmin[2]) / _2_0;
    for( i = 0; i < num; i++ ) {
        pos3d[i].x = ppos3d[i].x + off[0];
        pos3d[i].y = ppos3d[i].y + off[1];
        pos3d[i].z = ppos3d[i].z + off[2];
    }

    for( j = 0; j < num; j++ ) {
        wx = rot[0][0] * pos3d[j].x
           + rot[0][1] * pos3d[j].y
           + rot[0][2] * pos3d[j].z;
        wy = rot[1][0] * pos3d[j].x
           + rot[1][1] * pos3d[j].y
           + rot[1][2] * pos3d[j].z;
        wz = rot[2][0] * pos3d[j].x
           + rot[2][1] * pos3d[j].y
           + rot[2][2] * pos3d[j].z;
        mat_a->m[j*6+0] = mat_b->m[num*0+j*2] = cpara[0][0];
        mat_a->m[j*6+1] = mat_b->m[num*2+j*2] = cpara[0][1];
        mat_a->m[j*6+2] = mat_b->m[num*4+j*2] = cpara[0][2] - pos2d[j].x;
        mat_c->m[j*2+0] = wz * pos2d[j].x
               - cpara[0][0]*wx - cpara[0][1]*wy - cpara[0][2]*wz;
        mat_a->m[j*6+3] = mat_b->m[num*0+j*2+1] = 0.0;
        mat_a->m[j*6+4] = mat_b->m[num*2+j*2+1] = cpara[1][1];
        mat_a->m[j*6+5] = mat_b->m[num*4+j*2+1] = cpara[1][2] - pos2d[j].y;
        mat_c->m[j*2+1] = wz * pos2d[j].y
               - cpara[1][1]*wy - cpara[1][2]*wz;
    }
    arMatrixMul( mat_d, mat_b, mat_a );
    arMatrixMul( mat_e, mat_b, mat_c );
    arMatrixSelfInv( mat_d );
    arMatrixMul( mat_f, mat_d, mat_e );
    trans[0] = mat_f->m[0];
    trans[1] = mat_f->m[1];
    trans[2] = mat_f->m[2];

    free(pos3d);
    arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );
    arMatrixFree( mat_d );
    arMatrixFree( mat_e );
    arMatrixFree( mat_f );

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) conv[j][i] = rot[j][i];
        conv[j][3] = trans[j];
    }

    conv[0][3] = conv[0][0]*off[0] + conv[0][1]*off[1] + conv[0][2]*off[2] + conv[0][3];
    conv[1][3] = conv[1][0]*off[0] + conv[1][1]*off[1] + conv[1][2]*off[2] + conv[1][3];
    conv[2][3] = conv[2][0]*off[0] + conv[2][1]*off[1] + conv[2][2]*off[2] + conv[2][3];

    return;
}

#if 0
int arGetInitRot( ARdouble cpara[3][4], ARdouble rot[3][4],
                  ICP2DCoordT  pos2d[],
                  ICP3DCoordT  ppos3d[] )
{
    ARdouble  wdir[3][3];
    ARdouble  w, w1, w2, w3;
    int     dir;
    int     j;
    
    dir = marker_info->dir;
    
    for( j = 0; j < 2; j++ ) {
        w1 = marker_info->line[(4-dir+j)%4][0] * marker_info->line[(6-dir+j)%4][1]
           - marker_info->line[(6-dir+j)%4][0] * marker_info->line[(4-dir+j)%4][1];
        w2 = marker_info->line[(4-dir+j)%4][1] * marker_info->line[(6-dir+j)%4][2]
           - marker_info->line[(6-dir+j)%4][1] * marker_info->line[(4-dir+j)%4][2];
        w3 = marker_info->line[(4-dir+j)%4][2] * marker_info->line[(6-dir+j)%4][0]
           - marker_info->line[(6-dir+j)%4][2] * marker_info->line[(4-dir+j)%4][0];

        wdir[j][0] =  w1*(cpara[0][1]*cpara[1][2]-cpara[0][2]*cpara[1][1])
                   +  w2*cpara[1][1]
                   -  w3*cpara[0][1];
        wdir[j][1] = -w1*cpara[0][0]*cpara[1][2]
                   +  w3*cpara[0][0];
        wdir[j][2] =  w1*cpara[0][0]*cpara[1][1];
        w = SQRT( wdir[j][0]*wdir[j][0]
                + wdir[j][1]*wdir[j][1]
                + wdir[j][2]*wdir[j][2] );
        wdir[j][0] /= w;
        wdir[j][1] /= w;
        wdir[j][2] /= w;
    }
                
    if( check_dir(wdir[0], marker_info->vertex[(4-dir)%4],
                  marker_info->vertex[(5-dir)%4], cpara) < 0 ) return -1;
    if( check_dir(wdir[1], marker_info->vertex[(7-dir)%4],
                  marker_info->vertex[(4-dir)%4], cpara) < 0 ) return -1;
    if( check_rotation(wdir) < 0 ) return -1;
                
    wdir[2][0] = wdir[0][1]*wdir[1][2] - wdir[0][2]*wdir[1][1];
    wdir[2][1] = wdir[0][2]*wdir[1][0] - wdir[0][0]*wdir[1][2];
    wdir[2][2] = wdir[0][0]*wdir[1][1] - wdir[0][1]*wdir[1][0];
    w = SQRT( wdir[2][0]*wdir[2][0]
            + wdir[2][1]*wdir[2][1]
            + wdir[2][2]*wdir[2][2] );
    wdir[2][0] /= w;
    wdir[2][1] /= w;
    wdir[2][2] /= w;
/*        
    if( wdir[2][2] < 0 ) {
        wdir[2][0] /= -w;
        wdir[2][1] /= -w;
        wdir[2][2] /= -w;
    }   
    else {            
        wdir[2][0] /= w;
        wdir[2][1] /= w;
        wdir[2][2] /= w;
    }
*/

    rot[0][0] = wdir[0][0];
    rot[1][0] = wdir[0][1];
    rot[2][0] = wdir[0][2];
    rot[0][1] = wdir[1][0];
    rot[1][1] = wdir[1][1];
    rot[2][1] = wdir[1][2];
    rot[0][2] = wdir[2][0];
    rot[1][2] = wdir[2][1];
    rot[2][2] = wdir[2][2];

    return 0;
}

static int check_dir( ARdouble dir[3], ARdouble st[2], ARdouble ed[2],
                      ARdouble cpara[3][4] )
{
    ARMat     *mat_a;
    ARdouble    world[2][3];
    ARdouble    camera[2][2];
    ARdouble    v[2][2];
    ARdouble    h;
    int       i, j;

    mat_a = arMatrixAlloc( 3, 3 );
    for(j=0;j<3;j++) for(i=0;i<3;i++) mat_a->m[j*3+i] = cpara[j][i];
    arMatrixSelfInv( mat_a );
    world[0][0] = mat_a->m[0]*st[0]*10.0
                + mat_a->m[1]*st[1]*10.0
                + mat_a->m[2]*10.0;
    world[0][1] = mat_a->m[3]*st[0]*10.0
                + mat_a->m[4]*st[1]*10.0
                + mat_a->m[5]*10.0;
    world[0][2] = mat_a->m[6]*st[0]*10.0
                + mat_a->m[7]*st[1]*10.0
                + mat_a->m[8]*10.0;
    arMatrixFree( mat_a );
    world[1][0] = world[0][0] + dir[0];
    world[1][1] = world[0][1] + dir[1];
    world[1][2] = world[0][2] + dir[2];
    
    for( i = 0; i < 2; i++ ) {
        h = cpara[2][0] * world[i][0]
          + cpara[2][1] * world[i][1]
          + cpara[2][2] * world[i][2];
        if( h == 0.0 ) return -1;
        camera[i][0] = (cpara[0][0] * world[i][0]
                      + cpara[0][1] * world[i][1]
                      + cpara[0][2] * world[i][2]) / h;
        camera[i][1] = (cpara[1][0] * world[i][0]
                      + cpara[1][1] * world[i][1]
                      + cpara[1][2] * world[i][2]) / h;
    }
    
    v[0][0] = ed[0] - st[0];
    v[0][1] = ed[1] - st[1]; 
    v[1][0] = camera[1][0] - camera[0][0];
    v[1][1] = camera[1][1] - camera[0][1];
    
    if( v[0][0]*v[1][0] + v[0][1]*v[1][1] < 0 ) {
        dir[0] = -dir[0];
        dir[1] = -dir[1];
        dir[2] = -dir[2];
    }
    
    return 0;
}
#endif
#endif
