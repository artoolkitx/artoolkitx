/*
 *  cparamSearch.h
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef CPARAMSEARCH_H
#define CPARAMSEARCH_H

#include <ARX/ARVideo/video.h>

#if USE_CPARAM_SEARCH
#ifdef __cplusplus
extern "C" {
#endif

// cacheDir NULL to use current working directory for camera parameter database cache, otherwise path to folder in which to write.
// cacheInitDir NULL to look in current working directory for camera parameter database initial cache, otherwise path to folder in which to look.
// resetCache 1 to reset the cache to initial state, or 0 to use existing cache (if any).
// calibrationServerDownloadURL Full URL (including method) of calibration server download interface, or NULL to use default.
// calibrationServerAuthenticationToken Unencoded authentication token to be supplied to calibration server, or NULL to use default.
// Returns 0 if succesful, <0 if error.
int cparamSearchInit(const char *cacheDir, const char *cacheInitDir, int resetCache, const char *calibrationServerDownloadURL, const char *calibrationServerAuthenticationToken);

// Returns 0 if succesful, <0 if error.
int cparamSearchFinal(void);
    
// >= 0 is normal state.
// < 0 is error state.
typedef enum {
    CPARAM_SEARCH_STATE_INITIAL = 0,                          // The request has been received, but no action has been taken yet.
    CPARAM_SEARCH_STATE_IN_PROGRESS = 1,                      // The request has been received, and is being processed.
    CPARAM_SEARCH_STATE_RESULT_NULL = 2,                      // Request completed, but no cparam was available.
    CPARAM_SEARCH_STATE_OK = 3,                               // The request succeeded, a cparam was available and returned.
    CPARAM_SEARCH_STATE_FAILED_ERROR = -1,                    // The request failed because of some other unspecified error, typically a failure in the fetch module itself.
    CPARAM_SEARCH_STATE_FAILED_NO_NETWORK = -2,               // A network connection was required but was not available. The user should be asked to enable the network.
    CPARAM_SEARCH_STATE_FAILED_NETWORK_FAILED = -3,           // A network connection was required and available, but network I/O failed (e.g. connection dropped).
    CPARAM_SEARCH_STATE_FAILED_SERVICE_UNREACHABLE = -4,      // A network connection was required, and available, but the server could not be reached.
    CPARAM_SEARCH_STATE_FAILED_SERVICE_UNAVAILABLE = -5,      // The server reported itself temporarily unavailable. The search may be retried at a later time. Searches may be throttled.
    CPARAM_SEARCH_STATE_FAILED_SERVICE_FAILED = -6,           // The search failed due to an internal error in the server. The search may be retried at a later time. Searches may be throttled.
    CPARAM_SEARCH_STATE_FAILED_SERVICE_NOT_PERMITTED = -7,    // The search failed because access from this client is not permitted.
    CPARAM_SEARCH_STATE_FAILED_SERVICE_INVALID_REQUEST = -8   // The search failed because the server did not understand it. This should be considered a permanent failure.
} CPARAM_SEARCH_STATE;

// Type signature for a function which will be called while the fetch operation is progressing.
// May be called zero or more times with state=CPARAM_SEARCH_STATE_IN_PROGRESS and progress=[0.0,1.0].
// Any other state constitutes a final report and the end of the operation.
// If state=CPARAM_SEARCH_STATE_OK, the camera parameters are returned in *cparam, which should
// be considered valid only for the duration of the callback.
typedef void (*CPARAM_SEARCH_CALLBACK)(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata);

// Tell cparamSearch about the state of the Internet connection.
// -1 State of Internet connection unknown. cparamSearch will attempt to determine the state. This is the initial state.
//  0 Internet connection is down.
//  1 Internet connection is up.
int cparamSearchSetInternetState(int state);

// Normally returns CPARAM_SEARCH_STATE_INITIAL, and progress will be passed via the callback.
// If any other state is returned however, that is definitive, and the callback will never be called.
// The callback may occur on an arbitrary thread; ensure any variables it modifies are thread-safe.
CPARAM_SEARCH_STATE cparamSearch(const char *device_id, int camera_index, int width, int height, float focal_length, CPARAM_SEARCH_CALLBACK callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif // USE_CPARAM_SEARCH

#endif // !CPARAMSEARCH_H
