/*
 *  arGetTransMat.c
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

#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>


ARdouble arGetTransMatSquare( AR3DHandle *handle, ARMarkerInfo *marker_info, ARdouble width, ARdouble conv[3][4] )
{
    ICP2DCoordT    screenCoord[4];
    ICP3DCoordT    worldCoord[4];
    ICPDataT       data;
    ARdouble         initMatXw2Xc[3][4];
    ARdouble         err;
    int            dir;

    if(marker_info->idMatrix < 0)
        dir = marker_info->dirPatt;
    else if (marker_info->idPatt < 0)
        dir = marker_info->dirMatrix;
    else
        dir = marker_info->dir;
    
    screenCoord[0].x = marker_info->vertex[(4-dir)%4][0];
    screenCoord[0].y = marker_info->vertex[(4-dir)%4][1];
    screenCoord[1].x = marker_info->vertex[(5-dir)%4][0];
    screenCoord[1].y = marker_info->vertex[(5-dir)%4][1];
    screenCoord[2].x = marker_info->vertex[(6-dir)%4][0];
    screenCoord[2].y = marker_info->vertex[(6-dir)%4][1];
    screenCoord[3].x = marker_info->vertex[(7-dir)%4][0];
    screenCoord[3].y = marker_info->vertex[(7-dir)%4][1];
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
    data.screenCoord = screenCoord;
    data.worldCoord  = worldCoord;
    data.num         = 4;

    if( icpGetInitXw2Xc_from_PlanarData( handle->icpHandle->matXc2U, data.screenCoord, data.worldCoord, data.num, initMatXw2Xc ) < 0 ) return 100000000.0;


    if( icpPoint( handle->icpHandle, &data, initMatXw2Xc, conv, &err ) < 0 ) return 100000000.0;

    return err;
}

ARdouble arGetTransMatSquareCont( AR3DHandle *handle, ARMarkerInfo *marker_info, ARdouble initConv[3][4],
                                ARdouble width, ARdouble conv[3][4] )
{
    ICP2DCoordT    screenCoord[4];
    ICP3DCoordT    worldCoord[4];
    ICPDataT       data;
    ARdouble         err;
    int            dir;
    
    if(marker_info->idMatrix < 0)
        dir = marker_info->dirPatt;
    else if (marker_info->idPatt < 0)
        dir = marker_info->dirMatrix;
    else
        dir = marker_info->dir;
        
    screenCoord[0].x = marker_info->vertex[(4-dir)%4][0];
    screenCoord[0].y = marker_info->vertex[(4-dir)%4][1];
    screenCoord[1].x = marker_info->vertex[(5-dir)%4][0];
    screenCoord[1].y = marker_info->vertex[(5-dir)%4][1];
    screenCoord[2].x = marker_info->vertex[(6-dir)%4][0];
    screenCoord[2].y = marker_info->vertex[(6-dir)%4][1];
    screenCoord[3].x = marker_info->vertex[(7-dir)%4][0];
    screenCoord[3].y = marker_info->vertex[(7-dir)%4][1];
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
    data.screenCoord = screenCoord;
    data.worldCoord  = worldCoord;
    data.num         = 4;

    if( icpPoint( handle->icpHandle, &data, initConv, conv, &err ) < 0 ) return 100000000.0;

    return err;
}

ARdouble arGetTransMat( AR3DHandle *handle, ARdouble initConv[3][4], ARdouble pos2d[][2], ARdouble pos3d[][3], int num,
                      ARdouble conv[3][4] )
{
    ICPDataT       data;
    ARdouble         err;
    int            i;

    arMalloc( data.screenCoord, ICP2DCoordT, num );
    arMalloc( data.worldCoord,  ICP3DCoordT, num );

    for( i = 0; i < num; i++ ) {
        data.screenCoord[i].x = pos2d[i][0];
        data.screenCoord[i].y = pos2d[i][1];
        data.worldCoord[i].x  = pos3d[i][0];
        data.worldCoord[i].y  = pos3d[i][1];
        data.worldCoord[i].z  = pos3d[i][2];
    }
    data.num = num;

    if( icpPoint( handle->icpHandle, &data, initConv, conv, &err ) < 0 ) {
        err = 100000000.0;
    }

    free( data.screenCoord );
    free( data.worldCoord );

    return err;
}

ARdouble arGetTransMatRobust( AR3DHandle *handle, ARdouble initConv[3][4], ARdouble pos2d[][2], ARdouble pos3d[][3], int num,
                            ARdouble conv[3][4] )
{
    ICPDataT       data;
    ARdouble         err;
    int            i;

    arMalloc( data.screenCoord, ICP2DCoordT, num );
    arMalloc( data.worldCoord,  ICP3DCoordT, num );

    for( i = 0; i < num; i++ ) {
        data.screenCoord[i].x = pos2d[i][0];
        data.screenCoord[i].y = pos2d[i][1];
        data.worldCoord[i].x  = pos3d[i][0];
        data.worldCoord[i].y  = pos3d[i][1];
        data.worldCoord[i].z  = pos3d[i][2];
    }
    data.num = num;

    if( icpPointRobust( handle->icpHandle, &data, initConv, conv, &err ) < 0 ) {
        err = 100000000.0;
    }

    free( data.screenCoord );
    free( data.worldCoord );

    return err;
}
