/*
 *  thread_sub_winrt.h
 *  artoolkitX
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
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef THREAD_SUB_WINRT_H
#define THREAD_SUB_WINRT_H

#include <ARX/ARUtil/thread_sub.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

// Include Windows API.
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif
#include <sdkddkver.h> // Minimum supported version. See http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745.aspx
#include <windows.h>

// Define _WINRT for support Windows Runtime platforms.
#if defined(WINAPI_FAMILY)
#  if (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#    if (_WIN32_WINNT >= 0x0603) // (_WIN32_WINNT_WINBLUE)
#      define _WINRT
#    else
#      error artoolkitX for Windows Phone requires Windows Phone 8.1 or later. Please compile with Visual Studio 2013 or later with Windows Phone 8.1 SDK installed and with _WIN32_WINNT=0x0603 in your project compiler settings (setting /D_WIN32_WINNT=0x0603).
#    endif
#  elif (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP)
#    if (_WIN32_WINNT >= 0x0603) // (_WIN32_WINNT_WINBLUE)
#      define _WINRT
#    else
#      error artoolkitX for Windows Store requires Windows 8.1 or later. Please compile with Visual Studio 2013 or later with Windows 8.1 SDK installed and with _WIN32_WINNT=0x0603 in your project compiler settings (setting /D_WIN32_WINNT=0x0603).
#    endif
#  endif
#endif

#ifdef _WINRT
    
ARUTIL_EXTERN int arCreateDetachedThreadWinRT(void *(*start_routine)(THREAD_HANDLE_T*), THREAD_HANDLE_T*flag);

#endif // _WIN32
#endif // _WINRT

#ifdef __cplusplus
}
#endif
#endif // !THREAD_SUB_WINRT_H
