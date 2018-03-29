/*
 *  videoV4L2.h
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 5.4
 * Date: 2004/01/01
 *
 *******************************************************/

#ifndef AR_VIDEO_V4L2_H
#define AR_VIDEO_V4L2_H

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamV4L2T AR2VideoParamV4L2T;

int                  ar2VideoDispOptionV4L2     ( void );
AR2VideoParamV4L2T   *ar2VideoOpenV4L2          ( const char *config );
int                  ar2VideoCloseV4L2          ( AR2VideoParamV4L2T *vid );
int                  ar2VideoGetIdV4L2          ( AR2VideoParamV4L2T *vid, ARUint32 *id0, ARUint32 *id1 );
int                  ar2VideoGetSizeV4L2        ( AR2VideoParamV4L2T *vid, int *x,int *y );
AR_PIXEL_FORMAT      ar2VideoGetPixelFormatV4L2 ( AR2VideoParamV4L2T *vid );
AR2VideoBufferT     *ar2VideoGetImageV4L2       ( AR2VideoParamV4L2T *vid );
int                  ar2VideoCapStartV4L2       ( AR2VideoParamV4L2T *vid );
int                  ar2VideoCapStopV4L2        ( AR2VideoParamV4L2T *vid );

int                  ar2VideoGetParamiV4L2      ( AR2VideoParamV4L2T *vid, int paramName, int *value );
int                  ar2VideoSetParamiV4L2      ( AR2VideoParamV4L2T *vid, int paramName, int  value );
int                  ar2VideoGetParamdV4L2      ( AR2VideoParamV4L2T *vid, int paramName, double *value );
int                  ar2VideoSetParamdV4L2      ( AR2VideoParamV4L2T *vid, int paramName, double  value );
int                  ar2VideoGetParamsV4L2      ( AR2VideoParamV4L2T *vid, const int paramName, char **value );
int                  ar2VideoSetParamsV4L2      ( AR2VideoParamV4L2T *vid, const int paramName, const char  *value );

#if USE_CPARAM_SEARCH
int                  ar2VideoGetCParamAsyncV4L2 (AR2VideoParamV4L2T *vid, void (*callback)(const ARParam *, void *), void *userdata);
#endif // USE_CPARAM_SEARCH
ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListV4L2(const char *config_in);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_V4L2_H
