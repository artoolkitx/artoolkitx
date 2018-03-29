/*
 *  trackingSub.c
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */


#include "trackingSub.h"

#if HAVE_NFT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    KpmHandle              *kpmHandle;      // KPM-related data.
    ARUint8                *imageLumaPtr;   // Pointer to image being tracked.
    int                     imageSize;      // Bytes per image.
    float                   trans[3][4];    // Transform containing pose of tracked image.
    int                     page;           // Assigned page number of tracked image.
    int                     flag;           // Tracked successfully.
} TrackingInitHandle;

static void *trackingInitMain( THREAD_HANDLE_T *threadHandle );


int trackingInitQuit( THREAD_HANDLE_T **threadHandle_p )
{
    TrackingInitHandle  *trackingInitHandle;

    if (!threadHandle_p)  {
        ARLOGe("trackingInitQuit(): Error: NULL threadHandle_p.\n");
        return (-1);
    }
    if (!*threadHandle_p) return 0;
    
    threadWaitQuit( *threadHandle_p );
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(*threadHandle_p);
    if (trackingInitHandle) {
        free( trackingInitHandle->imageLumaPtr );
        free( trackingInitHandle );
    }
    threadFree( threadHandle_p );
    return 0;
}

THREAD_HANDLE_T *trackingInitInit( KpmHandle *kpmHandle )
{
    TrackingInitHandle  *trackingInitHandle;
    THREAD_HANDLE_T     *threadHandle;

    if (!kpmHandle) {
        ARLOGe("trackingInitInit(): Error: NULL KpmHandle.\n");
        return (NULL);
    }
    
    trackingInitHandle = (TrackingInitHandle *)malloc(sizeof(TrackingInitHandle));
    if( trackingInitHandle == NULL ) return NULL;
    trackingInitHandle->kpmHandle = kpmHandle;
    trackingInitHandle->imageSize = kpmHandleGetXSize(kpmHandle) * kpmHandleGetYSize(kpmHandle);
    trackingInitHandle->imageLumaPtr  = (ARUint8 *)malloc(trackingInitHandle->imageSize);
    trackingInitHandle->flag      = 0;

    threadHandle = threadInit(0, trackingInitHandle, trackingInitMain);
    return threadHandle;
}

int trackingInitStart( THREAD_HANDLE_T *threadHandle, ARUint8 *imageLumaPtr )
{
    TrackingInitHandle     *trackingInitHandle;

    if (!threadHandle || !imageLumaPtr) {
        ARLOGe("trackingInitStart(): Error: NULL threadHandle or imagePtr.\n");
        return (-1);
    }
    
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!trackingInitHandle) {
        ARLOGe("trackingInitStart(): Error: NULL trackingInitHandle.\n");
        return (-1);
    }
    memcpy( trackingInitHandle->imageLumaPtr, imageLumaPtr, trackingInitHandle->imageSize );
    threadStartSignal( threadHandle );

    return 0;
}

int trackingInitGetResult( THREAD_HANDLE_T *threadHandle, float trans[3][4], int *page )
{
    TrackingInitHandle     *trackingInitHandle;
    int  i, j;

    if (!threadHandle || !trans || !page)  {
        ARLOGe("trackingInitGetResult(): Error: NULL threadHandle or trans or page.\n");
        return (-1);
    }
    
    if( threadGetStatus( threadHandle ) == 0 ) return 0;
    threadEndWait( threadHandle );
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!trackingInitHandle) return (-1);
    if( trackingInitHandle->flag ) {
        for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) trans[j][i] = trackingInitHandle->trans[j][i];
        *page = trackingInitHandle->page;
        return 1;
    }

    return -1;
}

static void *trackingInitMain( THREAD_HANDLE_T *threadHandle )
{
    TrackingInitHandle     *trackingInitHandle;
    KpmHandle              *kpmHandle;
    KpmResult              *kpmResult = NULL;
    int                     kpmResultNum;
    ARUint8                *imageLumaPtr;
    float                  err;
    int                    i, j, k;

    if (!threadHandle) {
        ARLOGe("Error starting tracking thread: empty THREAD_HANDLE_T.\n");
        return (NULL);
    }
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!threadHandle) {
        ARLOGe("Error starting tracking thread: empty trackingInitHandle.\n");
        return (NULL);
    }
    kpmHandle          = trackingInitHandle->kpmHandle;
    imageLumaPtr       = trackingInitHandle->imageLumaPtr;
    if (!kpmHandle || !imageLumaPtr) {
        ARLOGe("Error starting tracking thread: empty kpmHandle/imageLumaPtr.\n");
        return (NULL);
    }
    ARLOGi("Start tracking thread.\n");
    
    kpmGetResult( kpmHandle, &kpmResult, &kpmResultNum );

    for(;;) {
        if( threadStartWait(threadHandle) < 0 ) break;

        kpmMatching(kpmHandle, imageLumaPtr);
        trackingInitHandle->flag = 0;
        for( i = 0; i < kpmResultNum; i++ ) {
            if( kpmResult[i].camPoseF != 0 ) continue;
            ARLOGd("kpmGetPose OK.\n");
            if( trackingInitHandle->flag == 0 || err > kpmResult[i].error ) { // Take the first or best result.
                trackingInitHandle->flag = 1;
                trackingInitHandle->page = kpmResult[i].pageNo;
                for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) trackingInitHandle->trans[j][k] = kpmResult[i].camPose[j][k];
                err = kpmResult[i].error;
            }
        }

        threadEndSignal(threadHandle);
    }

    ARLOGi("End tracking thread.\n");
    return (NULL);
}

#endif // HAVE_NFT
