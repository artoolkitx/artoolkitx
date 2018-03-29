/*
 *  videoWindowsMediaCapture.h
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
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_WIN_MC_H
#define AR_VIDEO_WIN_MC_H

#include <ARX/AR/ar.h>


#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamWinMCT AR2VideoParamWinMCT;

int                    ar2VideoDispOptionWinMC     (void);
AR2VideoParamWinMCT   *ar2VideoOpenWinMC           (const char *config);
int                    ar2VideoCloseWinMC          (AR2VideoParamWinMCT *vid);
int                    ar2VideoGetIdWinMC          (AR2VideoParamWinMCT *vid, ARUint32 *id0, ARUint32 *id1);
int                    ar2VideoGetSizeWinMC        (AR2VideoParamWinMCT *vid, int *x,int *y);
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatWinMC (AR2VideoParamWinMCT *vid);
AR2VideoBufferT       *ar2VideoGetImageWinMC       (AR2VideoParamWinMCT *vid);
int                    ar2VideoCapStartWinMC       (AR2VideoParamWinMCT *vid);
int                    ar2VideoCapStopWinMC        (AR2VideoParamWinMCT *vid);

int                    ar2VideoGetParamiWinMC      (AR2VideoParamWinMCT *vid, const int paramName, int *value);
int                    ar2VideoSetParamiWinMC      (AR2VideoParamWinMCT *vid, const int paramName, const int  value);
int                    ar2VideoGetParamdWinMC      (AR2VideoParamWinMCT *vid, const int paramName, double *value);
int                    ar2VideoSetParamdWinMC      (AR2VideoParamWinMCT *vid, const int paramName, const double  value);
int                    ar2VideoGetParamsWinMC      (AR2VideoParamWinMCT *vid, const int paramName, char **value);
int                    ar2VideoSetParamsWinMC      (AR2VideoParamWinMCT *vid, const int paramName, const char  *value);
 
#ifdef  __cplusplus
}
#endif
#endif
