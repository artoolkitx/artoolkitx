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

// ============================================================================
//    Private includes.
// ============================================================================
#include "arg_gles2.h"

#if HAVE_GLES2

#include <ARX/ARG/glStateCache2.h>

#include <stdio.h>        // fprintf(), stderr
#include <string.h>        // strchr(), strstr(), strlen()
#include <ARX/ARG/mtx.h>
#include <ARX/ARG/shader_gl.h>

// ============================================================================
//    Private types and defines.
// ============================================================================

//#define ARGL_DEBUG
#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 0 // Edit as required.
#endif

#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

#if !defined(GL_IMG_texture_format_BGRA8888) && !defined(GL_APPLE_texture_format_BGRA8888)
#  define GL_BGRA 0x80E1
#elif !defined(GL_BGRA)
#  define GL_BGRA 0x80E1
#endif
#ifndef GL_APPLE_rgb_422
#  define GL_RGB_422_APPLE 0x8A1F
#  define GL_RGB_RAW_422_APPLE 0x8A51
#  define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#endif

// Indices of GL ES program uniforms.
enum {
    ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX,
    ARGL_UNIFORM_TEXTURE0,
    ARGL_UNIFORM_TEXTURE1,
    ARGL_UNIFORM_COUNT
};

// Indices of of GL ES program attributes.
enum {
    ARGL_ATTRIBUTE_VERTEX,
    ARGL_ATTRIBUTE_TEXCOORD,
    ARGL_ATTRIBUTE_COUNT
};

struct _ARGL_CONTEXT_SETTINGS_GLES2 {
    GLuint  texture0; // For interleaved images all planes. For bi-planar images, plane 0.
    GLuint  texture1; // For interleaved images, not used.  For bi-planar images, plane 1.
    GLuint  program;
    GLint   uniforms[ARGL_UNIFORM_COUNT];
    float   *t2;
    float   *v2;
    GLuint  t2bo;     // Vertex buffer object for t2 data.
    GLuint  v2bo;     // Vertex buffer object for v2 data.
    float   zoom;
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
    int     textureDataReady;
    int     useTextureYCbCrBiPlanar;
    int     requestUpdateProjection;
};
typedef struct _ARGL_CONTEXT_SETTINGS_GLES2 ARGL_CONTEXT_SETTINGS_GLES2;
typedef struct _ARGL_CONTEXT_SETTINGS_GLES2 *ARGL_CONTEXT_SETTINGS_GLES2_REF;

// ============================================================================
//    Public globals.
// ============================================================================


// ============================================================================
//    Private globals.
// ============================================================================


#pragma mark -
// ============================================================================
//    Private functions.
// ============================================================================

#ifdef ARGL_DEBUG
static void arglGetErrorGLES2(const char *tag)
{
    // Report any errors we generated.
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ARLOGe("%s: GL error 0x%04X\n", tag, (int)err);
    }
}
#endif // ARGL_DEBUG

// Sets texture, t2, v2, textureGeometryHasBeenSetup.
static char arglSetupTextureGeometryGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    float    ty_prev, tx, ty;
    float    y_prev, x, y;
    ARdouble x1, x2, y1, y2;
    float    xx1, xx2, yy1, yy2;
    int      i, j;
    int      vertexCount, t2count, v2count;
    float    imageSizeX, imageSizeY;
    float    zoom;

    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;

    // Delete previous geometry, unless this is our first time here.
    if (acs->textureGeometryHasBeenSetup) {
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
        for (j = 1; j <= 20; j++) {    // Do 20 rows.
            y_prev = y;
            ty_prev = ty;
            y = imageSizeY * (float)j / 20.0f;
            ty = y / (float)acs->textureSizeY/zoom;
            
            
            for (i = 0; i <= 20; i++) {
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
    
#ifdef ARGL_DEBUG
    arglGetErrorGLES2("arglSetupTextureGeometryGLES2");
#endif

    return (TRUE);
}

// Create the shader program required for drawing the background texture.
// Sets program, uniforms.
static char arglSetupProgramGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLuint vertShader = 0, fragShader = 0;
    const char vertShaderString[] =
        "attribute vec4 position;\n"
        "attribute vec2 texCoord;\n"
        "uniform mat4 modelViewProjectionMatrix;\n"
        "varying vec2 texCoordVarying;\n"
        "void main()\n"
        "{\n"
            "gl_Position = modelViewProjectionMatrix * position;\n"
            "texCoordVarying = texCoord;\n"
        "}\n";
    const char *fragShaderString;
    const char fragShaderStringRGB[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
            "gl_FragColor = texture2D(texture0, texCoordVarying);\n"
        "}\n";
    const char fragShaderStringMono[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
        "gl_FragColor = texture2D(texture0, texCoordVarying).rrra;\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601FullRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"   // Column 0
                                                                    "0.0,   -0.344, 1.772,\n" // Column 1
                                                                    "1.402, -0.714, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).r;\n"
            "colourYCbCr.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601FullRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    const char fragShaderStringYCrCbITURec601FullRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCrCb;\n"
            "const mat3 transformYCrCbITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"    // Column 0
                                                                    "1.402, -0.714, 0.0,\n"    // Column 1
                                                                    "0.0,   -0.344, 1.772);\n" // Column 2
            "colourYCrCb.x  = texture2D(texture0, texCoordVarying).r;\n"
            "colourYCrCb.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCrCbITURec601FullRangeToRGB * colourYCrCb, 1.0);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601VideoRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).r - 0.0625;\n"
            "colourYCbCr.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601VideoRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601VideoRangeInterleaved[] =
        "#extension GL_APPLE_rgb_422 : require\n"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).g - 0.0625;\n"
            "colourYCbCr.yz = texture2D(texture0, texCoordVarying).br - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601VideoRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    
    if (acs->program) arglGLDestroyShaders(0, 0, acs->program);
    acs->program = glCreateProgram();
    if (!acs->program) {
        ARLOGe("ARG: Error creating shader program.\n");
        goto bail;
    }
    
    if (!arglGLCompileShaderFromString(&vertShader, GL_VERTEX_SHADER, vertShaderString)) {
        ARLOGe("ARG: Error compiling vertex shader.\n");
        goto bail1;
    }
    if (acs->format == AR_PIXEL_FORMAT_420f) fragShaderString = fragShaderStringYCbCrITURec601FullRangeBiPlanar;
    else if (acs->format == AR_PIXEL_FORMAT_420v) fragShaderString = fragShaderStringYCbCrITURec601VideoRangeBiPlanar;
    else if (acs->format == AR_PIXEL_FORMAT_NV21) fragShaderString = fragShaderStringYCrCbITURec601FullRangeBiPlanar;
    else if (acs->format == AR_PIXEL_FORMAT_2vuy || acs->format == AR_PIXEL_FORMAT_yuvs ) fragShaderString = fragShaderStringYCbCrITURec601VideoRangeInterleaved;
    else if (acs->format == AR_PIXEL_FORMAT_MONO ) fragShaderString = fragShaderStringMono;
    else fragShaderString = fragShaderStringRGB;
    if (!arglGLCompileShaderFromString(&fragShader, GL_FRAGMENT_SHADER, fragShaderString)) {
        ARLOGe("ARG: Error compiling fragment shader.\n");
        goto bail1;
    }
    glAttachShader(acs->program, vertShader);
    glAttachShader(acs->program, fragShader);
    
    glBindAttribLocation(acs->program, ARGL_ATTRIBUTE_VERTEX, "position");
    glBindAttribLocation(acs->program, ARGL_ATTRIBUTE_TEXCOORD, "texCoord");
    if (!arglGLLinkProgram(acs->program)) {
        ARLOGe("ARG: Error linking shader program.\n");
        goto bail1;
    }
    arglGLDestroyShaders(vertShader, fragShader, 0); // After linking, shader objects can be deleted.
    
    // Retrieve linked uniform locations.
    acs->uniforms[ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX] = glGetUniformLocation(acs->program, "modelViewProjectionMatrix");
    acs->uniforms[ARGL_UNIFORM_TEXTURE0] = glGetUniformLocation(acs->program, "texture0");
    if (acs->useTextureYCbCrBiPlanar) {
        acs->uniforms[ARGL_UNIFORM_TEXTURE1] = glGetUniformLocation(acs->program, "texture1");
    }
    
#ifdef ARGL_DEBUG
    arglGetErrorGLES2("arglSetupProgramGLES2");
#endif

    return (TRUE);
    
bail1:
    arglGLDestroyShaders(vertShader, fragShader, acs->program);
bail:
    return (FALSE);
}

// Set up the texture objects.
static char arglSetupTextureObjectsGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    
    // Delete previous textures, unless this is our first time here.
    if (acs->textureObjectsHaveBeenSetup) {
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(acs->texture0));
        if (acs->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(acs->texture1));
        }
        acs->textureObjectsHaveBeenSetup = FALSE;
    }

    glGenTextures(1, &(acs->texture0));
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(acs->texture0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (acs->useTextureYCbCrBiPlanar) {
        glGenTextures(1, &(acs->texture1));
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(acs->texture1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    acs->textureObjectsHaveBeenSetup = TRUE;
    
#ifdef ARGL_DEBUG
    arglGetErrorGLES2("arglSetupTextureObjectsGLES2");
#endif

    return (TRUE);
}

#pragma mark -
// ============================================================================
//    Public functions.
// ============================================================================

int arglSetupForCurrentContextGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT pixelFormat)
{
    if (!(arglOpenGLVersion() >= 0x0200)) {
        ARLOGe("Error: OpenGL ES v2.0 or later is required, but not found. Renderer reported '%s'\n", glGetString(GL_VERSION));
        return (FALSE);
    }
    
    contextSettings->apiContextSettings = calloc(1, sizeof(ARGL_CONTEXT_SETTINGS_GLES2));
    ((ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings)->requestUpdateProjection = TRUE;
    // Because of calloc used above, these are redundant.
    //contextSettings->apiContextSettings->program = 0;
    //contextSettings->apiContextSettings->disableDistortionCompensation = FALSE;
    //contextSettings->apiContextSettings->textureGeometryHasBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureObjectsHaveBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureDataReady = FALSE;
    
    // This sets pixIntFormat, pixFormat, pixType, pixSize, and resets textureDataReady.
    // It also calls arglSetupProgram to setup the shader program.
    ((ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings)->format = AR_PIXEL_FORMAT_INVALID;
    if (!arglPixelFormatSet(contextSettings, pixelFormat)) {
        ARLOGe("ARG: Error setting pixel format.\n");
        arglCleanupGLES2(contextSettings);
        return (FALSE);
    }
    
    // Set pixel buffer sizes to incoming image size, by default.
    if (!arglPixelBufferSizeSet(contextSettings, contextSettings->arParam.xsize, contextSettings->arParam.ysize)) {
        ARLOGe("ARG: Error setting pixel buffer size.\n");
        arglCleanupGLES2(contextSettings);
        return (FALSE);
    }
    
    return (TRUE);
}

void arglCleanupGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    if (acs->program) arglGLDestroyShaders(0, 0, acs->program);
    
    if (acs->textureObjectsHaveBeenSetup) {
        if (acs->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(acs->texture1));
        }
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(acs->texture0));
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

void arglDispImageGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int32_t viewport[4])
{
    float projection[16];
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    float left, right, bottom, top;
    int i;
    
    if (!contextSettings) {ARLOGe("arglDispImageGLES2 !contextSettings.\n"); return;}
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) {ARLOGe("arglDispImageGLES2 !acs.\n"); return;}
    
    if (!acs->textureObjectsHaveBeenSetup) {ARLOGe("arglDispImageGLES2 !textureObjectsHaveBeenSetup.\n"); return;}
    if (!acs->textureGeometryHasBeenSetup) {ARLOGe("arglDispImageGLES2 !textureGeometryHasBeenSetup.\n"); return;}
    if (!acs->textureDataReady) {ARLOGe("arglDispImageGLES2 !textureDataReady.\n"); return;}
    
    if (viewport) glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    glUseProgram(acs->program);
    
    // Prepare an orthographic projection, set camera position for 2D drawing.
    glStateCacheDisableDepthTest();
    if (acs->requestUpdateProjection) {
        if (contextSettings->rotate90) mtxLoadMatrixf(projection, ir90);
        else mtxLoadIdentityf(projection);
        if (contextSettings->flipV) {
            bottom = (float)contextSettings->arParam.ysize;
            top = 0.0f;
        } else {
            bottom = 0.0f;
            top = (float)contextSettings->arParam.ysize;
        }
        if (contextSettings->flipH) {
            left = (float)contextSettings->arParam.xsize;
            right = 0.0f;
        } else {
            left = 0.0f;
            right = (float)contextSettings->arParam.xsize;
        }
        mtxOrthof(projection, left, right, bottom, top, -1.0f, 1.0f);
        glUniformMatrix4fv(acs->uniforms[ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, projection);
        acs->requestUpdateProjection = FALSE;
    }
    
#ifdef ARGL_DEBUG
    if (!arglGLValidateProgram(acs->program)) {
        ARLOGe("arglDispImage(): Error: shader program %d validation failed.\n", acs->program);
        return;
    }
#endif

    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(acs->texture0);
    glUniform1i(acs->uniforms[ARGL_UNIFORM_TEXTURE0], 0);
    if (acs->useTextureYCbCrBiPlanar) {
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(acs->texture1);
        glUniform1i(acs->uniforms[ARGL_UNIFORM_TEXTURE1], 1);
    }
    glBindBuffer(GL_ARRAY_BUFFER, acs->v2bo);
    glVertexAttribPointer(ARGL_ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ARGL_ATTRIBUTE_VERTEX);
    glBindBuffer(GL_ARRAY_BUFFER, acs->t2bo);
    glVertexAttribPointer(ARGL_ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ARGL_ATTRIBUTE_TEXCOORD);
    
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifdef ARGL_DEBUG
    arglGetErrorGLES2("arglDispImageGLES2");
#endif
}

int arglDistortionCompensationSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    return (arglSetupTextureGeometryGLES2(contextSettings));
}

int arglSetPixelZoomGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    // Changing the zoom invalidates the geometry, so set it up.
    return (arglSetupTextureGeometryGLES2(contextSettings));
}

int arglPixelFormatSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (acs->format == format) return (TRUE);
    switch (format) {
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_NV21:
            acs->pixIntFormat = GL_LUMINANCE;
            acs->pixFormat = GL_LUMINANCE;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 1;
            acs->useTextureYCbCrBiPlanar = TRUE;
            break;
        case AR_PIXEL_FORMAT_RGBA:
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_RGBA;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 4;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_RGB:
            acs->pixIntFormat = GL_RGB;
            acs->pixFormat = GL_RGB;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 3;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_BGRA:
            if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_EXT_texture_format_BGRA8888")) {
                acs->pixIntFormat = GL_BGRA;
                acs->pixFormat = GL_BGRA;
                acs->pixType = GL_UNSIGNED_BYTE;
                acs->pixSize = 4;
                acs->useTextureYCbCrBiPlanar = FALSE;
            } else if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_texture_format_BGRA8888") || arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_IMG_texture_format_BGRA8888")) {
                acs->pixIntFormat = GL_RGBA;
                acs->pixFormat = GL_BGRA;
                acs->pixType = GL_UNSIGNED_BYTE;
                acs->pixSize = 4;
                acs->useTextureYCbCrBiPlanar = FALSE;
            } else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_BGRA, but GL_EXT_texture_format_BGRA8888 or GL_APPLE_texture_format_BGRA8888 or GL_IMG_texture_format_BGRA8888 are not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            acs->pixIntFormat = GL_LUMINANCE;
            acs->pixFormat = GL_LUMINANCE;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 1;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_2vuy:
            if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_rgb_422")) {
                acs->pixIntFormat = GL_RGB; // GL_RGB_RAW_422_APPLE
                acs->pixFormat = GL_RGB_422_APPLE;
                acs->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
                acs->pixSize = 1;
                acs->useTextureYCbCrBiPlanar = FALSE;
            } else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_2vuy, but GL_APPLE_rgb_422 is not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_yuvs:
            if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_rgb_422")) {
                acs->pixIntFormat = GL_RGB;
                acs->pixFormat = GL_RGB_422_APPLE;
                acs->pixType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
                acs->pixSize = 1;
                acs->useTextureYCbCrBiPlanar = FALSE;
            } else {
                ARLOGe("ARG: set pixel format called with AR_PIXEL_FORMAT_yuvs, but GL_APPLE_rgb_422 is not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            acs->pixIntFormat = GL_RGB;
            acs->pixFormat = GL_RGB;
            acs->pixType = GL_UNSIGNED_SHORT_5_6_5;
            acs->pixSize = 2;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_RGBA;
            acs->pixType = GL_UNSIGNED_SHORT_5_5_5_1;
            acs->pixSize = 2;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_RGBA;
            acs->pixType = GL_UNSIGNED_SHORT_4_4_4_4;
            acs->pixSize = 2;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        default:
            ARLOGe("ARG: set pixel format called with unsupported pixel format.\n");
            return (FALSE);
            break;
    }
    acs->format = format;
    ARLOGd("ARG: set pixel format %s.\n", arUtilGetPixelFormatName(format));
    acs->textureDataReady = FALSE;
    
    if (!arglSetupProgramGLES2(contextSettings)) return (FALSE);
    if (!arglSetupTextureObjectsGLES2(contextSettings)) return (FALSE);
    
    return (TRUE);
}

int arglPixelFormatGetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (format) *format = acs->format;
    if (size) *size = acs->pixSize;
    
    return (TRUE);
}

void arglSetRotate90GLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}

void arglSetFlipHGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}

void arglSetFlipVGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}

// Sets textureSizeMax, textureSizeX, textureSizeY.
int arglPixelBufferSizeSetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    // Check texturing capabilities.
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(acs->textureSizeMax));
    if (bufWidth > acs->textureSizeMax || bufHeight > acs->textureSizeMax) {
        ARLOGe("ARG: Your OpenGL implementation and/or hardware's texturing capabilities are insufficient.\n");
        return (FALSE); // Too big to handle.
    }
    
    acs->textureSizeX = bufWidth;
    acs->textureSizeY = bufHeight;

    // Changing the size of the data we'll be receiving invalidates the geometry, so set it up.
    return (arglSetupTextureGeometryGLES2(contextSettings));
}

int arglPixelBufferSizeGetGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureGeometryHasBeenSetup) return (FALSE);
    
    if (bufWidth) *bufWidth = acs->textureSizeX;
    if (bufHeight) *bufHeight = acs->textureSizeY;

    return (TRUE);
}

int arglPixelBufferDataUploadBiPlanarGLES2(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GLES2_REF acs = (ARGL_CONTEXT_SETTINGS_GLES2_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureObjectsHaveBeenSetup || !acs->textureGeometryHasBeenSetup || !acs->pixSize) return (FALSE);
    
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(acs->texture0);
    glStateCachePixelStoreUnpackAlignment(((acs->textureSizeX * acs->pixSize) & 0x3) == 0 ? 4 : 1);
    glTexImage2D(GL_TEXTURE_2D, 0, acs->pixIntFormat, acs->textureSizeX, acs->textureSizeY, 0, acs->pixFormat, acs->pixType, bufDataPtr0);
    if (bufDataPtr1 && acs->useTextureYCbCrBiPlanar) {
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(acs->texture1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, acs->textureSizeX / 2, acs->textureSizeY / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, bufDataPtr1);
    }
    acs->textureDataReady = TRUE;
    
#ifdef ARGL_DEBUG
    arglGetErrorGLES2("arglPixelBufferDataUploadBiPlanarGLES2");
#endif

    return (TRUE);
}

int arglGLHasExtensionGLES2(const unsigned char *extName)
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

#endif // HAVE_GLES2
