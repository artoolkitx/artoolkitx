/*
 *  arg_gl.c
 *  artoolkitX
 *
 *	Graphics Subroutines (OpenGL 1.x) for artoolkitX.
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
//	Private includes.
// ============================================================================

#include "arg_gl.h"

#if HAVE_GL

#include <stdio.h>		// fprintf(), stderr
#include <string.h>		// strchr(), strstr(), strlen()
#ifdef _WIN32
#  include <windows.h>
#endif
#ifndef __APPLE__
#  include <GL/gl.h>
#  ifdef _WIN32
#    include "GL/glext.h"
#    include "GL/wglext.h"
#  else
#    ifdef GL_VERSION_1_2
#      include <GL/glext.h>
#    endif
#    ifdef __linux
#      include <GL/glx.h>
#    endif
#  endif
#else
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#endif


// ============================================================================
//	Private types and defines.
// ============================================================================
#ifdef _MSC_VER
#  pragma warning (disable:4068)	// Disable MSVC warnings about unknown pragmas.
#endif

#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 1 // Edit as required.
#endif

// Make sure that required OpenGL constant definitions are available at compile-time.
// N.B. These should not be used unless the renderer indicates (at run-time) that it supports them.

// Define constants for extensions which became core in OpenGL 1.2
#ifndef GL_VERSION_1_2
#  if GL_EXT_bgra
#    define GL_BGR							GL_BGR_EXT
#    define GL_BGRA							GL_BGRA_EXT
#  else
#    define GL_BGR							0x80E0
#    define GL_BGRA							0x80E1
#  endif
#  ifndef GL_APPLE_packed_pixels
#    define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#    define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#    define GL_UNSIGNED_INT_8_8_8_8         0x8035
#    define GL_UNSIGNED_SHORT_5_6_5         0x8363
#    define GL_UNSIGNED_SHORT_5_6_5_REV     0x8364
#    define GL_UNSIGNED_SHORT_4_4_4_4_REV   0x8365
#    define GL_UNSIGNED_SHORT_1_5_5_5_REV   0x8366
#    define GL_UNSIGNED_INT_8_8_8_8_REV     0x8367
#  endif
#  if GL_SGIS_texture_edge_clamp
#    define GL_CLAMP_TO_EDGE				GL_CLAMP_TO_EDGE_SGIS
#  else
#    define GL_CLAMP_TO_EDGE				0x812F
#  endif
#endif

// Define constants for extensions which became core in OpenGL 3.1
#ifndef GL_VERSION_3_1
#  if GL_NV_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_NV
#    define GL_PROXY_TEXTURE_RECTANGLE		GL_PROXY_TEXTURE_RECTANGLE_NV
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_NV
#  elif GL_EXT_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_EXT
#    define GL_PROXY_TEXTURE_RECTANGLE      GL_PROXY_TEXTURE_RECTANGLE_EXT
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT
#  else
#    define GL_TEXTURE_RECTANGLE            0x84F5
#    define GL_PROXY_TEXTURE_RECTANGLE      0x84F7
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   0x84F8
#  endif
#endif

// Define constants for extensions (not yet core).
#ifndef GL_APPLE_ycbcr_422
#  define GL_YCBCR_422_APPLE				0x85B9
#  define GL_UNSIGNED_SHORT_8_8_APPLE		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE	0x85BB
#endif
#ifndef GL_EXT_abgr
#  define GL_ABGR_EXT						0x8000
#endif
#ifndef GL_MESA_ycbcr_texture
#  define GL_YCBCR_MESA						0x8757
#  define GL_UNSIGNED_SHORT_8_8_MESA		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_MESA	0x85BB
#endif

// On Windows, all OpenGL v1.3 and later API must be dynamically resolved against the actual driver. For Linux, the same applies to OpenGL v1.4 and later.
#if defined(_WIN32) || defined(__linux)
#  ifdef _WIN32
#    define ARGL_GET_PROC_ADDRESS wglGetProcAddress
#  else
#    define ARGL_GET_PROC_ADDRESS glXGetProcAddress
#  endif
static PFNGLGENBUFFERSPROC glGenBuffers = NULL; // (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffersARB");
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
static PFNGLBUFFERDATAPROC glBufferData = NULL;
#  ifdef _WIN32
static PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
static PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture = NULL;
#  endif
#endif

//#define ARGL_DEBUG

struct _ARGL_CONTEXT_SETTINGS_GL {
    GLuint  texture;
    float   *t2;
    float   *v2;
    GLuint  t2bo;     // Vertex buffer object for t2 data.
    GLuint  v2bo;     // Vertex buffer object for v2 data.
    GLint   textureSizeMax;
    GLsizei textureSizeX;
    GLsizei textureSizeY;
    GLenum  pixIntFormat;
    GLenum  pixFormat;
    GLenum  pixType;
    GLenum  pixSize;
    AR_PIXEL_FORMAT format;
    int     textureGeometryHasBeenSetup;
    int     textureObjectsHaveBeenSetup;
    GLsizei bufSizeX;
    GLsizei bufSizeY;
    int     bufSizeIsTextureSize;
    int     textureDataReady;
    int	    arglTexmapMode;
};
typedef struct _ARGL_CONTEXT_SETTINGS_GL ARGL_CONTEXT_SETTINGS_GL;
typedef struct _ARGL_CONTEXT_SETTINGS_GL *ARGL_CONTEXT_SETTINGS_GL_REF;



// ============================================================================
//	Private globals.
// ============================================================================


#pragma mark -
// ============================================================================
//	Private functions.
// ============================================================================

// Sets texture, t2, v2, textureGeometryHasBeenSetup.
static char arglSetupTextureGeometryGL(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    float    ty_prev, tx, ty;
    float    y_prev, x, y;
    ARdouble x1, x2, y1, y2;
    float    xx1, xx2, yy1, yy2;
    int      i, j;
    int      vertexCount, t2count, v2count;
    float    imageSizeX, imageSizeY;
    float    zoom;
    
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;

    // Delete previous geometry, unless this is our first time here.
    if (((ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings)->textureGeometryHasBeenSetup) {
        free(acs->t2);
        free(acs->v2);
        glDeleteBuffers(1, &acs->t2bo);
        glDeleteBuffers(1, &acs->v2bo);
        acs->textureGeometryHasBeenSetup = FALSE;
    }
    
    // Set up the geometry for the surface which we will texture upon.
    imageSizeX = (float)contextSettings->arParam.xsize;
    imageSizeY = (float)contextSettings->arParam.ysize;
    zoom = contextSettings->zoom;
    if (contextSettings->disableDistortionCompensation) vertexCount = 4;
    else vertexCount = 840; // 20 rows of 2 x 21 vertices.
    acs->t2 = (float *)malloc(sizeof(float) * 2 * vertexCount);
    acs->v2 = (float *)malloc(sizeof(float) * 2 * vertexCount);
    t2count = v2count = 0;
    if (contextSettings->disableDistortionCompensation) {
        acs->t2[t2count++] = 0.0f; // Top-left.
        acs->t2[t2count++] = 0.0f;
        acs->v2[v2count++] = 0.0f;
        acs->v2[v2count++] = imageSizeY;
        acs->t2[t2count++] = 0.0f; // Bottom-left.
        acs->t2[t2count++] = imageSizeY/(float)acs->textureSizeY/zoom;
        acs->v2[v2count++] = 0.0f;
        acs->v2[v2count++] = 0.0f;
        acs->t2[t2count++] = imageSizeX/(float)acs->textureSizeX/zoom; // Top-right.
        acs->t2[t2count++] = 0.0f;
        acs->v2[v2count++] = imageSizeX;
        acs->v2[v2count++] = imageSizeY;
        acs->t2[t2count++] = imageSizeX/(float)acs->textureSizeX/zoom; // Bottom-right.
        acs->t2[t2count++] = imageSizeY/(float)acs->textureSizeY/zoom;
        acs->v2[v2count++] = imageSizeX;
        acs->v2[v2count++] = 0.0f;
    } else {
        y = 0.0f;
        ty = 0.0f;
        for (j = 1; j <= 20; j++) {    // Do 20 rows of triangle strips.
            y_prev = y;
            ty_prev = ty;
            y = imageSizeY * (float)j / 20.0f;
            ty = y / (float)acs->textureSizeY/zoom;
            
            
            for (i = 0; i <= 20; i++) { // 21 columns of triangle strip vertices, 2 vertices per column.
                x = imageSizeX * (float)i / 20.0f;
                tx = x / (float)acs->textureSizeX/zoom;
                
                arParamObserv2Ideal(contextSettings->arParam.dist_factor, (ARdouble)x, (ARdouble)y_prev, &x1, &y1, contextSettings->arParam.dist_function_version);
                arParamObserv2Ideal(contextSettings->arParam.dist_factor, (ARdouble)x, (ARdouble)y,      &x2, &y2, contextSettings->arParam.dist_function_version);
                
                xx1 = (float)x1;
                yy1 = (imageSizeY - (float)y1);
                xx2 = (float)x2;
                yy2 = (imageSizeY - (float)y2);
                
                acs->t2[t2count++] = tx; // Top.
                acs->t2[t2count++] = ty_prev;
                acs->v2[v2count++] = xx1;
                acs->v2[v2count++] = yy1;
                acs->t2[t2count++] = tx; // Bottom.
                acs->t2[t2count++] = ty;
                acs->v2[v2count++] = xx2;
                acs->v2[v2count++] = yy2;
            } // columns.
        } // rows.
    }
    
    // Now setup VBOs.
    glGenBuffers(1, &acs->t2bo);
    glGenBuffers(1, &acs->v2bo);
    glBindBuffer(GL_ARRAY_BUFFER, acs->t2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, acs->t2, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, acs->v2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, acs->v2, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    acs->textureGeometryHasBeenSetup = TRUE;
    return (TRUE);
}


// Set up the texture objects.
static char arglSetupTextureObjectsGL(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    
    // Delete previous textures, unless this is our first time here.
    if (acs->textureObjectsHaveBeenSetup) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(acs->texture));
        acs->textureObjectsHaveBeenSetup = FALSE;
    }
    
    glGenTextures(1, &(acs->texture));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, acs->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    acs->textureObjectsHaveBeenSetup = TRUE;
    return (TRUE);
}


#pragma mark -
// ============================================================================
//	Public functions.
// ============================================================================

int arglSetupForCurrentContextGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT pixelFormat)
{
	// OpenGL 1.5 required.
	if (!(arglOpenGLVersion() >= 0x0150)) {
		ARLOGe("Error: OpenGL v1.5 or later is required, but not found. Renderer reported '%s'.\n", glGetString(GL_VERSION));
		return (FALSE);
	}
#if defined(_WIN32) || defined(__linux)
	if (!glGenBuffers) glGenBuffers = (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffers");
	if (!glDeleteBuffers) glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteBuffers");
	if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindBuffer");
	if (!glBufferData) glBufferData = (PFNGLBUFFERDATAPROC)ARGL_GET_PROC_ADDRESS("glBufferData");
#  ifdef _WIN32
	if (!glActiveTexture) glActiveTexture = (PFNGLACTIVETEXTUREPROC)ARGL_GET_PROC_ADDRESS("glActiveTexture");
	if (!glClientActiveTexture) glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)ARGL_GET_PROC_ADDRESS("glClientActiveTexture");
#  endif
	if (!glGenBuffers || !glDeleteBuffers || !glBindBuffer || !glBufferData
#  ifdef _WIN32
        || !glActiveTexture || !glClientActiveTexture
#  endif
        ) {
		ARLOGe("arglSetupForCurrentContextGL error: a required OpenGL function counld not be bound.\n");
		return (FALSE);
	}
#endif

    contextSettings->apiContextSettings = calloc(1, sizeof(ARGL_CONTEXT_SETTINGS_GL));
    // Because of calloc used above, these are redundant.
    //contextSettings->apiContextSettings->textureGeometryHasBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureObjectsHaveBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureDataReady = FALSE;
    
    // This sets pixIntFormat, pixFormat, pixType, pixSize, and resets textureDataReady.
    ((ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings)->format = AR_PIXEL_FORMAT_INVALID;
    if (!arglPixelFormatSet(contextSettings, pixelFormat)) {
        ARLOGe("ARG: Error setting pixel format.\n");
        arglCleanupGL(contextSettings);
        return (FALSE);
    }
    
    // Set pixel buffer sizes to incoming image size, by default.
    if (!arglPixelBufferSizeSet(contextSettings, contextSettings->arParam.xsize, contextSettings->arParam.ysize)) {
        ARLOGe("ARG: Error setting pixel buffer size.\n");
        arglCleanupGL(contextSettings);
        return (FALSE);
    }
    
    return (TRUE);
}

void arglCleanupGL(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    if (acs->textureObjectsHaveBeenSetup) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(acs->texture));
    }
    
    if (acs->textureGeometryHasBeenSetup) {
        free(acs->t2);
        free(acs->v2);
        glDeleteBuffers(1, &acs->t2bo);
        glDeleteBuffers(1, &acs->v2bo);
    }
    
    free(acs);
    acs = NULL;
}

void arglDispImageGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int32_t viewport[4])
{
    GLdouble left, right, bottom, top;
    GLboolean lightingSave;
    GLboolean depthTestSave;
    
    if (!contextSettings) return;
    
    if (viewport) glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    // Prepare an orthographic projection, set camera position for 2D drawing, and save GL state.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    if (contextSettings->rotate90) glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
    
    if (contextSettings->flipV) {
        bottom = (GLdouble)contextSettings->arParam.ysize;
        top = 0.0;
    } else {
        bottom = 0.0;
        top = (GLdouble)contextSettings->arParam.ysize;
    }
    if (contextSettings->flipH) {
        left = (GLdouble)contextSettings->arParam.xsize;
        right = 0.0;
    } else {
        left = 0.0;
        right = (GLdouble)contextSettings->arParam.xsize;
    }
    glOrtho(left, right, bottom, top, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    lightingSave = glIsEnabled(GL_LIGHTING);			// Save enabled state of lighting.
    if (lightingSave == GL_TRUE) glDisable(GL_LIGHTING);
    depthTestSave = glIsEnabled(GL_DEPTH_TEST);		// Save enabled state of depth test.
    if (depthTestSave == GL_TRUE) glDisable(GL_DEPTH_TEST);
    
    arglDispImageStatefulGL(contextSettings);
    
    if (depthTestSave == GL_TRUE) glEnable(GL_DEPTH_TEST);			// Restore enabled state of depth test.
    if (lightingSave == GL_TRUE) glEnable(GL_LIGHTING);			// Restore enabled state of lighting.
   
    // Restore previous projection & camera position.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
#ifdef ARGL_DEBUG
    // Report any errors we generated.
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ARLOGe("ARG: GL error 0x%04X\n", (int)err);
    }
#endif // ARGL_DEBUG
    
}

void arglDispImageStatefulGL(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLint texEnvModeSave;
    int        i;
    
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    
    if (!acs->textureObjectsHaveBeenSetup) return;
    if (!acs->textureGeometryHasBeenSetup) return;
    if (!acs->textureDataReady) return;
    
    glActiveTexture(GL_TEXTURE0);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    
    glBindTexture(GL_TEXTURE_2D, acs->texture);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texEnvModeSave); // Save GL texture environment mode.
    if (texEnvModeSave != GL_REPLACE) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    
    glClientActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, acs->t2bo);
    glTexCoordPointer(2, GL_FLOAT, 0, NULL);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, acs->v2bo);
    glVertexPointer(2, GL_FLOAT, 0, NULL);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDisable(GL_TEXTURE_2D);
    if (texEnvModeSave != GL_REPLACE) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnvModeSave); // Restore GL texture environment mode.
}

int arglDistortionCompensationSetGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable)
{
    if (!contextSettings) return (FALSE);
    return (arglSetupTextureGeometryGL(contextSettings));
}

int arglSetPixelZoomGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    // Changing the zoom invalidates the geometry, so set it up.
    return (arglSetupTextureGeometryGL(contextSettings));
}

int arglPixelFormatSetGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);

    if (acs->format == format) return (TRUE);
    switch (format) {
		case AR_PIXEL_FORMAT_RGBA:
			acs->pixIntFormat = GL_RGBA;
			acs->pixFormat = GL_RGBA;
			acs->pixType = GL_UNSIGNED_BYTE;
			acs->pixSize = 4;
			break;
		case AR_PIXEL_FORMAT_ABGR:	// SGI.
			if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_EXT_abgr")) {
				acs->pixIntFormat = GL_RGBA;
				acs->pixFormat = GL_ABGR_EXT;
				acs->pixType = GL_UNSIGNED_BYTE;
				acs->pixSize = 4;
			} else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_ABGR, but GL_EXT_abgr is not available.\n");
				return (FALSE);
			}
			break;
		case AR_PIXEL_FORMAT_BGRA:	// Windows.
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_BGRA;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 4;
			break;
		case AR_PIXEL_FORMAT_ARGB:	// Mac.
			acs->pixIntFormat = GL_RGBA;
			acs->pixFormat = GL_BGRA;
#ifdef AR_BIG_ENDIAN
			acs->pixType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
			acs->pixType = GL_UNSIGNED_INT_8_8_8_8;
#endif
			acs->pixSize = 4;
			break;
		case AR_PIXEL_FORMAT_RGB:
			acs->pixIntFormat = GL_RGB;
			acs->pixFormat = GL_RGB;
			acs->pixType = GL_UNSIGNED_BYTE;
			acs->pixSize = 3;
			break;
		case AR_PIXEL_FORMAT_BGR:
			acs->pixIntFormat = GL_RGB;
			acs->pixFormat = GL_BGR;
			acs->pixType = GL_UNSIGNED_BYTE;
			acs->pixSize = 3;
			break;
		case AR_PIXEL_FORMAT_MONO:
			acs->pixIntFormat = GL_LUMINANCE;
			acs->pixFormat = GL_LUMINANCE;
			acs->pixType = GL_UNSIGNED_BYTE;
			acs->pixSize = 1;
			break;
		case AR_PIXEL_FORMAT_2vuy:
			if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_ycbcr_422")) {
				acs->pixIntFormat = GL_RGB;
				acs->pixFormat = GL_YCBCR_422_APPLE;
#ifdef AR_BIG_ENDIAN
				acs->pixType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#else
				acs->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
#endif
			} else if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_MESA_ycbcr_texture")) {
				acs->pixIntFormat = GL_YCBCR_MESA;
				acs->pixFormat = GL_YCBCR_MESA;
#ifdef AR_BIG_ENDIAN
				acs->pixType = GL_UNSIGNED_SHORT_8_8_REV_MESA;
#else
				acs->pixType = GL_UNSIGNED_SHORT_8_8_MESA;
#endif
			} else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_2vuy, but GL_APPLE_ycbcr_422 or GL_MESA_ycbcr_texture are not available.\n");
				return (FALSE);
			}
			acs->pixSize = 2;
			break;
		case AR_PIXEL_FORMAT_yuvs:
			if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_ycbcr_422")) {
				acs->pixIntFormat = GL_RGB;
				acs->pixFormat = GL_YCBCR_422_APPLE;
#ifdef AR_BIG_ENDIAN
				acs->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
#else
				acs->pixType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#endif
			} else if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_MESA_ycbcr_texture")) {
				acs->pixIntFormat = GL_YCBCR_MESA;
				acs->pixFormat = GL_YCBCR_MESA;
#ifdef AR_BIG_ENDIAN
				acs->pixType = GL_UNSIGNED_SHORT_8_8_MESA;
#else
				acs->pixType = GL_UNSIGNED_SHORT_8_8_REV_MESA;
#endif
			} else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_yuvs, but GL_APPLE_ycbcr_422 or GL_MESA_ycbcr_texture are not available.\n");
				return (FALSE);
			}
			acs->pixSize = 2;
			break;
        case AR_PIXEL_FORMAT_RGB_565:
            acs->pixIntFormat = GL_RGB;
            acs->pixFormat = GL_RGB;
            acs->pixType = GL_UNSIGNED_SHORT_5_6_5;
            acs->pixSize = 2;
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_RGBA;
            acs->pixType = GL_UNSIGNED_SHORT_5_5_5_1;
            acs->pixSize = 2;
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_RGBA;
            acs->pixType = GL_UNSIGNED_SHORT_4_4_4_4;
            acs->pixSize = 2;
            break;
        // Do mono-only rendering as a better alternative to doing nothing.
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_NV21:
            acs->pixIntFormat = GL_LUMINANCE;
            acs->pixFormat = GL_LUMINANCE;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 1;
            ARLOGw("ARG: Request for to render multi-planar pixel format in colour not supported. Will render in mono.\n");
            break;
		default:
            ARLOGe("ARG: set pixel format called with unsupported pixel format.\n");
			return (FALSE);
			break;
	}
    acs->format = format;
    ARLOGd("ARG: set pixel format %s.\n", arUtilGetPixelFormatName(format));
    acs->textureDataReady = FALSE;
    
    if (!arglSetupTextureObjectsGL(contextSettings)) return (FALSE);
    
    return (TRUE);
}

int arglPixelFormatGetGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (format) *format = acs->format;
    if (size) *size = acs->pixSize;
    
    return (TRUE);
}

void arglSetRotate90GL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90)
{
}

void arglSetFlipHGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH)
{
}

void arglSetFlipVGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV)
{
}

int arglPixelBufferSizeSetGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    // Check texturing capabilities (sets textureSizeX, textureSizeY, textureSizeMax).
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(acs->textureSizeMax));
    if (bufWidth > acs->textureSizeMax || bufHeight > acs->textureSizeMax) {
        ARLOGe("Error: ARG: Your OpenGL implementation and/or hardware's texturing capabilities are insufficient.\n");
        return (FALSE);
    }
    
    if (arglGLCapabilityCheck(contextSettings, 0x0200, (const unsigned char *)"GL_ARB_texture_non_power_of_two")) {
        acs->textureSizeX = bufWidth;
        acs->textureSizeY = bufHeight;
        acs->bufSizeIsTextureSize = TRUE;
    } else {
        // Work out how big power-of-two textures needs to be.
        acs->textureSizeX = acs->textureSizeY = 1;
        while (acs->textureSizeX < bufWidth) acs->textureSizeX <<= 1;
        while (acs->textureSizeY < bufHeight) acs->textureSizeY <<= 1;
        acs->bufSizeIsTextureSize = FALSE;
        acs->bufSizeX = bufWidth;
        acs->bufSizeY = bufHeight;
    }
    
    // Changing the size of the data we'll be receiving invalidates the geometry, so set it up.
    return (arglSetupTextureGeometryGL(contextSettings));
}

int arglPixelBufferSizeGetGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureGeometryHasBeenSetup) return (FALSE);
    
    if (acs->bufSizeIsTextureSize) {
        if (bufWidth) *bufWidth = acs->textureSizeX;
        if (bufHeight) *bufHeight = acs->textureSizeY;
    } else {
        if (bufWidth) *bufWidth = acs->bufSizeX;
        if (bufHeight) *bufHeight = acs->bufSizeY;
    }
    return (TRUE);
}

int arglPixelBufferDataUploadBiPlanarGL(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    int arDebugMode = AR_DEBUG_DISABLE, arImageProcMode;
    
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL_REF acs = (ARGL_CONTEXT_SETTINGS_GL_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureObjectsHaveBeenSetup || !acs->textureGeometryHasBeenSetup || !acs->pixSize) return (FALSE);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, acs->texture);
    
    if (contextSettings->arhandle) {
        arDebugMode = arGetDebugMode(contextSettings->arhandle);
    }
    if (arDebugMode == AR_DEBUG_DISABLE) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, ((((acs->bufSizeIsTextureSize ? acs->textureSizeX : acs->bufSizeX) * acs->pixSize) & 0x3) == 0 ? 4 : 1));
        if (acs->bufSizeIsTextureSize) {
            glTexImage2D(GL_TEXTURE_2D, 0, acs->pixIntFormat, acs->textureSizeX, acs->textureSizeY, 0, acs->pixFormat, acs->pixType, bufDataPtr0);
        } else {
            // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
            // Then send the NPOT-data as a subimage.
            glTexImage2D(GL_TEXTURE_2D, 0, acs->pixIntFormat, acs->textureSizeX, acs->textureSizeY, 0, acs->pixFormat, acs->pixType, NULL);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, acs->bufSizeX, acs->bufSizeY, acs->pixFormat, acs->pixType, bufDataPtr0);
        }
    } else {
        if (contextSettings->arhandle->labelInfo.bwImage) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            arImageProcMode = arGetImageProcMode(contextSettings->arhandle);
            if (arImageProcMode == AR_IMAGE_PROC_FIELD_IMAGE) {
                if (acs->bufSizeIsTextureSize) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, acs->textureSizeX >> 1, acs->textureSizeY >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                } else {
                    // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
                    // Then send the NPOT-data as a subimage.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, acs->textureSizeX >> 1, acs->textureSizeY >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, acs->bufSizeX >> 1, acs->bufSizeY >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                }
            } else {
                if (acs->bufSizeIsTextureSize) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, acs->textureSizeX, acs->textureSizeY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                } else {
                    // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
                    // Then send the NPOT-data as a subimage.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, acs->textureSizeX, acs->textureSizeY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, acs->bufSizeX, acs->bufSizeY, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                }
            }
        }
    }
    
    acs->textureDataReady = TRUE;
    
    return (TRUE);
}

int arglGLHasExtensionGL(const unsigned char *extName)
{
    const unsigned char *strExtensions;
    const unsigned char *start;
    unsigned char *where, *terminator;
    
    strExtensions = glGetString(GL_EXTENSIONS);
    if (!strExtensions) return FALSE;
    
    // Extension names should not have spaces.
    where = (unsigned char *)strchr((const char *)extName, ' ');
    if (where || *extName == '\0')
        return FALSE;
    // It takes a bit of care to be fool-proof about parsing the
    //  OpenGL extensions string. Don't be fooled by sub-strings, etc.
    start = strExtensions;
    for (;;) {
        where = (unsigned char *) strstr((const char *)start, (const char *)extName);
        if (!where)
            break;
        terminator = where + strlen((const char *)extName);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return TRUE;
        start = terminator;
    }
    return FALSE;
}

#endif // HAVE_GL
