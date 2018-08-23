//
//  EdenUIInput.h
//  The Eden Library
//
//  Copyright (c) 2001-2018 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
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
	@header EdenUIInput
	@brief Text-input.
	@version 1.0.0
	@updated 2018-05-11
	@details
	@copyright 2001-2018 Philip Lamb
 */

#ifndef __EdenUIInput_h__
#define __EdenUIInput_h__

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EdenUIInput *EdenUIInput_t;

/*!
    @brief Create a new instance of input.
    @details
		Call to create an instance of the input processing.
	@param      prompt The message to show above and to the left of the user's input.
	@param      minLength Minimum number of characters the user must enter before the routine will
		allow the input to be terminated (with the return key.) The return is NOT
		included in this number.
	@param      intOnly If true, only keystrokes 0-9 will be accepted.
	@param      fpOnly If true, only keystrokes 0-9 and . will be accepted.
	@param      alphaOnly If true, only keystrokes A-Z and a-z will be accepted.
	@param      maxLength Maximum number of characters the user may enter.
    @result Pointer to an instance of opaque struct _EdenUIInput if succcessful, NULL in case of error.
 */
EdenUIInput_t EdenUIInputNew(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, bool intOnly, bool fpOnly, bool alphaOnly);

/*!
    @brief Process keystrokes for input
    @details
		This function processes keystrokes for input.
		When the global variable gEdenUIInputActive is set, any keystrokes collected
		from the user by the calling program should be passed to this function,
		which will absorb the keystroke and update the input buffer accordingly.
	@param      keyAsciiCode The ASCII-encoded keystroke.
    @result     Returns true if the user should update their copy of the input buffer by
        calling EdenUIInputGetInputForDrawing, or false otherwise.
*/
bool EdenUIInputProcessKeystrokes(EdenUIInput_t input, const unsigned char keyAsciiCode);

/*!
    @brief   Get the current input string, including prompt, input, and cursor.
    @details
		Every time the input string is to be drawn, this function should be
		called to get the updated input.
	@result C string containing the current prompt, input, and cursor. Points to
	    memory allocated internally and so should be copied if needed.
*/
unsigned char *EdenUIInputGetInputForDrawing(EdenUIInput_t input);

/*!
    @brief Determine if keystroke input is complete and the result can be read.
 */    
bool EdenUIInputIsComplete(EdenUIInput_t input);

/*!
    @brief   Blocking wait for input to become complete.
    @details
		The routine blocks until input is complete. It is thus designed to be
		used in a multi-threaded application, where other operations,
		including the actual drawing and keystroke collection, occur in a
		separate thread.
 */
void EdenUIInputWaitComplete(EdenUIInput_t input);

/*!
    @brief Get the result of an input operation
    @details
        Gets the result of the input operation via a string copy.
    @result
        A string containing the content.  If the user has cancelled input
        with the ESC key, NULL will be returned. If the user has not entered
        any input, this will instead point to a null string.
 
        Note that free() must be called on the result of this function
        when the caller has finished with it, to prevent leaking of the result.
 */
unsigned char *EdenUIInputGetInput(EdenUIInput_t input);

/*!
    @brief Delete an instance of input.
    @details
		Call to destroy an instance of the input processing.
	@param input_p Pointer to pointer to an instance of opaque struct _EdenUIInput.
	    The pointed-to location will be set to NULL on return.
 */
void EdenUIInputDelete(EdenUIInput_t *input_p);

#ifdef __cplusplus
}
#endif
#endif // !__EdenUIInput_h__
