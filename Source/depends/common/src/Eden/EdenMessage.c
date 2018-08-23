//
//  EdenMessage.c
//
//  Copyright (c) 2001-2018 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	Font loading code based on code by Jeff Molofee, 1999, http://nehe.gamedev.net/
//

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

// ============================================================================
//	Includes
// ============================================================================

#include <Eden/EdenMessage.h>
#include <Eden/EdenMath.h> // MIN()

#include <stdio.h>
#include <string.h>
#include <stdlib.h>				// malloc(), calloc(), free(), exit()
#include <stdbool.h>
#include <pthread.h>
#include <Eden/EdenUIInput.h>
#include <Eden/EdenTime.h>	// EdenTime_sleep()
#include <Eden/EdenSurfaces.h>	// TEXTURE_INFO_t, TEXTURE_INDEX_t, SurfacesTextureLoad(), SurfacesTextureSet(), SurfacesTextureUnload()
#include <Eden/EdenGLFont.h>
#if EDEN_USE_GL
#  define USE_GL_STATE_CACHE 0
#  include <Eden/glStateCache.h>
#elif EDEN_USE_GLES2
#  include <ARX/ARG/glStateCache2.h>
#  include <ARX/ARG/shader_gl.h>
#  include <ARX/ARG/mtx.h>
#endif

// ============================================================================
//  Types and constants
// ============================================================================

#define BOX_LINES_MAX 80
#define BOX_LINE_LENGTH_MAX 1023

typedef struct _boxSettings {
    pthread_mutex_t lock;
	float boxWidth;				// Pixel width of box.
    float boxHeight;            // Pixel height of box (calculated).
	float boxPaddingH;			// Pixels of padding between the left and right border and the text.
	float boxPaddingV;			// Pixels of padding between the top and bottom border and the text.
	float boxCornerRadius;		// Radius in pixels of the rounded corners. Typically set to MIN(boxPaddingH, boxPaddinV).
	float softwrapRatio;		// Percentage of the maximum text width at which to start soft-wrapping text.
    unsigned char *text;
	unsigned char *lines[BOX_LINES_MAX];
	int lineCount;
} boxSettings_t;

//#define DEBUG_MESSAGE					// Uncomment to show extra debugging info.

#pragma mark -
#pragma mark [GLOBAL VARIABLES]
// ============================================================================
//	Global variables
// ============================================================================

// Sanity checks.
static bool gMessageInited = false; // Set to true once EdenMessageInit() has succesfully completed.

static boxSettings_t *gBoxSettings = NULL;

// Screen size.
static float gScreenWidth = 640.0f;
static float gScreenHeight = 480.0f;
static float gScreenScale = 1.0f;

#if EDEN_USE_GLES2
// Indices of GL ES program uniforms.
enum {
    UNIFORM_MODELVIEW_PROJECTION_MATRIX,
    UNIFORM_COLOR,
    UNIFORM_COUNT
};
// Indices of of GL ES program attributes.
enum {
    ATTRIBUTE_VERTEX,
    ATTRIBUTE_COUNT
};
static GLint uniforms[UNIFORM_COUNT] = {0};
static GLuint program = 0;
#endif

// Use of gDrawLock allows EdenMessageDraw() to be called in a separate thread
// by protecting the global static data (below) that it uses.
//static pthread_mutex_t gDrawLock;
bool gEdenMessageDrawRequired = false;						// EdenMessageDraw() should be called if this is set to true;

static EdenUIInput_t gInput = NULL;

bool gEdenMessageKeyboardRequired = false;		// EdenMessageInputKeyboard should be called with keystrokes if this is set to true;

//
// Private functions.
//

// Can pass NULL for parameter 'text' in which case previous text is reused.
// Depends on settings 'text', 'boxWidth', 'boxPaddingH'.
// Sets settings 'text', 'lines', 'lineCount', 'boxHeight'.
static void boxSetText(boxSettings_t *settings, const unsigned char *text)
{
	int i;
	int textIndex;
	bool done;
 	float boxTextWidth;
	float boxTextWidthSoftwrap;
	float hyphenWidth;
    float interCharSpacingWidth;
    
	unsigned char c = '\0', c0, c1; // current, previous, next char.
	int lineLength = 0;
	unsigned char lineBuf[BOX_LINE_LENGTH_MAX + 1] = ""; // +1 for null terminator.
	float lineWidth = 0;
   
	if (!settings) return;
    
    pthread_mutex_lock(&settings->lock);
    
    if (text) {
        free(settings->text);
        settings->text = (unsigned char *)strdup((char *)text);
    }
    
	// Free old lines.
	if (settings->lineCount) {
        for (i = 0; i < settings->lineCount; i++) {
            free(settings->lines[i]);
            settings->lines[i] = NULL;
        }
		settings->lineCount = 0;
	}
    
	boxTextWidth = settings->boxWidth - 2*settings->boxPaddingH;
	boxTextWidthSoftwrap = boxTextWidth*settings->softwrapRatio;
	hyphenWidth = EdenGLFontGetCharacterWidth('-');
    interCharSpacingWidth = EdenGLFontGetLineWidth((const unsigned char *)"--") - 2.0f*hyphenWidth;
    
    if (settings->text) {
        // Split text into lines, softwrapping on whitespace if possible.
        textIndex = 0;
        done = false;
        do {
            
            bool newline = false;
            
            c0 = c;
            c = settings->text[textIndex];
            if (!c) {
                if (lineLength) newline = true;
                done = true;
            } else  if (c == '\n' || (c == ' ' && lineWidth >= boxTextWidthSoftwrap)) {
                textIndex++;
                newline = true;
            } else if (c < ' ') {
                textIndex++;
            } else {
                bool addChar = false;
                // Is there still room for a hyphen after this character?
                float predictedLineWidth = lineWidth + interCharSpacingWidth + EdenGLFontGetCharacterWidth(settings->text[textIndex]);
                if (predictedLineWidth < (boxTextWidth - hyphenWidth)) {
                    addChar = true;
                } else {
                    // No. But two exceptions:
                    // 1) this character is a space.
                    // 2) this character doesn't overflow and the next character is whitespace.
                    c1 = settings->text[textIndex + 1];
                    if (c == ' ') {
                        textIndex++;
                        newline = true;
                    } else if (predictedLineWidth <= boxTextWidth && (!c1 || c1 == ' ' || c1 == '\n')) {
                        addChar = true;
                    } else {
                        // Exception didn't apply, so insert hyphen, then newline, then continue with same char on next line (unless previous char was space, in which case no hyphen).
                        if (c0 != ' ') {
                            lineBuf[lineLength++] = '-';
                            lineBuf[lineLength] = '\0';
                        }
                        newline = true;
                    }
                }
                if (addChar) {
                    lineBuf[lineLength++] = c;
                    lineBuf[lineLength] = '\0';
                    lineWidth = EdenGLFontGetLineWidth(lineBuf);
                    if (lineLength == BOX_LINE_LENGTH_MAX) newline = true; // Next char would overflow buffer, so break now.
                    textIndex++;
                }
            }
            
            if (newline) {
                // Start a new line.
                settings->lines[settings->lineCount] = (unsigned char *)strdup((const char *)lineBuf);
                settings->lineCount++;
                if (settings->lineCount == BOX_LINES_MAX) done = true;
                lineLength = 0;
                lineBuf[0] = '\0';
                lineWidth = 0.0f;
            }
        } while (!done);
    }
    
    settings->boxHeight = EdenGLFontGetBlockHeight((const unsigned char **)settings->lines, settings->lineCount) + 2.0f*settings->boxPaddingV;

    pthread_mutex_unlock(&settings->lock);
}

static boxSettings_t *boxCreate(float width, float paddingH, float paddingV, float cornerRadius, float softwrapRatio)
{
    boxSettings_t *settings = (boxSettings_t *)calloc(1, sizeof(boxSettings_t));
    if (!settings) return (NULL);
    
    pthread_mutex_init(&settings->lock, NULL);
    settings->boxWidth = (width > 0.0f ? width : 400.0f)*gScreenScale;
    settings->boxPaddingH = (paddingH >= 0.0f ? paddingH : 20.0f)*gScreenScale;
    settings->boxPaddingV = (paddingV >= 0.0f ? paddingV : 20.0f)*gScreenScale;
    settings->boxCornerRadius = (cornerRadius >= 0.0f ? cornerRadius*gScreenScale : MIN(settings->boxPaddingH, settings->boxPaddingV));
    settings->softwrapRatio = (softwrapRatio > 0.0f ? softwrapRatio : 0.9f);
    return (settings);
}

static void boxDestroy(boxSettings_t **settings_p)
{
    int i;
    
    if (!settings_p || !*settings_p) return;
    
    pthread_mutex_destroy(&(*settings_p)->lock);
    
 	// Free lines.
	if ((*settings_p)->lineCount) {
		for (i = 0; i < (*settings_p)->lineCount; i++) free((*settings_p)->lines[i]);
		(*settings_p)->lineCount = 0;
	}
    free((*settings_p)->text);
    free(*settings_p);
    (*settings_p) = NULL;
}

// ============================================================================
//  Public functions
// ============================================================================

bool EdenMessageInit(const int contextsActiveCount)
{
	if (gMessageInited) return (false);

    // OpenGL setup.
#if EDEN_USE_GLES2
    if (!program) {
        GLuint vertShader = 0, fragShader = 0;
        // A simple shader pair which accepts just a vertex position. Fixed color, no lighting.
        const char vertShaderString[] =
        "attribute vec4 position;\n"
        "uniform vec4 color;\n"
        "uniform mat4 modelViewProjectionMatrix;\n"
        
        "varying vec4 colorVarying;\n"
        "void main()\n"
        "{\n"
        "gl_Position = modelViewProjectionMatrix * position;\n"
        "colorVarying = color;\n"
        "}\n";
        const char fragShaderString[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 colorVarying;\n"
        "void main()\n"
        "{\n"
        "gl_FragColor = colorVarying;\n"
        "}\n";
        
        if (program) arglGLDestroyShaders(0, 0, program);
        program = glCreateProgram();
        if (!program) {
            EDEN_LOGe("draw: Error creating shader program.\n");
            return (false);
        }
        
        if (!arglGLCompileShaderFromString(&vertShader, GL_VERTEX_SHADER, vertShaderString)) {
            EDEN_LOGe("draw: Error compiling vertex shader.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return (false);
        }
        if (!arglGLCompileShaderFromString(&fragShader, GL_FRAGMENT_SHADER, fragShaderString)) {
            EDEN_LOGe("draw: Error compiling fragment shader.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return (false);
        }
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        
        glBindAttribLocation(program, ATTRIBUTE_VERTEX, "position");
        if (!arglGLLinkProgram(program)) {
            EDEN_LOGe("draw: Error linking shader program.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return (false);
        }
        arglGLDestroyShaders(vertShader, fragShader, 0); // After linking, shader objects can be deleted.
        
        // Retrieve linked uniform locations.
        uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX] = glGetUniformLocation(program, "modelViewProjectionMatrix");
        uniforms[UNIFORM_COLOR] = glGetUniformLocation(program, "color");
    }
#endif
    
    gBoxSettings = boxCreate(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f); // Use defaults.
    
	gMessageInited = true;
	return (true);
}

bool EdenMessageFinal(void)
{
	bool ok = true;

	if (!gMessageInited) return (false);
    
    if (gInput) EdenUIInputDelete(&gInput);

    boxDestroy(&gBoxSettings);
    
#if EDEN_USE_GLES2
    arglGLDestroyShaders(0, 0, program);
#endif

	gMessageInited = false;
	return (ok);
}

unsigned char *EdenMessageInputGetInput(void)
{
    if (!gInput) return NULL;
    return (EdenUIInputGetInput(gInput));
}


#pragma mark -
// ----------------------------------------------------------------------------
//	These functions should only be called directly in single- threaded apps.
//	In multi-threaded apps, they will be called indirectly.
// ----------------------------------------------------------------------------

EDEN_E_t EdenMessageShow(const unsigned char *msg)
{
	if (!gMessageInited) return (EDEN_E_INVALID_COMMAND);
    
	if (msg) {
        boxSetText(gBoxSettings, msg);
    }
    
	gEdenMessageDrawRequired = true;

	return (EDEN_E_NONE);
}

EDEN_E_t EdenMessageInputShow(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, bool intOnly, bool fpOnly, bool alphaOnly)
{
    EDEN_E_t messageErr = EDEN_E_NONE;
    
    if (!gMessageInited) return (EDEN_E_INVALID_COMMAND);
    
    if (gInput) EdenUIInputDelete(&gInput);
    gInput = EdenUIInputNew(prompt, minLength, maxLength, intOnly, fpOnly, alphaOnly);
    
    if ((messageErr = EdenMessageShow(EdenUIInputGetInputForDrawing(gInput))) != EDEN_E_NONE) {
        goto done;
    }
    
done:
    return (messageErr);
}

bool EdenMessageInputIsComplete(void)
{
    if (!gMessageInited) return (true);
    
    return (EdenUIInputIsComplete(gInput));
}

EDEN_E_t EdenMessageHide(void)
{
	if (!gMessageInited) return (EDEN_E_INVALID_COMMAND);

	gEdenMessageDrawRequired = false;
    
	return (EDEN_E_NONE);
}

#pragma mark -

// ----------------------------------------------------------------------------
//	Functions for use in multi-threaded apps only.
// ----------------------------------------------------------------------------
EDEN_E_t EdenMessage(unsigned char *msg, const unsigned int secs)
{
	EDEN_E_t err;
	
	err = EdenMessageShow(msg);
	if (err != EDEN_E_NONE) return (err);
	
	EdenTime_sleep(secs);

	err = EdenMessageHide();
	if (err != EDEN_E_NONE) return (err);

	return (EDEN_E_NONE);
}

EDEN_E_t EdenMessageInput(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, bool intOnly, bool fpOnly, bool alphaOnly)
{
	EDEN_E_t messageErr = EDEN_E_NONE;

	if ((messageErr = EdenMessageInputShow(prompt, minLength, maxLength, intOnly, fpOnly, alphaOnly)) != EDEN_E_NONE) {
        goto done;
    }
    
    EdenUIInputWaitComplete(gInput);
    
	messageErr = EdenMessageHide();

done:
	return (messageErr);
}

#pragma mark -
// ----------------------------------------------------------------------------
//	Functions for use in single- and double-threaded apps.
// ----------------------------------------------------------------------------

void EdenMessageSetViewSize(const float width, const float height)
{
    bool changed = false;
    if (gScreenWidth != width) {
        gScreenWidth = width;
        changed = true;
    }
    if (gScreenHeight != height) {
        gScreenHeight = height;
        changed = true;
    }
    if (changed) boxSetText(gBoxSettings, NULL);
}

void EdenMessageSetBoxParams(const float width, const float padding)
{
    bool changed = false;
    if (gBoxSettings->boxWidth != width) {
        gBoxSettings->boxWidth = width;
        changed = true;
    }
    if (gBoxSettings->boxPaddingH != padding || gBoxSettings->boxPaddingV != padding) {
        gBoxSettings->boxPaddingH = padding;
        gBoxSettings->boxPaddingV = padding;
        changed = true;
    }
    if (changed) boxSetText(gBoxSettings, NULL);
}

void EdenMessageDraw(const int contextIndex, const float viewProjection[16])
{
    GLfloat boxcentrex, boxcentrey, boxwd2, boxhd2;
    GLfloat boxVertices[4][2];

    if (!gBoxSettings) return;

    // Check if we need to show a blinking cursor, and if so, what state it is in.
    if (gInput) {
        EdenMessageShow(EdenUIInputGetInputForDrawing(gInput));
    }
    
    boxcentrex = gScreenWidth / 2.0f;
    boxwd2 = gBoxSettings->boxWidth / 2;
    boxcentrey = gScreenHeight / 2.0f;
    boxhd2 = gBoxSettings->boxHeight / 2;
    boxVertices[0][0] = boxcentrex - boxwd2; boxVertices[0][1] = boxcentrey - boxhd2;
    boxVertices[1][0] = boxcentrex + boxwd2; boxVertices[1][1] = boxcentrey - boxhd2;
    boxVertices[2][0] = boxcentrex + boxwd2; boxVertices[2][1] = boxcentrey + boxhd2;
    boxVertices[3][0] = boxcentrex - boxwd2; boxVertices[3][1] = boxcentrey + boxhd2;

    // Draw box.
    pthread_mutex_lock(&gBoxSettings->lock);
	if (gBoxSettings->lineCount) {
        // Draw the semi-transparent black shaded box and white outline.
#if EDEN_USE_GL
        glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glStateCacheEnableBlend();
        glVertexPointer(2, GL_FLOAT, 0, boxVertices);
        glStateCacheEnableClientStateVertexArray();
        glStateCacheDisableClientStateNormalArray();
        glStateCacheClientActiveTexture(GL_TEXTURE0);
        glStateCacheDisableClientStateTexCoordArray();
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glStateCacheDisableBlend();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        
        EdenGLFontDrawBlock(contextIndex, NULL, (const unsigned char **)gBoxSettings->lines, gBoxSettings->lineCount, 0.0f, 0.0f, H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER, V_OFFSET_VIEW_CENTER_TO_TEXT_CENTER);
#elif EDEN_USE_GLES2
        glUseProgram(program);
        glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, viewProjection);
        glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glStateCacheEnableBlend();
        glStateCacheDisableDepthTest();
        glVertexAttribPointer(ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, boxVertices);
        glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
        glUniform4f(uniforms[UNIFORM_COLOR], 0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glStateCacheDisableBlend();
        glUniform4f(uniforms[UNIFORM_COLOR], 1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
        glDrawArrays(GL_LINE_LOOP, 0, 4);

        EdenGLFontDrawBlock(contextIndex, viewProjection, (const unsigned char **)gBoxSettings->lines, gBoxSettings->lineCount, 0.0f, 0.0f, H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER, V_OFFSET_VIEW_CENTER_TO_TEXT_CENTER);
#endif
    }
	pthread_mutex_unlock(&gBoxSettings->lock);

	return;
}

bool EdenMessageKeyboardRequired(void)
{
    if (!gInput) return (false);
    return (!EdenUIInputIsComplete(gInput));
}

bool EdenMessageInputKeyboard(const unsigned char keyAsciiCode)
{
    if (!gInput) return (false);
    if (EdenUIInputProcessKeystrokes(gInput, keyAsciiCode)) {
        if (EdenMessageShow(EdenUIInputGetInputForDrawing(gInput)) != EDEN_E_NONE) {
            return (false);
        }
    }
    return (true);
}
