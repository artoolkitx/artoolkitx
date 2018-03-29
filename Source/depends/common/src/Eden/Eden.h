//
//  Eden.h
//
//	Compiler and platform specific constants, needed for entire projects.
//  This file should be included with the project as a prefix header
//  (i.e., implicitly included from every source file.)
//
//  Copyright (c) 2001-2016 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
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

#ifndef __Eden_h__
#define __Eden_h__

//
// Keep it simple: define compiler, platform and architecture capabilities here
// rather than using a configure script.
//

#include <stdio.h>
#ifndef _WIN32 // errno is defined in stdlib.h on Windows.
#  include <sys/errno.h>
#endif
#ifdef __ANDROID__
#  include <android/log.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ANDROID__
#  define EDEN_LOG(...)  __android_log_print(ANDROID_LOG_INFO, "libeden", __VA_ARGS__);
#  define EDEN_LOGe(...) __android_log_print(ANDROID_LOG_ERROR, "libeden", __VA_ARGS__);
#  define EDEN_LOGperror(s) __android_log_print(ANDROID_LOG_ERROR, "libeden", (s ? "%s: %s\n" : "%s%s\n"), (s ? s : ""), strerror(errno))
#else
#  define EDEN_LOG(...)  printf(__VA_ARGS__)
#  define EDEN_LOGe(...) fprintf(stderr, __VA_ARGS__)
#  define EDEN_LOGperror(s) fprintf(stderr, (s ? "%s: %s\n" : "%s%s\n"), (s ? s : ""), strerror(errno))
#endif

// Check architecture endianess using gcc's macro, or assume little-endian by default.
// Also, define attribute to produce a tightly packed structure (i.e. all structures
// byte-aligned.)
// I don't know how to do this in compilers other than GCC at the moment.
#if defined(__GNUC__)
#  if defined(__BIG_ENDIAN__)
#    define EDEN_BIGENDIAN  // Most Significant Byte has greatest address in memory.
#  endif
#  define EDEN_INLINE_H extern inline
#  define EDEN_INLINE_C
#  define EDEN_PACKED __attribute__ ((__packed__))
#else
#  define EDEN_PACKED
#endif

// GCC on Mac OS X.
#if defined(__APPLE__)
#  define EDEN_UNIX
#  define	EDEN_HAVE_PTHREAD_RELATIVE_TIMEDWAIT
#  include <TargetConditionals.h>
#  include <AvailabilityMacros.h>
#  if TARGET_RT_BIG_ENDIAN
#    define  EDEN_BIGENDIAN  // Most Significant Byte has greatest address in memory (ppc).
#  elif TARGET_RT_LITTLE_ENDIAN
#    undef   EDEN_BIGENDIAN
#  else
#    error
#  endif

#  if TARGET_OS_IPHONE
#    define EDEN_IPHONEOS
#    define EDEN_USE_GL 0
#    define EDEN_USE_GLES2 1
#    define EDNE_USE_GL3 0
#  else
#    define EDEN_USE_GL 1
#    define EDEN_USE_GLES2 0
#    define EDEN_USE_GL3 0
#  endif

#  if !defined(DARWINONLY) && !TARGET_OS_IPHONE
#    define EDEN_MACOSX	// Running under Mac OS X.
//#    define EDEN_HAVE_HID		// Has HID API available (for joystick).
//#    define EDEN_HAVE_ARTOOLKIT
#    define EDEN_HAVE_MACOSX_CGL
#    if !defined(__LP64__)
#      define EDEN_HAVE_CARBON
#    endif
#  endif

#  define EDEN_HAVE_LIBJPEG
#  define EDEN_HAVE_OPENAL

// GCC on Android (NDK)
#elif defined(__ANDROID__)
#  define EDEN_UNIX
#  define EDEN_HAVE_LIBJPEG
#  ifndef ANDROID
#    define ANDROID
#  endif
#  define EDEN_USE_GL 0
#  define EDEN_USE_GLES2 1
#  define EDEN_USE_GL3 0

// GCC on Cygnus GNU for Windows.
#elif defined(__CYGWIN__)
#  define EDEN_UNIX		// Its a Unix system too!
#  define EDEN_SERIAL_POSIX_ONLY // Use only POSIX-compliant serial calls.
#  define EDEN_USE_GL 1
#  define EDEN_USE_GLES2 0
#  define EDEN_USE_GL3 0

// GCC on Linux.
#elif defined(__linux__)	
#  define EDEN_UNIX		// Its a Unix-like system.
//#  define EDEN_HAVE_ARTOOLKIT
#  ifdef __arm__
#    define EDEN_USE_GL 0
#    define EDEN_USE_GLES2 1
#    define EDEN_USE_GL3 0
#  else
#    define EDEN_USE_GL 1
#    define EDEN_USE_GLES2 0
#    define EDEN_USE_GL3 0
#  endif

// GCC on NetBSD.
#elif defined(__NetBSD__)
#  define EDEN_UNIX
#  define EDEN_USE_GL 1
#  define EDEN_USE_GLES2 0
#  define EDEN_USE_GL3 0

// Microsoft C++ on Windows.
#elif defined(_MSC_VER)		
#  include <windows.h>	// Is this correct?
#  undef EDEN_BIGENDIAN	// Least Significant Byte is highest in memory.
#  define EDEN_HAVE_LIBJPEG
#  define EDEN_HAVE_OPENAL
//#  define EDEN_HAVE_ARTOOLKIT
#  pragma warning (disable:4068)	// Disable bogus unknown pragma warnings.
#  pragma warning (disable:4244)	// Disable bogus conversion warnings.
#  pragma warning (disable:4305)	// Disable bogus conversion warnings.
#  define EDEN_INLINE_H __inline
#  define EDEN_INLINE_C __inline
#  define EDEN_USE_GL 1

// Generic POSIX-compliant Unix.
#elif defined(_POSIX)
#  define EDEN_UNIX
#  define EDEN_SERIAL_POSIX_ONLY // Use only POSIX-compliant serial calls.
#  define EDEN_INLINE_H
#  define EDEN_INLINE_C
#  define EDEN_USE_GL 1
#  define EDEN_USE_GLES2 0
#  define EDEN_USE_GL3 0

#else
#  error Unrecognised compiler in __FILE__.
#endif


//
// Application code which is dependent on platform capabilities.
//

// Features common to all our supported Unix platforms.
#ifdef EDEN_UNIX
#  define EDEN_HAVE_STRINGS_H
#  define EDEN_HAVE_CTIME_R_IN_TIME_H
#endif // EDEN_UNIX

//
// Miscellany.
//

// Use these definitions rather than those in <stdbool.h>.
typedef int EDEN_BOOL;
#define EDEN_BOOL_DEFINED
#ifndef TRUE
#  define TRUE 1
#else
#  if (TRUE != 1)
#    error 'TRUE incorrectly defined somewhere other than __FILE__.'
#  endif
#endif
#ifndef FALSE
#  define FALSE 0
#else
#  if (FALSE != 0)
#    error 'FALSE incorrectly defined somewhere other than __FILE__.'
#  endif
#endif
	
// ASCII keycodes.
#define EDEN_ASCII_ESC	27
#define EDEN_ASCII_TAB	9
#define EDEN_ASCII_BS	8
#define EDEN_ASCII_CR	13
#define EDEN_ASCII_DEL	127
	
	
#ifdef __cplusplus
}
#endif

#endif		// !__Eden_h__

