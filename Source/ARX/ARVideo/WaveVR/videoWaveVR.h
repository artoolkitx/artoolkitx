/*
 *	videoWaveVR.h
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
 *  Copyright 2020 Mozilla
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_WAVEVR_H
#define AR_VIDEO_WAVEVR_H

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamWaveVRT AR2VideoParamWaveVRT;

int                    ar2VideoDispOptionWaveVR     (void);
AR2VideoParamWaveVRT   *ar2VideoOpenWaveVR          (const char *config);
int                    ar2VideoCloseWaveVR          (AR2VideoParamWaveVRT *vid);
int                    ar2VideoGetIdWaveVR          (AR2VideoParamWaveVRT *vid, ARUint32 *id0, ARUint32 *id1);
int                    ar2VideoGetSizeWaveVR        (AR2VideoParamWaveVRT *vid, int *x, int *y);
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatWaveVR (AR2VideoParamWaveVRT *vid);
AR2VideoBufferT       *ar2VideoGetImageWaveVR       (AR2VideoParamWaveVRT *vid);
int                    ar2VideoCapStartWaveVR       (AR2VideoParamWaveVRT *vid);
int                    ar2VideoCapStopWaveVR        (AR2VideoParamWaveVRT *vid);

int                    ar2VideoGetParamiWaveVR      (AR2VideoParamWaveVRT *vid, int paramName, int *value);
int                    ar2VideoSetParamiWaveVR      (AR2VideoParamWaveVRT *vid, int paramName, int  value);
int                    ar2VideoGetParamdWaveVR      (AR2VideoParamWaveVRT *vid, int paramName, double *value);
int                    ar2VideoSetParamdWaveVR      (AR2VideoParamWaveVRT *vid, int paramName, double  value);
int                    ar2VideoGetParamsWaveVR      (AR2VideoParamWaveVRT *vid, const int paramName, char **value);
int                    ar2VideoSetParamsWaveVR      (AR2VideoParamWaveVRT *vid, const int paramName, const char  *value);

int                    ar2VideoGetCParamWaveVR      (AR2VideoParamWaveVRT *vid, ARParam *cparam);
int                    ar2VideoGetBufferSizeWaveVR  (AR2VideoParamWaveVRT *vid, int *x, int *y);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_WAVEVR_H
