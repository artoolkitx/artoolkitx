/*
 *  AR2/tracking.c
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <strings.h>
#endif
#include <math.h>
#include <ARX/AR/icp.h>
#include <ARX/AR2/coord.h>
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/template.h>
#include <ARX/AR2/tracking.h>

static float  ar2GetTransMat            ( ICPHandleT *icpHandle, float  initConv[3][4],
                                          float  pos2d[][2], float  pos3d[][3], int num, float  conv[3][4], int robustMode );
static float  ar2GetTransMatHomography        ( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, 
                                          float  conv[3][4], int robustMode, float inlierProb );
static float  ar2GetTransMatHomography2       ( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, float  conv[3][4] );
static float  ar2GetTransMatHomographyRobust  ( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, float  conv[3][4], float inlierProb );
static int    extractVisibleFeatures    ( const ARParamLT *cparamLT, const float  trans1[][3][4], AR2SurfaceSetT *surfaceSet,
                                          AR2TemplateCandidateT candidate[],
                                          AR2TemplateCandidateT candidate2[] );
static int    extractVisibleFeaturesHomography( int xsize, int ysize, float  trans1[][3][4], AR2SurfaceSetT *surfaceSet,
                                          AR2TemplateCandidateT candidate[],
                                          AR2TemplateCandidateT candidate2[] );
static int    getDeltaS( float  H[8], float  dU[], float  J_U_H[][8], int n );


int ar2Tracking( AR2HandleT *ar2Handle, AR2SurfaceSetT *surfaceSet, ARUint8 *dataPtr, float  trans[3][4], float  *err )
{
    AR2TemplateCandidateT  *candidatePtr;
    AR2TemplateCandidateT  *cp[AR2_THREAD_MAX];
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    float                   aveBlur;
#endif
    int                     num, num2;
    int                     i, j, k;

    if (!ar2Handle || !surfaceSet || !dataPtr || !trans || !err) return (-1);

    if( surfaceSet->contNum <= 0  ) {
        ARLOGd("ar2Tracking() error: ar2SetInitTrans() must be called first.\n");
        return -2;
    }

    *err = 0.0F;

    for( i = 0; i < surfaceSet->num; i++ ) {
        arUtilMatMulf( (const float (*)[4])surfaceSet->trans1, (const float (*)[4])surfaceSet->surface[i].trans, ar2Handle->wtrans1[i] );
        if( surfaceSet->contNum > 1 ) arUtilMatMulf( (const float (*)[4])surfaceSet->trans2, (const float (*)[4])surfaceSet->surface[i].trans, ar2Handle->wtrans2[i] );
        if( surfaceSet->contNum > 2 ) arUtilMatMulf( (const float (*)[4])surfaceSet->trans3, (const float (*)[4])surfaceSet->surface[i].trans, ar2Handle->wtrans3[i] );
    }

    if( ar2Handle->trackingMode == AR2_TRACKING_6DOF ) {
        extractVisibleFeatures(ar2Handle->cparamLT, ar2Handle->wtrans1, surfaceSet, ar2Handle->candidate, ar2Handle->candidate2);
    }
    else {
        extractVisibleFeaturesHomography(ar2Handle->xsize, ar2Handle->ysize, ar2Handle->wtrans1, surfaceSet, ar2Handle->candidate, ar2Handle->candidate2);
    }

    candidatePtr = ar2Handle->candidate;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    aveBlur = 0.0F;
#endif
    i = 0; // Counts up to searchFeatureNum.
    num = 0;
    while( i < ar2Handle->searchFeatureNum ) {
        num2 = num;
        for( j = 0; j < ar2Handle->threadNum; j++ ) {
            if( i == ar2Handle->searchFeatureNum ) break;

            k = ar2SelectTemplate( candidatePtr, surfaceSet->prevFeature, num2, ar2Handle->pos, ar2Handle->xsize, ar2Handle->ysize );
            if( k < 0 ) {
                if( candidatePtr == ar2Handle->candidate ) {
                    candidatePtr = ar2Handle->candidate2;
                    k = ar2SelectTemplate( candidatePtr, surfaceSet->prevFeature, num2, ar2Handle->pos, ar2Handle->xsize, ar2Handle->ysize );
                    if( k < 0 ) break; // PRL 2012-05-15: Give up if we can't select template from alternate candidate either.
                }
                else break;
            }

            cp[j] = &(candidatePtr[k]);
            ar2Handle->pos[num2][0] = candidatePtr[k].sx;
            ar2Handle->pos[num2][1] = candidatePtr[k].sy;
            ar2Handle->arg[j].ar2Handle  = ar2Handle;
            ar2Handle->arg[j].surfaceSet = surfaceSet;
            ar2Handle->arg[j].candidate  = &(candidatePtr[k]);
            ar2Handle->arg[j].dataPtr    = dataPtr;

            threadStartSignal( ar2Handle->threadHandle[j] );
            num2++;
            if( num2 == 5 ) num2 = num;
            i++;
        }
        k = j;
        if( k == 0 ) break;

        for( j = 0; j < k; j++ ) {
            threadEndWait( ar2Handle->threadHandle[j] );

            if( ar2Handle->arg[j].ret == 0 && ar2Handle->arg[j].result.sim > ar2Handle->simThresh ) {
                if( ar2Handle->trackingMode == AR2_TRACKING_6DOF ) {
#ifdef ARDOUBLE_IS_FLOAT
                    arParamObserv2Ideal(ar2Handle->cparamLT->param.dist_factor,
                                        ar2Handle->arg[j].result.pos2d[0], ar2Handle->arg[j].result.pos2d[1],
                                        &ar2Handle->pos2d[num][0], &ar2Handle->pos2d[num][1], ar2Handle->cparamLT->param.dist_function_version);
#else
                    ARdouble pos2d0, pos2d1;
                    arParamObserv2Ideal(ar2Handle->cparamLT->param.dist_factor,                    
                                        (ARdouble)(ar2Handle->arg[j].result.pos2d[0]), (ARdouble)(ar2Handle->arg[j].result.pos2d[1]),
                                        &pos2d0, &pos2d1, ar2Handle->cparamLT->param.dist_function_version);
                    ar2Handle->pos2d[num][0] = (float)pos2d0;
                    ar2Handle->pos2d[num][1] = (float)pos2d1;
#endif
                }
                else {
                    ar2Handle->pos2d[num][0] = ar2Handle->arg[j].result.pos2d[0];
                    ar2Handle->pos2d[num][1] = ar2Handle->arg[j].result.pos2d[1];
                }
                ar2Handle->pos3d[num][0] = ar2Handle->arg[j].result.pos3d[0];
                ar2Handle->pos3d[num][1] = ar2Handle->arg[j].result.pos3d[1];
                ar2Handle->pos3d[num][2] = ar2Handle->arg[j].result.pos3d[2];
                ar2Handle->pos[num][0] = cp[j]->sx;
                ar2Handle->pos[num][1] = cp[j]->sy;
                ar2Handle->usedFeature[num].snum  = cp[j]->snum;
                ar2Handle->usedFeature[num].level = cp[j]->level;
                ar2Handle->usedFeature[num].num   = cp[j]->num;
                ar2Handle->usedFeature[num].flag  = 0;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                aveBlur += ar2Handle->arg[j].result.blurLevel;
#endif
                num++;
            }
        }
    }
    for( i = 0; i < num; i++ ) {
        surfaceSet->prevFeature[i] = ar2Handle->usedFeature[i];
    }
    surfaceSet->prevFeature[num].flag = -1;
    //ARLOGd("------\nNum = %d\n", num);

    if( ar2Handle->trackingMode == AR2_TRACKING_6DOF ) {
        if( num < 3 ) {
            surfaceSet->contNum = 0;
            return -3;
        }
        *err = ar2GetTransMat( ar2Handle->icpHandle, surfaceSet->trans1, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 0 );
        //ARLOGd("outlier  0%%: err = %f, num = %d\n", *err, num);
        if( *err > ar2Handle->trackingThresh ) {
            icpSetInlierProbability( ar2Handle->icpHandle, 0.8F );
            *err = ar2GetTransMat( ar2Handle->icpHandle, trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1 );
            //ARLOGd("outlier 20%%: err = %f, num = %d\n", *err, num);
            if( *err > ar2Handle->trackingThresh ) {
                icpSetInlierProbability( ar2Handle->icpHandle, 0.6F );
                *err = ar2GetTransMat( ar2Handle->icpHandle, trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1 );
                //ARLOGd("outlier 60%%: err = %f, num = %d\n", *err, num);
                if( *err > ar2Handle->trackingThresh ) {
                    icpSetInlierProbability( ar2Handle->icpHandle, 0.4F );
                    *err = ar2GetTransMat( ar2Handle->icpHandle, trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1 );
                    //ARLOGd("outlier 60%%: err = %f, num = %d\n", *err, num);
                    if( *err > ar2Handle->trackingThresh ) {
                        icpSetInlierProbability( ar2Handle->icpHandle, 0.0F );
                        *err = ar2GetTransMat( ar2Handle->icpHandle, trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1 );
                        //ARLOGd("outlier Max: err = %f, num = %d\n", *err, num);
                        if( *err > ar2Handle->trackingThresh ) {
                            surfaceSet->contNum = 0;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                            if( ar2Handle->blurMethod == AR2_ADAPTIVE_BLUR ) ar2Handle->blurLevel = AR2_DEFAULT_BLUR_LEVEL; // Reset the blurLevel.
#endif
                            return -4;
                        }
                    }
                }
            }
        }
    }
    else {
        if( num < 3 ) {
            surfaceSet->contNum = 0;
            return -3;
        }
        *err = ar2GetTransMatHomography( surfaceSet->trans1, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 0, 1.0F );
        //ARLOGd("outlier  0%%: err = %f, num = %d\n", *err, num);
        if( *err > ar2Handle->trackingThresh ) {
            *err = ar2GetTransMatHomography( trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1, 0.8F );
            //ARLOGd("outlier 20%%: err = %f, num = %d\n", *err, num);
            if( *err > ar2Handle->trackingThresh ) {
                *err = ar2GetTransMatHomography( trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1, 0.6F );
                //ARLOGd("outlier 40%%: err = %f, num = %d\n", *err, num);
                if( *err > ar2Handle->trackingThresh ) {
                    *err = ar2GetTransMatHomography( trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1, 0.4F );
                    //ARLOGd("outlier 60%%: err = %f, num = %d\n", *err, num);
                    if( *err > ar2Handle->trackingThresh ) {
                        *err = ar2GetTransMatHomography( trans, ar2Handle->pos2d, ar2Handle->pos3d, num, trans, 1, 0.0F );
                        //ARLOGd("outlier Max: err = %f, num = %d\n", *err, num);
                        if( *err > ar2Handle->trackingThresh ) {
                            surfaceSet->contNum = 0;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                            if( ar2Handle->blurMethod == AR2_ADAPTIVE_BLUR ) ar2Handle->blurLevel = AR2_DEFAULT_BLUR_LEVEL; // Reset the blurLevel.
#endif
                            return -4;
                        }
                    }
                }
            }
        }
    }

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    if( ar2Handle->blurMethod == AR2_ADAPTIVE_BLUR ) {
        aveBlur = aveBlur/num + 0.5F;
        ar2Handle->blurLevel += (int)aveBlur - 1;
        if( ar2Handle->blurLevel < 1 ) ar2Handle->blurLevel = 1;
        if( ar2Handle->blurLevel >= AR2_BLUR_IMAGE_MAX-1 ) ar2Handle->blurLevel = AR2_BLUR_IMAGE_MAX-2;
    }
#endif

    surfaceSet->contNum++;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) surfaceSet->trans3[j][i] = surfaceSet->trans2[j][i];
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) surfaceSet->trans2[j][i] = surfaceSet->trans1[j][i];
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) surfaceSet->trans1[j][i] = trans[j][i];
    }

    return 0;
}

static int extractVisibleFeatures(const ARParamLT *cparamLT, const float  trans1[][3][4], AR2SurfaceSetT *surfaceSet,
                                  AR2TemplateCandidateT candidate[],  // candidates inside DPI range of [mindpi, maxdpi].
                                  AR2TemplateCandidateT candidate2[]) // candidates inside DPI range of [mindpi/2, maxdpi*2].
{
    float       trans2[3][4];
    float       sx, sy;
    float       wpos[2], w[2];
    float       vdir[3], vlen;
    int         xsize, ysize;
    int         i, j, k, l, l2;

    xsize = cparamLT->param.xsize;
    ysize = cparamLT->param.ysize;

    l = l2 = 0;
    for( i = 0; i < surfaceSet->num; i++ ) {
        for(j=0;j<3;j++) for(k=0;k<4;k++) trans2[j][k] = trans1[i][j][k];

        for( j = 0; j < surfaceSet->surface[i].featureSet->num; j++ ) {
            for( k = 0; k < surfaceSet->surface[i].featureSet->list[j].num; k++ ) {

                if( ar2MarkerCoord2ScreenCoord2( cparamLT, (const float (*)[4])trans2,
                                                 surfaceSet->surface[i].featureSet->list[j].coord[k].mx,
                                                 surfaceSet->surface[i].featureSet->list[j].coord[k].my,
                                                 &sx, &sy) < 0 ) continue;
                if( sx < 0 || sx >= xsize ) continue;
                if( sy < 0 || sy >= ysize ) continue;

                vdir[0] = trans2[0][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[0][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[0][3];
                vdir[1] = trans2[1][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[1][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[1][3];
                vdir[2] = trans2[2][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[2][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[2][3];
                vlen = sqrtf( vdir[0]*vdir[0] + vdir[1]*vdir[1] + vdir[2]*vdir[2] );
                vdir[0] /= vlen;
                vdir[1] /= vlen;
                vdir[2] /= vlen;
                if( vdir[0]*trans2[0][2] + vdir[1]*trans2[1][2] + vdir[2]*trans2[2][2] > -0.1f ) continue;

                wpos[0] = surfaceSet->surface[i].featureSet->list[j].coord[k].mx;
                wpos[1] = surfaceSet->surface[i].featureSet->list[j].coord[k].my;
                ar2GetResolution( cparamLT, (const float (*)[4])trans2, wpos, w );
                //if( w[0] <= surfaceSet->surface[i].featureSet->list[j].maxdpi
                // && w[0] >= surfaceSet->surface[i].featureSet->list[j].mindpi ) {
                if( w[1] <= surfaceSet->surface[i].featureSet->list[j].maxdpi
                 && w[1] >= surfaceSet->surface[i].featureSet->list[j].mindpi ) {
                    if( l == AR2_TRACKING_CANDIDATE_MAX ) {
                        ARLOGe("### Feature candidates for tracking are overflow.\n");
                        candidate[l].flag = -1;
                        return -1;
                    }
                    candidate[l].snum  = i;
                    candidate[l].level = j;
                    candidate[l].num   = k;
                    candidate[l].sx    = sx;
                    candidate[l].sy    = sy;
                    candidate[l].flag  = 0;
                    l++;
                }
                else if( w[1] <= surfaceSet->surface[i].featureSet->list[j].maxdpi*2
                      && w[1] >= surfaceSet->surface[i].featureSet->list[j].mindpi/2 ) {
                    if( l2 == AR2_TRACKING_CANDIDATE_MAX ) {
                        candidate2[l2].flag = -1;
                    }
                    else {
                        candidate2[l2].snum  = i;
                        candidate2[l2].level = j;
                        candidate2[l2].num   = k;
                        candidate2[l2].sx    = sx;
                        candidate2[l2].sy    = sy;
                        candidate2[l2].flag  = 0;
                        l2++;
                    }
                }
            }
        }
    }
    candidate[l].flag = -1;
    candidate2[l2].flag = -1;

    return 0;
}

static int extractVisibleFeaturesHomography(int xsize, int ysize, float  trans1[][3][4], AR2SurfaceSetT *surfaceSet,
                                      AR2TemplateCandidateT candidate[],
                                      AR2TemplateCandidateT candidate2[])
{
    float       trans2[3][4];
    float       sx, sy;
    float       wpos[2], w[2];
    //float       vdir[3], vlen;
    int         i, j, k, l, l2;

    l = l2 = 0;
    for( i = 0; i < surfaceSet->num; i++ ) {
        for(j=0;j<3;j++) for(k=0;k<4;k++) trans2[j][k] = trans1[i][j][k];

        for( j = 0; j < surfaceSet->surface[i].featureSet->num; j++ ) {
            for( k = 0; k < surfaceSet->surface[i].featureSet->list[j].num; k++ ) {

                if( ar2MarkerCoord2ScreenCoord2( NULL, (const float (*)[4])trans2,
                                                 surfaceSet->surface[i].featureSet->list[j].coord[k].mx,
                                                 surfaceSet->surface[i].featureSet->list[j].coord[k].my,
                                                 &sx, &sy) < 0 ) continue;
                if( sx < 0 || sx >= xsize ) continue;
                if( sy < 0 || sy >= ysize ) continue;

/*
                vdir[0] = trans2[0][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[0][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[0][3];
                vdir[1] = trans2[1][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[1][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[1][3];
                vdir[2] = trans2[2][0] * surfaceSet->surface[i].featureSet->list[j].coord[k].mx
                        + trans2[2][1] * surfaceSet->surface[i].featureSet->list[j].coord[k].my
                        + trans2[2][3];
                vlen = sqrtf( vdir[0]*vdir[0] + vdir[1]*vdir[1] + vdir[2]*vdir[2] );
                vdir[0] /= vlen;
                vdir[1] /= vlen;
                vdir[2] /= vlen;
                if( vdir[0]*trans2[0][2] + vdir[1]*trans2[1][2] + vdir[2]*trans2[2][2] > -0.1 ) continue;
*/

                wpos[0] = surfaceSet->surface[i].featureSet->list[j].coord[k].mx;
                wpos[1] = surfaceSet->surface[i].featureSet->list[j].coord[k].my;
                ar2GetResolution( NULL, (const float (*)[4])trans2, wpos, w );
                //if( w[0] <= surfaceSet->surface[i].featureSet->list[j].maxdpi
                // && w[0] >= surfaceSet->surface[i].featureSet->list[j].mindpi ) {
                if( w[1] <= surfaceSet->surface[i].featureSet->list[j].maxdpi
                 && w[1] >= surfaceSet->surface[i].featureSet->list[j].mindpi ) {
                    if( l == AR2_TRACKING_CANDIDATE_MAX ) {
                        ARLOGe("### Feature candidates for tracking are overflow.\n");
                        candidate[l].flag = -1;
                        return -1;
                    }
                    candidate[l].snum  = i;
                    candidate[l].level = j;
                    candidate[l].num   = k;
                    candidate[l].sx    = sx;
                    candidate[l].sy    = sy;
                    candidate[l].flag  = 0;
                    l++;
                }
                else if( w[1] <= surfaceSet->surface[i].featureSet->list[j].maxdpi*2
                      && w[1] >= surfaceSet->surface[i].featureSet->list[j].mindpi/2 ) {
                    if( l2 == AR2_TRACKING_CANDIDATE_MAX ) {
                        candidate2[l2].flag = -1;
                    }
                    else {
                        candidate2[l2].snum  = i;
                        candidate2[l2].level = j;
                        candidate2[l2].num   = k;
                        candidate2[l2].sx    = sx;
                        candidate2[l2].sy    = sy;
                        candidate2[l2].flag  = 0;
                        l2++;
                    }
                }
            }
        }
    }
    candidate[l].flag = -1;
    candidate2[l2].flag = -1;

    return 0;
}

static float  ar2GetTransMat( ICPHandleT *icpHandle, float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num,
                              float  conv[3][4], int robustMode )
{   
    ICPDataT       data;
    float          dx, dy, dz;
    ARdouble       initMat[3][4], mat[3][4];
    ARdouble       err;
    int            i, j;
    
    arMalloc( data.screenCoord, ICP2DCoordT, num );
    arMalloc( data.worldCoord,  ICP3DCoordT, num );
    
    dx = dy = dz = 0.0;
    for( i = 0; i < num; i++ ) {
        dx += pos3d[i][0];
        dy += pos3d[i][1];
        dz += pos3d[i][2];
    }
    dx /= num;
    dy /= num;
    dz /= num;
    
    for( i = 0; i < num; i++ ) {
        data.screenCoord[i].x = pos2d[i][0];
        data.screenCoord[i].y = pos2d[i][1];
        data.worldCoord[i].x  = pos3d[i][0] - dx;
        data.worldCoord[i].y  = pos3d[i][1] - dy;
        data.worldCoord[i].z  = pos3d[i][2] - dz;
    }
    data.num = num;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) initMat[j][i] = (ARdouble)(initConv[j][i]);
    }
    initMat[0][3] = (ARdouble)(initConv[0][0] * dx + initConv[0][1] * dy + initConv[0][2] * dz + initConv[0][3]);
    initMat[1][3] = (ARdouble)(initConv[1][0] * dx + initConv[1][1] * dy + initConv[1][2] * dz + initConv[1][3]);
    initMat[2][3] = (ARdouble)(initConv[2][0] * dx + initConv[2][1] * dy + initConv[2][2] * dz + initConv[2][3]);
    
    if( robustMode == 0 ) {
        if( icpPoint( icpHandle, &data, initMat, mat, &err ) < 0 ) {
            err = 100000000.0F;
        }
    }
    else {
        if( icpPointRobust( icpHandle, &data, initMat, mat, &err ) < 0 ) {
            err = 100000000.0F;
        }
    }

    free( data.screenCoord );
    free( data.worldCoord );

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) conv[j][i] = (float)mat[j][i];
    }
    conv[0][3] = (float)(mat[0][3] - mat[0][0] * dx - mat[0][1] * dy - mat[0][2] * dz);
    conv[1][3] = (float)(mat[1][3] - mat[1][0] * dx - mat[1][1] * dy - mat[1][2] * dz);
    conv[2][3] = (float)(mat[2][3] - mat[2][0] * dx - mat[2][1] * dy - mat[2][2] * dz);

    return (float)err;
}

static float  ar2GetTransMatHomography( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, 
                                  float  conv[3][4], int robustMode, float inlierProb )
{
    if( robustMode == 0 ) {
        return ar2GetTransMatHomography2( initConv, pos2d, pos3d, num, conv );
    }
    else {
        return ar2GetTransMatHomographyRobust( initConv, pos2d, pos3d, num, conv, inlierProb );
    }
}

static float  ar2GetTransMatHomography2( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, float  conv[3][4] )
{
    float         err = 100000000.0F;
    float        *J_U_H;
    float        *dU;
    float         hx, hy, h, hh, ux, uy, dx, dy;
    float         dH[8];
    float         err0, err1;
    int           i, j;

    if( num < 4 ) return err;
    if( initConv[2][3] == 0.0F ) return err;

    if( (J_U_H = (float  *)malloc( sizeof(float)*16*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        return -1;
    }
    if( (dU = (float  *)malloc( sizeof(float)*2*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_H);
        return -1;
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) conv[j][i] = initConv[j][i]/ initConv[2][3];
    }

    for( i = 0;; i++ ) {
        err1 = 0.0F;
        for( j = 0; j < num; j++ ) {
            hx = conv[0][0] * pos3d[j][0] + conv[0][1] * pos3d[j][1] + conv[0][3];
            hy = conv[1][0] * pos3d[j][0] + conv[1][1] * pos3d[j][1] + conv[1][3];
            h  = conv[2][0] * pos3d[j][0] + conv[2][1] * pos3d[j][1] + 1.0f;
            if( h == 0.0 ) {
                free(J_U_H);
                free(dU);
                return err;
            }
            hh = h*h;
            ux = hx / h;
            uy = hy / h;
            dx = pos2d[j][0] - ux;
            dy = pos2d[j][1] - uy;
            err1 += dx*dx + dy*dy;
            dU[j*2+0] = dx;
            dU[j*2+1] = dy;
            J_U_H[16*j+ 0] = pos3d[j][0]/h;
            J_U_H[16*j+ 1] = pos3d[j][1]/h;
            J_U_H[16*j+ 2] = 1.0f/h;
            J_U_H[16*j+ 3] = 0.0f;
            J_U_H[16*j+ 4] = 0.0f;
            J_U_H[16*j+ 5] = 0.0f;
            J_U_H[16*j+ 6] = -pos3d[j][0]*hx/hh;
            J_U_H[16*j+ 7] = -pos3d[j][1]*hx/hh;
            J_U_H[16*j+ 8] = 0.0f;
            J_U_H[16*j+ 9] = 0.0f;
            J_U_H[16*j+10] = 0.0f;
            J_U_H[16*j+11] = pos3d[j][0]/h;
            J_U_H[16*j+12] = pos3d[j][1]/h;
            J_U_H[16*j+13] = 1.0f/h;
            J_U_H[16*j+14] = -pos3d[j][0]*hy/hh;
            J_U_H[16*j+15] = -pos3d[j][1]*hy/hh;
        }
        err1 /= num;
        //ARLOGd("Loop[%d]: err = %15.10f\n", i, err1);
        if( err1 < ICP_BREAK_LOOP_ERROR_THRESH ) break;
        if( i > 0 && err1 < ICP_BREAK_LOOP_ERROR_THRESH2 && err1/err0 > ICP_BREAK_LOOP_ERROR_RATIO_THRESH ) break;
        if( i == ICP_MAX_LOOP ) break;
        err0 = err1;

        if( getDeltaS( dH, dU, (float  (*)[8])J_U_H, num*2 ) < 0 ) {
            free(J_U_H);
            free(dU);
            return err;
        }
        //for(j=0;j<8;j++) ARLOGd("%f\t", dH[j]); ARLOGd("\n");
        conv[0][0] += dH[0];
        conv[0][1] += dH[1];
        conv[0][3] += dH[2];
        conv[1][0] += dH[3];
        conv[1][1] += dH[4];
        conv[1][3] += dH[5];
        conv[2][0] += dH[6];
        conv[2][1] += dH[7];
    }

    //ARLOGd("*********** %f\n", err1);
    //ARLOGd("Loop = %d\n", i);

    free(J_U_H);
    free(dU);

    return err1;
}

#define     K2_FACTOR     4.0F

static int compE( const void *a, const void *b )
{
    float   c;
    c = *(float  *)a - *(float  *)b;
    if( c < 0.0F ) return -1;
    if( c > 0.0F ) return  1;
    return 0;
}

static float  ar2GetTransMatHomographyRobust  ( float  initConv[3][4], float  pos2d[][2], float  pos3d[][3], int num, float  conv[3][4], float inlierProb )
{
    float         err = 100000000.0F;
    float        *J_U_H;
    float        *dU;
    float        *E, *E2, K2, W;
    float         hx, hy, h, hh, ux, uy, dx, dy;
    float         dH[8];
    float         err0, err1;
    int           inlierNum;
    int           i, j, k;

    if( num < 4 ) return err;
    if( initConv[2][3] == 0.0F ) return err;

    inlierNum = (int)(num * inlierProb) - 1;
    if( inlierNum < 4 ) inlierNum = 4;

    if( (J_U_H = (float  *)malloc( sizeof(float)*16*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        return -1;
    }
    if( (dU = (float  *)malloc( sizeof(float)*2*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_H);
        return -1;
    }
    if( (E = (float  *)malloc( sizeof(float)*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_H);
        free(dU);
        return -1;
    }
    if( (E2 = (float  *)malloc( sizeof(float)*num )) == NULL ) {
        ARLOGe("Error: malloc\n");
        free(J_U_H);
        free(dU);
        free(E);
        return -1;
    }

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) conv[j][i] = initConv[j][i]/ initConv[2][3];
    }

    for( i = 0;; i++ ) {
        for( j = 0; j < num; j++ ) {
            hx = conv[0][0] * pos3d[j][0] + conv[0][1] * pos3d[j][1] + conv[0][3];
            hy = conv[1][0] * pos3d[j][0] + conv[1][1] * pos3d[j][1] + conv[1][3];
            h  = conv[2][0] * pos3d[j][0] + conv[2][1] * pos3d[j][1] + 1.0F;
            if( h == 0.0f ) {
                free(J_U_H);
                free(dU);
                free(E);
                free(E2);
                return err;
            }
            hh = h*h;
            ux = hx / h;
            uy = hy / h;
            dx = pos2d[j][0] - ux;
            dy = pos2d[j][1] - uy;
            dU[j*2+0] = dx;
            dU[j*2+1] = dy;
            E[j] = E2[j] = dx*dx + dy*dy;

            J_U_H[16*j+ 0] = pos3d[j][0]/h;
            J_U_H[16*j+ 1] = pos3d[j][1]/h;
            J_U_H[16*j+ 2] = 1.0f/h;
            J_U_H[16*j+ 3] = 0.0f;
            J_U_H[16*j+ 4] = 0.0f;
            J_U_H[16*j+ 5] = 0.0f;
            J_U_H[16*j+ 6] = -pos3d[j][0]*hx/hh;
            J_U_H[16*j+ 7] = -pos3d[j][1]*hx/hh;
            J_U_H[16*j+ 8] = 0.0f;
            J_U_H[16*j+ 9] = 0.0f;
            J_U_H[16*j+10] = 0.0f;
            J_U_H[16*j+11] = pos3d[j][0]/h;
            J_U_H[16*j+12] = pos3d[j][1]/h;
            J_U_H[16*j+13] = 1.0f/h;
            J_U_H[16*j+14] = -pos3d[j][0]*hy/hh;
            J_U_H[16*j+15] = -pos3d[j][1]*hy/hh;
        }
        qsort(E2, num, sizeof(float), compE);
        K2 = E2[inlierNum] * K2_FACTOR;
        if( K2 < 16.0F ) K2 = 16.0F;

        err1 = 0.0F;
        for( j = 0; j < num; j++ ) {
            if( E2[j] > K2 ) err1 += K2/6.0F;
            else err1 += K2/6.0F * (1.0F - (1.0F-E2[j]/K2)*(1.0F-E2[j]/K2)*(1.0F-E2[j]/K2));
        }
        err1 /= num;
        //ARLOGd("Loop[%d]: err = %15.10f\n", i, err1);
        if( err1 < ICP_BREAK_LOOP_ERROR_THRESH ) break;
        if( i > 0 && err1 < ICP_BREAK_LOOP_ERROR_THRESH2 && err1/err0 > ICP_BREAK_LOOP_ERROR_RATIO_THRESH ) break;
        if( i == ICP_MAX_LOOP ) break;
        err0 = err1;

        k = 0;
        for( j = 0; j < num; j++ ) {
            if( E[j] <= K2 ) {
                W = (1.0F - E[j]/K2)*(1.0F - E[j]/K2);
                J_U_H[k*8+ 0] = W * J_U_H[16*j+0];
                J_U_H[k*8+ 1] = W * J_U_H[16*j+1];
                J_U_H[k*8+ 2] = W * J_U_H[16*j+2];
                J_U_H[k*8+ 3] = W * J_U_H[16*j+3];
                J_U_H[k*8+ 4] = W * J_U_H[16*j+4];
                J_U_H[k*8+ 5] = W * J_U_H[16*j+5];
                J_U_H[k*8+ 6] = W * J_U_H[16*j+6];
                J_U_H[k*8+ 7] = W * J_U_H[16*j+7];
                J_U_H[k*8+ 8] = W * J_U_H[16*j+8];
                J_U_H[k*8+ 9] = W * J_U_H[16*j+9];
                J_U_H[k*8+10] = W * J_U_H[16*j+10];
                J_U_H[k*8+11] = W * J_U_H[16*j+11];
                J_U_H[k*8+12] = W * J_U_H[16*j+12];
                J_U_H[k*8+13] = W * J_U_H[16*j+13];
                J_U_H[k*8+14] = W * J_U_H[16*j+14];
                J_U_H[k*8+15] = W * J_U_H[16*j+15];
                dU[k+0] = W * dU[j*2+0];
                dU[k+1] = W * dU[j*2+1];
                k+=2;
            }
        }
        if( k < 6 ) {
            free(J_U_H);
            free(dU);
            free(E);
            free(E2);
            return -1;
        }

        if( getDeltaS( dH, dU, (float (*)[8])J_U_H, k ) < 0 ) {
            free(J_U_H);
            free(dU);
            free(E);
            free(E2);
            return err;
        }
        //for(j=0;j<8;j++) ARLOGd("%f\t", dH[j]); ARLOGd("\n");
        conv[0][0] += dH[0];
        conv[0][1] += dH[1];
        conv[0][3] += dH[2];
        conv[1][0] += dH[3];
        conv[1][1] += dH[4];
        conv[1][3] += dH[5];
        conv[2][0] += dH[6];
        conv[2][1] += dH[7];
    }

    //ARLOGd("*********** %f\n", err1);
    //ARLOGd("Loop = %d\n", i);

    free(J_U_H);
    free(dU);
    free(E);
    free(E2);

    return err1;
}

static int getDeltaS( float  H[8], float  dU[], float  J_U_H[][8], int n )
{
    ARMatf  matH, matU, matJ;
    ARMatf *matJt, *matJtJ, *matJtU;
    int ret = 0;

    matH.row = 8;
    matH.clm = 1;
    matH.m   = H;

    matU.row = n;
    matU.clm = 1;
    matU.m   = dU;

    matJ.row = n;
    matJ.clm = 8;
    matJ.m   = &J_U_H[0][0];

    matJt = arMatrixAllocTransf( &matJ );
    if( matJt == NULL ) {
        ret = -1;
        goto bail;
    }
    matJtJ = arMatrixAllocMulf( matJt, &matJ );
    if( matJtJ == NULL ) {
        ret = -1;
        goto bail1;
    }
    matJtU = arMatrixAllocMulf( matJt, &matU );
    if( matJtU == NULL ) {
        ret = -1;
        goto bail2;
    }
    if( arMatrixSelfInvf(matJtJ) < 0 ) {
        ret = -1;
        goto bail3;
    }

    arMatrixMulf( &matH, matJtJ, matJtU );
bail3:
    arMatrixFreef( matJtU );
bail2:
    arMatrixFreef( matJtJ );
bail1:
    arMatrixFreef( matJt );
bail:
    return (ret);
}
