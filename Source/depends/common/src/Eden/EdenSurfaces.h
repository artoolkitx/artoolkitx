/*
 *  EdenSurfaces.h
 *  The Eden Library
 *
 *	Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *
 *	Rev		Date		Who		Changes
 *	1.0.0	2001-11-11	PRL     Initial version for The SRMS simulator.
 *  1.1.0   2008-07-18  PRL     Repurposed for OpenGL ES.
 *  1.2.0   2010-09-22  PRL     Remove redundant static material handling.
 */

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

/*!
    @header EdenSurfaces
    @brief Texture loading, selection, and drawing.
    @details
        EdenSurfaces forms one part of the Eden library.
    @copyright 2001-2013 Philip Lamb
 */

#ifndef __EdenSurfaces_h__
#define __EdenSurfaces_h__

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================ *
 *	Includes
 * ============================================================================ */
#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif


#if EDEN_USE_GL3
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl3.h>
#  else
#    include <GL3/gl3.h>
#  endif
#elif EDEN_USE_GLES2
#  ifndef EDEN_IPHONEOS
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  else
#    include <OpenGLES/ES2/gl.h>
#    include <OpenGLES/ES2/glext.h>
#  endif
#else
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif


/* ============================================================================ *
 *	defines
 * ============================================================================ */
#if 0
// Symbolic names for faces of objects to texture.
#define TEXTURE_FACE_TOP_BIT	0x01
#define TEXTURE_FACE_RIGHT_BIT	0x02
#define TEXTURE_FACE_BACK_BIT	0x04
#define TEXTURE_FACE_LEFT_BIT	0x08
#define TEXTURE_FACE_FRONT_BIT	0x10
#define TEXTURE_FACE_BOTTOM_BIT	0x20
#endif

/*!
    @typedef
    @brief Scaling mode to use when drawing a texture on a quad.
    @constant STRETCH Match texture size to quad, stretching as necessary.
    @constant FILL Scale the texture so that its smallest side exactly fills
        the largest side of the quad. If the texture and the quad are not the same
        ratio, the texture will be cropped.
    @constant FIT Scale the texture so that it's largest side exactly fits in
        the smallest side of the quad. If the texture and the quad are not the same
        ratio, the texture will be surrounded by background texture (letterbox/pillarbox).
    @constant UNITY Draw the texture with one pixel per one OpenGL unit, without stretching.
        If the texture is larger than the quad, it will be cropped. If it is smaller, it will
        be surrounded by background texture.
 */
typedef enum {
    STRETCH,
    FILL,
    FIT,
    UNITY
} EDEN_TEXTURE_SCALING_MODE;

/*!
     @typedef
     @brief How to align the texture content when drawing a texture on a quad.
 */
typedef enum {
    CENTRE,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    LEFT,
    TOP,
    RIGHT,
    BOTTOM
} EDEN_TEXTURE_ALIGNMENT_MODE;

/* ============================================================================ *
*	Public types
* ============================================================================ */

typedef unsigned int TEXTURE_INDEX_t;

/*!
    @typedef
    @field pathname Pathname of the texture file to load.
    @field mipmaps  TRUE if mipmaps should be generated (e.g. if the texture will
        be drawn scaled to sizes smaller than unity), FALSE otherwise.
    @field internalformat OpenGL internal texture format to use.
        E.g. this allows a single-channel texture to be loaded as GL_ALPHA rather
        than GL_LUMINANCE. Typically GL_RGB or GL_RGBA.
    @field min_filter
    @field mag_filter
    @field wrap_s
    @field wrap_t
    @field priority
    @field env_mode
*/
typedef struct {
	const char *pathname;
	GLboolean	mipmaps;
	GLint		internalformat;
	GLint		min_filter;
	GLint		mag_filter;
	GLint		wrap_s;
	GLint		wrap_t;
	GLclampf	priority;
	GLint		env_mode;
	//GLfloat	env_color[4];
} TEXTURE_INFO_t;

/* ============================================================================ *
 *	Public functions
 * ============================================================================ */

/*!
    @brief Initialise the EdenSurfaces library.
    @details
        This function must be called prior to any other EdenSurfaces*() functions being called.
 
        Does NOT require that a valid OpenGL context be available.
    @param contextsActiveCount Maximum number of OpenGL contexts that will be in use. Typically 1.
    @param textureIndicesMax Maximum number of textures that can be loaded via calls to EdenSurfacesTextureLoad.
    @result TRUE if the function succeeded, FALSE otherwise.
 */
EDEN_BOOL EdenSurfacesInit(const int contextsActiveCount, const int textureIndicesMax);

/*!
    @brief Loads textures into OpenGL texture objects.
    @details
        Loads numTextures textures into OpenGL texture objects, for the specified OpenGL
        context (the first context has a contextIndex of 0).
    @param contextIndex The index of the OpenGL context into which this texture is being loaded.
        (The context must already be active.) Normally 0 if using only one context.
        May not be more than contextsActiveCount-1 (as passed to EdenSurfacesInit).
    @param numTextures The number of textures being loaded.
    @param textureInfo An array (of length numTextures) holding details of each texture
        being loaded.
 
        On OpenGL ES devices with limited non-power-of-two (NPOT) texture support,
        the NPOT support will only be available if {mipmaps = FALSE, min_filter/mag_filter
        = GL_NEAREST/GL_LINEAR, wrap_s/wrap_t = GL_CLAMP_TO_EDGE}.
    @param textureIndices
        After loading, the array textureIndices[] holds the index
        numbers assigned to each texture. These index numbers may then be used in calls to
        EdenSurfacesTextureSet() and EdenSurfacesTextureUnload(). (These indices are of type
        TEXTURE_INDEX_t, an unsigned int, greater than or equal to 1, and are only valid
        per-OpenGL context.)
        In the event of a loading error, the index value of any textures not loaded will
        be equal to 0.
    @param hasAlpha_out If the calling routine wishes to know if the returned texture contains
        an alpha channel it should supply a pointer to a char in parameter hasAlpha_out, and on
        return, this char will equal 0 if no alpha channel is present, or 1 otherwise. If this
        information is not needed, set hasAlpha_out to NULL
    @result TRUE if all textures were loaded correctly, FALSE if one or more were not.
 */
EDEN_BOOL EdenSurfacesTextureLoad(const int contextIndex, const int numTextures, const TEXTURE_INFO_t *textureInfo, TEXTURE_INDEX_t *textureIndices, char *hasAlpha_out);
    
EDEN_BOOL EdenSurfacesTextureLoad2(const int contextIndex, const int numTextures, const TEXTURE_INFO_t *textureInfo, TEXTURE_INDEX_t *textureIndices, char *hasAlpha_out, const EDEN_BOOL flipH, const EDEN_BOOL flipV);

/*!
    @brief Binds the OpenGL texture.
    @details
    @param textureIndex The texture bound is specified by parameter textureIndex.
        This index is only valid if the current OpenGL context is contextIndex, and the index
        was returned by EdenSurfacesTextureLoad() with the same OpenGL context.
    @param contextIndex The index of the OpenGL context with which this texture is being bound.
        (The context must already be active.) Normally 0 if using only one context.
        May not be more than contextsActiveCount-1 (as passed to EdenSurfacesInit).
 */
void EdenSurfacesTextureSet(const int contextIndex, TEXTURE_INDEX_t textureIndex);

/*!
    @brief Unloads textures and frees OpenGL texture objects.
    @details
        Unloads numTextures textures from OpenGL texture objects, for the specified OpenGL context.
    @param numTextures The number of textures being unloaded.
    @param textureIndices After unloading, the corresponding index in textureIndices for each
        record in textureInfo[] is set to 0.
    @param contextIndex The index of the OpenGL context from this texture is being unloaded.
        (The context must already be active.) Normally 0 if using only one context.
        May not be more than contextsActiveCount-1 (as passed to EdenSurfacesInit).
    @result TRUE if the function succeeded, FALSE otherwise.
 */
EDEN_BOOL EdenSurfacesTextureUnload(const int contextIndex, const int numTextures, TEXTURE_INDEX_t *textureIndices);

/*!
    @brief Finalise the EdenSurfaces library.
    @details 
        This function should be called once no more calls to any other EdenSurfaces*()
        functions are required, to free up allocated memory.
        Does NOT require that a valid OpenGL context be available.
    @result TRUE if the function succeeded, FALSE otherwise.
 */
EDEN_BOOL EdenSurfacesFinal(void);

/*!
    @brief Draw a texture on a quad using OpenGL.
    @details
        Draws the texture pointed to by index on a surface of size (width, height) OpenGL units,
        with the lower-left corner of the surface at the origin, and the surface lying in the X-Y plane.
 
        To draw at a different position and/or orientation precede with glTranslate/glRotate calls.
    @param width Width, in OpenGL units, of the quad on which to draw.
    @param height Height, in OpenGL units, of the quad on which to draw.
    @param scaleMode determines how the scale factor between pixels in the source
        image and OpenGL units on the surface.
    @param alignMode determines the placement of the image inside the surface.
    @result TRUE if the function succeeded, FALSE otherwise.
 */
EDEN_BOOL EdenSurfacesDraw(const int contextIndex, const TEXTURE_INDEX_t textureIndex, const int width, const int height, const EDEN_TEXTURE_SCALING_MODE scaleMode, const EDEN_TEXTURE_ALIGNMENT_MODE alignMode);

EDEN_BOOL EdenSurfacesDraw2(const int contextIndex, const TEXTURE_INDEX_t textureIndex, const int width, const int height, const float S0, const float S1, const float T0, const float T1);

/*!
    @brief   Check for the availability of an OpenGL extension.
    @details
        Provides the same functionality as the gluCheckExtension() function,
        since some platforms don't have GLU version 1.3 or later.
    @param      extName Name of the extension, e.g. "GL_EXT_texture".
    @param      extString The OpenGL extensions string, as returned by glGetString(GL_EXTENSIONS);
    @result     TRUE, if the extension is found, FALSE otherwise.
*/
GLboolean EdenGluCheckExtension(const GLubyte* extName, const GLubyte *extString);

/*!
    @brief   Checks for the presence of an OpenGL capability by version or extension.
    @details
        Checks for the presence of an OpenGL capability by version or extension.
        The test returns true if EITHER the OpenGL driver's OpenGL implementation
        version meets or exceeds a minimum value (passed in in minVersion) OR
        if an OpenGL extension identifier passed in as a character string
        is non-NULL, and is found in the current driver's list of supported extensions.
    @param      minVersion
        A binary-coded decimal (i.e. version 1.0 is represented as 0x0100) version number.
        If minVersion is zero, (i.e. there is no version of OpenGL with this extension in core)
        the version test will always fail, and the result will only be true if the extension
        string test passes.
    @param      extension A string with an extension name to search the drivers extensions
        string for. E.g. "GL_EXT_texture". If NULL, the extension name test will always fail,
        and the result will only be true if the version number test passes.
    @result     TRUE If either of the tests passes, or FALSE if both fail.
*/
int EdenGLCapabilityCheck(const unsigned short minVersion, const unsigned char *extension);

#ifdef __cplusplus
}
#endif

#endif		/* !__EdenSurfaces_h__ */
