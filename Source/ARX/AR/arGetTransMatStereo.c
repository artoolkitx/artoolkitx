/*
 *  arGetTransMatStereo.c
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
 *  Author(s): Hirokazu Kato
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 3.1
 * Date: 01/12/07
 *
 *******************************************************/

#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>



ARdouble arGetTransMatSquareStereo( AR3DStereoHandle *handle, 
                                  ARMarkerInfo *marker_infoL, ARMarkerInfo *marker_infoR,
                                  ARdouble width, ARdouble conv[3][4] )
{
    ICPStereoDataT     data;
    ICP2DCoordT        screenCoordL[4];
    ICP2DCoordT        screenCoordR[4];
    ICP3DCoordT        worldCoord[4];
    ARdouble             matXc2C[3][4];
    ARdouble             matXw2C[3][4];
    ARdouble             matXw2Xc[3][4];
    ARdouble             err;
    int                dir;
    int                i, j;

    worldCoord[0].x = -width/2.0;
    worldCoord[0].y =  width/2.0;
    worldCoord[0].z =  0.0;
    worldCoord[1].x =  width/2.0;
    worldCoord[1].y =  width/2.0;
    worldCoord[1].z =  0.0;
    worldCoord[2].x =  width/2.0;
    worldCoord[2].y = -width/2.0;
    worldCoord[2].z =  0.0;
    worldCoord[3].x = -width/2.0;
    worldCoord[3].y = -width/2.0;
    worldCoord[3].z =  0.0;
    if( marker_infoL != NULL ) {
        dir = marker_infoL->dir;
        screenCoordL[0].x = marker_infoL->vertex[(4-dir)%4][0];
        screenCoordL[0].y = marker_infoL->vertex[(4-dir)%4][1];
        screenCoordL[1].x = marker_infoL->vertex[(5-dir)%4][0];
        screenCoordL[1].y = marker_infoL->vertex[(5-dir)%4][1];
        screenCoordL[2].x = marker_infoL->vertex[(6-dir)%4][0];
        screenCoordL[2].y = marker_infoL->vertex[(6-dir)%4][1];
        screenCoordL[3].x = marker_infoL->vertex[(7-dir)%4][0];
        screenCoordL[3].y = marker_infoL->vertex[(7-dir)%4][1];
        data.numL         = 4;
        data.screenCoordL = screenCoordL;
        data.worldCoordL  = worldCoord;
    }
    else {
        data.numL         = 0;
        data.screenCoordL = NULL;
        data.worldCoordL  = NULL;
    }
    if( marker_infoR != NULL ) {
        dir = marker_infoR->dir;
        screenCoordR[0].x = marker_infoR->vertex[(4-dir)%4][0];
        screenCoordR[0].y = marker_infoR->vertex[(4-dir)%4][1];
        screenCoordR[1].x = marker_infoR->vertex[(5-dir)%4][0];
        screenCoordR[1].y = marker_infoR->vertex[(5-dir)%4][1];
        screenCoordR[2].x = marker_infoR->vertex[(6-dir)%4][0];
        screenCoordR[2].y = marker_infoR->vertex[(6-dir)%4][1];
        screenCoordR[3].x = marker_infoR->vertex[(7-dir)%4][0];
        screenCoordR[3].y = marker_infoR->vertex[(7-dir)%4][1];
        data.numR         = 4;
        data.screenCoordR = screenCoordR;
        data.worldCoordR  = worldCoord;
    }
    else {
        data.numR         = 0;
        data.screenCoordR = NULL;
        data.worldCoordR  = NULL;
    }


    if( marker_infoL != NULL
     && icpGetInitXw2Xc_from_PlanarData(handle->icpStereoHandle->matXcl2Ul, screenCoordL, worldCoord, 4, matXw2Xc) == 0 ) {
        arUtilMatInv( (const ARdouble (*)[4])handle->icpStereoHandle->matC2L, matXc2C );
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                matXw2C[j][i] = matXc2C[j][0]*matXw2Xc[0][i]
                              + matXc2C[j][1]*matXw2Xc[1][i]
                              + matXc2C[j][2]*matXw2Xc[2][i];
            }
            matXw2C[j][3] += matXc2C[j][3];
        }
    }
    else if( marker_infoR != NULL
     && icpGetInitXw2Xc_from_PlanarData(handle->icpStereoHandle->matXcr2Ur, screenCoordR, worldCoord, 4, matXw2Xc) == 0 ) {
        arUtilMatInv( (const ARdouble (*)[4])(handle->icpStereoHandle->matC2R), matXc2C );
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                matXw2C[j][i] = matXc2C[j][0]*matXw2Xc[0][i]
                              + matXc2C[j][1]*matXw2Xc[1][i]
                              + matXc2C[j][2]*matXw2Xc[2][i];
            }
            matXw2C[j][3] += matXc2C[j][3];
        }
    }
    else return 100000000.0;

    if( icpStereoPoint(handle->icpStereoHandle, &data, matXw2C, conv, &err) < 0 ) return 100000000.0;

    return err;
}

ARdouble arGetTransMatSquareContStereo( AR3DStereoHandle *handle,
                                      ARMarkerInfo *marker_infoL, ARMarkerInfo *marker_infoR, ARdouble prev_conv[3][4],
                                      ARdouble width, ARdouble conv[3][4] )
{
    ICPStereoDataT     data;
    ICP2DCoordT        screenCoordL[4];
    ICP2DCoordT        screenCoordR[4];
    ICP3DCoordT        worldCoord[4];
    ARdouble             err;
    int                dir;

    worldCoord[0].x = -width/2.0;
    worldCoord[0].y =  width/2.0;
    worldCoord[0].z =  0.0;
    worldCoord[1].x =  width/2.0;
    worldCoord[1].y =  width/2.0;
    worldCoord[1].z =  0.0;
    worldCoord[2].x =  width/2.0;
    worldCoord[2].y = -width/2.0;
    worldCoord[2].z =  0.0;
    worldCoord[3].x = -width/2.0;
    worldCoord[3].y = -width/2.0;
    worldCoord[3].z =  0.0;
    if( marker_infoL != NULL ) {
        dir = marker_infoL->dir;
        screenCoordL[0].x = marker_infoL->vertex[(4-dir)%4][0];
        screenCoordL[0].y = marker_infoL->vertex[(4-dir)%4][1];
        screenCoordL[1].x = marker_infoL->vertex[(5-dir)%4][0];
        screenCoordL[1].y = marker_infoL->vertex[(5-dir)%4][1];
        screenCoordL[2].x = marker_infoL->vertex[(6-dir)%4][0];
        screenCoordL[2].y = marker_infoL->vertex[(6-dir)%4][1];
        screenCoordL[3].x = marker_infoL->vertex[(7-dir)%4][0];
        screenCoordL[3].y = marker_infoL->vertex[(7-dir)%4][1];
        data.numL         = 4;
        data.screenCoordL = screenCoordL;
        data.worldCoordL  = worldCoord;
    }
    else {
        data.numL         = 0;
        data.screenCoordL = NULL;
        data.worldCoordL  = NULL;
    }
    if( marker_infoR != NULL ) {
        dir = marker_infoR->dir;
        screenCoordR[0].x = marker_infoR->vertex[(4-dir)%4][0];
        screenCoordR[0].y = marker_infoR->vertex[(4-dir)%4][1];
        screenCoordR[1].x = marker_infoR->vertex[(5-dir)%4][0];
        screenCoordR[1].y = marker_infoR->vertex[(5-dir)%4][1];
        screenCoordR[2].x = marker_infoR->vertex[(6-dir)%4][0];
        screenCoordR[2].y = marker_infoR->vertex[(6-dir)%4][1];
        screenCoordR[3].x = marker_infoR->vertex[(7-dir)%4][0];
        screenCoordR[3].y = marker_infoR->vertex[(7-dir)%4][1];
        data.numR         = 4;
        data.screenCoordR = screenCoordR;
        data.worldCoordR  = worldCoord;
    }
    else {
        data.numR         = 0;
        data.screenCoordR = NULL;
        data.worldCoordR  = NULL;
    }
    if( data.numL == 0 && data.numR == 0 ) return 100000000.0;

    if( icpStereoPoint(handle->icpStereoHandle, &data, prev_conv, conv, &err) < 0 ) return 100000000.0;

    return err;
}


ARdouble arGetTransMatStereo( AR3DStereoHandle *handle, ARdouble initConv[3][4], 
                            ARdouble pos2dL[][2], ARdouble pos3dL[][3], int numL,
                            ARdouble pos2dR[][2], ARdouble pos3dR[][3], int numR,
                            ARdouble conv[3][4] )
{
    ICPStereoDataT       data;
    ARdouble               err;
    int                  i;
    
    if( numL > 0 ) {
        arMalloc( data.screenCoordL, ICP2DCoordT, numL );
        arMalloc( data.worldCoordL,  ICP3DCoordT, numL );
        data.numL = numL;
        for( i = 0; i < numL; i++ ) {
            data.screenCoordL[i].x = pos2dL[i][0];
            data.screenCoordL[i].y = pos2dL[i][1];
            data.worldCoordL[i].x  = pos3dL[i][0];
            data.worldCoordL[i].y  = pos3dL[i][1];
            data.worldCoordL[i].z  = pos3dL[i][2];
        }
    }
    else {
        data.numL = 0;
        data.screenCoordL = NULL;
        data.worldCoordL = NULL;
    }
    if( numR > 0 ) {
        arMalloc( data.screenCoordR, ICP2DCoordT, numR );
        arMalloc( data.worldCoordR,  ICP3DCoordT, numR );
        data.numR = numR;
        for( i = 0; i < numR; i++ ) {
            data.screenCoordR[i].x = pos2dR[i][0];
            data.screenCoordR[i].y = pos2dR[i][1];
            data.worldCoordR[i].x  = pos3dR[i][0];
            data.worldCoordR[i].y  = pos3dR[i][1];
            data.worldCoordR[i].z  = pos3dR[i][2];
        }
    }
    else {
        data.numR = 0;
        data.screenCoordR = NULL;
        data.worldCoordR = NULL;
    }
    if( data.numL == 0 && data.numR == 0 ) return 100000000.0;
    
    if( icpStereoPoint(handle->icpStereoHandle, &data, initConv, conv, &err) < 0 ) return 100000000.0;

    if( data.numL != 0 ) {
        free( data.screenCoordL );
        free( data.worldCoordL );
    }
    if( data.numR != 0 ) {
        free( data.screenCoordR );
        free( data.worldCoordR );
    }

    return err;
}

ARdouble arGetTransMatStereoRobust( AR3DStereoHandle *handle, ARdouble initConv[3][4], 
                                  ARdouble pos2dL[][2], ARdouble pos3dL[][3], int numL,
                                  ARdouble pos2dR[][2], ARdouble pos3dR[][3], int numR,
                                  ARdouble conv[3][4] )
{
    ICPStereoDataT       data;
    ARdouble               err;
    int                  i;
    
    if( numL > 0 ) {
        arMalloc( data.screenCoordL, ICP2DCoordT, numL );
        arMalloc( data.worldCoordL,  ICP3DCoordT, numL );
        data.numL = numL;
        for( i = 0; i < numL; i++ ) {
            data.screenCoordL[i].x = pos2dL[i][0];
            data.screenCoordL[i].y = pos2dL[i][1];
            data.worldCoordL[i].x  = pos3dL[i][0];
            data.worldCoordL[i].y  = pos3dL[i][1];
            data.worldCoordL[i].z  = pos3dL[i][2];
        }
    }
    else {
        data.numL = 0;
        data.screenCoordL = NULL;
        data.worldCoordL = NULL;
    }
    if( numR > 0 ) {
        arMalloc( data.screenCoordR, ICP2DCoordT, numR );
        arMalloc( data.worldCoordR,  ICP3DCoordT, numR );
        data.numR = numR;
        for( i = 0; i < numR; i++ ) {
            data.screenCoordR[i].x = pos2dR[i][0];
            data.screenCoordR[i].y = pos2dR[i][1];
            data.worldCoordR[i].x  = pos3dR[i][0];
            data.worldCoordR[i].y  = pos3dR[i][1];
            data.worldCoordR[i].z  = pos3dR[i][2];
        }
    }
    else {
        data.numR = 0;
        data.screenCoordR = NULL;
        data.worldCoordR = NULL;
    }
    if( data.numL == 0 && data.numR == 0 ) return 100000000.0;
    
    if( icpStereoPointRobust(handle->icpStereoHandle, &data, initConv, conv, &err) < 0 ) return 100000000.0;

    if( data.numL != 0 ) {
        free( data.screenCoordL );
        free( data.worldCoordL );
    }
    if( data.numR != 0 ) {
        free( data.screenCoordR );
        free( data.worldCoordR );
    }

    return err;
}
