//
//  timers.cpp
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
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
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#include "timers.h"
#include "error.h"
#include "logger.h"

#ifdef _WIN32
#  include <sys/timeb.h>				// struct _timeb, _ftime
#else
#  include <sys/time.h>					// gettimeofday(), struct timeval
#endif

using namespace vision;

Timer::Timer()
: mStartTime(-1)
, mStopTime(-1) {}

Timer::~Timer() {}

void Timer::start() {
#ifdef _WIN32
	struct _timeb sys_time;
	_ftime(&sys_time);
	mStartTime = (double)sys_time.time + (double)sys_time.millitm*1e-3;
#else
    timeval t;
    gettimeofday(&t, NULL);
    mStartTime = (double)t.tv_sec + (double)t.tv_usec*1e-6;
#endif
}

void Timer::stop() {
    ASSERT(mStartTime >= 0, "Clock has not been started");
#ifdef _WIN32
	struct _timeb sys_time;
	_ftime(&sys_time);
	mStopTime = (double)sys_time.time + (double)sys_time.millitm*1e-3;
#else
	timeval t;
	gettimeofday(&t, NULL);
	mStopTime = (double)t.tv_sec + (double)t.tv_usec*1e-6;
#endif
}

double Timer::duration_in_seconds() const {
    ASSERT(mStartTime >= 0, "Clock has not been started");
    ASSERT(mStopTime >= 0, "Clock has not been stopped");
    return mStopTime - mStartTime;
}

double Timer::duration_in_milliseconds() const {
    return duration_in_seconds()*1000;
}

ScopedTimer::ScopedTimer(const char* str)
: mStr(str) {
    mTimer.start();
}

ScopedTimer::~ScopedTimer() {
    mTimer.stop();
    LOG_INFO("%s: %f ms", mStr.c_str(), mTimer.duration_in_milliseconds());
}