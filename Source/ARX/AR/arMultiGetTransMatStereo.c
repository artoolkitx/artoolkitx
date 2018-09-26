/*
 *  arMultiGetTransMatStereo.c
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
 * Revision: 1.0
 * Date: 01/12/11
 *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/arMulti.h>

static ARdouble  arGetTransMatMultiSquareStereo2(AR3DStereoHandle *handle,
                                               ARMarkerInfo *marker_infoL, int marker_numL,
                                               ARMarkerInfo *marker_infoR, int marker_numR, 
                                               ARMultiMarkerInfoT *config, int robustFlag);


ARdouble  arGetTransMatMultiSquareStereo(AR3DStereoHandle *handle,
                                       ARMarkerInfo *marker_infoL, int marker_numL,
                                       ARMarkerInfo *marker_infoR, int marker_numR, 
                                       ARMultiMarkerInfoT *config)
{
    return arGetTransMatMultiSquareStereo2(handle, marker_infoL, marker_numL, marker_infoR, marker_numR, 
                                           config, 0);
}

ARdouble  arGetTransMatMultiSquareStereoRobust(AR3DStereoHandle *handle,
                                             ARMarkerInfo *marker_infoL, int marker_numL,
                                             ARMarkerInfo *marker_infoR, int marker_numR, 
                                             ARMultiMarkerInfoT *config)
{
    return arGetTransMatMultiSquareStereo2(handle, marker_infoL, marker_numL, marker_infoR, marker_numR, 
                                           config, 1);
}


static ARdouble  arGetTransMatMultiSquareStereo2(AR3DStereoHandle *handle,
                                               ARMarkerInfo *marker_infoL, int marker_numL,
                                               ARMarkerInfo *marker_infoR, int marker_numR, 
                                               ARMultiMarkerInfoT *config, int robustFlag)

{
    ARdouble                *pos2dL = NULL, *pos3dL = NULL;
    ARdouble                *pos2dR = NULL, *pos3dR = NULL;
    ARdouble                trans1[3][4], trans2[3][4];
    ARdouble                err, err2;
    int                   max, maxArea;
    int                   vnumL, vnumR;
    int                   dir;
    int                   i, j, k;

    for( i = 0; i < config->marker_num; i++ ) {
        k = -1;
        if( config->marker[i].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE ) {
            for( j = 0; j < marker_numL; j++ ) {
                if( marker_infoL[j].idPatt != config->marker[i].patt_id ) continue;
                if( marker_infoL[j].cfPatt < 0.50 ) continue;
                if( k == -1 ) k = j;
                else if( marker_infoL[k].cfPatt < marker_infoL[j].cfPatt ) k = j;
            }
            config->marker[i].visible = k;
            if( k >= 0 ) {
                marker_infoL[k].dir = marker_infoL[k].dirPatt;
            }
        }
        else {
            for( j = 0; j < marker_numL; j++ ) {
                if( marker_infoL[j].idMatrix != config->marker[i].patt_id ) continue;
                if( marker_infoL[j].cfMatrix < 0.50 ) continue;
                if( k == -1 ) k = j;
                else if( marker_infoL[k].cfMatrix < marker_infoL[j].cfMatrix ) k = j;
            }
            config->marker[i].visible = k;
            if( k >= 0 ) {
                marker_infoL[k].dir = marker_infoL[k].dirMatrix;
            }
        }

        k = -1;
        if( config->marker[i].patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE ) {
            for( j = 0; j < marker_numR; j++ ) {
                if( marker_infoR[j].idPatt != config->marker[i].patt_id ) continue;
                if( marker_infoR[j].cfPatt < 0.50 ) continue;
                if( k == -1 ) k = j;
                else if( marker_infoR[k].cfPatt < marker_infoR[j].cfPatt ) k = j;
            }
            config->marker[i].visibleR = k;
            if( k >= 0 ) {
                marker_infoR[k].dir = marker_infoR[k].dirPatt;
            }
        }
        else {
            for( j = 0; j < marker_numR; j++ ) {
                if( marker_infoR[j].idMatrix != config->marker[i].patt_id ) continue;
                if( marker_infoR[j].cfMatrix < 0.50 ) continue;
                if( k == -1 ) k = j;
                else if( marker_infoR[k].cfMatrix < marker_infoR[j].cfMatrix ) k = j;
            }
            config->marker[i].visibleR = k;
            if( k >= 0 ) {
                marker_infoR[k].dir = marker_infoR[k].dirMatrix;
            }
        }
    }

    vnumL = 0;
    for( i = 0; i < config->marker_num; i++ ) {
        if( (j=config->marker[i].visible) == -1 ) continue;

        err = arGetTransMatSquareStereo( handle, &marker_infoL[j], NULL, config->marker[i].width, trans2 );
        if( err > AR_MULTI_POSE_ERROR_CUTOFF_EACH_DEFAULT ) {
            config->marker[i].visible = -1;
            //ARLOGd("err = %f\n", err);
            continue;
        }

        if( vnumL == 0 || maxArea < marker_infoL[j].area ) {
            maxArea = marker_infoL[j].area;
            max = i; 
            for( j = 0; j < 3; j++ ) { 
                for( k = 0; k < 4; k++ ) trans1[j][k] = trans2[j][k];
            }
        }
        vnumL++;
    }
    vnumR = 0;
    for( i = 0; i < config->marker_num; i++ ) {
        if( (j=config->marker[i].visibleR) == -1 ) continue;

        err = arGetTransMatSquareStereo( handle, NULL, &marker_infoR[j], config->marker[i].width, trans2 );
        if( err > AR_MULTI_POSE_ERROR_CUTOFF_EACH_DEFAULT ) {
            config->marker[i].visibleR = -1;
            //ARLOGd("err = %f\n", err);
            continue;
        }

        if( (vnumL == 0 && vnumR == 0) || maxArea < marker_infoR[j].area ) {
            maxArea = marker_infoR[j].area;
            max = i; 
            for( j = 0; j < 3; j++ ) { 
                for( k = 0; k < 4; k++ ) trans1[j][k] = trans2[j][k];
            }
        }
        vnumR++;
    }
    //ARLOGd"vnumL=%d, vnumR=%d\n", vnumL, vnumR);
    if( (vnumL == 0 && vnumR == 0) || (vnumL < config->min_submarker && vnumR < config->min_submarker) ) {
        config->prevF = 0;
        //ARLOGd("**** NG.\n");
        return -1;
    }


    if(vnumL > 0) {
        arMalloc(pos2dL, ARdouble, vnumL*4*2);
        arMalloc(pos3dL, ARdouble, vnumL*4*3);
    }
    if(vnumR > 0) {
        arMalloc(pos2dR, ARdouble, vnumR*4*2);
        arMalloc(pos3dR, ARdouble, vnumR*4*3);
    }

    j = 0;
    for( i = 0; i < config->marker_num; i++ ) {
        if( (k=config->marker[i].visible) < 0 ) continue;
        dir = marker_infoL[k].dir;
        pos2dL[j*8+0] = marker_infoL[k].vertex[(4-dir)%4][0];
        pos2dL[j*8+1] = marker_infoL[k].vertex[(4-dir)%4][1];
        pos2dL[j*8+2] = marker_infoL[k].vertex[(5-dir)%4][0];
        pos2dL[j*8+3] = marker_infoL[k].vertex[(5-dir)%4][1];
        pos2dL[j*8+4] = marker_infoL[k].vertex[(6-dir)%4][0];
        pos2dL[j*8+5] = marker_infoL[k].vertex[(6-dir)%4][1];
        pos2dL[j*8+6] = marker_infoL[k].vertex[(7-dir)%4][0];
        pos2dL[j*8+7] = marker_infoL[k].vertex[(7-dir)%4][1];
        pos3dL[j*12+0] = config->marker[i].pos3d[0][0];
        pos3dL[j*12+1] = config->marker[i].pos3d[0][1];
        pos3dL[j*12+2] = config->marker[i].pos3d[0][2];
        pos3dL[j*12+3] = config->marker[i].pos3d[1][0];
        pos3dL[j*12+4] = config->marker[i].pos3d[1][1];
        pos3dL[j*12+5] = config->marker[i].pos3d[1][2];
        pos3dL[j*12+6] = config->marker[i].pos3d[2][0];
        pos3dL[j*12+7] = config->marker[i].pos3d[2][1];
        pos3dL[j*12+8] = config->marker[i].pos3d[2][2];
        pos3dL[j*12+9] = config->marker[i].pos3d[3][0];
        pos3dL[j*12+10] = config->marker[i].pos3d[3][1];
        pos3dL[j*12+11] = config->marker[i].pos3d[3][2];
        j++;
    }

    j = 0;
    for( i = 0; i < config->marker_num; i++ ) {
        if( (k=config->marker[i].visibleR) < 0 ) continue;
        dir = marker_infoR[k].dir;
        pos2dR[j*8+0] = marker_infoR[k].vertex[(4-dir)%4][0];
        pos2dR[j*8+1] = marker_infoR[k].vertex[(4-dir)%4][1];
        pos2dR[j*8+2] = marker_infoR[k].vertex[(5-dir)%4][0];
        pos2dR[j*8+3] = marker_infoR[k].vertex[(5-dir)%4][1];
        pos2dR[j*8+4] = marker_infoR[k].vertex[(6-dir)%4][0];
        pos2dR[j*8+5] = marker_infoR[k].vertex[(6-dir)%4][1];
        pos2dR[j*8+6] = marker_infoR[k].vertex[(7-dir)%4][0];
        pos2dR[j*8+7] = marker_infoR[k].vertex[(7-dir)%4][1];
        pos3dR[j*12+0] = config->marker[i].pos3d[0][0];
        pos3dR[j*12+1] = config->marker[i].pos3d[0][1];
        pos3dR[j*12+2] = config->marker[i].pos3d[0][2];
        pos3dR[j*12+3] = config->marker[i].pos3d[1][0];
        pos3dR[j*12+4] = config->marker[i].pos3d[1][1];
        pos3dR[j*12+5] = config->marker[i].pos3d[1][2];
        pos3dR[j*12+6] = config->marker[i].pos3d[2][0];
        pos3dR[j*12+7] = config->marker[i].pos3d[2][1];
        pos3dR[j*12+8] = config->marker[i].pos3d[2][2];
        pos3dR[j*12+9] = config->marker[i].pos3d[3][0];
        pos3dR[j*12+10] = config->marker[i].pos3d[3][1];
        pos3dR[j*12+11] = config->marker[i].pos3d[3][2];
        j++;
    }

    if (config->prevF == 0) {
        arUtilMatMul((const ARdouble (*)[4])trans1, (const ARdouble (*)[4])config->marker[max].itrans, trans2);
        if (robustFlag) {
            ARdouble inlierProb = 1.0;
            do {
                icpStereoSetInlierProbability(handle->icpStereoHandle, inlierProb);
                err = arGetTransMatStereo(handle, trans2, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                          (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, config->trans);
                if (err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT) break;
                inlierProb -= 0.2;
            } while (inlierProb >= config->minInlierProb && inlierProb >= 0.0);
        } else {
            err = arGetTransMatStereo(handle, trans2, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                      (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, config->trans);
        }
    } else {
        arUtilMatMul((const ARdouble (*)[4])trans1, (const ARdouble (*)[4])config->marker[max].itrans, trans2);
        if (robustFlag) {
            ARdouble inlierProb = 1.0;
            do {
                icpStereoSetInlierProbability(handle->icpStereoHandle, inlierProb);
                err2 = arGetTransMatStereo(handle, trans2, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                           (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, trans1);
                err = arGetTransMatStereo(handle, config->trans, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                          (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, config->trans);
                if (err2 < err) {
                    for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) config->trans[j][i] = trans1[j][i];
                    err = err2;
                }
                if (err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT) break;
                inlierProb -= 0.2;
            } while (inlierProb >= config->minInlierProb && inlierProb >= 0.0);
        } else {
            err2 = arGetTransMatStereo(handle, trans2, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                       (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, trans1);
            err = arGetTransMatStereo(handle, config->trans, (ARdouble (*)[2])pos2dL, (ARdouble (*)[3])pos3dL, vnumL*4,
                                      (ARdouble (*)[2])pos2dR, (ARdouble (*)[3])pos3dR, vnumR*4, config->trans);
            if (err2 < err) {
                for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) config->trans[j][i] = trans1[j][i];
                err = err2;
            }
        }
    }

    if( vnumL > 0 ) {
        free(pos3dL);
        free(pos2dL);
    }
    if( vnumR > 0 ) {
        free(pos3dR);
        free(pos2dR);
   }

    if( err < AR_MULTI_POSE_ERROR_CUTOFF_COMBINED_DEFAULT ) {
        config->prevF = 1;
    }
    else {
        config->prevF = 0;
    }

    return err;
}
