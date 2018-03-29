/*
 *  log.c
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/ARUtil/log.h>

#ifndef _WIN32
#  include <pthread.h> // pthread_self(), pthread_equal()
#  ifdef __ANDROID__
#    include <android/log.h>
#  endif
#  ifdef __APPLE__
#    include <os/log.h>
#  endif
#else
#  include <Windows.h>
#  define snprintf _snprintf
#endif

//
// Global required for logging functions.
//
int arLogLevel = AR_LOG_LEVEL_DEFAULT;
static AR_LOG_LOGGER_CALLBACK arLogLoggerCallback = NULL;
static int arLogLoggerCallBackOnlyIfOnSameThread = 0;
#ifndef _WIN32
static pthread_t arLogLoggerThread;
#else
static DWORD arLogLoggerThreadID;
#endif
#define AR_LOG_WRONG_THREAD_BUFFER_SIZE 4096
static char *arLogWrongThreadBuffer = NULL;
static size_t arLogWrongThreadBufferSize = 0;
static size_t arLogWrongThreadBufferCount = 0;


void arLogSetLogger(AR_LOG_LOGGER_CALLBACK callback, int callBackOnlyIfOnSameThread)
{
    arLogLoggerCallback = callback;
    arLogLoggerCallBackOnlyIfOnSameThread = callBackOnlyIfOnSameThread;
    if (callback && callBackOnlyIfOnSameThread) {
#ifndef _WIN32
        arLogLoggerThread = pthread_self();
#else
        arLogLoggerThreadID = GetCurrentThreadId();
#endif
		if (!arLogWrongThreadBuffer) {
            if ((arLogWrongThreadBuffer = malloc(sizeof(char) * AR_LOG_WRONG_THREAD_BUFFER_SIZE))) {
                arLogWrongThreadBufferSize = AR_LOG_WRONG_THREAD_BUFFER_SIZE;
            }
		}
    } else {
		if (arLogWrongThreadBuffer) {
			free(arLogWrongThreadBuffer);
			arLogWrongThreadBuffer = NULL;
			arLogWrongThreadBufferSize = 0;
		}
	}
}

void arLog(const char *tag, const int logLevel, const char *format, ...)
{
    if (logLevel < arLogLevel) return;
    if (!format || !format[0]) return;
    
    va_list ap;
    va_start(ap, format);
    arLogv(tag, logLevel, format, ap);
    va_end(ap);
}

void arLogv(const char *tag, const int logLevel, const char *format, va_list ap)
{
    va_list ap2;
    char *buf = NULL;
    size_t len;
    const char *logLevelStrings[] = {
        "debug",
        "info",
        "warning",
        "error"
    };
    const size_t logLevelStringsCount = (sizeof(logLevelStrings)/sizeof(logLevelStrings[0]));
    size_t logLevelStringLen;

    if (logLevel < arLogLevel) return;
    if (!format || !format[0]) return;

    // Count length required to unpack varargs.
    va_copy(ap2, ap);
#ifdef _WIN32
    len = _vscprintf(format, ap);
#else
    len = vsnprintf(NULL, 0, format, ap2);
#endif
    va_end(ap2);
    if (len < 1) return;
    
    // Add characters required for logLevelString.
    if (logLevel >= 0 && logLevel < (int)logLevelStringsCount) {
        logLevelStringLen = 3 + strlen(logLevelStrings[logLevel]); // +3 for brackets and a space, e.g. "[debug] ".
    } else {
        logLevelStringLen = 0;
    }
    
    buf = (char *)malloc((logLevelStringLen + len + 1) * sizeof(char)); // +1 for nul-term.
    
    if (logLevelStringLen > 0) {
        snprintf(buf, logLevelStringLen + 1, "[%s] ", logLevelStrings[logLevel]);
    }
    
    vsnprintf(buf + logLevelStringLen, len + 1, format, ap);
    len += logLevelStringLen;
    
    if (arLogLoggerCallback) {
        
        if (!arLogLoggerCallBackOnlyIfOnSameThread) {
            (*arLogLoggerCallback)(buf);
        } else {
#ifndef _WIN32
            if (!pthread_equal(pthread_self(), arLogLoggerThread))
#else
            if (GetCurrentThreadId() != arLogLoggerThreadID)
#endif
            {
                // On non-log thread, put it into buffer if we can.
                if (arLogWrongThreadBuffer && (arLogWrongThreadBufferCount < arLogWrongThreadBufferSize)) {
                    if (len <= (arLogWrongThreadBufferSize - (arLogWrongThreadBufferCount + 4))) { // +4 to reserve space for "...\0".
                        strncpy(&arLogWrongThreadBuffer[arLogWrongThreadBufferCount], buf, len + 1);
                        arLogWrongThreadBufferCount += len;
                    } else {
                        strncpy(&arLogWrongThreadBuffer[arLogWrongThreadBufferCount], "...", 4);
                        arLogWrongThreadBufferCount = arLogWrongThreadBufferSize; // Mark buffer as full.
                    }
                }
            } else {
                // On log thread, print buffer if anything was in it, then the current message.
                if (arLogWrongThreadBufferCount > 0) {
                    (*arLogLoggerCallback)(arLogWrongThreadBuffer);
                    arLogWrongThreadBufferCount = 0;
                }
                (*arLogLoggerCallback)(buf);
            }
        }
        
    } else {
#if defined(__ANDROID__)
        int logLevelA;
        switch (logLevel) {
            case AR_LOG_LEVEL_REL_INFO:         logLevelA = ANDROID_LOG_ERROR; break;
            case AR_LOG_LEVEL_ERROR:            logLevelA = ANDROID_LOG_ERROR; break;
            case AR_LOG_LEVEL_WARN:             logLevelA = ANDROID_LOG_WARN;  break;
            case AR_LOG_LEVEL_INFO:             logLevelA = ANDROID_LOG_INFO;  break;
            case AR_LOG_LEVEL_DEBUG: default:   logLevelA = ANDROID_LOG_DEBUG; break;
        }
        __android_log_write(logLevelA, (tag ? tag : "libAR"), buf);
        //#elif defined(_WINRT)
        //            OutputDebugStringA(buf);
#elif defined(__APPLE__)
        if (os_log_create == NULL) { // os_log only available macOS 10.12 / iOS 10.0 and later.
            fprintf(stderr, "%s", buf);
        } else {
            os_log_type_t type;
            switch (logLevel) {
                case AR_LOG_LEVEL_REL_INFO:         type = OS_LOG_TYPE_DEFAULT; break;
                case AR_LOG_LEVEL_ERROR:            type = OS_LOG_TYPE_ERROR; break;
                case AR_LOG_LEVEL_WARN:             type = OS_LOG_TYPE_DEFAULT;  break;
                case AR_LOG_LEVEL_INFO:             type = OS_LOG_TYPE_INFO;  break;
                case AR_LOG_LEVEL_DEBUG: default:   type = OS_LOG_TYPE_DEBUG; break;
            }
            os_log_with_type(OS_LOG_DEFAULT, type, "%{public}s", buf);
        }
#else
        fprintf(stderr, "%s", buf);
#endif
    }
    free(buf);
}
