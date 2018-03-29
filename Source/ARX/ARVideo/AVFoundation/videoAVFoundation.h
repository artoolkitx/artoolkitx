/*
 *	videoAVFoundation.h
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
 *  Copyright 2008-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 *	Rev		Date		Who		Changes
 *	1.0.0	2008-05-04	PRL		Written.
 *
 */

#ifndef AR_VIDEO_AVFOUNDATION_H
#define AR_VIDEO_AVFOUNDATION_H

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamAVFoundationT AR2VideoParamAVFoundationT;


int                    ar2VideoDispOptionAVFoundation     ( void );
AR2VideoParamAVFoundationT *ar2VideoOpenAVFoundation      ( const char *config );
int                    ar2VideoCloseAVFoundation          ( AR2VideoParamAVFoundationT *vid );
int                    ar2VideoGetIdAVFoundation          ( AR2VideoParamAVFoundationT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeAVFoundation        ( AR2VideoParamAVFoundationT *vid, int *x,int *y );
int                    ar2VideoGetPixelFormatAVFoundation ( AR2VideoParamAVFoundationT *vid );
AR2VideoBufferT       *ar2VideoGetImageAVFoundation       ( AR2VideoParamAVFoundationT *vid );
int                    ar2VideoCapStartAVFoundation       ( AR2VideoParamAVFoundationT *vid );
int                    ar2VideoCapStopAVFoundation        ( AR2VideoParamAVFoundationT *vid );

int                    ar2VideoGetParamiAVFoundation      ( AR2VideoParamAVFoundationT *vid, int paramName, int *value );
int                    ar2VideoSetParamiAVFoundation      ( AR2VideoParamAVFoundationT *vid, int paramName, int  value );
int                    ar2VideoGetParamdAVFoundation      ( AR2VideoParamAVFoundationT *vid, int paramName, double *value );
int                    ar2VideoSetParamdAVFoundation      ( AR2VideoParamAVFoundationT *vid, int paramName, double  value );
int                    ar2VideoGetParamsAVFoundation      ( AR2VideoParamAVFoundationT *vid, const int paramName, char **value );
int                    ar2VideoSetParamsAVFoundation      ( AR2VideoParamAVFoundationT *vid, const int paramName, const char  *value );

int ar2VideoCapStartAsyncAVFoundation(AR2VideoParamAVFoundationT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata);
int ar2VideoSetBufferSizeAVFoundation(AR2VideoParamAVFoundationT *vid, const int width, const int height);
int ar2VideoGetBufferSizeAVFoundation(AR2VideoParamAVFoundationT *vid, int *width, int *height);

int                    ar2VideoGetCParamAVFoundation      (AR2VideoParamAVFoundationT *vid, ARParam *cparam);
AR2VideoParamAVFoundationT  *ar2VideoOpenAsyncAVFoundation      (const char *config, void (*callback)(void *), void *userdata);
#if USE_CPARAM_SEARCH
int                    ar2VideoGetCParamAsyncAVFoundation (AR2VideoParamAVFoundationT *vid, void (*callback)(const ARParam *, void *), void *userdata);
#endif // USE_CPARAM_SEARCH
ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListAVFoundation(const char *config_in);

// videoAVFoundation uses underlying native classes (CameraVideo, MovieVideo).
// This function retrieves a pointer, allowing direct access to the underlying native
// object instance.
#ifdef __OBJC__
id ar2VideoGetNativeVideoInstanceAVFoundation(AR2VideoParamAVFoundationT *vid); 
#endif

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_AVFOUNDATION_H
