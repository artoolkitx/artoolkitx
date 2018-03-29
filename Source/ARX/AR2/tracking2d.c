/*
 *  AR2/tracking2d.c
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
#include <math.h>
#include <ARX/AR2/config.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/template.h>
#include <ARX/AR2/searchPoint.h>
#include <ARX/AR2/tracking.h>

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static int ar2Tracking2dSub ( AR2HandleT *handle, AR2SurfaceSetT *surfaceSet, AR2TemplateCandidateT *candidate,
                              ARUint8 *dataPtr, ARUint8 *mfImage, AR2TemplateT **templ,
                              AR2Template2T **templ2, AR2Tracking2DResultT *result );
#else
static int ar2Tracking2dSub ( AR2HandleT *handle, AR2SurfaceSetT *surfaceSet, AR2TemplateCandidateT *candidate,
                              ARUint8 *dataPtr, ARUint8 *mfImage, AR2TemplateT **templ,
                              AR2Tracking2DResultT *result );
#endif

void *ar2Tracking2d( THREAD_HANDLE_T *threadHandle )
{
    AR2Tracking2DParamT  *arg;
    int                   ID;

    arg          = (AR2Tracking2DParamT *)threadGetArg(threadHandle);
    ID           = threadGetID(threadHandle);
    
    ARLOGi("Start tracking_thread #%d.\n", ID);
    for(;;) {
        if( threadStartWait(threadHandle) < 0 ) break;

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        arg->ret = ar2Tracking2dSub( arg->ar2Handle, arg->surfaceSet, arg->candidate,
                                     arg->dataPtr, arg->mfImage, &(arg->templ), &(arg->templ2), &(arg->result) );
#else
        arg->ret = ar2Tracking2dSub( arg->ar2Handle, arg->surfaceSet, arg->candidate,
                                     arg->dataPtr, arg->mfImage, &(arg->templ), &(arg->result) );
#endif
        threadEndSignal(threadHandle);
    }
    ARLOGi("End tracking_thread #%d.\n", ID);

    return NULL;
}


#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static int ar2Tracking2dSub ( AR2HandleT *handle, AR2SurfaceSetT *surfaceSet, AR2TemplateCandidateT *candidate,
                              ARUint8 *dataPtr, ARUint8 *mfImage, AR2TemplateT **templ,
                              AR2Template2T **templ2, AR2Tracking2DResultT *result )
#else
static int ar2Tracking2dSub ( AR2HandleT *handle, AR2SurfaceSetT *surfaceSet, AR2TemplateCandidateT *candidate,
                              ARUint8 *dataPtr, ARUint8 *mfImage, AR2TemplateT **templ,
                              AR2Tracking2DResultT *result )
#endif
{
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    AR2Template2T        *templ2;
#endif
    int                   snum, level, fnum;
    int                   search[3][2];
    int                   bx, by;

    snum  = candidate->snum;
    level = candidate->level;
    fnum  = candidate->num;

    if( *templ == NULL )  *templ = ar2GenTemplate( handle->templateSize1, handle->templateSize2 );
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    if( *templ2 == NULL ) *templ2 = ar2GenTemplate2( handle->templateSize1, handle->templateSize2 );
#endif

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    if( handle->blurMethod == AR2_CONSTANT_BLUR ) {
        if( ar2SetTemplateSub( handle->cparamLT,
                               (const float (*)[4])handle->wtrans1[snum],
                               surfaceSet->surface[snum].imageSet,
                             &(surfaceSet->surface[snum].featureSet->list[level]),
                               fnum,
                               handle->blurLevel,
                              *templ ) < 0 ) return -1;

        if( (*templ)->vlen * (*templ)->vlen
              < ((*templ)->xts1+(*templ)->xts2+1) * ((*templ)->yts1+(*templ)->yts2+1)
               * AR2_DEFAULT_TRACKING_SD_THRESH * AR2_DEFAULT_TRACKING_SD_THRESH ) {
            return -1;
        }
    }
    else {
        if( ar2SetTemplate2Sub( handle->cparamLT,
                                (const float (*)[4])handle->wtrans1[snum],
                                surfaceSet->surface[snum].imageSet,
                              &(surfaceSet->surface[snum].featureSet->list[level]),
                                fnum,
                                handle->blurLevel,
                               *templ2 ) < 0 ) return -1;

        if( (*templ2)->vlen[1] * (*templ2)->vlen[1]
              < ((*templ2)->xts1+(*templ2)->xts2+1) * ((*templ2)->yts1+(*templ2)->yts2+1)
               * AR2_DEFAULT_TRACKING_SD_THRESH * AR2_DEFAULT_TRACKING_SD_THRESH ) {
            return -1;
        }
    }
#else
    if( ar2SetTemplateSub( handle->cparamLT,
                           (const float (*)[4])handle->wtrans1[snum],
                           surfaceSet->surface[snum].imageSet,
                         &(surfaceSet->surface[snum].featureSet->list[level]),
                           fnum,
                          *templ ) < 0 ) return -1;

    if( (*templ)->vlen * (*templ)->vlen
          < ((*templ)->xts1 + (*templ)->xts2 + 1) * ((*templ)->yts1 + (*templ)->yts2 + 1)
           * AR2_DEFAULT_TRACKING_SD_THRESH * AR2_DEFAULT_TRACKING_SD_THRESH ) {
        return -1;
    }
#endif

    // Get the screen coordinates for up to three previous positions of this feature into search[][].
    if( surfaceSet->contNum == 1 ) {
        ar2GetSearchPoint( handle->cparamLT,
                           (const float (*)[4])handle->wtrans1[snum], NULL, NULL,
                         &(surfaceSet->surface[snum].featureSet->list[level].coord[fnum]),
                           search );
    }
    else if( surfaceSet->contNum == 2 ) {
        ar2GetSearchPoint( handle->cparamLT,
                           (const float (*)[4])handle->wtrans1[snum],
                           (const float (*)[4])handle->wtrans2[snum], NULL,
                         &(surfaceSet->surface[snum].featureSet->list[level].coord[fnum]),
                           search );
    }
    else {
        ar2GetSearchPoint( handle->cparamLT,
                           (const float (*)[4])handle->wtrans1[snum],
                           (const float (*)[4])handle->wtrans2[snum],
                           (const float (*)[4])handle->wtrans3[snum],
                         &(surfaceSet->surface[snum].featureSet->list[level].coord[fnum]),
                           search );
    }

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    if( handle->blurMethod == AR2_CONSTANT_BLUR ) {
        if( ar2GetBestMatching( dataPtr,
                                mfImage,
                                handle->xsize,
                                handle->ysize,
                                handle->pixFormat,
                               *templ,
                                handle->searchSize,
                                handle->searchSize,
                                search,
                                &bx, &by,
                              &(result->sim)) < 0 ) {
            return -1;
        }
        result->blurLevel = handle->blurLevel;
    }
    else {
        if( ar2GetBestMatching2( dataPtr,
                                 mfImage,
                                 handle->xsize,
                                 handle->ysize,
                                 handle->pixFormat,
                                *templ2,
                                 handle->searchSize,
                                 handle->searchSize,
                                 search,
                                 &bx, &by,
                               &(result->sim),
                               &(result->blurLevel)) < 0 ) {
            return -1;
        }
    }
#else
    if( ar2GetBestMatching( dataPtr,
                            mfImage,
                            handle->xsize,
                            handle->ysize,
                            handle->pixFormat,
                           *templ,
                            handle->searchSize,
                            handle->searchSize,
                            search,
                            &bx, &by,
                          &(result->sim)) < 0 ) {
        return -1;
    }
#endif

    result->pos2d[0] = (float)bx;
    result->pos2d[1] = (float)by;
    result->pos3d[0] = surfaceSet->surface[snum].trans[0][0] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].mx
                     + surfaceSet->surface[snum].trans[0][1] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].my
                     + surfaceSet->surface[snum].trans[0][3];
    result->pos3d[1] = surfaceSet->surface[snum].trans[1][0] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].mx
                     + surfaceSet->surface[snum].trans[1][1] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].my
                     + surfaceSet->surface[snum].trans[1][3];
    result->pos3d[2] = surfaceSet->surface[snum].trans[2][0] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].mx
                     + surfaceSet->surface[snum].trans[2][1] * surfaceSet->surface[snum].featureSet->list[level].coord[fnum].my
                     + surfaceSet->surface[snum].trans[2][3];

    return 0;
}
