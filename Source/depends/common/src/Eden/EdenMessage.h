//
//  EdenMessage.h
//  The Eden Library
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

/*!
	@header EdenMessage
	@brief Send messages and show text-input dialog boxes on the screen.
	@version 1.0.0
	@updated 2018-05-11
	@details
	@copyright 2001-2018 Philip Lamb
 */

#ifndef __EdenMessage_h__
#define __EdenMessage_h__

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#include <Eden/EdenError.h>        // EDEN_E_t
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
	
// When set to true, signals that EdenMessageDraw() should be called each drawing frame.
extern bool gEdenMessageDrawRequired;

/*!
    @function 
    @brief   Initialise the message library.
    @details
		Must be called before any access to EdenMessage*() functions.
    @param      contextsActiveCount Maximum number of OpenGL contexts.
    @result     true if succcessful, false in case of error.
*/
bool EdenMessageInit(const int contextsActiveCount);

/*!
    @function 
    @brief   Finalise the message library.
    @details (description)
		Should be called after no more EdenMessage*() functions need be called.
	@result     true if succcessful, false in case of error.
*/
bool EdenMessageFinal(void);

/*!
    @function 
    @brief   Prepare a message for drawing on the screen.
    @details
		Chops up a message of not more than MESSAGE_LENGTH_MAX characters into lines to
		fit inside a shaded box. The maximum dimensions of the box are 50% of the
		screen width and 80% of the screen height. The box is padded left and right 4
		character widths and top and bottom one line width. The width of the screen
		must allow at least 20 characters to fit on each line. This routine only chops
		up the lines. The actual drawing into the framebuffer should done with calls to
		EdenMessageDraw().
		This function should only be used in single-threaded applications. In
		multi-threaded applications (in which a separate, blockable thread is
		available to handle this kind of messaging), the function EdenMessage() should
		be used instead, and it will automatically control showing and hiding of
		the text.
	@param      msg The text to place into the message box.
	@result     EDEN_E_NONE in case of no error, or an error code.
*/
EDEN_E_t EdenMessageShow(const unsigned char *msg);

/*!
    @function
	@brief   Prepare a text-input dialog for user interaction.
	@details
		Prepares a prompt message for drawing on the screen, and then place
		a cursor at the end of the message, and collects input keystrokes
		from the user until the user presses return.
	@param      prompt The message to show above and to the left of the user's input.
	@param      minLength Minimum number of characters the user must enter before the routine will
		allow the input to be terminated (with the return key.) The return is NOT
		included in this number.
	@param      maxLength Maximum number of characters the user may enter.
	@result     EDEN_E_NONE in case of no error, or an error code.
*/
EDEN_E_t EdenMessageInputShow(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, bool intOnly, bool fpOnly, bool alphaOnly);

/*!
    @function 
	@brief   Remove a message from drawing on the screen.
	@details
		Removes a message previously shown with EdenMessageShow() or
        EdenMessageInputShow() from the screen.
		This function should only be used in single-threaded applications. In
		multi-threaded applications (in which a separate, blockable thread is
		available to handle this kind of messaging), the function EdenMessage() or
        EdenMessageInput() should be used instead, and it will automatically control
        showing and hiding of the text.
	@result     EDEN_E_NONE in case of no error, or an error code.
*/
EDEN_E_t EdenMessageHide(void);

bool EdenMessageInputIsComplete(void);

/*!
    @function 
    @brief   Prepare a message for drawing on the screen.
    @details
		Prepares a message for drawing on the screen (by calling through
		to EdenMessageShow()), and waits the indicated number of seconds
		before hiding the message (by calling through to EdenMessageHide())
		The routine blocks during the wait. It is thus designed to be
		used in a multi-threaded application, where other operations,
		including the actual drawing, occur in a separate thread.
		Applications requiring that the routine routine immediately, such
		as single-threaded applications, should not call this routine, but
		should manually control the showing and hiding.
	@param      msg (description)
	@param      secs How many seconds to show the message for.
	@result     EDEN_E_NONE in case of no error, or an error code.
*/
EDEN_E_t EdenMessage(unsigned char *msg, const unsigned int secs);

/*!
    @function 
    @brief   Prepare a text-input dialog for user interaction.
    @details
		Prepares a prompt message for drawing on the screen, and then place
		a cursor at the end of the message, and collects input keystrokes
		from the user until the user presses return.
		The routine blocks during the wait. It is thus designed to be
		used in a multi-threaded application, where other operations,
		including the actual drawing and keystroke collection, occur in a
		separate thread.
		Applications requiring that the routine routine immediately, such
		as single-threaded applications, should not call this routine, but
		should manually control the showing and hiding.
	@param      prompt The message to show above and to the left of the user's input.
	@param      minLength Minimum number of characters the user must enter before the routine will
		allow the input to be terminated (with the return key.) The return is NOT
		included in this number.
	@param      maxLength Maximum number of characters the user may enter.
    @result     EDEN_E_NONE in case of no error, or an error code.
*/
EDEN_E_t EdenMessageInput(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, bool intOnly, bool fpOnly, bool alphaOnly);

/*!
    @brief Get the result of an input operation
    @details
        Gets the result of the input operation via a string copy.
    @result
        A string containing the content.  If the user has cancelled input
        with the ESC key, NULL will be returned. If the user has not entered
        any input, this will point to a null string.
 
        Note that free() must be called on the result of this function
        when the caller has finished with it, to prevent leaking of the result.
 */
unsigned char *EdenMessageInputGetInput(void);

/*!
    @brief Set the view size to use in calculations when aligning message box.
    @details
        The positioning of the message box is calculated each frame using
        the values supplied here.
    @param width Viewport width in pixels.
    @param height Viewport height in pixels.
 */
void EdenMessageSetViewSize(const float width, const float height);

void EdenMessageSetBoxParams(const float width, const float padding);
    
/*!
    @function 
    @brief   Draw previously-prepared text in a box onto the framebuffer.
    @details
		Draws a shaded white-outlined box box onto the screen and draws
		lines of text previously prepared with EdenMessageShow() into it.
		When the variable gEdenMessageDrawRequired is set, this function
		should be called once per frame, generally after all other
		drawing is complete.
 
        This function is typically called with the projection set to
        a 2D orthographic projection.
 
		This drawing call is required in order to see the output of the
		message routines, in both single- and multi-threaded applications.
	@param      contextIndex (description)
*/
void EdenMessageDraw(const int contextIndex, const float viewProjection[16]);

/*!
     @brief   EdenMessageInputKeyboard should be called with keystrokes if this is set to true
 */
bool EdenMessageKeyboardRequired(void);

/*!
    @function 
    @brief Pass user keystrokes to message library for use in dialogs.
    @details
		This function processes keystrokes for a message dialog.
		When the global variable gEdenMessageKeyboardRequired is set, any keystrokes collected
		from the user by the calling program should be passed to this function,
		which will absorb the keystroke and update the on-screen dialog box appropriately.
		This processing call is required in order to capture the user's input for the
		message routines, in both single- and multi-threaded applications.
	@param      keyAsciiCode The ASCII-encoded keystroke.
    @result     Returns true if the keystroke was processed successfully, false in case of error.
*/
bool EdenMessageInputKeyboard(const unsigned char keyAsciiCode);

#ifdef __cplusplus
}
#endif
#endif // !__EdenMessage_h__
