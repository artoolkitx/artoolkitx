/*
 *  arMultiGetTransMat.c
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
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 1.0
 * Date: 01/09/05
 *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/arMulti.h>

static ARdouble  arGetTransMatMultiSquare2(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                         ARMultiMarkerInfoT *config, int robustFlag);

ARdouble  arGetTransMatMultiSquare(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                 ARMultiMarkerInfoT *config)
{
    return arGetTransMatMultiSquare2(handle, marker_info, marker_num, config, 0);
}

ARdouble  arGetTransMatMultiSquareRobust(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                       ARMultiMarkerInfoT *config)
{
    return arGetTransMatMultiSquare2(handle, marker_info, marker_num, config, 1);
}

static ARdouble  arGetTransMatMultiSquare2(AR3DHandle *handle, ARMarkerInfo *marker_info, int marker_num,
                                         ARMultiMarkerInfoT *config, int robustFlag)
{
    ARdouble              *pos2d, *pos3d;
    ARdouble              trans1[3][4], trans2[3][4];
    ARdouble              err, err2;
    int                   max, maxArea;
    int                   vnum;
    int                   dir;
    int                   i, j, k;
    //char  mes[12];

    //ARLOGd("-- Pass1--\n");
    for( i = 0; i < config->marker_num; i++ ) {
        k = -1;
        if( config->marker[i].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE ) {
            for( j = 0; j < marker_num; j++ ) {
                if( marker_info[j].idPatt != config->marker[i].patt_id ) continue;
                if( marker_info[j].cfPatt < config->cfPattCutoff ) continue;
                if( k == -1 ) k = j;
                else if( marker_info[k].cfPatt < marker_info[j].cfPatt ) k = j;
            }
            config->marker[i].visible = k;
            if( k >= 0 ) marker_info[k].dir = marker_info[k].dirPatt;
        }
        else { // config->marker[i].patt_type == AR_MULTI_PATTERN_TYPE_MATRIX
            for( j = 0; j < marker_num; j++ ) {
                // Check if we need to examine the globalID rather than patt_id.
                if (marker_info[j].idMatrix == 0 && marker_info[j].globalID != 0ULL) {
                    if( marker_info[j].globalID != config->marker[i].globalID ) continue;
                } else {
                    if( marker_info[j].idMatrix != config->marker[i].patt_id ) continue;
                }
                if( marker_info[j].cfMatrix < config->cfMatrixCutoff ) continue;
                if( k == -1 ) k = j;
                else if( marker_info[k].cfMatrix < marker_info[j].cfMatrix ) k = j;
            }
            config->marker[i].visible = k;
            if( k >= 0 ) marker_info[k].dir = marker_info[k].dirMatrix;
        }
        //if(k>=0) ARLOGd(" *%d\n",i);
    }

    //ARLOGd("-- Pass2--\n");
    vnum = 0;
    for( i = 0; i < config->marker_num; i++ ) {
        if( (j=config->marker[i].visible) < 0 ) continue;

        //glColor3f( 1.0, 1.0, 0.0 );
        //sprintf(mes,"%d",i);
        //argDrawStringsByIdealPos( mes, marker_info[j].pos[0], marker_info[j].pos[1] );
        err = arGetTransMatSquare(handle, &marker_info[j], config->marker[i].width, trans2);
        //ARLOGd(" [%d:dir=%d] err = %f (%f,%f,%f)\n", i, marker_info[j].dir, err, trans2[0][3], trans2[1][3], trans2[2][3]);
        if( err > AR_MULTI_POSE_ERROR_CUTOFF_EACH_DEFAULT ) {
            config->marker[i].visible = -1;
            if (marker_info[j].cutoffPhase == AR_MARKER_INFO_CUTOFF_PHASE_NONE) marker_info[j].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR;
            continue;
        }
        //ARLOGd(" *%d\n",i);
        
        // Use the largest (in terms of 2D coordinates) marker's pose estimate as the
        // input for the initial estimate for the pose estimator. 
        if( vnum == 0 || maxArea < marker_info[j].area ) {
            maxArea = marker_info[j].area;
            max = i; 
            for( j = 0; j < 3; j++ ) { 
                for( k = 0; k < 4; k++ ) trans1[j][k] = trans2[j][k];
            }
        }
        vnum++;
    }
    if( vnum == 0 || vnum < config->min_submarker) { 
        config->prevF = 0;
        return -1;
    }
    arUtilMatMul( (const ARdouble (*)[4])trans1, (const ARdouble (*)[4])config->marker[max].itrans, trans2 ); 
    
    arMalloc(pos2d, ARdouble, vnum*4*2);
    arMalloc(pos3d, ARdouble, vnum*4*3);
    
    j = 0; 
    for( i = 0; i < config->marker_num; i++ ) { 
        if( (k=config->marker[i].visible) < 0 ) continue;
        
        dir = marker_info[k].dir;
        pos2d[j*8+0] = marker_info[k].vertex[(4-dir)%4][0];
        pos2d[j*8+1] = marker_info[k].vertex[(4-dir)%4][1];
        pos2d[j*8+2] = marker_info[k].vertex[(5-dir)%4][0];
        pos2d[j*8+3] = marker_info[k].vertex[(5-dir)%4][1];
        pos2d[j*8+4] = marker_info[k].vertex[(6-dir)%4][0];
        pos2d[j*8+5] = marker_info[k].vertex[(6-dir)%4][1];
        pos2d[j*8+6] = marker_info[k].vertex[(7-dir)%4][0];
        pos2d[j*8+7] = marker_info[k].vertex[(7-dir)%4][1];
        pos3d[j*12+0] = config->marker[i].pos3d[0][0];
        pos3d[j*12+1] = config->marker[i].pos3d[0][1];
        pos3d[j*12+2] = config->marker[i].pos3d[0][2];
        pos3d[j*12+3] = config->marker[i].pos3d[1][0];
        pos3d[j*12+4] = config->marker[i].pos3d[1][1];
        pos3d[j*12+5] = config->marker[i].pos3d[1][2];
        pos3d[j*12+6] = config->marker[i].pos3d[2][0];
        pos3d[j*12+7] = config->marker[i].pos3d[2][1];
        pos3d[j*12+8] = config->marker[i].pos3d[2][2];
        pos3d[j*12+9] = config->marker[i].pos3d[3][0];
        pos3d[j*12+10] = config->marker[i].pos3d[3][1];
        pos3d[j*12+11] = config->marker[i].pos3d[3][2];
        j++;
    }

    if (config->prevF == 0) {
        if (robustFlag) {
            ARdouble inlierProb = 1.0;
            do {
                icpSetInlierProbability(handle->icpHandle, inlierProb);
                err = arGetTransMat(handle, trans2, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, config->trans);
                if (err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT) break;
                inlierProb -= 0.2;
            } while (inlierProb >= config->minInlierProb && inlierProb >= 0.0);
        } else {
            err = arGetTransMat(handle, trans2, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, config->trans);
        }
        free(pos3d);
        free(pos2d);
    } else {
        if (robustFlag) {
            ARdouble inlierProb = 1.0;
            do {
                icpSetInlierProbability(handle->icpHandle, inlierProb);
                err2 = arGetTransMat(handle, trans2, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, trans1);
                err = arGetTransMat(handle, config->trans, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, config->trans);
                if (err2 < err) {
                    for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) config->trans[j][i] = trans1[j][i];
                    err = err2;
                }
                if (err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT) break;
                inlierProb -= 0.2;
            } while (inlierProb >= config->minInlierProb && inlierProb >= 0.0);
        } else {
            err2 = arGetTransMat(handle, trans2, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, trans1);
            err = arGetTransMat(handle, config->trans, (ARdouble (*)[2])pos2d, (ARdouble (*)[3])pos3d, vnum*4, config->trans);
            if (err2 < err) {
                for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) config->trans[j][i] = trans1[j][i];
                err = err2;
            }
        }
        free(pos3d);
        free(pos2d);
    }
    
    if (err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT) config->prevF = 1;
    else {
        config->prevF = 0;
        for (i = 0; i < config->marker_num; i++) { 
            if ((k = config->marker[i].visible) < 0) continue;
            if (marker_info[k].cutoffPhase == AR_MARKER_INFO_CUTOFF_PHASE_NONE) marker_info[k].cutoffPhase = AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR_MULTI;
        }
    }

    return err;
}
