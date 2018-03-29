/*
 *  time.h
 *  artoolkitX
 *
 *  Time-related functions.
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

#ifndef __ARUtil_time_h__
#define __ARUtil_time_h__

#include <stdint.h>
#include <ARX/ARUtil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
    @brief Get the time in seconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC).
    @param sec Pointer to an unsigned 64-bit interger which will be filled with the seconds portion of the time, or NULL if this value is not required.
    @param sec Pointer to an unsigned 32-bit interger which will be filled with the microseconds portion the time or NULL if this value is not required.
        N.B.: The resolution of the returned time is system-specific, and is not guaranteed to have microsecond-resolution.
 */
ARUTIL_EXTERN void arUtilTimeSinceEpoch(uint64_t *sec, uint32_t *usec);

/*!
    @brief Read the timer.
    @return Elapsed seconds since last invocation of arUtilTimerReset().
    @see arUtilTimerReset
 */
ARUTIL_EXTERN double arUtilTimer(void);

/*!
    @brief Reset the timer.
    @see arUtilTimer
 */
ARUTIL_EXTERN void arUtilTimerReset(void);

#ifndef _WINRT
/*!
    @brief   Relinquish CPU to the system for specified number of milliseconds.
    @details
        This function calls the native system-provided sleep routine to relinquish
        CPU control to the system for the specified time.
    @param      msec Sleep time in milliseconds (thousandths of a second).
    @since Not available on Windows Runtime (WinRT).
 */
ARUTIL_EXTERN void arUtilSleep( int msec );
#endif // !_WINRT


#ifdef __cplusplus
}
#endif
#endif // !__ARUtil_time_h__
