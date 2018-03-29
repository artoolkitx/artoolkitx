/*
 * 1394-Based Digital Camera Control Library
 *
 * Error logging functions
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net> and
 *            Rudolf Leitgeb
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/***************************************************************************
 * logging facility for libdc1394
 *
 * These functions provide the logging of error, warning and debug messages
 * They allow registering of custom logging functions or the use
 * of the builtin loggers which redirect the output to stderr.
 * Three log levels are supported:
 * error:   Indicates that an error has been detected which mandates
 *          shutdown of the program as soon as feasible
 * warning: Indicates that something happened which prevents libdc1394
 *          from working but which could possibly be resolved by the
 *          application or the user: plugging in a camera, resetting the
 *          firewire bus, ....
 * debug:   A sort of way point for the library. This log level is supposed
 *          to report that a specific function has been entered or has
 *          passed a certain stage. This log level is turned off by default
 *          and may produce a lot of output during regular operation.
 *          The main purpose for this log level is for debugging libdc1394
 *          and for generating meaningful problem reports.
 ***************************************************************************/

#include <dc1394/dc1394.h>

#ifndef __DC1394_LOG_H__
#define __DC1394_LOG_H__

/*! \file dc1394/log.h
    \brief Functions to log errors, warning and debug messages
    \author Damien Douxchamps: coding
    \author Rudolf Leitgeb: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

/**
 * Error codes returned by most libdc1394 functions.
 *
 * General rule: 0 is success, negative denotes a problem.
 */
typedef enum {
    DC1394_SUCCESS                     =  0,
    DC1394_FAILURE                     = -1,
    DC1394_NOT_A_CAMERA                = -2,
    DC1394_FUNCTION_NOT_SUPPORTED      = -3,
    DC1394_CAMERA_NOT_INITIALIZED      = -4,
    DC1394_MEMORY_ALLOCATION_FAILURE   = -5,
    DC1394_TAGGED_REGISTER_NOT_FOUND   = -6,
    DC1394_NO_ISO_CHANNEL              = -7,
    DC1394_NO_BANDWIDTH                = -8,
    DC1394_IOCTL_FAILURE               = -9,
    DC1394_CAPTURE_IS_NOT_SET          = -10,
    DC1394_CAPTURE_IS_RUNNING          = -11,
    DC1394_RAW1394_FAILURE             = -12,
    DC1394_FORMAT7_ERROR_FLAG_1        = -13,
    DC1394_FORMAT7_ERROR_FLAG_2        = -14,
    DC1394_INVALID_ARGUMENT_VALUE      = -15,
    DC1394_REQ_VALUE_OUTSIDE_RANGE     = -16,
    DC1394_INVALID_FEATURE             = -17,
    DC1394_INVALID_VIDEO_FORMAT        = -18,
    DC1394_INVALID_VIDEO_MODE          = -19,
    DC1394_INVALID_FRAMERATE           = -20,
    DC1394_INVALID_TRIGGER_MODE        = -21,
    DC1394_INVALID_TRIGGER_SOURCE      = -22,
    DC1394_INVALID_ISO_SPEED           = -23,
    DC1394_INVALID_IIDC_VERSION        = -24,
    DC1394_INVALID_COLOR_CODING        = -25,
    DC1394_INVALID_COLOR_FILTER        = -26,
    DC1394_INVALID_CAPTURE_POLICY      = -27,
    DC1394_INVALID_ERROR_CODE          = -28,
    DC1394_INVALID_BAYER_METHOD        = -29,
    DC1394_INVALID_VIDEO1394_DEVICE    = -30,
    DC1394_INVALID_OPERATION_MODE      = -31,
    DC1394_INVALID_TRIGGER_POLARITY    = -32,
    DC1394_INVALID_FEATURE_MODE        = -33,
    DC1394_INVALID_LOG_TYPE            = -34,
    DC1394_INVALID_BYTE_ORDER          = -35,
    DC1394_INVALID_STEREO_METHOD       = -36,
    DC1394_BASLER_NO_MORE_SFF_CHUNKS   = -37,
    DC1394_BASLER_CORRUPTED_SFF_CHUNK  = -38,
    DC1394_BASLER_UNKNOWN_SFF_CHUNK    = -39
} dc1394error_t;
#define DC1394_ERROR_MIN  DC1394_BASLER_UNKNOWN_SFF_CHUNK
#define DC1394_ERROR_MAX  DC1394_SUCCESS
#define DC1394_ERROR_NUM (DC1394_ERROR_MAX-DC1394_ERROR_MIN+1)

/**
 * Types of logging messages
 *
 * Three types exist:
 * - ERROR for real, hard, unrecoverable errors that will result in the program terminating.
 * - WARNING for things that have gone wrong, but are not requiring a termination of the program.
 * - DEBUG for debug messages that can be very verbose but may help the developers to fix bugs.
 */
typedef enum {
    DC1394_LOG_ERROR=768,
    DC1394_LOG_WARNING,
    DC1394_LOG_DEBUG
} dc1394log_t;
#define DC1394_LOG_MIN               DC1394_LOG_ERROR
#define DC1394_LOG_MAX               DC1394_LOG_DEBUG
#define DC1394_LOG_NUM              (DC1394_LOG_MAX - DC1394_LOG_MIN + 1)

#if ! defined (_MSC_VER)
/* Error logging/checking macros. Logs an error string on stderr and exit current function
   if error is positive. Neg errors are messages and are thus ignored */

/* Some macros to log errors, etc... conditionally */
#define DC1394_WRN(err,message)                           \
  do {                                                    \
    if ((err>0)||(err<=-DC1394_ERROR_NUM))                \
      err=DC1394_INVALID_ERROR_CODE;                      \
                                                          \
    if (err!=DC1394_SUCCESS) {                            \
      dc1394_log_warning("%s: in %s (%s, line %d): %s\n", \
      dc1394_error_get_string(err),                       \
          __FUNCTION__, __FILE__, __LINE__, message);     \
    }                                                     \
  } while (0);

#define DC1394_ERR(err,message)                           \
  do {                                                    \
    if ((err>0)||(err<=-DC1394_ERROR_NUM))                \
      err=DC1394_INVALID_ERROR_CODE;                      \
                                                          \
    if (err!=DC1394_SUCCESS) {                            \
      dc1394_log_error("%s: in %s (%s, line %d): %s\n",   \
      dc1394_error_get_string(err),                       \
          __FUNCTION__, __FILE__, __LINE__, message);     \
      return;                                             \
    }                                                     \
  } while (0);

#define DC1394_ERR_RTN(err,message)                       \
  do {                                                    \
    if ((err>0)||(err<=-DC1394_ERROR_NUM))                \
      err=DC1394_INVALID_ERROR_CODE;                      \
                                                          \
    if (err!=DC1394_SUCCESS) {                            \
      dc1394_log_error("%s: in %s (%s, line %d): %s\n",   \
      dc1394_error_get_string(err),                       \
          __FUNCTION__, __FILE__, __LINE__, message);     \
      return err;                                         \
    }                                                     \
  } while (0);

#define DC1394_ERR_CLN(err,cleanup,message)               \
  do {                                                    \
    if ((err>0)||(err<=-DC1394_ERROR_NUM))                \
      err=DC1394_INVALID_ERROR_CODE;                      \
                                                          \
    if (err!=DC1394_SUCCESS) {                            \
      dc1394_log_error("%s: in %s (%s, line %d): %s\n",   \
      dc1394_error_get_string(err),                       \
          __FUNCTION__, __FILE__, __LINE__, message);     \
      cleanup;                                            \
      return;                                             \
    }                                                     \
  } while (0);

#define DC1394_ERR_CLN_RTN(err,cleanup,message)           \
  do {                                                    \
    if ((err>0)||(err<=-DC1394_ERROR_NUM))                \
      err=DC1394_INVALID_ERROR_CODE;                      \
                                                          \
    if (err!=DC1394_SUCCESS) {                            \
      dc1394_log_error("%s: in %s (%s, line %d): %s\n",   \
      dc1394_error_get_string(err),                       \
          __FUNCTION__, __FILE__, __LINE__, message);     \
      cleanup;                                            \
      return err;                                         \
    }                                                     \
  } while (0);


#endif /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * dc1394_log_register_handler: register log handler for reporting error, warning or debug statements
 * Passing NULL as argument turns off this log level.
 * @param [in] log_handler: pointer to a function which takes a character string as argument
 *             type: the type of log
 * @param [in] type: message type (\a debug, \a err or \a warning)
 * @param [in] message: log message
 */
dc1394error_t dc1394_log_register_handler(dc1394log_t type, void(*log_handler)(dc1394log_t type,
                                          const char *message, void* user), void* user);

/**
 * dc1394_log_set_default_handler: set the log handler to the default handler
 * At boot time, debug logging is OFF (handler is NULL). Using this function for the debug statements
 * will start logging of debug statements usng the default handler.
 */
dc1394error_t dc1394_log_set_default_handler(dc1394log_t type);

/**
 * dc1394_log_error: logs a fatal error condition to the registered facility
 * This function shall be invoked if a fatal error condition is encountered.
 * The message passed as argument is delivered to the registered error reporting
 * function registered before.
 * @param [in] format,...: error message to be logged, multiple arguments allowed (printf style)
 */
void dc1394_log_error(const char *format,...);

/**
 * dc1394_log_warning: logs a nonfatal error condition to the registered facility
 * This function shall be invoked if a nonfatal error condition is encountered.
 * The message passed as argument is delivered to the registered warning reporting
 * function registered before.
 * @param [in] format,...: warning message to be logged, multiple arguments allowed (printf style)
 */
void dc1394_log_warning(const char *format,...);

/**
 * dc1394_log_debug: logs a debug statement to the registered facility
 * This function shall be invoked if a debug statement is to be logged.
 * The message passed as argument is delivered to the registered debug reporting
 * function registered before ONLY IF the environment variable DC1394_DEBUG has been set before the
 * program starts.
 * @param [in] format,...: debug statement to be logged, multiple arguments allowed (printf style)
 */
void dc1394_log_debug(const char *format,...);

#ifdef __cplusplus
}
#endif

#endif
