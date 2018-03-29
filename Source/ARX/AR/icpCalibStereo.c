/*
 *  icpCalibStereo.c
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
#include <ARX/AR/icpCalib.h>


static int  icp2GetTS( ARdouble TS[], ARdouble dU[], ARdouble J[], int dataNum, int paramNum );


int icpCalibStereo( ICPCalibDataT data[], int num,
                    ARdouble matXcl2Ul[3][4], ARdouble matXcr2Ur[3][4], ARdouble initMatL2R[3][4],
                    ARdouble matL2R[3][4],
                    ARdouble *err )
{
    ARdouble       *J, *dU, *dTS;
    ARdouble       *matXw2Xcl;
    ARdouble        matXw2Ul[3][4];
    ARdouble        matXw2Ur[3][4];
    ARdouble        matXcl2Ur[3][4];
    ARdouble        J_UL_S[2][6];
    ARdouble        oldErr;
    ICP3DCoordT   cameraCoordL;
    ICP2DCoordT   screenCoord;
    int           Jrow, Jclm;
    int           dataNum;
    int           i, j, k, l;
    int           i1;
    int           ret = 0;

    if( num <= 0 ) {
        ARLOGe("Data num error!!\n");
        return -1;
    }

    dataNum = 0;
    for( i = 0; i < num; i++ ) {
        if( data[i].numL <= 0 || data[i].numR <= 0 ) {
            ARLOGe("Data num error!!\n");
            return -1;
        }
        dataNum += data[i].numL;
        dataNum += data[i].numR;
    }

    Jrow = dataNum * 2;
    Jclm = (num + 1) * 6;
    //ARLOGe("Jrow = %d, Jclm = %d\n", Jrow, Jclm );
    J = (ARdouble *)malloc(sizeof(ARdouble)*Jrow*Jclm);
    if( J == NULL ) {
        ARLOGe("Out of memory!!\n");
        return -1;
    }
    dU = (ARdouble *)malloc(sizeof(ARdouble)*Jrow);
    if( dU == NULL ) {
        ARLOGe("Out of memory!!\n");
        free(J);
        return -1;
    }
    dTS = (ARdouble *)malloc(sizeof(ARdouble)*Jclm);
    if( dTS == NULL ) {
        ARLOGe("Out of memory!!\n");
        free(J);
        free(dU);
        return -1;
    }
    matXw2Xcl = (ARdouble *)malloc(sizeof(ARdouble)*3*4*num);
    if( matXw2Xcl == NULL ) {
        ARLOGe("Out of memory!!\n");
        free(J);
        free(dU);
        free(dTS);
        return -1;
    }

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matL2R[j][i] = initMatL2R[j][i];
    }
    for( k = 0; k < num; k++ ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                matXw2Xcl[k*12+j*4+i] = data[k].initMatXw2Xcl[j][i];
            }
        }
    }

    for( l = 0; l < ICP_CALIB_STEREO_MAX_LOOP; l++ ) {
        k = 0;
        for( i = 0; i < Jrow*Jclm; i++ )  J[i] = 0.0;
        for( i = 0; i < Jrow;      i++ ) dU[i] = 0.0;
        arUtilMatMul( (const ARdouble (*)[4])matXcr2Ur, (const ARdouble (*)[4])matL2R, matXcl2Ur );

        *err = 0.0;
        for( j = 0; j < num; j++ ) {
            arUtilMatMul( (const ARdouble (*)[4])matXcl2Ul, (const ARdouble (*)[4])&(matXw2Xcl[j*12]), matXw2Ul );
            arUtilMatMul( (const ARdouble (*)[4])matXcl2Ur, (const ARdouble (*)[4])&(matXw2Xcl[j*12]), matXw2Ur );

            for( i = 0; i <  data[j].numL; i++ ) {
                if( icpGetJ_U_S( J_UL_S, matXcl2Ul, (ARdouble (*)[4])&(matXw2Xcl[j*12]), &(data[j].worldCoordL[i]) ) < 0 ) {
                    ARLOGe("Error icpGetJ_U_S\n");
                    ret = -1;
                    break;
                }
                for( i1 = 0; i1 < 6; i1++ ) {
                    J[(k*2  )*Jclm+(1+j)*6+i1] = J_UL_S[0][i1];
                    J[(k*2+1)*Jclm+(1+j)*6+i1] = J_UL_S[1][i1];
                }

                if( icpGetU_from_X_by_MatX2U( &screenCoord, matXw2Ul, &(data[j].worldCoordL[i]) ) < 0 ) {
                    ARLOGe("Error icpGetU_from_X_by_MatX2U\n");
                    ret = -1;
                    break;
                }
                dU[k*2  ] = data[j].screenCoordL[i].x - screenCoord.x;
                dU[k*2+1] = data[j].screenCoordL[i].y - screenCoord.y;
                *err += dU[k*2]*dU[k*2] + dU[k*2+1]*dU[k*2+1];
                k++;
            }
            if( ret == -1 ) break;

            for( i = 0; i <  data[j].numR; i++ ) {
                if( icpGetJ_U_S( J_UL_S, matXcl2Ur, (ARdouble (*)[4])&(matXw2Xcl[j*12]), &(data[j].worldCoordR[i]) ) < 0 ) {
                    ARLOGe("Error icpGetJ_U_S\n");
                    ret = -1;
                    break;
                }
                for( i1 = 0; i1 < 6; i1++ ) {
                    J[(k*2  )*Jclm+(1+j)*6+i1] = J_UL_S[0][i1];
                    J[(k*2+1)*Jclm+(1+j)*6+i1] = J_UL_S[1][i1];
                }

                if( icpGetU_from_X_by_MatX2U( &screenCoord, matXw2Ur, &(data[j].worldCoordR[i]) ) < 0 ) {
                    ARLOGe("Error icpGetU_from_X_by_MatX2U\n");
                    ret = -1;
                    break;
                }
                dU[k*2  ] = data[j].screenCoordR[i].x - screenCoord.x;
                dU[k*2+1] = data[j].screenCoordR[i].y - screenCoord.y;
                *err += dU[k*2]*dU[k*2] + dU[k*2+1]*dU[k*2+1];

                if( icpGetXc_from_Xw_by_MatXw2Xc( &cameraCoordL, (ARdouble (*)[4])&(matXw2Xcl[j*12]), &(data[j].worldCoordR[i]) ) < 0 ) {
                    ARLOGe("Error icpGetXc_from_Xw_by_MatXw2Xc\n");
                    ret = -1;
                    break;
                }
                if( icpGetJ_U_S( J_UL_S, matXcr2Ur, matL2R, &cameraCoordL ) < 0 ) {
                    ARLOGe("Error icpGetJ_U_S\n");
                    ret = -1;
                    break;
                }
                for( i1 = 0; i1 < 6; i1++ ) {
                    J[(k*2  )*Jclm+i1] = J_UL_S[0][i1];
                    J[(k*2+1)*Jclm+i1] = J_UL_S[1][i1];
                }
                k++;
            }
            if( ret == -1 ) break;
        }
        if( ret == -1 ) break;

        *err /= dataNum;
        ARLOGe("Error = %f\n", *err);
        if( *err < ICP_CALIB_STEREO_BREAK_LOOP_ERROR_THRESH ) break;
        if( l > 0 && *err/oldErr > ICP_CALIB_STEREO_BREAK_LOOP_ERROR_RATIO_THRESH ) break;
        oldErr = *err;

        if( icp2GetTS( dTS, dU, J, Jrow, Jclm ) < 0 ) {
            ARLOGe("Error icp2GetTS\n");
            ret = -1;
            break;
        }

        icpUpdateMat( matL2R, &(dTS[0]) );
        for( j = 0; j < num; j++ ) {
            icpUpdateMat( (ARdouble (*)[4])&(matXw2Xcl[j*12]), &(dTS[6*(j+1)]) );
        }
    }

    free(J);
    free(dU);
    free(dTS);
    free(matXw2Xcl);

    return ret;
}

static int icp2GetTS( ARdouble TS[], ARdouble dU[], ARdouble J[], int dataNum, int paramNum )
{
    ARMat   matTS, matU, matJ;
    ARMat  *matJt, *matJtJ, *matJtU;   

    matTS.row = paramNum;
    matTS.clm = 1;
    matTS.m   = TS; 

    matU.row = dataNum;
    matU.clm = 1;
    matU.m   = dU;

    matJ.row = dataNum;
    matJ.clm = paramNum;
    matJ.m   = J;

    matJt = arMatrixAllocTrans( &matJ );
    if( matJt == NULL ) return -1;
    matJtJ = arMatrixAllocMul( matJt, &matJ );
    if( matJtJ == NULL ) {    
        arMatrixFree( matJt );
        return -1;  
    }
    matJtU = arMatrixAllocMul( matJt, &matU );
    if( matJtJ == NULL ) {
        arMatrixFree( matJt );
        arMatrixFree( matJtJ );
        return -1;
    }
    if( arMatrixSelfInv(matJtJ) < 0 ) {
        arMatrixFree( matJt );
        arMatrixFree( matJtJ );
        arMatrixFree( matJtU );
        return -1;
    }

    arMatrixMul( &matTS, matJtJ, matJtU );
    arMatrixFree( matJt );
    arMatrixFree( matJtJ );
    arMatrixFree( matJtU );

    return 0;
}
