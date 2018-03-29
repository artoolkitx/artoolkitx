/*
 *  profile.h
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef __ARUtil_profile_h__
#define __ARUtil_profile_h__

#include <ARX/ARUtil/types.h>

#define MAX_PROF_NUM     20

#ifdef __cplusplus
extern "C" {
#endif


/*!
    @brief   Reset profiling.
	@details
        The profiling facility provides a single profile per process.
        There are up to MAX_PROF_NUM timing buckets available, numbered 0 to MAX_PROF_NUM-1.
        Call this function before the first call to profileSet to have the first call to
        profileSet function set the initial time and timing bucket.
    @see profileSet
*/
ARUTIL_EXTERN void profileClear (void);

/*!
    @brief   Set profiling initial time and timing bucket, or add elapsed time to a bucket.
	@details
        This function sets a profiling timing point and selects the timing bucket.
        If being called for the first time, or the first time since profileClear was called,
        it sets the initial time, and subsequent calls will log time into the timing bucket n.
        On subsequent calls, the elapsed time since the last call will be added to the
        previously set timing bucket, and will again set the timing bucket.
    @param      n The timing bucket to add to on the next call.
    @see profileClear profilePrint
*/
ARUTIL_EXTERN void profileSet   (int n);

/*!
    @brief   Print all profiling buckets.
	@details
        Prints all non-zero timing buckets and the percentage time occupied and total time.
*/
ARUTIL_EXTERN void profilePrint (void);

#ifdef __cplusplus
}
#endif

#endif // !__ARUtil_profile_h__
