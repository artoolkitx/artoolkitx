//
//  EdenTime.c
//
//  Copyright (c) 2001-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	
//	Rev		Date		Who		Changes
//	1.0.0	20011126	PRL		Initial version for The SRMS simulator.
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

// ============================================================================
//	Includes
// ============================================================================
#include <Eden/EdenTime.h>
#include <stdio.h>						// NULL, sprintf()
#include <time.h>						// ctime(), time_t
#include <string.h>						// strncpy()
#if defined(EDEN_UNIX)
#  include <sys/time.h>					// gettimeofday(), struct timeval
#  include <unistd.h>					// sleep(), usleep()
#elif defined(_WIN32)
#  include <windows.h>					// FILETIME, GetSystemTimeAsFileTime(), <winbase.h> Sleep()
#elif defined(EDEN_MACOS)
#  include <Timer.h>					// <CoreServices/CarbonCore/Timer.h> Microseconds()
#  include <OSUtils.h>					// <CoreServices/CarbonCore/OSUtils.h> Delay()
#else
#  include <GL/glut.h>
#endif

// ============================================================================
//	Private defines
// ============================================================================
#ifdef _WIN32
#  define FILETIME_TO_EPOCH_OFFSET (((LONGLONG)27111902 << 32) + (LONGLONG)3577643008)
#endif // _WIN32

// ============================================================================
//	Global variables
// ============================================================================

// ============================================================================
//	Private functions
// ============================================================================

// ============================================================================
//	Public functions
// ============================================================================


//
//  Get time in fractional seconds.
//  Supported via system calls on Unix, WIN32, and MacOS.
//  Other systems which support GLUT are also supported (with lesser accuracy.)
//
double EdenTimeInSeconds(void)
{
#if defined(EDEN_UNIX)
	struct timeval tv;  // Seconds and microseconds since Jan 1, 1970.
#elif defined(_WIN32)
	FILETIME ft;	// Hundreds of nanoseconds since Jan 1, 1601.
#elif defined(EDEN_MACOS)
	UnsignedWide _time;
#else
	int ms;
#endif

	
#if defined(EDEN_UNIX)
	gettimeofday(&tv, NULL);
	return ((double)tv.tv_sec + (double)tv.tv_usec * 0.000001);
#elif defined(_WIN32)
	GetSystemTimeAsFileTime(&ft);
	return ((double)(*((LONGLONG *)&ft) - FILETIME_TO_EPOCH_OFFSET) * 0.0000001);
#elif defined(EDEN_MACOS)
	Microseconds(&_time);
	return (4294.967296 * (double)_time.hi + 0.000001 * (double)_time.lo); // 2^32 = 4294967296.
#else
	ms = glutGet(GLUT_ELAPSED_TIME);
	return ((double)ms / 1000.0);
#endif
}

//
//  Get an absolute time in seconds and nanoseconds elapsed since
//  epoch (Jan 1, 1970), add parameter 'microseconds' microseconds
//  and return in a timespec structure, suitable e.g. for passing to
//  functions such as pthread_cond_timedwait().
//  Supported via system calls on Unix, and WIN32.
//
void EdenTimeAbsolutePlusOffset(struct timespec *result, const long microseconds)
{
	long overflow;
#if defined(EDEN_UNIX)
	struct timeval tv;  // Seconds and microseconds elapsed since Jan 1, 1970.
#elif defined(_WIN32)
	FILETIME ft;	// Number of hundred-nanosecond intervals elapsed since Jan 1, 1601.
	LONGLONG epocht; // Number of hundred-nanosecond intervals elapsed since Jan 1, 1970.
#else
#  error "Don't know how to get the time on this platform."
#endif
	
#if defined(EDEN_UNIX)
	gettimeofday(&tv, NULL);
	result->tv_sec = (long)tv.tv_sec;
	result->tv_nsec = ((long)tv.tv_usec + microseconds) * 1000l;
#elif defined(_WIN32)
	GetSystemTimeAsFileTime(&ft);
	epocht = *((LONGLONG *)&ft) - FILETIME_TO_EPOCH_OFFSET;
	result->tv_sec = (long)(epocht / (LONGLONG)10000000l); // 10e6.
	result->tv_nsec = (long)(epocht - ((LONGLONG)(result->tv_sec) * (LONGLONG)10000000l)) * 100l + microseconds * 1000l;
#endif
	overflow = result->tv_nsec / 1000000000l; // 1e9.
	if (overflow) {
		result->tv_sec += overflow;
		result->tv_nsec -= overflow * 1000000000l; // 1e9.
	}	
}

//
//  Put character string with a human-readable representation of the
//  time passed in parameter 'seconds' into 's'. Returns 's'.
//
char *EdenTimeInSecondsToText(const double seconds, char s[25])
{
	static char buf[64];
#if defined(EDEN_UNIX) || defined(_WIN32)
	time_t time;
#endif

#if defined(EDEN_UNIX) || defined(_WIN32)
	// Get 24-char-wide time & date string, plus \n and \0 for total of 26 bytes.
	time = (time_t)seconds; // Truncate to integer.
#ifdef EDEN_HAVE_CTIME_R_IN_TIME_H
	ctime_r(&time, buf);   // Use reentrant ctime if it's available.
#else
	strcpy(buf, ctime(&time));
#endif // EDEN_HAVE_CTIME_R_IN_TIME_H
	buf[24] = '\0'; // Nuke the newline.
#else
	// No way of knowing what seconds is measured relative to.
	// so just write seconds as number out to string to 3 decimal places.
	sprintf(buf, "%.3f");
#endif // EDEN_UNIX || _WIN32
	strncpy(s, buf, 24);
	s[24] = '\0';   // Make sure that s is null-terminated even if buf was 24 or more characters long.
	return (s);
}

#ifndef _WINRT
void EdenTime_usleep(const unsigned int microseconds)
{
#if defined(EDEN_UNIX)
	usleep(microseconds);
#elif defined(_WIN32)
	Sleep((DWORD)(microseconds/1000u));
#elif defined(EDEN_MACOS)
	UInt32 finalCount;
	Delay((unsigned long)((float)microseconds*(60.0f/1000000.0f)), &finalCount);
#else
#  error sleep not defined on this platform.
#endif
}
#endif // !_WINRT

void EdenTime_sleep(const unsigned int seconds)
{
#if defined(EDEN_UNIX)
	sleep(seconds);
#elif defined(_WIN32)
	Sleep((DWORD)(seconds*1000u));
#elif defined(EDEN_MACOS)
	UInt32 finalCount;
	Delay((unsigned long)(seconds*60u), &finalCount);
#else
#  error sleep not defined on this platform.
#endif
}
