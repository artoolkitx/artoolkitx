/*
 *  kpmHandle.cpp
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
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <stdio.h>
#include <ARX/AR/ar.h>
#include <ARX/KPM/kpm.h>
#include "kpmPrivate.h"
#if !BINARY_FEATURE
#include "AnnMatch.h"
#include "AnnMatch2.h"
#endif

static KpmHandle *kpmCreateHandleCore(ARParamLT *cparamLT, int xsize, int ysize, int poseMode);

KpmHandle *kpmCreateHandle(ARParamLT *cparamLT)
{
    return kpmCreateHandleCore(cparamLT, cparamLT->param.xsize, cparamLT->param.ysize, KpmPose6DOF);
}

KpmHandle *kpmCreateHandleHomography(int xsize, int ysize)
{
    return kpmCreateHandleCore(NULL, xsize, ysize, KpmPoseHomography);
}

KpmHandle *kpmCreateHandle2(int xsize, int ysize)
{
    return kpmCreateHandleCore(NULL, xsize, ysize, KpmPoseHomography);
}

static KpmHandle *kpmCreateHandleCore( ARParamLT *cparamLT, int xsize, int ysize, int poseMode)
{
    KpmHandle       *kpmHandle;
#if !BINARY_FEATURE
    int surfXSize, surfYSize;
#endif
    
    arMallocClear( kpmHandle, KpmHandle, 1 );

#if BINARY_FEATURE
    kpmHandle->freakMatcher             = new vision::VisualDatabaseFacade;
#else
    kpmHandle->ann2                     = NULL;
#endif
    
    kpmHandle->cparamLT                = cparamLT;
    kpmHandle->poseMode                = poseMode;
    kpmHandle->xsize                   = xsize;
    kpmHandle->ysize                   = ysize;
    kpmHandle->procMode                = KpmDefaultProcMode;
    kpmHandle->detectedMaxFeature      = -1;
#if !BINARY_FEATURE
    kpmHandle->surfThreadNum           = -1;
#endif
    
    kpmHandle->refDataSet.refPoint     = NULL;
    kpmHandle->refDataSet.num          = 0;
    kpmHandle->refDataSet.pageInfo     = NULL;
    kpmHandle->refDataSet.pageNum      = 0;

    kpmHandle->inDataSet.coord         = NULL;
    kpmHandle->inDataSet.num           = 0;

#if !BINARY_FEATURE
    kpmHandle->preRANSAC.num           = 0;
    kpmHandle->preRANSAC.match         = NULL;
    kpmHandle->aftRANSAC.num           = 0;
    kpmHandle->aftRANSAC.match         = NULL;

    kpmHandle->skipRegion.regionMax    = 0;
    kpmHandle->skipRegion.regionNum    = 0;
    kpmHandle->skipRegion.region       = NULL;
#endif

    kpmHandle->result                  = NULL;
    kpmHandle->resultNum               = 0;

#if !BINARY_FEATURE
    switch (kpmHandle->procMode) {
        case KpmProcFullSize:     surfXSize = xsize;     surfYSize = ysize;     break;
        case KpmProcHalfSize:     surfXSize = xsize/2;   surfYSize = ysize/2;   break;
        case KpmProcQuatSize:     surfXSize = xsize/4;   surfYSize = ysize/4;   break;
        case KpmProcOneThirdSize: surfXSize = xsize/3;   surfYSize = ysize/3;   break;
        case KpmProcTwoThirdSize: surfXSize = xsize/3*2; surfYSize = ysize/3*2; break;
        default: ARLOGe("Error: Unknown kpmProcMode %d.\n", kpmHandle->procMode); goto bail; break;
    }

    kpmHandle->surfHandle = surfSubCreateHandle(surfXSize, surfYSize, AR_PIXEL_FORMAT_MONO);
    if (!kpmHandle->surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        goto bail;
    }
    
    surfSubSetMaxPointNum(kpmHandle->surfHandle, kpmHandle->detectedMaxFeature);
#endif
    
    return kpmHandle;
    
#if !BINARY_FEATURE
bail:
    free(kpmHandle);
    return (NULL);
#endif
}

int kpmHandleGetXSize(const KpmHandle *kpmHandle)
{
    if (!kpmHandle) return 0;
    return kpmHandle->xsize;
}

int kpmHandleGetYSize(const KpmHandle *kpmHandle)
{
    if (!kpmHandle) return 0;
    return kpmHandle->ysize;
}

int kpmSetProcMode( KpmHandle *kpmHandle,  KPM_PROC_MODE mode )
{
#if !BINARY_FEATURE
   int    thresh;
    int    maxPointNum;
    int surfXSize, surfYSize;
#endif
    
    if( kpmHandle == NULL ) return -1;
    
    if( kpmHandle->procMode == mode ) return 0;
    kpmHandle->procMode = mode;

#if !BINARY_FEATURE
    surfSubGetThresh( kpmHandle->surfHandle, &thresh );
    surfSubGetMaxPointNum( kpmHandle->surfHandle, &maxPointNum );
    
    surfSubDeleteHandle( &(kpmHandle->surfHandle) );

    switch (kpmHandle->procMode) {
        case KpmProcFullSize:     surfXSize = kpmHandle->xsize;     surfYSize = kpmHandle->ysize;     break;
        case KpmProcHalfSize:     surfXSize = kpmHandle->xsize/2;   surfYSize = kpmHandle->ysize/2;   break;
        case KpmProcQuatSize:     surfXSize = kpmHandle->xsize/4;   surfYSize = kpmHandle->ysize/4;   break;
        case KpmProcOneThirdSize: surfXSize = kpmHandle->xsize/3;   surfYSize = kpmHandle->ysize/3;   break;
        case KpmProcTwoThirdSize: surfXSize = kpmHandle->xsize/3*2; surfYSize = kpmHandle->ysize/3*2; break;
        default: ARLOGe("Error: Unknown kpmProcMode %d.\n", kpmHandle->procMode); goto bail; break;
    }
    kpmHandle->surfHandle = surfSubCreateHandle(surfXSize, surfYSize, AR_PIXEL_FORMAT_MONO);
    if (!kpmHandle->surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        goto bail;
    }

    surfSubSetThresh( kpmHandle->surfHandle, thresh );
    surfSubSetMaxPointNum( kpmHandle->surfHandle, maxPointNum );
#endif
    
    return 0;
    
#if !BINARY_FEATURE
bail:
    return (-1);
#endif
}

int kpmGetProcMode( KpmHandle *kpmHandle, KPM_PROC_MODE *mode )
{
    if( kpmHandle == NULL ) return -1;
    *mode = kpmHandle->procMode;
    return 0;
}

int kpmSetDetectedFeatureMax( KpmHandle *kpmHandle, int  detectedMaxFeature )
{
    kpmHandle->detectedMaxFeature = detectedMaxFeature;
#if !BINARY_FEATURE
    surfSubSetMaxPointNum(kpmHandle->surfHandle, kpmHandle->detectedMaxFeature);
#endif
    return 0;
}

int kpmGetDetectedFeatureMax( KpmHandle *kpmHandle, int *detectedMaxFeature )
{
    *detectedMaxFeature = kpmHandle->detectedMaxFeature;
    return 0;
}

int kpmSetSurfThreadNum( KpmHandle *kpmHandle, int surfThreadNum )
{
#if !BINARY_FEATURE
    kpmHandle->surfThreadNum = surfThreadNum;
    surfSubSetThreadNum(kpmHandle->surfHandle, kpmHandle->surfThreadNum);
#endif
    return 0;
}



int kpmDeleteHandle( KpmHandle **kpmHandle )
{
    if( *kpmHandle == NULL ) return -1;

#if BINARY_FEATURE
    delete (*kpmHandle)->freakMatcher;
#else
    CAnnMatch2  *ann2 = (CAnnMatch2 *)((*kpmHandle)->ann2);
    delete ann2;

    surfSubDeleteHandle( &((*kpmHandle)->surfHandle) );
    
#endif
    
    if( (*kpmHandle)->refDataSet.refPoint != NULL ) {
        free( (*kpmHandle)->refDataSet.refPoint );
    }
    if( (*kpmHandle)->refDataSet.pageInfo != NULL ) {
        free( (*kpmHandle)->refDataSet.pageInfo );
    }
#if !BINARY_FEATURE
    if( (*kpmHandle)->preRANSAC.match != NULL ) {
        free( (*kpmHandle)->preRANSAC.match );
    }
    if( (*kpmHandle)->aftRANSAC.match != NULL ) {
        free( (*kpmHandle)->aftRANSAC.match );
    }

    if( (*kpmHandle)->skipRegion.region != NULL ) {
        free( (*kpmHandle)->skipRegion.region );
    }
#endif
    if( (*kpmHandle)->result != NULL ) {
        free( (*kpmHandle)->result );
    }
    if( (*kpmHandle)->inDataSet.coord != NULL ) {
        free( (*kpmHandle)->inDataSet.coord );
    }

    free( *kpmHandle );
    *kpmHandle = NULL;

    return 0;
}
