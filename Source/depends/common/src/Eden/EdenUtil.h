//
//  EdenUtil.h
//  The Eden Library
//
//  Various OS-related bits and pieces.
//
//  Copyright (c) 2004-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	
//	Rev		Date		Who		Changes
//	1.0.0	2004-04-23	PRL		Added functions for checking keyboard.
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

	@header EdenUtil
	@brief Miscellaneous utility routines.
	@details
		EdenUtil forms one part of the Eden library.
	@copyright 2004-2013 Philip Lamb
 */

#ifndef __EdenUtil_h__
#define __EdenUtil_h__

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
    @brief Do any required setup so that we can check for single keypresses.
    @result     (description)
*/
EDEN_BOOL EdenKeyboardHitSetup(void);

/*!
    @brief Check to see if the user has pressed a key on the keyboard.
    @result     (description)
*/
EDEN_BOOL EdenKeyboardHit(void);

/*!
    @brief Do any required cleanup from our use of single keypress detection.
    @result     (description)
*/
EDEN_BOOL EdenKeyboardHitCleanup(void);

/*!
    @brief Get file name from a path.
    @details Given a full or partial pathname passed in string path,
        returns a pointer to the first char of the filename
        portion of path.
	@param path Full or partial pathname.
    @result A pointer to the first char of the filename
        portion of path.
*/
char *EdenGetFileNameFromPath(const char *path);

/*!
    @brief Get file extension from a path.
    @details Given a full or partial pathname passed in string path,
        returns a string with the extension portion of path,
        i.e. the text after the rightmost '.' character, if any.
        If the filename contains no '.', NULL is returned.
	@param path Full or partial pathname.
    @param convertToLowercase If convertToLowercase is TRUE, uppercase
        ASCII characters in the extension will be converted to lowercase.
    @result A string with the extension portion of path.
        NB: The returned string must be freed by the caller.
*/
char *EdenGetFileExtensionFromPath(const char *path, int convertToLowercase);

/*!
    @brief Get directory name from a path.
    @details Given a full or partial pathname passed in string path,
        returns a string with the directory name portion of path.
        The string is not terminated by the directory separator.
	@param path Full or partial pathname.
    @result A string with the directory name portion of path.
        NB: The returned string must be freed by the caller.
*/
char *EdenGetDirectoryNameFromPath(const char *path);

/*!
    @brief Return the path to the current executable
    @details Return the path to the current executable, where such a
        linkage between path and execution exists and is readable.
    @result NULL if the path could not be determined,
        otherwise, the path as a NULL-terminated dynamically-allocated
        string. N.B.: free() MUST be called on the returned string
        when it is finished with.
*/
char *EdenGetExecutablePath(void);
    

/*!
    @brief Get a path as a file URI.
    @details Given a full or partial pathname passed in string path,
        returns a string with the file URI for that path.
 
        Partial pathnames are handled by concatening with the
        process's current working directory.
	@param path Full or partial pathname.
 
        On Windows, both partial pathnames, full pathnames including
        the drive letter, or UNC pathnames (beginning with "\\" are
        all OK.
    @result A string with the the file URI for that path.
        NB: The returned string must be freed by the caller (by
        calling free() once its use is complete).
*/
char *EdenGetFileURI(const char *path);

#ifdef __cplusplus
}
#endif

#endif	/* !__EdenUtil_h__ */
