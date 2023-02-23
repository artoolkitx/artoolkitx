/*
 *  arg_gl3.c
 *  artoolkitX
 *
 *  Graphics Subroutines (OpenGL 3.1 and later) for artoolkitX.
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
#include "arg_gl3.h"

#if HAVE_GL3

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  ifndef _WIN32
#    define GL_GLEXT_PROTOTYPES
#  endif 
#  include <GL/glcorearb.h>
#endif
#include <stdio.h>        // fprintf(), stderr
#include <string.h>        // strchr(), strstr(), strlen()
#include <ARX/ARG/mtx.h>
#include <ARX/ARG/shader_gl.h>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h> // HMODULE, LoadLibraryA(), FreeLibrary(), GetProcAddress()
#endif

// ============================================================================
//    Private types and defines.
// ============================================================================

//#define ARGL_DEBUG 1
#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 0 // Edit as required.
#endif

#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

#ifndef GL_APPLE_rgb_422
#  define GL_RGB_422_APPLE 0x8A1F
#  define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#endif

// On Windows, all OpenGL v3 and later API must be dynamically resolved against the actual driver
#ifdef _WIN32
    # define ARGL_GET_PROC_ADDRESS get_proc
    static PFNGLBINDTEXTUREPROC glBindTexture = NULL;
    static PFNGLDELETETEXTURESPROC glDeleteTextures = NULL;
    static PFNGLGENTEXTURESPROC glGenTextures = NULL;
    static PFNGLTEXPARAMETERIPROC glTexParameteri = NULL;
    static PFNGLGETSTRINGPROC glGetString = NULL;
    static PFNGLVIEWPORTPROC glViewport = NULL;
    static PFNGLDISABLEPROC glDisable = NULL;
    static PFNGLDRAWARRAYSPROC glDrawArrays = NULL;
    static PFNGLGETINTEGERVPROC glGetIntegerv = NULL;
    static PFNGLPIXELSTOREIPROC glPixelStorei = NULL;
    static PFNGLTEXIMAGE2DPROC glTexImage2D = NULL;
    static PFNGLATTACHSHADERPROC glAttachShader = NULL; // (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffersARB");
    static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
    static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
    static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
    static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
    static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
    static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
    static PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
    static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
    static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
    static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
    static PFNGLUNIFORM1I64ARBPROC glUniform1i = NULL;
    static PFNGLGETSTRINGIPROC glGetStringi = NULL;
    static PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
    static PFNGLGENBUFFERSPROC glGenBuffers = NULL;
    static PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
    static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
    static PFNGLBUFFERDATAPROC glBufferData = NULL;
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

struct _ARGL_CONTEXT_SETTINGS_GL3 {
    GLuint  texture0; // For interleaved images all planes. For bi-planar images, plane 0.
    GLuint  texture1; // For interleaved images, not used.  For bi-planar images, plane 1.
    GLuint  program;
    GLint   uniforms[ARGL_UNIFORM_COUNT];
    GLuint  vao;     // Vertex array object.
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
    int     textureDataReady;
    int     useTextureYCbCrBiPlanar;
    int     requestUpdateProjection;
};
typedef struct _ARGL_CONTEXT_SETTINGS_GL3 ARGL_CONTEXT_SETTINGS_GL3;
typedef struct _ARGL_CONTEXT_SETTINGS_GL3 *ARGL_CONTEXT_SETTINGS_GL3_REF;

// ============================================================================
//    Public globals.
// ============================================================================


// ============================================================================
//    Private globals.
// ============================================================================

#ifdef _WIN32
static HMODULE libgl;
#endif

#pragma mark -
// ============================================================================
//    Private functions.
// ============================================================================

#ifdef _WIN32
static void *get_proc(const char *proc)
{
	void *res;
	res = wglGetProcAddress(proc);
	if (!res)
		res = GetProcAddress(libgl, proc);
	return res;
}
#endif

#ifdef ARGL_DEBUG
static void arglGetErrorGL3(const char *tag)
{
    // Report any errors we generated.
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ARLOGe("%s: GL error 0x%04X\n", tag, (int)err);
    }
}
#endif // ARGL_DEBUG

// Sets texture, t2, v2, textureGeometryHasBeenSetup.
static char arglSetupTextureGeometryGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    float    ty_prev, tx, ty;
    float    y_prev, x, y;
    ARdouble x1, x2, y1, y2;
    float    xx1, xx2, yy1, yy2;
    int      i, j;
    int      vertexCount, t2count, v2count;
    float    imageSizeX, imageSizeY;
    float    zoom;

    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;

    // Delete previous geometry, unless this is our first time here.
    if (acs->textureGeometryHasBeenSetup) {
        free(acs->t2);
        free(acs->v2);
        glDeleteBuffers(1, &acs->t2bo);
        glDeleteBuffers(1, &acs->v2bo);
        glDeleteVertexArrays(1, &acs->vao);
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
    glGenVertexArrays(1, &acs->vao);
    glBindVertexArray(acs->vao);
    glGenBuffers(1, &acs->t2bo);
    glBindBuffer(GL_ARRAY_BUFFER, acs->t2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, acs->t2, GL_STATIC_DRAW);
    glVertexAttribPointer(ARGL_ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ARGL_ATTRIBUTE_TEXCOORD);
    glGenBuffers(1, &acs->v2bo);
    glBindBuffer(GL_ARRAY_BUFFER, acs->v2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, acs->v2, GL_STATIC_DRAW);
    glVertexAttribPointer(ARGL_ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ARGL_ATTRIBUTE_VERTEX);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    acs->textureGeometryHasBeenSetup = TRUE;

#ifdef ARGL_DEBUG
    arglGetErrorGL3("arglSetupTextureGeometryGL3");
#endif

    return (TRUE);
}

// Create the shader program required for drawing the background texture.
// Sets program, uniforms.
static char arglSetupProgramGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLuint vertShader = 0, fragShader = 0;
    const char vertShaderString[] =
        "#version 140\n"
        "in vec4 position;\n"
        "in vec2 texCoord;\n"
        "uniform mat4 modelViewProjectionMatrix;\n"
        "out vec2 texCoordVarying;\n"
        "void main()\n"
        "{\n"
            "gl_Position = modelViewProjectionMatrix * position;\n"
            "texCoordVarying = texCoord;\n"
        "}\n";
    const char *fragShaderString;
    const char fragShaderStringMono[] =
        "#version 140\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "FragColor = texture(texture0, texCoordVarying).rrra;\n"
        "}\n";
    const char fragShaderStringRGB[] =
        "#version 140\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
            "FragColor = texture(texture0, texCoordVarying);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601FullRangeBiPlanar[] =
        "#version 140\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"   // Column 0
                                                                    "0.0,   -0.344, 1.772,\n" // Column 1
                                                                    "1.402, -0.714, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture(texture0, texCoordVarying).r;\n"
            "colourYCbCr.yz = texture(texture1, texCoordVarying).rg - 0.5;\n"
            "FragColor = vec4(transformYCbCrITURec601FullRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    const char fragShaderStringYCrCbITURec601FullRangeBiPlanar[] =
        "#version 140\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCrCb;\n"
            "const mat3 transformYCrCbITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"    // Column 0
                                                                    "1.402, -0.714, 0.0,\n"    // Column 1
                                                                    "0.0,   -0.344, 1.772);\n" // Column 2
            "colourYCrCb.x  = texture(texture0, texCoordVarying).r;\n"
            "colourYCrCb.yz = texture(texture1, texCoordVarying).rg - 0.5;\n"
            "FragColor = vec4(transformYCrCbITURec601FullRangeToRGB * colourYCrCb, 1.0);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601VideoRangeBiPlanar[] =
        "#version 140\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture(texture0, texCoordVarying).r - 0.0625;\n"
            "colourYCbCr.yz = texture(texture1, texCoordVarying).rg - 0.5;\n"
            "FragColor = vec4(transformYCbCrITURec601FullRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    /*const char fragShaderStringYCbCrITURec601VideoRangeInterleaved[] =
        "#version 140\n"
        "#extension GL_APPLE_rgb_422​ : require\n"
        "in vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture(texture0, texCoordVarying).r - 0.0625;\n"
            "colourYCbCr.yz = texture(texture0, texCoordVarying).gb - 0.5;\n"
            "FragColor = vec4(transformYCbCrITURec601VideoRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";*/
    
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    
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
    else if (acs->format == AR_PIXEL_FORMAT_MONO) fragShaderString = fragShaderStringMono;
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
    arglGetErrorGL3("arglSetupProgramGL3");
#endif

    return (TRUE);
    
bail1:
    arglGLDestroyShaders(vertShader, fragShader, acs->program);
bail:
    return (FALSE);
}

// Set up the texture objects.
static char arglSetupTextureObjectsGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    
    // Delete previous textures, unless this is our first time here.
    if (acs->textureObjectsHaveBeenSetup) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(acs->texture0));
        if (acs->useTextureYCbCrBiPlanar) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &(acs->texture1));
        }
        acs->textureObjectsHaveBeenSetup = FALSE;
    }

    glGenTextures(1, &(acs->texture0));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, acs->texture0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (acs->useTextureYCbCrBiPlanar) {
        glGenTextures(1, &(acs->texture1));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, acs->texture1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    acs->textureObjectsHaveBeenSetup = TRUE;

#ifdef ARGL_DEBUG
    arglGetErrorGL3("arglSetupTextureObjectsGL3");
#endif

    return (TRUE);
}

#pragma mark -
// ============================================================================
//    Public functions.
// ============================================================================

int arglSetupForCurrentContextGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT pixelFormat)
{
    if (!(arglOpenGLVersion() >= 0x0310)) {
        ARLOGe("Error: OpenGL v3.1 or later is required, but not found. Renderer reported '%s'\n", glGetString(GL_VERSION));
        return (FALSE);
    }

#ifdef _WIN32
    libgl = LoadLibraryA("opengl32.dll");
    if (!glBindTexture) glBindTexture = (PFNGLBINDTEXTUREPROC) ARGL_GET_PROC_ADDRESS("glBindTexture");
    if (!glDeleteTextures) glDeleteTextures = (PFNGLDELETETEXTURESPROC) ARGL_GET_PROC_ADDRESS("glDeleteTextures");
    if (!glGenTextures) glGenTextures = (PFNGLGENTEXTURESPROC) ARGL_GET_PROC_ADDRESS("glGenTextures");
    if (!glTexParameteri) glTexParameteri = (PFNGLTEXPARAMETERIPROC) ARGL_GET_PROC_ADDRESS("glTexParameteri");
    if (!glGetString) glGetString = (PFNGLGETSTRINGPROC) ARGL_GET_PROC_ADDRESS("glGetString");
    if (!glViewport) glViewport = (PFNGLVIEWPORTPROC) ARGL_GET_PROC_ADDRESS("glViewport");
    if (!glDisable) glDisable = (PFNGLDISABLEPROC) ARGL_GET_PROC_ADDRESS("glDisable");
    if (!glDrawArrays) glDrawArrays = (PFNGLDRAWARRAYSPROC) ARGL_GET_PROC_ADDRESS("glDrawArrays");
    if (!glGetIntegerv) glGetIntegerv = (PFNGLGETINTEGERVPROC) ARGL_GET_PROC_ADDRESS("glGetIntegerv");
    if (!glPixelStorei) glPixelStorei = (PFNGLPIXELSTOREIPROC) ARGL_GET_PROC_ADDRESS("glPixelStorei");
    if (!glTexImage2D) glTexImage2D = (PFNGLTEXIMAGE2DPROC) ARGL_GET_PROC_ADDRESS("glTexImage2D");
	if (!glAttachShader) glAttachShader = (PFNGLATTACHSHADERPROC) ARGL_GET_PROC_ADDRESS("glAttachShader");
    if (!glDeleteVertexArrays) glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) ARGL_GET_PROC_ADDRESS("glDeleteVertexArrays");
    if (!glGenVertexArrays) glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) ARGL_GET_PROC_ADDRESS("glGenVertexArrays");
    if (!glBindVertexArray) glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) ARGL_GET_PROC_ADDRESS("glBindVertexArray");
    if (!glVertexAttribPointer) glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) ARGL_GET_PROC_ADDRESS("glVertexAttribPointer");
    if (!glEnableVertexAttribArray) glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) ARGL_GET_PROC_ADDRESS("glEnableVertexAttribArray");
    if (!glCreateProgram) glCreateProgram = (PFNGLCREATEPROGRAMPROC) ARGL_GET_PROC_ADDRESS("glCreateProgram");
    if (!glBindAttribLocation) glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) ARGL_GET_PROC_ADDRESS("glBindAttribLocation");
    if (!glGetUniformLocation) glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) ARGL_GET_PROC_ADDRESS("glGetUniformLocation");
    if (!glUseProgram) glUseProgram = (PFNGLUSEPROGRAMPROC) ARGL_GET_PROC_ADDRESS("glUseProgram");
    if (!glUniformMatrix4fv) glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) ARGL_GET_PROC_ADDRESS("glUniformMatrix4fv");
    if (!glUniform1i) glUniform1i = (PFNGLUNIFORM1I64ARBPROC) ARGL_GET_PROC_ADDRESS("glUniform1i");
    if (!glGetStringi) glGetStringi = (PFNGLGETSTRINGIPROC) ARGL_GET_PROC_ADDRESS("glGetStringi");
    if (!glActiveTexture) glActiveTexture = (PFNGLACTIVETEXTUREPROC)ARGL_GET_PROC_ADDRESS("glActiveTexture");
    if (!glGenBuffers) glGenBuffers = (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffers");
    if (!glDeleteBuffers) glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteBuffers");
	if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindBuffer");
	if (!glBufferData) glBufferData = (PFNGLBUFFERDATAPROC)ARGL_GET_PROC_ADDRESS("glBufferData");
	FreeLibrary(libgl);

	if (!glDeleteTextures || !glGenTextures || !glTexParameteri || !glGetString || !glViewport ||
        !glDisable || !glDrawArrays || !glGetIntegerv || !glPixelStorei || !glTexImage2D ||
        !glAttachShader || !glDeleteVertexArrays || !glBindAttribLocation || !glCreateProgram ||
        !glEnableVertexAttribArray || !glVertexAttribPointer || !glBindVertexArray || !glGenVertexArrays || 
        !glGetUniformLocation || !glUseProgram || !glUniformMatrix4fv || !glUniform1i || !glGetStringi || 
        !glActiveTexture || !glGenBuffers ) {
            ARLOGe("arglSetupForCurrentContextGL3 error: a required OpenGL function counld not be bound.\n");
		    return (FALSE);
    }
#endif

    contextSettings->apiContextSettings = calloc(1, sizeof(ARGL_CONTEXT_SETTINGS_GL3));
    ((ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings)->requestUpdateProjection = TRUE;
    // Because of calloc used above, these are redundant.
    //contextSettings->apiContextSettings->program = 0;
    //contextSettings->apiContextSettings->textureGeometryHasBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureObjectsHaveBeenSetup = FALSE;
    //contextSettings->apiContextSettings->textureDataReady = FALSE;
    
    // This sets pixIntFormat, pixFormat, pixType, pixSize, and resets textureDataReady.
    // It also calls arglSetupProgram to setup the shader program.
    ((ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings)->format = AR_PIXEL_FORMAT_INVALID;
    if (!arglPixelFormatSet(contextSettings, pixelFormat)) {
        ARLOGe("ARG: Error setting pixel format.\n");
        arglCleanupGL3(contextSettings);
        return (FALSE);
    }
    
    // Set pixel buffer sizes to incoming image size, by default.
    if (!arglPixelBufferSizeSet(contextSettings, contextSettings->arParam.xsize, contextSettings->arParam.ysize)) {
        ARLOGe("ARG: Error setting pixel buffer size.\n");
        arglCleanupGL3(contextSettings);
        return (FALSE);
    }
    
    return (TRUE);
}

void arglCleanupGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    if (acs->program) arglGLDestroyShaders(0, 0, acs->program);
    
    if (acs->textureObjectsHaveBeenSetup) {
        if (acs->useTextureYCbCrBiPlanar) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &(acs->texture1));
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(acs->texture0));
    }
    
    if (acs->textureGeometryHasBeenSetup) {
        free(acs->t2);
        free(acs->v2);
        glDeleteBuffers(1, &acs->t2bo);
        glDeleteBuffers(1, &acs->v2bo);
        glDeleteVertexArrays(1, &acs->vao);
    }
    
    free(acs);
    acs = NULL;
}

void arglDispImageGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int32_t viewport[4])
{
    float projection[16];
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    float left, right, bottom, top;
    int i;
    
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    if (!acs->textureObjectsHaveBeenSetup) return;
    if (!acs->textureGeometryHasBeenSetup) return;
    if (!acs->textureDataReady) return;
    
    if (viewport) glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    glUseProgram(acs->program);
    
    // Prepare an orthographic projection, set camera position for 2D drawing.
    glDisable(GL_DEPTH_TEST);
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
    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, acs->texture0);
    glUniform1i(acs->uniforms[ARGL_UNIFORM_TEXTURE0], 0);
    if (acs->useTextureYCbCrBiPlanar) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, acs->texture1);
        glUniform1i(acs->uniforms[ARGL_UNIFORM_TEXTURE1], 1);
    }
    
    glBindVertexArray(acs->vao);
#ifdef ARGL_DEBUG
    if (!arglGLValidateProgram(acs->program)) {
        ARLOGe("arglDispImage(): Error: shader program %d validation failed.\n", acs->program);
        return;
    }
#endif
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    glBindVertexArray(0);

#ifdef ARGL_DEBUG
    arglGetErrorGL3("arglDispImageGL3");
#endif
}

int arglDistortionCompensationSetGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);

    return (arglSetupTextureGeometryGL3(contextSettings));
}

int arglSetPixelZoomGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    // Changing the zoom invalidates the geometry, so set it up.
    return (arglSetupTextureGeometryGL3(contextSettings));
}

int arglPixelFormatSetGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (acs->format == format) return (TRUE);
    switch (format) {
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_NV21:
            acs->pixIntFormat = GL_R8;
            acs->pixFormat = GL_RED;
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
            acs->pixIntFormat = GL_RGBA;
            acs->pixFormat = GL_BGRA;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 4;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_MONO:
            acs->pixIntFormat = GL_R8;
            acs->pixFormat = GL_RED;
            acs->pixType = GL_UNSIGNED_BYTE;
            acs->pixSize = 1;
            acs->useTextureYCbCrBiPlanar = FALSE;
            break;
        case AR_PIXEL_FORMAT_2vuy:
            if (arglGLCapabilityCheck(contextSettings, 0, (unsigned char *)"GL_APPLE_rgb_422")) {
                acs->pixIntFormat = GL_RGB;
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
                acs->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
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
    
    if (!arglSetupProgramGL3(contextSettings)) return (FALSE);
    if (!arglSetupTextureObjectsGL3(contextSettings)) return (FALSE);
    
    return (TRUE);
}

int arglPixelFormatGetGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (format) *format = acs->format;
    if (size) *size = acs->pixSize;
    
    return (TRUE);
}

void arglSetRotate90GL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}

void arglSetFlipHGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}

void arglSetFlipVGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV)
{
    if (!contextSettings) return;
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return;
    
    acs->requestUpdateProjection = TRUE;
}


// Sets textureSizeMax, textureSizeX, textureSizeY.
int arglPixelBufferSizeSetGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
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
    return (arglSetupTextureGeometryGL3(contextSettings));
}

int arglPixelBufferSizeGetGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureGeometryHasBeenSetup) return (FALSE);
    
    if (bufWidth) *bufWidth = acs->textureSizeX;
    if (bufHeight) *bufHeight = acs->textureSizeY;

    return (TRUE);
}

int arglPixelBufferDataUploadBiPlanarGL3(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    if (!contextSettings) return (FALSE);
    ARGL_CONTEXT_SETTINGS_GL3_REF acs = (ARGL_CONTEXT_SETTINGS_GL3_REF)contextSettings->apiContextSettings;
    if (!acs) return (FALSE);
    
    if (!acs->textureObjectsHaveBeenSetup || !acs->textureGeometryHasBeenSetup || !acs->pixSize) return (FALSE);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, acs->texture0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, (((acs->textureSizeX * acs->pixSize) & 0x3) == 0 ? 4 : 1));
    glTexImage2D(GL_TEXTURE_2D, 0, acs->pixIntFormat, acs->textureSizeX, acs->textureSizeY, 0, acs->pixFormat, acs->pixType, bufDataPtr0);
    if (bufDataPtr1 && acs->useTextureYCbCrBiPlanar) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, acs->texture1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, acs->textureSizeX / 2, acs->textureSizeY / 2, 0, GL_RG, GL_UNSIGNED_BYTE, bufDataPtr1);
    }
    acs->textureDataReady = TRUE;

#ifdef ARGL_DEBUG
    arglGetErrorGL3("arglPixelBufferDataUploadBiPlanarGL3");
#endif

    return (TRUE);
}

int arglGLHasExtensionGL3(const unsigned char *extName)
{
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++) {
        const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, i);
        if (strcmp((const char *)extName, extension) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

#endif // HAVE_GL3
