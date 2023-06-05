/*
 *  videoExternal.h
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
 *  Copyright 2023 Philip Lamb
  *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_EXTERNAL_H
#define AR_VIDEO_EXTERNAL_H


#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamExternalT AR2VideoParamExternalT;

#define AR_VIDEO_EXTERNAL_FOCAL_LENGTH_DEFAULT 0.3 // Metres.

int                     ar2VideoDispOptionExternal     (void);
AR2VideoParamExternalT *ar2VideoOpenAsyncExternal      (const char *config, void (*callback)(void *), void *userdata);
int                     ar2VideoCloseExternal          (AR2VideoParamExternalT *vid);
int                     ar2VideoGetIdExternal          (AR2VideoParamExternalT *vid, ARUint32 *id0, ARUint32 *id1);
int                     ar2VideoGetSizeExternal        (AR2VideoParamExternalT *vid, int *x,int *y);
AR_PIXEL_FORMAT         ar2VideoGetPixelFormatExternal (AR2VideoParamExternalT *vid);
AR2VideoBufferT        *ar2VideoGetImageExternal       (AR2VideoParamExternalT *vid);
int                     ar2VideoCapStartExternal       (AR2VideoParamExternalT *vid);
int                     ar2VideoCapStopExternal        (AR2VideoParamExternalT *vid);

int                     ar2VideoGetParamiExternal      (AR2VideoParamExternalT *vid, int paramName, int *value);
int                     ar2VideoSetParamiExternal      (AR2VideoParamExternalT *vid, int paramName, int  value);
int                     ar2VideoGetParamdExternal      (AR2VideoParamExternalT *vid, int paramName, double *value);
int                     ar2VideoSetParamdExternal      (AR2VideoParamExternalT *vid, int paramName, double  value);
int                     ar2VideoGetParamsExternal      (AR2VideoParamExternalT *vid, const int paramName, char **value);
int                     ar2VideoSetParamsExternal      (AR2VideoParamExternalT *vid, const int paramName, const char  *value);

int ar2VideoPushInitExternal(AR2VideoParamExternalT *vid, int width, int height, const char *pixelFormat, int cameraIndex, int cameraPosition);
int ar2VideoPushExternal(AR2VideoParamExternalT *vid,
                         ARUint8 *buf0p, int buf0Size, int buf0PixelStride, int buf0RowStride,
                         ARUint8 *buf1p, int buf1Size, int buf1PixelStride, int buf1RowStride,
                         ARUint8 *buf2p, int buf2Size, int buf2PixelStride, int buf2RowStride,
                         ARUint8 *buf3p, int buf3Size, int buf3PixelStride, int buf3RowStride);
int ar2VideoPushFinalExternal(AR2VideoParamExternalT *vid);


#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_EXTERNAL_H
