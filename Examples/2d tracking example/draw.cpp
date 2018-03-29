//
//  draw.cpp
//  artoolkitX 2d Tracking Example
//
//  Copyright 2018 Realmax, Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#include "draw.h"
#include <ARX/ARController.h>
#if HAVE_GL
#  if ARX_TARGET_PLATFORM_MACOS
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif // HAVE_GL
#if HAVE_GLES2 || HAVE_GL3
#  include <ARX/ARG/mtx.h>
#  include <ARX/ARG/shader_gl.h>
#  if HAVE_GLES2
#    if ARX_TARGET_PLATFORM_IOS
#      include <OpenGLES/ES2/gl.h>
#    else
#      include <GLES2/gl2.h>
#    endif
#  else
#    if ARX_TARGET_PLATFORM_MACOS
#      include <OpenGL/gl3.h>
#    else
#      ifndef _WIN32
#			define GL_GLEXT_PROTOTYPES
#      endif
#      include "GL/glcorearb.h"
#    endif
#  endif
#endif // HAVE_GLES2 || HAVE_GL3

#define DRAW_MODELS_MAX 32

#if HAVE_GLES2 || HAVE_GL3
// Indices of GL program uniforms.
enum {
    UNIFORM_MODELVIEW_PROJECTION_MATRIX,
    UNIFORM_COUNT
};
// Indices of of GL program attributes.
enum {
    ATTRIBUTE_VERTEX,
    ATTRIBUTE_COLOUR,
    ATTRIBUTE_COUNT
};
static GLint uniforms[UNIFORM_COUNT] = {0};
static GLuint program = 0;

#if HAVE_GL3
static GLuint gCubeVAOs[2] = {0};
static GLuint gCubeV3BO = 0;
static GLuint gCubeC4BO = 0;
static GLuint gCubeCb4BO = 0;
static GLuint gCubeEABO = 0;
#if defined(_WIN32)
	# define ARGL_GET_PROC_ADDRESS wglGetProcAddress
	PFNGLBINDBUFFERPROC glBindBuffer = NULL; // (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffersARB");
	PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
	PFNGLGENBUFFERSPROC glGenBuffers = NULL;
	PFNGLBUFFERDATAPROC glBufferData = NULL;
	PFNGLATTACHSHADERPROC glAttachShader = NULL;
	PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
	PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
	PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
	PFNGLUSEPROGRAMPROC glUseProgram = NULL;

	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
	PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
	PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
	PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
	#endif
#endif // HAVE_GL3

#endif // HAVE_GLES2 || HAVE_GL3

static ARG_API drawAPI = ARG_API_None;
static bool rotate90 = false;
static bool flipH = false;
static bool flipV = false;

static int32_t gViewport[4] = {0};
static float gProjection[16];
static float gView[16];
static bool gModelLoaded[DRAW_MODELS_MAX] = {false};
static float gModelPoses[DRAW_MODELS_MAX][16];
static bool gModelVisbilities[DRAW_MODELS_MAX];

static void drawCube(float viewProjection[16], float pose[16]);

void drawSetup(ARG_API drawAPI_in, bool rotate90_in, bool flipH_in, bool flipV_in)
{
    #if defined(_WIN32)
	if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindBuffer");
	if (!glDeleteBuffers) glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteBuffers");
	if (!glGenBuffers) glGenBuffers = (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffers");
	if (!glBufferData) glBufferData = (PFNGLBUFFERDATAPROC)ARGL_GET_PROC_ADDRESS("glBufferData");
	if (!glAttachShader) glAttachShader = (PFNGLATTACHSHADERPROC)ARGL_GET_PROC_ADDRESS("glAttachShader");

	if (!glBindAttribLocation) glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)ARGL_GET_PROC_ADDRESS("glBindAttribLocation");
	if (!glCreateProgram) glCreateProgram = (PFNGLCREATEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glCreateProgram");
	if (!glDeleteProgram) glDeleteProgram = (PFNGLDELETEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glDeleteProgram");
	if (!glEnableVertexAttribArray) glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)ARGL_GET_PROC_ADDRESS("glEnableVertexAttribArray");
	if (!glGetUniformLocation) glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)ARGL_GET_PROC_ADDRESS("glGetUniformLocation");
	if (!glUseProgram) glUseProgram = (PFNGLUSEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glUseProgram");

	if (!glUniformMatrix4fv) glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)ARGL_GET_PROC_ADDRESS("glUniformMatrix4fv");
	if (!glVertexAttribPointer) glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)ARGL_GET_PROC_ADDRESS("glVertexAttribPointer");
	if (!glBindVertexArray) glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)ARGL_GET_PROC_ADDRESS("glBindVertexArray");
	if (!glDeleteVertexArrays) glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)ARGL_GET_PROC_ADDRESS("glDeleteVertexArrays");
	if (!glGenVertexArrays) glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)ARGL_GET_PROC_ADDRESS("glGenVertexArrays");

	if (!glBindBuffer || !glDeleteBuffers || !glGenBuffers || !glBufferData ||
		!glAttachShader || !glBindAttribLocation || !glCreateProgram || !glDeleteProgram ||
		!glEnableVertexAttribArray || !glGetUniformLocation || !glUseProgram || !glUniformMatrix4fv || 
		!glVertexAttribPointer || !glBindVertexArray || !glDeleteVertexArrays || !glGenVertexArrays) {
		ARLOGe("Error: a required OpenGL function counld not be bound.\n");
	}
#endif

    drawAPI = drawAPI_in;
    rotate90 = rotate90_in;
    flipH = flipH_in;
    flipV = flipV_in;
    
    return;
}

void drawCleanup()
{
#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }
#if HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            if (gCubeVAOs[0]) {
                glDeleteBuffers(1, &gCubeCb4BO);
                glDeleteBuffers(1, &gCubeEABO);
                glDeleteBuffers(1, &gCubeC4BO);
                glDeleteBuffers(1, &gCubeV3BO);
                glDeleteVertexArrays(2, gCubeVAOs);
                gCubeVAOs[0] = gCubeVAOs[1] = 0;
            }
        }
#endif // HAVE_GL3
    }
#endif // HAVE_GLES2 || HAVE_GL3
    for (int i = 0; i < DRAW_MODELS_MAX; i++) gModelLoaded[i] = false;

    return;
}

int drawLoadModel(const char *path)
{
    // Ignore path, we'll always draw a cube.
    for (int i = 0; i < DRAW_MODELS_MAX; i++) {
        if (!gModelLoaded[i]) {
            gModelLoaded[i] = true;
            return i;
        }
    }
    return -1;
}

void drawSetViewport(int32_t viewport[4])
{
    gViewport[0] = viewport[0];
    gViewport[1] = viewport[1];
    gViewport[2] = viewport[2];
    gViewport[3] = viewport[3];
}

void drawSetCamera(float projection[16], float view[16])
{
    if (projection) {
        if (flipH || flipV) {
            mtxLoadIdentityf(gProjection);
            mtxScalef(gProjection, flipV ? -1.0f : 1.0f,  flipH ? -1.0f : 1.0f, 1.0f);
            mtxMultMatrixf(gProjection, projection);
        } else {
            mtxLoadMatrixf(gProjection, projection);
        }
    } else {
        mtxLoadIdentityf(gProjection);
    }
    if (view) {
        mtxLoadMatrixf(gView, view);
    } else {
        mtxLoadIdentityf(gView);
    }
}

void drawSetModel(int modelIndex, bool visible, float pose[16])
{
    if (modelIndex < 0 || modelIndex >= DRAW_MODELS_MAX) return;
    if (!gModelLoaded[modelIndex]) return;
    
    gModelVisbilities[modelIndex] = visible;
    if (visible) mtxLoadMatrixf(&(gModelPoses[modelIndex][0]), pose);
}

void draw()
{
    float viewProjection[16];

    glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]);

#if HAVE_GL
    if (drawAPI == ARG_API_GL) {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(gProjection);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(gView);
    }
#endif

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (!program) {
            GLuint vertShader = 0, fragShader = 0;
            // A simple shader pair which accepts just a vertex position and colour, no lighting.
            const char vertShaderStringGLES2[] =
                "attribute vec4 position;\n"
                "attribute vec4 colour;\n"
                "uniform mat4 modelViewProjectionMatrix;\n"
                "varying vec4 colourVarying;\n"
                "void main()\n"
                "{\n"
                    "gl_Position = modelViewProjectionMatrix * position;\n"
                    "colourVarying = colour;\n"
                "}\n";
            const char fragShaderStringGLES2[] =
                "#ifdef GL_ES\n"
                "precision mediump float;\n"
                "#endif\n"
                "varying vec4 colourVarying;\n"
                "void main()\n"
                "{\n"
                    "gl_FragColor = colourVarying;\n"
                "}\n";
            const char vertShaderStringGL3[] =
                "#version 150\n"
                "in vec4 position;\n"
                "in vec4 colour;\n"
                "uniform mat4 modelViewProjectionMatrix;\n"
                "out vec4 colourVarying;\n"
                "void main()\n"
                "{\n"
                "gl_Position = modelViewProjectionMatrix * position;\n"
                "colourVarying = colour;\n"
                "}\n";
            const char fragShaderStringGL3[] =
                "#version 150\n"
                "in vec4 colourVarying;\n"
                "out vec4 FragColor;\n"
                "void main()\n"
                "{\n"
                "FragColor = colourVarying;\n"
                "}\n";

            if (program) arglGLDestroyShaders(0, 0, program);
            program = glCreateProgram();
            if (!program) {
                ARLOGe("draw: Error creating shader program.\n");
                return;
            }
            
            if (!arglGLCompileShaderFromString(&vertShader, GL_VERTEX_SHADER, drawAPI == ARG_API_GLES2 ? vertShaderStringGLES2 : vertShaderStringGL3)) {
                ARLOGe("draw: Error compiling vertex shader.\n");
                arglGLDestroyShaders(vertShader, fragShader, program);
                program = 0;
                return;
            }
            if (!arglGLCompileShaderFromString(&fragShader, GL_FRAGMENT_SHADER, drawAPI == ARG_API_GLES2 ? fragShaderStringGLES2 : fragShaderStringGL3)) {
                ARLOGe("draw: Error compiling fragment shader.\n");
                arglGLDestroyShaders(vertShader, fragShader, program);
                program = 0;
                return;
            }
            glAttachShader(program, vertShader);
            glAttachShader(program, fragShader);
            
            glBindAttribLocation(program, ATTRIBUTE_VERTEX, "position");
            glBindAttribLocation(program, ATTRIBUTE_COLOUR, "colour");
            if (!arglGLLinkProgram(program)) {
                ARLOGe("draw: Error linking shader program.\n");
                arglGLDestroyShaders(vertShader, fragShader, program);
                program = 0;
                return;
            }
            arglGLDestroyShaders(vertShader, fragShader, 0); // After linking, shader objects can be deleted.
            
            // Retrieve linked uniform locations.
            uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX] = glGetUniformLocation(program, "modelViewProjectionMatrix");
        }
        glUseProgram(program);
        mtxLoadMatrixf(viewProjection, gProjection);
        mtxMultMatrixf(viewProjection, gView);
    }
#endif // HAVE_GLES2 || HAVE_GL3

    glEnable(GL_DEPTH_TEST);
    
    for (int i = 0; i < DRAW_MODELS_MAX; i++) {
        if (gModelLoaded[i] && gModelVisbilities[i]) {
            drawCube(viewProjection, &(gModelPoses[i][0]));
        }
    }
    
}

// Something to look at, draw a rotating colour cube.
static void drawCube(float viewProjection[16], float pose[16])
{
    // Colour cube data.
    const GLfloat cube_vertices [8][3] = {
        /* +z */ {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        /* -z */ {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f} };
    const GLubyte cube_vertex_colors [8][4] = {
        {255, 255, 255, 255}, {255, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 255, 255},
        {255, 0, 255, 255}, {255, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 255, 255} };
    const GLubyte cube_vertex_colors_black [8][4] = {
        {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255},
        {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255} };
    const GLushort cube_faces [6][4] = { /* ccw-winding */
        /* +z */ {3, 2, 1, 0}, /* -y */ {2, 3, 7, 6}, /* +y */ {0, 1, 5, 4},
        /* -x */ {3, 0, 4, 7}, /* +x */ {1, 2, 6, 5}, /* -z */ {4, 5, 6, 7} };
    int i;
#if HAVE_GLES2 || HAVE_GL3
    float modelViewProjection[16];
#endif

#if HAVE_GL
    if (drawAPI == ARG_API_GL) {
        glPushMatrix(); // Save world coordinate system.
        glMultMatrixf(pose);
        glScalef(40.0f, 40.0f, 40.0f);
        glTranslatef(0.0f, 0.0f, 0.5f); // Place base of cube on marker surface.
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, cube_vertex_colors);
        glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        for (i = 0; i < 6; i++) {
            glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
        }
        glDisableClientState(GL_COLOR_ARRAY);
        glColor4ub(0, 0, 0, 255);
        for (i = 0; i < 6; i++) {
            glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
        }
        glPopMatrix();    // Restore world coordinate system.
    }
#endif // HAVE_GL

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        mtxLoadMatrixf(modelViewProjection, viewProjection);
        mtxMultMatrixf(modelViewProjection, pose);
        mtxScalef(modelViewProjection, 40.0f, 40.0f, 40.0f);
        mtxTranslatef(modelViewProjection, 0.0f, 0.0f, 0.5f); // Place base of cube on marker surface.
        glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, modelViewProjection);
#  if HAVE_GLES2
        if (drawAPI == ARG_API_GLES2) {
            glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, cube_vertices);
            glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
            glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cube_vertex_colors);
            glEnableVertexAttribArray(ATTRIBUTE_COLOUR);
#    ifdef DEBUG
            if (!arglGLValidateProgram(program)) {
                ARLOGe("drawCube(): Error: shader program %d validation failed.\n", program);
                return;
            }
#    endif // DEBUG
            for (i = 0; i < 6; i++) {
                glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
            }
            glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cube_vertex_colors_black);
            glEnableVertexAttribArray(ATTRIBUTE_COLOUR);
            for (i = 0; i < 6; i++) {
                glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
            }
        }
#  else // HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            if (!gCubeVAOs[0]) {
                glGenVertexArrays(2, gCubeVAOs);
                glBindVertexArray(gCubeVAOs[0]);
                glGenBuffers(1, &gCubeV3BO);
                glBindBuffer(GL_ARRAY_BUFFER, gCubeV3BO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
                glGenBuffers(1, &gCubeC4BO);
                glBindBuffer(GL_ARRAY_BUFFER, gCubeC4BO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertex_colors), cube_vertex_colors, GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_COLOUR);
                glGenBuffers(1, &gCubeEABO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCubeEABO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_faces), cube_faces, GL_STATIC_DRAW);
                glBindVertexArray(gCubeVAOs[1]);
                glBindBuffer(GL_ARRAY_BUFFER, gCubeV3BO);
                glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
                glGenBuffers(1, &gCubeCb4BO);
                glBindBuffer(GL_ARRAY_BUFFER, gCubeCb4BO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertex_colors_black), cube_vertex_colors_black, GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_COLOUR);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCubeEABO);
            }
            
            glBindVertexArray(gCubeVAOs[0]);
    #ifdef DEBUG
            if (!arglGLValidateProgram(program)) {
                ARLOGe("drawCube() Error: shader program %d validation failed.\n", program);
                return;
            }
    #endif
            for (i = 0; i < 6; i++) {
                glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (void *)(i*4*sizeof(GLushort)));
            }
            glBindVertexArray(gCubeVAOs[1]);
            for (i = 0; i < 6; i++) {
                glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (void *)(i*4*sizeof(GLushort)));
            }
            glBindVertexArray(0);
        }
#  endif // HAVE_GL3
    }
#endif // HAVE_GLES2 || HAVE_GL3
}
