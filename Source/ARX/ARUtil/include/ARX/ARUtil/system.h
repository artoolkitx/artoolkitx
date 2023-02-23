/*
 *  system.h
 *  artoolkitX
 *
 *  Functions to query various system-related parameters.
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef __ARUtil_system_h__
#define __ARUtil_system_h__

#include <ARX/ARUtil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
    @brief   Get a string holding a descriptive name of the current operating system.
    @details
        For applications requiring some information on operating system, this function
        provides the type of the operating system currently executing. It is defined
        to be one of the following values: "macos", "ios", "linux", "android", "windows",
        or "unknown" if the current OS cannot be determined.
    @result     A c-string holding a descriptive name of the current operating system.
        It is the responsibility of the caller to dispose of the string (by calling free()).
 */
ARUTIL_EXTERN char *arUtilGetOSName(void);

/*!
    @brief   Get a string holding a descriptive name of current operating system version.
    @details
        For applications requiring some information on operating system, this function
        provides the version of the operating system currently executing. It is determined
        in an operating system-dependent manner.
 
        If the version cannot be determined, the string "unknown" will be returned.
    @result     A c-string holding a descriptive name of the current operating system version.
        It is the responsibility of the caller to dispose of the string (by calling free()).
 */
ARUTIL_EXTERN char *arUtilGetOSVersion(void);

/*!
    @brief   Get a string holding a descriptive name of the current CPU type.
    @details
        For applications requiring some information on CPU type, this function
        provides the type of the CPU currently executing. It is determined in an
        operating system-dependent manner, and thus the results may not be directly
        comparable between different operating systems running on the same
        hardware.
 
        If the CPU type cannot be determined, the string "unknown" will be returned.
    @result     A c-string holding a descriptive name of the current CPU type.
        It is the responsibility of the caller to dispose of the string (by calling free()).
 */
ARUTIL_EXTERN char *arUtilGetCPUName(void);

/*!
 @brief   Get the full pathname of the code module in which this function exists.
 @details
    Gets the full filesystem path of the module in which this function is running.
    This is typically either an executable path or a dynamic library path.
 @result
    Returns A null-terminated string with the filesystem path. The returned value
    is malloc()ed internally and must be free()d by the caller. NULL in case of error.
 */
ARUTIL_EXTERN char *arUtilGetModulePath(void);

#ifdef __cplusplus
}
#endif
#endif // !__ARUtil_system_h__
