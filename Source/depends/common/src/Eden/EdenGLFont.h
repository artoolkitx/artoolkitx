//
//  EdenGLFont.h
//  The Eden Library
//
//  Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	Some portions of font loading code based on code by Jeff Molofee, 1999, http://nehe.gamedev.net/
//
//	Rev		Date		Who		Changes
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

// HeaderDoc documentation included. See http://developer.apple.com/darwin/projects/headerdoc/

/*!
    @header EdenGLFont
    @brief Handle font-related operations under OpenGL.
    @version 1.0.0
    @updated 2013-11-12
    @details
    @copyright 2001-2013 Philip Lamb
 */


#ifndef __EdenGLFont_h__
#define __EdenGLFont_h__

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//	Includes.
// ============================================================================
#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#include <Eden/EdenError.h>		// EDEN_E_t

// ============================================================================
//  Defines and types.
// ============================================================================
    
typedef struct _EDEN_GL_FONT_INFO_t EDEN_GL_FONT_INFO_t;

extern EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Stroke_Roman;
extern EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Stroke_MonoRoman;
extern EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Bitmap16_Geneva;
extern EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Bitmap16_OCR_B_10;

typedef enum {
    H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE,
    H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE,
    H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER
} H_OFFSET_TYPE;

typedef enum {
    V_OFFSET_TEXT_TOP_TO_VIEW_TOP,
    V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE,
    V_OFFSET_VIEW_CENTER_TO_TEXT_CENTER
} V_OFFSET_TYPE;

// ============================================================================
//  Functions.
// ============================================================================

/*!
    @function 
    @brief   Initialise the font library.
    @details
		Must be called before any access to EdenGLFont*() functions.
    @param      contextsActiveCount Maximum number of OpenGL contexts.
    @result     TRUE if succcessful, FALSE in case of error.
*/
EDEN_BOOL EdenGLFontInit(const int contextsActiveCount);

/*!
    @function 
    @brief   Finalise the font library.
    @details
		Should be called after no more EdenGLFont*() functions need be called.
	@result     TRUE if succcessful, FALSE in case of error.
*/
EDEN_BOOL EdenGLFontFinal(void);

/*!
    @function 
	@brief   Create a texture font for later use.
    @details
		Creates font info required to use the font contained in the texture file
        named at pathname.
 
		Each character in the texture occupies 16 pixels x 16 pixels. Its width
		and height should be stretched to 16 even if this introduces perspective
		distortion- this will guarantee the best onscreen resolution.
		Each character from the font will be scaled to naturalWidth pixels wide,
		naturalHeight pixels tall.
 
        Note that this function does not load the actual texture resources into
        OpenGL and does not require a valid OpenGL context. OpenGL resource
        loading is achieved by calling EdenGLFontLoadTextureFontForContext().
    @param      fontName A short descriptive name for the font, suitable for presentation to a user, e.g. in a font menu.
    @param      naturalHeight The height in pixels at which each character, drawn unscaled, will occupy.
    @param      naturalWidth The width in pixels at which each character, drawn unscaled, will occupy.
	@param      pathname Pathname (absolute, or relative to current working directory) to font texture file.
	@result     Pointer to a EDEN_GL_FONT_INFO_t opaque structure. This pointer can be used with other
        EdenGLFont functions just as if it was a built-in font.
    @see EdenGLFontDeleteTextureFont EdenGLFontDeleteTextureFont
    @see EdenGLFontLoadTextureFontForContext EdenGLFontLoadTextureFontForContext
*/
EDEN_GL_FONT_INFO_t *EdenGLFontNewTextureFont(const char *fontName, const char *pathname, const float naturalHeight, const float naturalWidth);

/*!
    @function 
    @brief   Delete a previously created texture font.
    @details (description)
    @param      fontInfo_p Pointer to a pointer to EDEN_GL_FONT_INFO_t opaque structure
        previously returned from EdenGLFontLoadBitmapFont(). On return, the location
        pointed to will be set to NULL.

        Note that this function does not unload the actual texture resources from
        OpenGL and does not require a valid OpenGL context. OpenGL resource
        loading is achieved by calling EdenGLFontLoadTextureFontForContext() PRIOR
        to calling this function.
    @see EdenGLFontNewTextureFont EdenGLFontNewTextureFont
    @see EdenGLFontUnloadTextureFontForContext EdenGLFontUnloadTextureFontForContext
*/
void EdenGLFontDeleteTextureFont(EDEN_GL_FONT_INFO_t **fontInfo_p);

/*!
    @function 
    @brief   Set the font to be used for subsequent draw calls.
    @details
        Drawing by default uses the font EDEN_GL_FONT_ID_Stroke_MonoRoman.
        This function changes the font for subsequent drawing calls.
    @param      font Pointer to a EDEN_GL_FONT_INFO_t opaque structure, either for one
        of the built-in fonts, or previously returned from EdenGLFontNewTextureFont()
    @see EdenGLFontNewTextureFont EdenGLFontNewTextureFont
    @see EdenGLFontLoadTextureFontForContext EdenGLFontLoadTextureFontForContext
 */
void EdenGLFontSetFont(EDEN_GL_FONT_INFO_t *font);

/*!
    @brief   Get the font to be used for subsequent draw calls.
    @details
    @result Pointer to a EDEN_GL_FONT_INFO_t opaque structure, either for one
        of the built-in fonts, or previously returned from EdenGLFontNewTextureFont()
 */
EDEN_GL_FONT_INFO_t * EdenGLFontGetFont(void);

/*!
    @brief Set the font size.
    @details 
    @param points Font size in points. Default is 16 point.
 */
void EdenGLFontSetSize(const float points);

/*!
    @brief Get the currently set font size.
    @details 
    @result Font size in points. Default is 16 point.
 */
float EdenGLFontGetSize(void);

void EdenGLFontSetColor(const float rgba[4]);

void EdenGLFontGetColor(float rgba[4]);

/*!
    @brief Set character spacing (i.e. "kerning").
    @details
        Character spacing is the spacing between the edges of adjacent characters.
    @param spacing Spacing expressed as a percentage of the font size. Default is 0.0625f (i.e. 1/16).
 */
void EdenGLFontSetCharacterSpacing(const float spacing);

/*!
    @brief Get character spacing (i.e. "kerning").
    @details
        Character spacing is the spacing between the edges of adjacent characters.
    @result Spacing expressed as a percentage of the font size. Default is 0.0625f (i.e. 1/16).
 */
float EdenGLFontGetCharacterSpacing(void);

/*!
    @brief Set line spacing (i.e. "leading").
    @details
        Default is  to 1.125f (i.e. 9/8).
    @param spacing Line spacing.
 */
void EdenGLFontSetLineSpacing(const float spacing);

/*!
    @brief Get line spacing (i.e. "leading").
    @details
        Default is  to 1.125f (i.e. 9/8).
    @result Line spacing.
 */
float EdenGLFontGetLineSpacing(void);

/*!
    @brief Set word spacing (i.e. adjust width of the space character).
    @details
        Word spacing is the spacing between adjacent words.

        This adjustment applies only to non-monospaced fonts.
    @param spacing Spacing expressed as a percentage of the normal width of the space character size. Default is 1.0.
 */
void EdenGLFontSetWordSpacing(const float spacing);

/*!
    @brief Get word spacing (i.e. adjust width of the space character).
    @details
        Word spacing is the spacing between adjacent words.
 
        This adjustment applies only to non-monospaced fonts.
    @result pacing expressed as a percentage of the normal width of the space character size. Default is 1.0.
 */
float EdenGLFontGetWordSpacing(void);

/*!
    @brief Set set display resolution.
    @details
        The display resolition is used to calculate font heights in pixels.
        Default is 72.0f, at which a 16-point font will occupy 16 pixels.
    @param pixelsPerInch Display resolution, in pixels per inch.
 */
void EdenGLFontSetDisplayResolution(const float pixelsPerInch); // Pixels per inch. Used to calculate font heights in pixels. Default is 72.0f, at which a 16-point font will occupy 16 pixels.

/*!
    @brief Get previously set display resolution.
    @details
        The display resolition is used to calculate font heights in pixels.
        Default is 72.0f, at which a 16-point font will occupy 16 pixels.
    @result Previously set display resolution, in pixels per inch.
 */
float EdenGLFontGetDisplayResolution(void); // Pixels per inch.

/*!
    @function 
    @brief   Tell the font library about the view size.
    @details
		Normally, this will be called during the windowing system's reshape callback.
    @param      widthInPixels Width of the view, in pixels.
	@param      heightInPixels Height of the view, in pixels.
*/
void EdenGLFontSetViewSize(const float widthInPixels, const float heightInPixels);
    
/*!
    @brief Gets the height (in pixels) of the currently selected font.
    @details 
        Takes into account font size (in points) and display resolution (in pixels per inch).
    @result Height in pixels.
 */
float EdenGLFontGetHeight(void);

/*!
    @brief Gets the width (in pixels) of a chracter of the currently selected font.
    @details
        Takes into account font size (in points) and display resolution (in pixels per inch).
        Does not include any inter-character spacing.
    @param c ASCII character code of the character. For texture fonts, the full 8 buts of c are considered.
    @result Width in pixels.
 */
float EdenGLFontGetCharacterWidth(const unsigned char c);

/*!
    @brief Gets the width (in pixels) of a line of characters of the currently selected font.
    @details
        Takes into account font size (in points) and display resolution (in pixels per inch).
        Includes inter-character spacing.
    @param line A null-terminated (C string) of the characters to measure.
    @result Width in pixels.
 */
float EdenGLFontGetLineWidth(const unsigned char *line);

/*!
    @brief Calculate the width of a block of text (i.e. the width of the widest line) in pixels.
    @details 
        Includes inter-character spacing.
    @param lines An array of C strings (i.e. an array of (char *)). Each item in the array points to a null-terminated C string.
    @param lineCount Number of strings in array 'lines'.
    @result Width in pixels.
 */
float EdenGLFontGetBlockWidth(const unsigned char **lines, const unsigned int lineCount);  // Returns width in pixels, taking into account font, font size, display resolution, and character spacing.

/*!
    @brief Calculate the height of a block of text in pixels.
    @details
        Includes effects of line spacing greater than 1.0.
    @param lines An array of C strings (i.e. an array of (char *)). Each item in the array points to a null-terminated C string.
    @param lineCount Number of strings in array 'lines'.
    @result Height in pixels.
 */
float EdenGLFontGetBlockHeight(const unsigned char **lines, const unsigned int lineCount);  // Returns height in pixels, taking into account font, font size, display resolution, and line spacing.

/*!
    @brief 
    @details
        Requires a valid OpenGL context at the time of the call.
        Make sure EdenSurfacesInit() has been previously called with a valid number of contexts.
    @param contextIndex
    @param fontInfo
    @result 
 */
EDEN_BOOL EdenGLFontSetupFontForContext(const int contextIndex, EDEN_GL_FONT_INFO_t *fontInfo);

/*!
    @brief 
    @details 
        Requires a valid OpenGL context at the time of the call.
    @param contextIndex
    @param fontInfo
    @result
 */
EDEN_BOOL EdenGLFontCleanupFontForContext(const int contextIndex, EDEN_GL_FONT_INFO_t *fontInfo);

/*!
    @function 
    @brief   Draw a single line of text into the framebuffer.
    @details
		Should be called once per frame, generally after all other drawing is complete.
        Requires a valid OpenGL context at the time of the call.

        Drawing functions respect current projection and modelview matrices,
        and depth mode and lighting/color settings, and these are unmodified on return.
        Typically however, the aim is to draw window-aligned text, and this function
        should be called with the projection matrix set to a 2D orthographic projection
        in window coordinates and with the modelview matrix set to identity, i.e.
        <pre>
        glMatrixMode(GL_PROJECTION);
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        </pre>

        Texture font use modifies blend settings, texturing enable settings,
        and vertex, texture coordinate pointers, and enabled client state of vertex, texture
        and normal arrays. It also leaves texture matrix indeterminate on return.
    @param      contextIndex (description)
    @param      line null-terminated string of characters to draw.
	@param      hOffset Horizontal offset (in OpenGL coordinates) between the reference points
        specified in hOffsetType.
 
		E.g. when hOffsetType = H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, passing 0.0f
        will place the text against the left-hand edge of the window.
	@param      vOffset Vertical offset (in OpenGL coordinates) between the reference points
        specified in vOffsetType.
 
		E.g. when vOffsetType = V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE, passing 0.0f
        will place the text against the bottom edge of the window.
    @param hOffsetType Specifies left, centered, or right alignment horizontal alignment.
    @param vOffsetType Specifies top, centered, or bottom alignment vertical alignment.
*/
void EdenGLFontDrawLine(const int contextIndex, const float viewProjection[16], const unsigned char *line, const float hOffset, const float vOffset, H_OFFSET_TYPE hOffsetType, V_OFFSET_TYPE vOffsetType);

/*!
    @function 
    @brief   Draw a block of multiple lines of text into the framebuffer.
    @details
		Should be called once per frame, generally after all other drawing is complete.
        Requires a valid OpenGL context at the time of the call.
 
        Drawing functions respect current projection and modelview matrices,
        and depth mode and lighting/color settings, and these are unmodified on return.
        Typically however, the aim is to draw window-aligned text, and this function
        should be called with the projection matrix set to a 2D orthographic projection
        in window coordinates and with the modelview matrix set to identity, i.e.
        <pre>
        glMatrixMode(GL_PROJECTION);
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        </pre>

        Texture font use modifies blend settings, texturing enable settings,
        and vertex, texture coordinate pointers, and enabled client state of vertex, texture
        and normal arrays. It also leaves texture matrix indeterminate on return.
	@param      contextIndex (description)
    @param      lines Array of null-terminated string of characters to draw.
    @param      lineCount Number of strings in array 'lines'.
    @param      hOffset Horizontal offset (in OpenGL coordinates) between the reference points
        specified in hOffsetType.
 
		E.g. when hOffsetType = H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, passing 0.0f
        will place the text against the left-hand edge of the window.
	@param      vOffset Vertical offset (in OpenGL coordinates) between the reference points
        specified in vOffsetType.
 
		E.g. when vOffsetType = V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE, passing 0.0f
        will place the text against the bottom edge of the window.
    @param hOffsetType Specifies left, centered, or right alignment horizontal alignment.
    @param vOffsetType Specifies top, centered, or bottom alignment vertical alignment.
*/
void EdenGLFontDrawBlock(const int contextIndex, const float viewProjection[16], const unsigned char **lines, const unsigned int lineCount, const float hOffset, const float vOffset, H_OFFSET_TYPE hOffsetType, V_OFFSET_TYPE vOffsetType);
    
#ifdef __cplusplus
}
#endif
#endif // !__EdenGLFont_h__
