/*
 *  log.h
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

/*!
	@file log.h
	@brief Logging utilities.
	@details
        Various routines to format and redirect log output.
	@Copyright 2015-2017 Daqri, LLC.
 */

#ifndef __ARUtil_log_h__
#define __ARUtil_log_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _WIN32 // errno is defined in stdlib.h on Windows.
#  ifdef EMSCRIPTEN // errno is not in sys/
#    include <errno.h>
#  else
#    include <sys/errno.h>
#  endif
#endif
#include <ARX/ARUtil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AR_LOG_LEVEL_DEBUG = 0,
    AR_LOG_LEVEL_INFO,
    AR_LOG_LEVEL_WARN,
    AR_LOG_LEVEL_ERROR,
    AR_LOG_LEVEL_REL_INFO
};
#define AR_LOG_LEVEL_DEFAULT AR_LOG_LEVEL_INFO

/*!
    @var int arLogLevel
    @brief   Sets the severity level. Log messages below the set severity level are not logged.
	@details
        All calls to artoolkitX's logging facility include a "log level" parameter, which specifies
        the severity of the log message. (The severities are defined in &lt;ARUtil/log.h&gt;.)
        Setting this global allows for filtering of log messages. All log messages lower than
        the set level will not be logged by arLog().
        Note that debug log messages created using the ARLOGd() macro will be logged only in
        debug builds, irrespective of the log level.
    @see arLog
*/
ARUTIL_EXTERN extern int arLogLevel;

/*!
    @brief   Write a string to the current logging facility.
	@details
        The default logging facility varies by platform, but on Unix-like platforms is typically
        the standard error file descriptor. However, logging may be redirected to some other
        facility by arLogSetLogger.

        Newlines are not automatically appended to log output.
    @param      tag A tag to supply to an OS-specific logging function to specify the source
        of the error message. May be NULL, in which case "libAR" will be used.
    @param      logLevel The severity of the log message. Defined in %lt;ARUtil/log.h&gt;.
        Log output is written to the logging facility provided the logLevel meets or
        exceeds the minimum level specified in global arLogLevel.
    @param      format Log format string, in the form of printf().
    @see arLogLevel
    @see arLogSetLogger
*/

ARUTIL_EXTERN void arLog(const char *tag, const int logLevel, const char *format, ...);
ARUTIL_EXTERN void arLogv(const char *tag, const int logLevel, const char *format, va_list ap);

typedef void (ARUTIL_CALLBACK *AR_LOG_LOGGER_CALLBACK)(const char *logMessage);

/*!
    @brief   Divert logging to a callback, or revert to default logging.
	@details
        The default logging facility varies by platform, but on Unix-like platforms is typically
        the standard error file descriptor. However, logging may be redirected to some other
        facility by this function.
    @param      callback The function which will be called with the log output, or NULL to
        cancel redirection.
    @param      callBackOnlyIfOnSameThread If non-zero, then the callback will only be called
        if the call to arLog is made on the same thread as the thread which called this function,
        and if the arLog call is made on a different thread, log output will be buffered until
        the next call to arLog on the original thread.

        The purpose of this is to prevent logging from secondary threads in cases where the
        callback model of the target platform precludes this.
    @see arLog
*/
ARUTIL_EXTERN void arLogSetLogger(AR_LOG_LOGGER_CALLBACK callback, int callBackOnlyIfOnSameThread);

#ifdef DEBUG
#  define ARLOGd(...) arLog(NULL, AR_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#  define ARLOGd(...)
#endif
#define ARLOGi(...) arLog(NULL, AR_LOG_LEVEL_INFO, __VA_ARGS__)
#define ARLOGw(...) arLog(NULL, AR_LOG_LEVEL_WARN, __VA_ARGS__)
#define ARLOGe(...) arLog(NULL, AR_LOG_LEVEL_ERROR, __VA_ARGS__)
#define ARLOGperror(s) arLog(NULL, AR_LOG_LEVEL_ERROR, ((s != NULL) ? "%s: %s\n" : "%s%s\n"), ((s != NULL) ? s : ""), strerror(errno))

#ifdef __cplusplus
}
#endif
#endif // !__ARUtil_log_h__
