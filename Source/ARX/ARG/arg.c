/*
 *  arg.c
 *  artoolkitX
 *
 *	Graphics Subroutines for artoolkitX.
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

// ============================================================================
//  Private includes.
// ============================================================================

#include <ARX/ARG/arg.h>

#if HAVE_GL3 && !HAVE_GL && !HAVE_GLES2
#  ifdef __APPLE__
#    include <OpenGL/gl3.h>
#  else
#    ifndef _WIN32
#      define GL_GLEXT_PROTOTYPES
#    endif
#    include <GL/glcorearb.h>
#  endif
#elif HAVE_GL
#  ifdef __APPLE__
#    include <OpenGL/gl.h>
#  else
#    ifdef _WIN32
#      include <windows.h>
#    endif
#    include <GL/gl.h>
#  endif
#elif HAVE_GLES2
#  ifdef __APPLE__
#    include <OpenGLES/ES2/gl.h>
#  else
#    include <GLES2/gl2.h>
#  endif
#else
#  error Require one of: HAVE_GL3, HAVE_GL, HAVE_GLES2
#endif


#include <string.h> // strncmp()
#include "arg_gl.h"
#include "arg_gl3.h"
#include "arg_gles2.h"

// ============================================================================
//  Private types and defines.
// ============================================================================


// ============================================================================
//  Private globals.
// ============================================================================
#if HAVE_GL3 && !HAVE_GL && !HAVE_GLES2
#  ifdef _WIN32
#    define ARGL_GET_PROC_ADDRESS wglGetProcAddress
static PFNGLGETINTEGERVPROC glGetIntegerv = NULL; // (PFNGLGETINTEGERVPROC)ARGL_GET_PROC_ADDRESS("glGetIntegerv");
#  endif
#endif

#pragma mark -
// ============================================================================
//  Private functions.
// ============================================================================

// Gets binary-coded decimal gl version (ie. 1.4 is 0x0140).
uint16_t arglOpenGLVersion()
{
    uint16_t version = 0;

#if HAVE_GL3 && !HAVE_GL && !HAVE_GLES2
#  ifdef _WIN32
	if (!glGetIntegerv) glGetIntegerv = (PFNGLGETINTEGERVPROC)ARGL_GET_PROC_ADDRESS("glGetIntegerv");
	if (!glGetIntegerv) {
		ARLOGe("arglOpenGLVersion error: a required OpenGL function counld not be bound.\n");
		return 0;
	}
#  endif
	// Available from OpenGL v3.0 and later.
	GLint majorVersion, minorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	version = (uint16_t)((majorVersion << 8) + (minorVersion << 4)); // Last nibble is vendorVersion.
#else
    short j, shiftVal;
    const unsigned char *strVersion = glGetString(GL_VERSION);
    if (!strVersion) return (FALSE);
#  if HAVE_GLES2
    j = 10; // Of the form "OpenGL ES 2.0" etc.
#  else
    j = 0;
#  endif
    shiftVal = 8;
    // Construct BCD version.
    while (((strVersion[j] <= '9') && (strVersion[j] >= '0')) || (strVersion[j] == '.')) { // Get only basic version info (until first non-digit or non-.)
        if ((strVersion[j] <= '9') && (strVersion[j] >= '0')) {
            version += (strVersion[j] - '0') << shiftVal;
            shiftVal -= 4;
        }
        j++;
    }
#endif
    return version;
}

#pragma mark -
// ============================================================================
//  Public functions.
// ============================================================================

ARGL_CONTEXT_SETTINGS_REF arglSetupForCurrentContext(const ARParam *cparam, AR_PIXEL_FORMAT pixelFormat)
{
    ARGL_CONTEXT_SETTINGS_REF contextSettings;
    
    // Determine rendering API.
    ARG_API api = ARG_API_None;
	uint16_t version = arglOpenGLVersion();
#if HAVE_GLES2
    if (version >= 0x0200) {
        api = ARG_API_GLES2;
        ARLOGd("arglSetupForCurrentContext: api = ARG_API_GLES2.\n");
    } else {
        ARLOGe("Error: OpenGL ES version 2.0 or later required but not available.\n");
    }
#elif HAVE_GL || HAVE_GL3
    if (version >= 0x0310) {
        api = ARG_API_GL3;
        ARLOGd("arglSetupForCurrentContext: api = ARG_API_GL3.\n");
    } else if (version >= 0x0150) {
        api = ARG_API_GL;
        ARLOGd("arglSetupForCurrentContext: api = ARG_API_GL.\n");
    } else {
        ARLOGe("Error: OpenGL version 1.5 or later required but not available.\n");
    }
#endif
    if (api == ARG_API_None) {
        return NULL;
    }
    
    contextSettings = (ARGL_CONTEXT_SETTINGS_REF)calloc(1, sizeof(ARGL_CONTEXT_SETTINGS));
    contextSettings->api = api;
    contextSettings->arParam = *cparam; // Copy it.
    contextSettings->arhandle = NULL;
    contextSettings->zoom = 1.0f;
    // Because of calloc used above, these are redundant.
    //contextSettings->rotate90 = contextSettings->flipH = contextSettings->flipV = contextSettings->disableDistortionCompensation = FALSE;
    
    int ok = FALSE;
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) ok = arglSetupForCurrentContextGL(contextSettings, pixelFormat);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) ok = arglSetupForCurrentContextGL3(contextSettings, pixelFormat);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) ok = arglSetupForCurrentContextGLES2(contextSettings, pixelFormat);
#endif
    if (!ok) {
        free(contextSettings);
        return NULL;
    }

    return (contextSettings);
}

int arglSetupDebugMode(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARHandle *arHandle)
{
    contextSettings->arhandle = arHandle;
    return (TRUE);
}

void arglCleanup(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return; // Sanity check.
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) arglCleanupGL(contextSettings);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) arglCleanupGL3(contextSettings);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) arglCleanupGLES2(contextSettings);
#endif
    
    free(contextSettings);
}

void arglDispImage(ARGL_CONTEXT_SETTINGS_REF contextSettings, int32_t viewport[4])
{
    if (!contextSettings) return;
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) arglDispImageGL(contextSettings, viewport);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) arglDispImageGL3(contextSettings, viewport);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) arglDispImageGLES2(contextSettings, viewport);
#endif
}

int arglDistortionCompensationSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable)
{
    if (!contextSettings) return (FALSE);

    contextSettings->disableDistortionCompensation = !enable;
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglDistortionCompensationSetGL(contextSettings, enable);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglDistortionCompensationSetGL3(contextSettings, enable);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglDistortionCompensationSetGLES2(contextSettings, enable);
#endif
    return (FALSE);
}

int arglDistortionCompensationGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *enable)
{
    if (!contextSettings || !enable) return (FALSE);
    *enable = !contextSettings->disableDistortionCompensation;
    return (TRUE);
}

int arglSetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    if (contextSettings->zoom == zoom) return (TRUE);
    contextSettings->zoom = zoom;
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglSetPixelZoomGL(contextSettings, zoom);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglSetPixelZoomGL3(contextSettings, zoom);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglSetPixelZoomGLES2(contextSettings, zoom);
#endif
    return (FALSE);
}

int arglGetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float *zoom)
{
    if (!contextSettings) return (FALSE);
    *zoom = contextSettings->zoom;
    return (TRUE);
}

int arglPixelFormatSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
    if (!contextSettings) return (FALSE);
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglPixelFormatSetGL(contextSettings, format);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglPixelFormatSetGL3(contextSettings, format);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglPixelFormatSetGLES2(contextSettings, format);
#endif
    return (FALSE);
}

int arglPixelFormatGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglPixelFormatGetGL(contextSettings, format, size);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglPixelFormatGetGL3(contextSettings, format, size);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglPixelFormatGetGLES2(contextSettings, format, size);
#endif
    return (FALSE);
}

void arglSetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90)
{
    if (!contextSettings) return;

    contextSettings->rotate90 = rotate90;
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) {
        arglSetRotate90GL(contextSettings, rotate90);
        return;
    }
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) {
        arglSetRotate90GL3(contextSettings, rotate90);
        return;
    }
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) {
        arglSetRotate90GLES2(contextSettings, rotate90);
        return;
    }
#endif
}

int arglGetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->rotate90);
}

void arglSetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH)
{
    if (!contextSettings) return;
    
    contextSettings->flipH = flipH;
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) {
        arglSetFlipHGL(contextSettings, flipH);
        return;
    }
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) {
        arglSetFlipHGL3(contextSettings, flipH);
        return;
    }
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) {
        arglSetFlipHGLES2(contextSettings, flipH);
        return;
    }
#endif
}

int arglGetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipH);
}

void arglSetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV)
{
    if (!contextSettings) return;

    contextSettings->flipV = flipV;
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) {
        arglSetFlipVGL(contextSettings, flipV);
        return;
    }
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) {
        arglSetFlipVGL3(contextSettings, flipV);
        return;
    }
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) {
        arglSetFlipVGLES2(contextSettings, flipV);
        return;
    }
#endif
}

int arglGetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipV);
}

int arglPixelBufferSizeSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);

#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglPixelBufferSizeSetGL(contextSettings, bufWidth, bufHeight);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglPixelBufferSizeSetGL3(contextSettings, bufWidth, bufHeight);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglPixelBufferSizeSetGLES2(contextSettings, bufWidth, bufHeight);
#endif
    return (FALSE);
}

int arglPixelBufferSizeGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);

#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglPixelBufferSizeGetGL(contextSettings, bufWidth, bufHeight);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglPixelBufferSizeGetGL3(contextSettings, bufWidth, bufHeight);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglPixelBufferSizeGetGLES2(contextSettings, bufWidth, bufHeight);
#endif
    return (FALSE);
}

int arglPixelBufferDataUploadBiPlanar(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    if (!contextSettings) return (FALSE);
    
#if HAVE_GL
    if (contextSettings->api == ARG_API_GL) return arglPixelBufferDataUploadBiPlanarGL(contextSettings, bufDataPtr0, bufDataPtr1);
#endif
#if HAVE_GL3
    if (contextSettings->api == ARG_API_GL3) return arglPixelBufferDataUploadBiPlanarGL3(contextSettings, bufDataPtr0, bufDataPtr1);
#endif
#if HAVE_GLES2
    if (contextSettings->api == ARG_API_GLES2) return arglPixelBufferDataUploadBiPlanarGLES2(contextSettings, bufDataPtr0, bufDataPtr1);
#endif
    return (FALSE);
}

int arglGLCapabilityCheck(ARGL_CONTEXT_SETTINGS_REF contextSettings, const uint16_t minVersion, const unsigned char *extension)
{
    if (minVersion > 0) {
        if (arglOpenGLVersion() >= minVersion) return (TRUE);
    }
    
    if (extension && contextSettings) {
#if HAVE_GL
        if (contextSettings->api == ARG_API_GL) if (arglGLHasExtensionGL(extension)) return (TRUE);
#endif
#if HAVE_GL3
        if (contextSettings->api == ARG_API_GL3) if (arglGLHasExtensionGL3(extension)) return (TRUE);
#endif
#if HAVE_GLES2
        if (contextSettings->api == ARG_API_GLES2) if (arglGLHasExtensionGLES2(extension)) return (TRUE);
#endif
    }
    
    return (FALSE);
}
