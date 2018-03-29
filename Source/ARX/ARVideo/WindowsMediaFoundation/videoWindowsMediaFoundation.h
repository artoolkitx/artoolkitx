/*
 *  videoWindowsMediaFoundation.h
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_WIN_MF_H
#define AR_VIDEO_WIN_MF_H

#include <ARX/ARVideo/video.h>


#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamWinMFT AR2VideoParamWinMFT;

int                    ar2VideoDispOptionWinMF     (void);
ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListWinMF(const char *config);
AR2VideoParamWinMFT   *ar2VideoOpenWinMF           (const char *config);
int                    ar2VideoCloseWinMF          (AR2VideoParamWinMFT *vid);
int                    ar2VideoGetIdWinMF          (AR2VideoParamWinMFT *vid, ARUint32 *id0, ARUint32 *id1);
int                    ar2VideoGetSizeWinMF        (AR2VideoParamWinMFT *vid, int *x,int *y);
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatWinMF (AR2VideoParamWinMFT *vid);
AR2VideoBufferT       *ar2VideoGetImageWinMF       (AR2VideoParamWinMFT *vid);
int                    ar2VideoCapStartWinMF       (AR2VideoParamWinMFT *vid);
int                    ar2VideoCapStopWinMF        (AR2VideoParamWinMFT *vid);

int                    ar2VideoGetParamiWinMF      (AR2VideoParamWinMFT *vid, const int paramName, int *value);
int                    ar2VideoSetParamiWinMF      (AR2VideoParamWinMFT *vid, const int paramName, const int  value);
int                    ar2VideoGetParamdWinMF      (AR2VideoParamWinMFT *vid, const int paramName, double *value);
int                    ar2VideoSetParamdWinMF      (AR2VideoParamWinMFT *vid, const int paramName, const double  value);
int                    ar2VideoGetParamsWinMF      (AR2VideoParamWinMFT *vid, const int paramName, char **value);
int                    ar2VideoSetParamsWinMF      (AR2VideoParamWinMFT *vid, const int paramName, const char  *value);
 
#ifdef  __cplusplus
}
#endif
#endif
