/*
 *  AR2/handle.c
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
#include <string.h>
#include <stdlib.h>
#include <ARX/AR/icp.h>
#include <ARX/AR2/tracking.h>
#include <ARX/AR2/util.h>

static AR2HandleT *ar2CreateHandleSub( AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int threadNum );


AR2HandleT *ar2CreateHandle( ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat, int threadNum )
{
    AR2HandleT   *ar2Handle;

    ar2Handle = ar2CreateHandleSub( pixFormat, cparamLT->param.xsize, cparamLT->param.ysize, threadNum );

    ar2Handle->trackingMode      = AR2_TRACKING_6DOF;
    ar2Handle->cparamLT          = cparamLT;
    ar2Handle->icpHandle         = icpCreateHandle( cparamLT->param.mat );
    icpSetInlierProbability( ar2Handle->icpHandle, 0.0 );

    return ar2Handle;
}

AR2HandleT *ar2CreateHandleHomography( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat, int threadNum )
{
    AR2HandleT   *ar2Handle;

    ar2Handle = ar2CreateHandleSub( pixFormat, xsize, ysize, threadNum );

    ar2Handle->trackingMode      = AR2_TRACKING_HOMOGRAPHY;
    ar2Handle->cparamLT          = NULL;
    ar2Handle->icpHandle         = NULL;

    return ar2Handle;
}

static AR2HandleT *ar2CreateHandleSub( int pixFormat, int xsize, int ysize, int threadNum )
{
    AR2HandleT   *ar2Handle;
    int           i;

    arMalloc(ar2Handle, AR2HandleT, 1);
    ar2Handle->pixFormat         = pixFormat;
    ar2Handle->xsize             = xsize;
    ar2Handle->ysize             = ysize;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    ar2Handle->blurMethod        = AR2_DEFAULT_BLUR_METHOD;
    ar2Handle->blurLevel         = AR2_DEFAULT_BLUR_LEVEL;
#endif
    ar2Handle->searchSize        = AR2_DEFAULT_SEARCH_SIZE;
    ar2Handle->templateSize1     = AR2_DEFAULT_TS1;
    ar2Handle->templateSize2     = AR2_DEFAULT_TS2;
    ar2Handle->searchFeatureNum  = AR2_DEFAULT_SEARCH_FEATURE_NUM;
    if( ar2Handle->searchFeatureNum > AR2_SEARCH_FEATURE_MAX ) {
        ar2Handle->searchFeatureNum = AR2_SEARCH_FEATURE_MAX;
    }
    ar2Handle->simThresh         = AR2_DEFAULT_SIM_THRESH;
    ar2Handle->trackingThresh    = AR2_DEFAULT_TRACKING_THRESH;



    if( threadNum == AR2_TRACKING_DEFAULT_THREAD_NUM ) {
        threadNum = threadGetCPU();
    }
    if( threadNum < 1 ) {
         threadNum = 1;
    }
    if( threadNum > AR2_THREAD_MAX ) {
        threadNum = AR2_THREAD_MAX;
    }
    ar2Handle->threadNum = threadNum;
    ARLOGi("Tracking thread = %d\n", threadNum);
    for( i = 0; i < ar2Handle->threadNum; i++ ) {
        arMalloc( ar2Handle->arg[i].mfImage, ARUint8, xsize*ysize );
        ar2Handle->arg[i].templ = NULL;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        ar2Handle->arg[i].templ2 = NULL;
#endif
        ar2Handle->threadHandle[i] = threadInit(i, &(ar2Handle->arg[i]), ar2Tracking2d);
    }

    return ar2Handle;
}

int ar2DeleteHandle( AR2HandleT **ar2Handle )
{
    int     i;

    if( *ar2Handle == NULL ) return -1;

    for( i = 0; i < (*ar2Handle)->threadNum; i++ ) {
        threadWaitQuit( (*ar2Handle)->threadHandle[i] );
        threadFree( &((*ar2Handle)->threadHandle[i]) );
        if( (*ar2Handle)->arg[i].mfImage   != NULL )  free( (*ar2Handle)->arg[i].mfImage );
        if( (*ar2Handle)->arg[i].templ  != NULL ) ar2FreeTemplate( (*ar2Handle)->arg[i].templ );
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        if( (*ar2Handle)->arg[i].templ2 != NULL ) ar2FreeTemplate ( (*ar2Handle)->arg[i].templ2 );
#endif
    }

    if( (*ar2Handle)->icpHandle != NULL ) icpDeleteHandle( &((*ar2Handle)->icpHandle) );
    //if( (*ar2Handle)->cparamLT  != NULL ) arParamLTFree( (*ar2Handle)->cparamLT );
    free( *ar2Handle );
    *ar2Handle = NULL;

    return 0;
}

int ar2SetTrackingMode( AR2HandleT *ar2Handle, int  trackingMode )
{
    if( ar2Handle == NULL ) return -1;
    if( trackingMode == AR2_TRACKING_6DOF && ar2Handle->icpHandle == NULL ) return -1;
    ar2Handle->trackingMode = trackingMode;
    return 0;
}

int ar2GetTrackingMode( AR2HandleT *ar2Handle, int *trackingMode )
{
    if( ar2Handle == NULL ) return -1;
    *trackingMode = ar2Handle->trackingMode;
    return 0;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2SetBlurLevel( AR2HandleT *ar2Handle, int blurLevel )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->blurLevel = blurLevel;
    return 0;
}

int ar2GetBlurLevel( AR2HandleT *ar2Handle, int *blurLevel )
{
    if( ar2Handle == NULL ) return -1;
    *blurLevel = ar2Handle->blurLevel;
    return 0;
}

int ar2SetBlurMethod( AR2HandleT *ar2Handle, int blurMethod )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->blurMethod = blurMethod;
    return 0;
}

int ar2GetBlurMethod( AR2HandleT *ar2Handle, int *blurMethod )
{
    if( ar2Handle == NULL ) return -1;
    *blurMethod = ar2Handle->blurMethod;
    return 0;
}
#endif

int ar2SetSimThresh( AR2HandleT *ar2Handle, float  simThresh )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->simThresh = simThresh;
    return 0;
}

int ar2GetSimThresh( AR2HandleT *ar2Handle, float  *simThresh )
{
    if( ar2Handle == NULL ) return -1;
    *simThresh = ar2Handle->simThresh;
    return 0;
}

int ar2SetTrackingThresh( AR2HandleT *ar2Handle, float  trackingThresh )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->trackingThresh = trackingThresh;
    return 0;
}

int ar2GetTrackingThresh( AR2HandleT *ar2Handle, float  *trackingThresh )
{
    if( ar2Handle == NULL ) return -1;
    *trackingThresh = ar2Handle->trackingThresh;
    return 0;
}

int ar2SetSearchSize( AR2HandleT *ar2Handle, int searchSize )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->searchSize = searchSize;
    return 0;
}

int ar2GetSearchSize( AR2HandleT *ar2Handle, int *searchSize )
{
    if( ar2Handle == NULL ) return -1;
    *searchSize = ar2Handle->searchSize;
    return 0;
}

int ar2SetSearchFeatureNum( AR2HandleT *ar2Handle, int searchFeatureNum )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->searchFeatureNum = searchFeatureNum;
    if( ar2Handle->searchFeatureNum > AR2_SEARCH_FEATURE_MAX ) {
        ar2Handle->searchFeatureNum = AR2_SEARCH_FEATURE_MAX;
    }
    if( ar2Handle->searchFeatureNum < 3 ) {
        ar2Handle->searchFeatureNum = 3;
    }
    return 0;
}

int ar2GetSearchFeatureNum( AR2HandleT *ar2Handle, int *searchFeatureNum )
{
    if( ar2Handle == NULL ) return -1;
    *searchFeatureNum = ar2Handle->searchFeatureNum;
    return 0;
}

int ar2SetTemplateSize1( AR2HandleT *ar2Handle, int  templateSize1 )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->templateSize1 = templateSize1;
    return 0;
}

int ar2GetTemplateSize1( AR2HandleT *ar2Handle, int *templateSize1 )
{
    if( ar2Handle == NULL ) return -1;
    *templateSize1 = ar2Handle->templateSize1;
    return 0;
}

int ar2SetTemplateSize2( AR2HandleT *ar2Handle, int  templateSize2 )
{
    if( ar2Handle == NULL ) return -1;
    ar2Handle->templateSize2 = templateSize2;
    return 0;
}

int ar2GetTemplateSize2( AR2HandleT *ar2Handle, int *templateSize2 )
{
    if( ar2Handle == NULL ) return -1;
    *templateSize2 = ar2Handle->templateSize2;
    return 0;
}
