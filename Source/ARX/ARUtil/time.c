/*
 *  time.c
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

#define _POSIX_C_SOURCE 199309L // nanosleep is posix, not C99.

#include <ARX/ARUtil/time.h>
#ifdef _WIN32
#  include <windows.h>
#  include <sys/timeb.h>
#else
#  include <time.h>
#  include <sys/time.h>
#endif

void arUtilTimeSinceEpoch(uint64_t *sec, uint32_t *usec)
{
#ifdef _WIN32
    struct _timeb sys_time;

    _ftime_s(&sys_time);
    if (sec) *sec = (uint64_t)sys_time.time;
    if (usec) *usec = (uint32_t)sys_time.millitm * 1000u;
#else
    struct timeval     time;

#  if defined(__linux) || defined(__APPLE__) || defined(EMSCRIPTEN)
    gettimeofday(&time, NULL);
#  else
    gettimeofday(&time);
#  endif
    if (sec) *sec = (uint64_t)time.tv_sec;
    if (usec) *usec = (uint32_t)time.tv_usec;
#endif
}

static long ss = 0;
static int sms = 0;

double arUtilTimer(void)
{
    double             tt;
    long               s1;
    int                s2;
#ifdef _WIN32
    struct _timeb sys_time;

    _ftime_s(&sys_time);
    s1 = (long)sys_time.time  - ss;
    s2 = sys_time.millitm - sms;
#else
    struct timeval     time;

#  if defined(__linux) || defined(__APPLE__) || defined(EMSCRIPTEN)
    gettimeofday( &time, NULL );
#  else
    gettimeofday( &time );
#  endif
    s1 = time.tv_sec - ss;
    s2 = time.tv_usec/1000 - sms;
#endif

    tt = (double)s1 + (double)s2 / 1000.0;

    return( tt );
}

void arUtilTimerReset(void)
{
#ifdef _WIN32
    struct _timeb sys_time;

    _ftime_s(&sys_time);
    ss  = (long)sys_time.time;
    sms = sys_time.millitm;
#else
    struct timeval     time;

#  if defined(__linux) || defined(__APPLE__) || defined(EMSCRIPTEN)
    gettimeofday( &time, NULL );
#  else
    gettimeofday( &time );
#  endif
    ss  = time.tv_sec;
    sms = time.tv_usec / 1000;
#endif
}

#ifndef _WINRT
void arUtilSleep( int msec )
{
#ifndef _WIN32
    struct timespec  req;

    req.tv_sec = 0;
    req.tv_nsec = msec * 1000000;
    nanosleep( &req, NULL );
#else
	Sleep( msec );
#endif

    return;
}
#endif

