/*
 *  profile.c
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
 *  Author(s): Hirokazu Kato
 *
 */

#ifdef __ANDROID__
#  include <jni.h>
#  include <android/log.h>
#endif
#include <stdio.h>
#ifdef _WIN32
#  include <sys/timeb.h>
#else
#  include <sys/time.h>
#endif
#include <ARX/ARUtil/profile.h>

#ifdef __ANDROID__
#  define LOG(...)  __android_log_print(ANDROID_LOG_INFO, "libARUtil", __VA_ARGS__)
#  define LOGe(...) __android_log_print(ANDROID_LOG_ERROR, "libARUtil", __VA_ARGS__)
#else
#  define LOG(...)  printf(__VA_ARGS__)
#  define LOGe(...) fprintf(stderr, __VA_ARGS__)
#endif

long profTable[MAX_PROF_NUM];
int  profCount[MAX_PROF_NUM];

static long s;
static int us, num = -1;

void profileClear(void)
{
	num = -1;
}

void profileSet(int n)
{
    int i;
#ifdef _WIN32
    struct _timeb sys_time;

    _ftime_s(&sys_time);
    if(num != -1) {
		profTable[num] += ((long)sys_time.time - s) * 1000000L + sys_time.millitm * 1000L - us;
        profCount[num]++;
    }
#else
    struct timeval     time;

#  if defined(__linux) || defined(__APPLE__) || defined(EMSCRIPTEN)
    gettimeofday( &time, NULL );
#  else
    gettimeofday( &time );
#  endif
    if(num != -1) {
        profTable[num] += (time.tv_sec  - s) * 1000000L + time.tv_usec - us; // Add elapsed microseconds to previously set profile,
        profCount[num]++; // and count one more measurement.
    }
#endif
    else {
        for(i = 0; i < MAX_PROF_NUM; i++) {
            profTable[i] = 0;
            profCount[i] = 0;
        }
    }

#ifdef _WIN32
    s = (long)sys_time.time;
    us = sys_time.millitm * 1000L;
#else
    s = time.tv_sec;
    us = time.tv_usec;
#endif
    num = n;
}

void profilePrint(void)
{
    int i;
    double sum = 0;

    LOG("\n=== PROFILE RESULT ===\n");
    for(i = 0; i < MAX_PROF_NUM; i++) {
        sum += profTable[i];
    }

    for(i = 0; i < MAX_PROF_NUM; i++) {
        if(profTable[i] == 0) {
            continue;
        }
        LOG("REGION %2d : %6.3lf%% (%lf sec)\n", i,
                profTable[i] / sum * 100.0, profTable[i] / profCount[i] / 1000000.0);
    }
}

