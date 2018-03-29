//
//  EdenError.h
//
//  Copyright (c) 2004-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	
//	Rev		Date		Who		Changes
//	1.0.0	2004-06-01	PRL		Pulled together from other headers.
//
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
	@header EdenError
	@brief Error codes and error reporting for the Eden library.
	@version 1.0.0
	@updated 2004-06-01
	@details
*/

#ifndef __EdenError_h__
#define __EdenError_h__

// ============================================================================
//	Includes
// ============================================================================

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif
#include <stdio.h>				// stderr, fprintf(), vfprintf()

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//	Public defines
// ============================================================================

/*!
    @enum EDEN_E_*
    @brief Symbolic codes for errors reported by routines in the Eden Library.
	@details
		As the actual numerical values for certain errors may change from time
		to time, these symbolic codes should be used instead.
	@constant EDEN_E_NONE No error.
    @constant EDEN_E_GENERIC Error.
	@constant EDEN_E_OUT_OF_MEMORY Out of memory.
	@constant EDEN_E_OVERFLOW Overflow.
	@constant EDEN_E_NODATA Data was requested but none was available.
	@constant EDEN_E_IOERROR Error during input / output operation.
	@constant EDEN_E_EOF End of file reached.
	@constant EDEN_E_TIMEOUT Timed out.
	@constant EDEN_E_INVALID_COMMAND Invalid command.
	@constant EDEN_E_INVALID_ENUM Invalid enumeration.
	@constant EDEN_E_THREADS An error occured during a thread-management operation.
	@constant EDEN_E_FILE_NOT_FOUND File not found.
	@constant EDEN_E_LENGTH_UNAVAILABLE Length not available.
    @constant EDEN_E_GENERIC_TOOLBOX Error returned from System tool.
	@constant EDEN_E_LIBRARY_NOT_AVAILABLE A required library is not installed.
	@constant EDEN_E_LIBRARY_TOO_OLD The minimum version requirement of a library was not met.
	@constant EDEN_E_LIBRARY_TOO_NEW The maximum version requirement of a library was not met.
    @constant EDEN_E_HARDWARE_GENERIC Hardware error.
	@constant EDEN_E_HARDWARE_NOT_AVAILABLE Required hardware is not available.
	@constant EDEN_E_BIRD_CONFIGURATION The bird hardware is incorrectly configured.
	@constant EDEN_E_BIRD_PHASEERROR Data from the bird was lost and arrived from the bird out-of-phase.
    @constant EDEN_E_NET_GENERIC Network error.
	@constant EDEN_E_NET_NOT_AVAILABLE Network not available.
	@constant EDEN_E_NET_NOT_CONNECTED Network not connected.
 */
enum {
	EDEN_E_NONE					=    0,
	EDEN_E_GENERIC				=   -1,
	EDEN_E_OUT_OF_MEMORY		=   -2,
	EDEN_E_OVERFLOW				=   -3,
	EDEN_E_NODATA				=   -4,
	EDEN_E_IOERROR				=   -5,
	EDEN_E_EOF					=	-6,
	EDEN_E_TIMEOUT				=   -7,
	EDEN_E_INVALID_COMMAND		=   -8,
	EDEN_E_INVALID_ENUM			=   -9,
	EDEN_E_THREADS				=   -10,
	EDEN_E_FILE_NOT_FOUND		=   -11,
	EDEN_E_LENGTH_UNAVAILABLE	=	-12,
	EDEN_E_GENERIC_TOOLBOX		=   -10001,
	EDEN_E_LIBRARY_NOT_AVAILABLE =	-10002,
	EDEN_E_LIBRARY_TOO_OLD		=	-10003,
	EDEN_E_LIBRARY_TOO_NEW		=	-10004,
	EDEN_E_HARDWARE_GENERIC		=   -11001,
	EDEN_E_HARDWARE_NOT_AVAILABLE = -11002,
	EDEN_E_BIRD_CONFIGURATION	=   -11202,
	EDEN_E_BIRD_PHASEERROR		=   -11203,
	EDEN_E_NET_GENERIC			=   -12001,
	EDEN_E_NET_NOT_AVAILABLE	=	-12002,
	EDEN_E_NET_NOT_CONNECTED	=   -12003,
};

// ============================================================================
//	Public types.
// ============================================================================
/*!
	@typedef EDEN_E_t
	@brief Return type of routines in the Eden Library.
	@details
		Zero and positive values are used to report success, and possibly, return data.
		An error is indicated by a value less than zero. Symbolic values for the error
		codes are available.
*/
typedef signed int EDEN_E_t;

// ============================================================================
//	Public globals.
// ============================================================================

// ============================================================================
//	Public functions.
// ============================================================================
 
/*!
    @brief Produce an error string from an Eden Library error code.
    @details
		Produces a null-terminated ASCII string with a description of the error
		represented by the supplied code, followed by the code itself, in brackets.
		See also EdenError_strerror_r, EdenError_perror.
    @param code The code to produce the string for, as returned by various routines
		in the Eden Library.
    @result Pointer to a null-terminated constant character string.
    @see EdenError_strerror_r EdenError_strerror_r
 */
const char *EdenError_strerror(const EDEN_E_t code);

/*!
	@brief Produce an error string from an Eden Library error code.
	@details
		Produces an ASCII string with a description of the error
		corresponding to the supplied code, and copies up to buflen
		characters of the string into strerrbuf
		See also EdenError_strerror, EdenError_perror.
	@param code The code to produce the string for, as returned by various routines
		in the Eden Library.
	@param strerrbuf Buffer to copy the error string into.
	@param buflen Number of characters available in strerrbuf. The null
		terminator counts as one character.
	@result
		Returns 0 on success.
		If the error number is not recognized, returns EINVAL as a warning.
		If insufficient storage is provided in strerrbuf (as specified in buflen)
		to contain the error string, returns ERANGE and strerrbuf
		will contain an error message that has been truncated and NUL terminated
		to fit the length specified by buflen.
    @see EdenError_strerror EdenError_strerror
 */
int EdenError_strerror_r(const EDEN_E_t code, char *strerrbuf, const size_t buflen);

/*!
    @brief Print an Eden Library error message and code to stderr, with optional string.
    @details
		Finds the error message corresponding to the supplied code,
		and writes it, followed by a newline, to the standard error file
		descriptor.  If the argument string is non-NULL and does not point
		to the null character, this string is prepended to the message string
		and separated from it by a colon and space (": "); otherwise, only
		the error message string is printed.
	@param code The code to produce the string for, as returned by various routines
		in the Eden Library.
    @see EdenError_strerror EdenError_strerror
    @see EdenError_strerror_r EdenError_strerror_r
 */
void EdenError_perror(const EDEN_E_t code, const char *string);

#ifdef __cplusplus
}
#endif

#endif // !__EdenError_h__

