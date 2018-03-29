/*
 *  arg_gles2.h
 *  artoolkitX
 *
 *	Graphics Subroutines (OpenGL ES 2.x) for artoolkitX.
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "arg_private.h"

#if HAVE_GLES2

#ifndef __arg_gles2_h__
#define __arg_gles2_h__

#ifdef __cplusplus
extern "C" {
#endif

int arglSetupForCurrentContextGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT pixelFormat);
void arglCleanupGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings);
void arglDispImageGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int32_t viewport[4]);
void arglDispImageStatefulGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings);
int arglDistortionCompensationSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable);
int arglSetPixelZoomGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom);
int arglPixelFormatSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format);
int arglPixelFormatGetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size);
void arglSetRotate90GLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90);
void arglSetFlipHGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH);
void arglSetFlipVGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV);
int arglPixelBufferSizeSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight);
int arglPixelBufferSizeGetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight);
int arglPixelBufferDataUploadBiPlanarGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1);
int arglGLHasExtensionGLES2(const unsigned char *extName);


#ifdef __cplusplus
}
#endif

#endif // !__arg_gles2_h__

#endif // HAVE_GLES2
