/*
 *  videoEmscripten.h
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
 *  Copyright 2020-2020 Philip Lamb
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_EMSCRIPTEN_H
#define AR_VIDEO_EMSCRIPTEN_H


#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamEmscriptenT AR2VideoParamEmscriptenT;

int                       ar2VideoDispOptionEmscripten     (void);
AR2VideoParamEmscriptenT *ar2VideoOpenEmscripten           (const char *config);
int                       ar2VideoCloseEmscripten          (AR2VideoParamEmscriptenT *vid);
int                       ar2VideoGetIdEmscripten          (AR2VideoParamEmscriptenT *vid, ARUint32 *id0, ARUint32 *id1);
int                       ar2VideoGetSizeEmscripten        (AR2VideoParamEmscriptenT *vid, int *x,int *y);
AR_PIXEL_FORMAT           ar2VideoGetPixelFormatEmscripten (AR2VideoParamEmscriptenT *vid);
AR2VideoBufferT          *ar2VideoGetImageEmscripten       (AR2VideoParamEmscriptenT *vid);
int                       ar2VideoCapStartEmscripten       (AR2VideoParamEmscriptenT *vid);
int                       ar2VideoCapStopEmscripten        (AR2VideoParamEmscriptenT *vid);

int                       ar2VideoGetParamiEmscripten      (AR2VideoParamEmscriptenT *vid, int paramName, int *value);
int                       ar2VideoSetParamiEmscripten      (AR2VideoParamEmscriptenT *vid, int paramName, int  value);
int                       ar2VideoGetParamdEmscripten      (AR2VideoParamEmscriptenT *vid, int paramName, double *value);
int                       ar2VideoSetParamdEmscripten      (AR2VideoParamEmscriptenT *vid, int paramName, double  value);
int                       ar2VideoGetParamsEmscripten      (AR2VideoParamEmscriptenT *vid, const int paramName, char **value);
int                       ar2VideoSetParamsEmscripten      (AR2VideoParamEmscriptenT *vid, const int paramName, const char  *value);

int                       ar2VideoSetBufferSizeEmscripten  (AR2VideoParamEmscriptenT *vid, const int width, const int height);
int                       ar2VideoGetBufferSizeEmscripten  (AR2VideoParamEmscriptenT *vid, int *width, int *height);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_EMSCRIPTEN_H
