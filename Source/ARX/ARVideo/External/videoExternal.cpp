/*
 *  videoExternal.cpp
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
 *  Copyright 2023 Philip Lamb
 *
 *  Author(s): Philip Lamb
 *
 */


#include "videoExternal.h"

#ifdef ARVIDEO_INPUT_EXTERNAL

#include <string.h> // memset()
#include <pthread.h>
#include <ARX/ARUtil/time.h>
#include <ARX/ARVideo/videoRGBA.h>
#include "../cparamSearch.h"
#if ARX_TARGET_PLATFORM_ANDROID
#  include <ARX/ARUtil/android.h>
#endif

typedef enum {
    ARVideoExternalIncomingPixelFormat_UNKNOWN,
    ARVideoExternalIncomingPixelFormat_NV21,
    ARVideoExternalIncomingPixelFormat_NV12,
    ARVideoExternalIncomingPixelFormat_RGBA,
    ARVideoExternalIncomingPixelFormat_RGB_565,
    ARVideoExternalIncomingPixelFormat_YUV_420_888,
    ARVideoExternalIncomingPixelFormat_MONO // Not official Android.
} ARVideoExternalIncomingPixelFormat;

struct _AR2VideoParamExternalT {
    int                width;
    int                height;
    AR_PIXEL_FORMAT    pixelFormat;
    int                bufWidth;
    int                bufHeight;
    int                convertToRGBA;
    float              focal_length; // metres.
    int                cameraIndex; // 0 = first camera, 1 = second etc.
    int                cameraPosition; // enum AR_VIDEO_POSITION_*
    char              *device_id;
    void             (*cparamSearchCallback)(const ARParam *, void *);
    void              *cparamSearchUserdata;
    AR2VideoBufferT    buffer;
    bool               native;
    bool               capturing; // Between capStart and capStop.
    pthread_mutex_t    frameLock;  // Protects: capturing, pushInited, pushNewFrameReady.
    void             (*callback)(void *);
    void              *userdata;
    bool               pushInited; // videoPushInit called.
    bool               openingAsync; // true when openAsync is active. If set to false, indicates video closed and it will cleanup.
    pthread_cond_t     pushInitedCond; // At least one frame received.
    bool               pushNewFrameReady; // New frame ready since last arVideoGetImage.
    ARVideoExternalIncomingPixelFormat incomingPixelFormat;
};

static void cleanupVid(AR2VideoParamExternalT *vid);
static void *openAsyncThread(void *arg);

int ar2VideoDispOptionExternal( void )
{
    ARPRINT(" -module=External\n");
    ARPRINT("\n");
    ARPRINT(" -format=[0|RGBA].\n");
    ARPRINT("    Specifies the pixel format for output images.\n");
    ARPRINT("    0=use system default. RGBA=output RGBA, including conversion if necessary.\n");
    ARPRINT(" -prefer=(any|exact|closestsameaspect|closestpixelcount|sameaspect|\n");
    ARPRINT(" -cachedir=/path/to/cparam_cache.db\n");
    ARPRINT("    Specifies the path in which to look for/store camera parameter cache files.\n");
    ARPRINT("    Default is app's cache directory, or on Android a folder 'cparam_cache' in the current working directory.\n");
    ARPRINT(" -cacheinitdir=/path/to/cparam_cache_init.db\n");
    ARPRINT("    Specifies the path in which to look for/store initial camera parameter cache file.\n");
    ARPRINT("    Default is app's bundle directory, or on Android a folder 'cparam_cache' in the current working directory.\n");
    ARPRINT("\n");

    return 0;
}

AR2VideoParamExternalT *ar2VideoOpenAsyncExternal(const char *config, void (*callback)(void *), void *userdata)
{
    char                     *cacheDir = NULL;
    char                     *cacheInitDir = NULL;
    char                     *csdu = NULL;
    char                     *csat = NULL;
    AR2VideoParamExternalT   *vid;
    const char               *a;
    char                      line[1024];
    int err_i = 0;
    int i;
    int width = 0, height = 0;
    int convertToRGBA = 0;

    arMallocClear(vid, AR2VideoParamExternalT, 1);

    a = config;
    if (a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;
            if (strcmp(line, "-module=External") == 0) {
            } else if (strncmp(line, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &width) == 0) {
                    ARLOGe("Error: Configuration option '-width=' must be followed by width in integer pixels.\n");
                    err_i = 1;
                }
            } else if (strncmp(line, "-height=", 8) == 0) {
                if (sscanf(&line[8], "%d", &height) == 0) {
                    ARLOGe("Error: Configuration option '-height=' must be followed by height in integer pixels.\n");
                    err_i = 1;
                }
            } else if (strncmp(line, "-format=", 8) == 0) {
                if (strcmp(line+8, "0") == 0) {
                    convertToRGBA = 0;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(line+8, "RGBA") == 0) {
                    convertToRGBA = 1;
                    ARLOGi("Requesting images in RGBA format.\n");
                } else {
                    ARLOGe("Ignoring unsupported request for conversion to video format '%s'.\n", line+8);
                }
            } else if (strncmp(a, "-cachedir=", 10) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 10; // Skip "-cachedir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cachedir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheDir);
                    cacheDir = strdup(line);
                }
            } else if (strncmp(a, "-cacheinitdir=", 14) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 14; // Skip "-cacheinitdir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cacheinitdir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheInitDir);
                    cacheInitDir = strdup(line);
                }
            } else if (strncmp(a, "-csdu=", 6) == 0) {
                // Attempt to read in download URL.
                a += 6; // Skip "-csdu=" characters.
                sscanf(a, "%s", line);
                free(csdu);
                if (!strlen(line)) {
                    csdu = NULL;
                } else {
                    csdu = strdup(line);
                }
            } else if (strncmp(a, "-csat=", 6) == 0) {
                // Attempt to read in authentication token, allowing for quoting of whitespace.
                a += 6; // Skip "-csat=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                free(csat);
                if (!strlen(line)) {
                    csat = NULL;
                } else {
                    csat = strdup(line);
                }
            } else {
                 err_i = 1;
            }

            if (err_i) {
                ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionExternal();
                goto bail;
            }

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    // Check for option compatibility.
    if (width != 0 || height != 0) {
        ARLOGw("Warning: Video frame size is determined by pushed video. Configuration options '-width=' and '-height=' will be ignored.\n");
    }

#if USE_CPARAM_SEARCH
    // Initialisation required before cparamSearch can be used.
#if !ARX_TARGET_PLATFORM_ANDROID
    if (!cacheDir) {
        cacheDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR);
    }
    if (!cacheInitDir) {
        cacheInitDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST); // Bundle dir on iOS and macOS, exe dir on windows, exedir/../share/exename/ on Linux, cache dir on Android.
    }
#  endif
    // Initialisation required before cparamSearch can be used.
    if (cparamSearchInit(cacheDir ? cacheDir : "cparam_cache", cacheInitDir ? cacheInitDir : "cparam_cache", false, csdu, csat) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };
#endif

    // Initial state.
    vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_UNKNOWN;
    vid->pixelFormat = AR_PIXEL_FORMAT_INVALID;
    vid->convertToRGBA = convertToRGBA;
    if (!vid->focal_length) vid->focal_length = AR_VIDEO_EXTERNAL_FOCAL_LENGTH_DEFAULT;
    vid->pushInited = false;
    vid->openingAsync = false;
    vid->capturing = false;
    vid->pushNewFrameReady = false;
    vid->cameraIndex = -1;
    vid->cameraPosition = AR_VIDEO_POSITION_UNKNOWN;
    vid->callback = callback;
    vid->userdata = userdata;

#if ARX_TARGET_PLATFORM_ANDROID
    vid->device_id = (char *)calloc(1, PROP_VALUE_MAX*3+2); // From <sys/system_properties.h>. 3 properties plus separators.
    // In lieu of identifying the actual camera, we use manufacturer/model/board to identify a device,
    // and assume that identical devices have identical cameras.
    // Handset ID, via <sys/system_properties.h>.
    int len;
    len = android_system_property_get(ANDROID_OS_BUILD_MANUFACTURER, vid->device_id); // len = (int)strlen(device_id).
    vid->device_id[len] = '/';
    len++;
    len += android_system_property_get(ANDROID_OS_BUILD_MODEL, vid->device_id + len);
    vid->device_id[len] = '/';
    len++;
    len += android_system_property_get(ANDROID_OS_BUILD_BOARD, vid->device_id + len);
#else
    vid->device_id = NULL;
#endif

    pthread_mutex_init(&(vid->frameLock), NULL);
    pthread_cond_init(&(vid->pushInitedCond), NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1); // Preclude the need to do pthread_join on the thread after it exits.
    pthread_t t;
    vid->openingAsync = true;
    err_i = pthread_create(&t, &attr, openAsyncThread, vid);
    pthread_attr_destroy(&attr);
    if (err_i != 0) {
        ARLOGe("ar2VideoOpenAsyncExternal(): pthread_create error %s (%d).\n", strerror(err_i), err_i);
        goto bail1;
    }

    goto done;

bail1:
#if USE_CPARAM_SEARCH
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
#endif
    pthread_cond_destroy(&vid->pushInitedCond);
    pthread_mutex_destroy(&vid->frameLock);
bail:
    free(vid->device_id);
    free(vid);
    vid = NULL;

done:
    free(cacheDir);
    free(cacheInitDir);
    free(csdu);
    free(csat);
    return (vid);
}

// Wait for ar2VideoPushInitExternal to be called before
// invoking user's callback, so that frame parameters (w, h etc) are known.
static void *openAsyncThread(void *arg)
{
    int err;

    AR2VideoParamExternalT *vid = (AR2VideoParamExternalT *)arg; // Cast the thread start arg to the correct type.

    pthread_mutex_lock(&(vid->frameLock));
    while (!vid->pushInited && vid->openingAsync) {
        // Android "Bionic" libc doesn't implement cancelation, so need to let wait expire somewhat regularly.
        uint64_t sec;
        uint32_t usec;
        arUtilTimeSinceEpoch(&sec, &usec);
        struct timespec ts;
        ts.tv_sec = sec + 2;
        ts.tv_nsec = usec * 1000;
        err = pthread_cond_timedwait(&(vid->pushInitedCond), &(vid->frameLock), &ts);
        if (err != ETIMEDOUT && err != 0) {
            ARLOGe("openAsyncThread(): pthread_cond_timedwait error %s (%d).\n", strerror(err), err);
            break;
        }
    }
    pthread_mutex_unlock(&(vid->frameLock));

    if (!vid->openingAsync) {
        cleanupVid(vid);
    } else {
        vid->openingAsync = false;
        (vid->callback)(vid->userdata);
    }

    return (NULL);
}

static void cleanupVid(AR2VideoParamExternalT *vid)
{
#if USE_CPARAM_SEARCH
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
#endif
    pthread_cond_destroy(&vid->pushInitedCond);
    pthread_mutex_destroy(&vid->frameLock);
    free(vid->device_id);
    free(vid);
}

int ar2VideoCloseExternal( AR2VideoParamExternalT *vid )
{
    if (!vid) return (-1); // Sanity check.

    bool pushInited;
    pthread_mutex_lock(&(vid->frameLock));
    pushInited = vid->pushInited;
    pthread_mutex_unlock(&(vid->frameLock));
    if (pushInited) {
        ARLOGe("Error: cannot close video while frames are still being pushed.\n");
        return (-1);
    }

    if (vid->capturing) ar2VideoCapStopExternal(vid);

    if (vid->openingAsync) {
        vid->openingAsync = false;
    } else {
        cleanupVid(vid);
    }

    return 0;
}

int ar2VideoCapStartExternal( AR2VideoParamExternalT *vid )
{
    int ret = -1;

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (vid->capturing) goto done; // Already capturing.
    vid->capturing = true;
    vid->pushNewFrameReady = false;

    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

int ar2VideoCapStopExternal( AR2VideoParamExternalT *vid )
{
    int ret = -1;

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->capturing) goto done; // Not capturing.
    vid->capturing = false;
    vid->pushNewFrameReady = false;

    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

AR2VideoBufferT *ar2VideoGetImageExternal( AR2VideoParamExternalT *vid )
{
    AR2VideoBufferT *ret = NULL;

    if (!vid || !vid->capturing) return NULL; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (vid->pushInited && vid->pushNewFrameReady) {
        vid->pushNewFrameReady = false;
        ret = &vid->buffer;
    }
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

int ar2VideoGetSizeExternal(AR2VideoParamExternalT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    if (x) *x = vid->width;
    if (y) *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatExternal( AR2VideoParamExternalT *vid )
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID; // Sanity check.
    
    if (vid->convertToRGBA) {
        return (AR_PIXEL_FORMAT_RGBA);
    } else {
        return vid->pixelFormat;
    }
}

int ar2VideoGetIdExternal( AR2VideoParamExternalT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiExternal( AR2VideoParamExternalT *vid, int paramName, int *value )
{
    if (!value) return -1;

    switch (paramName) {
        case AR_VIDEO_PARAM_GET_IMAGE_ASYNC:
            *value = 0;
            break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX:
            *value = vid->cameraIndex;
            break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_FACE:
            // Translate to the Android equivalent.
            *value = (vid->cameraPosition == AR_VIDEO_POSITION_FRONT ? AR_VIDEO_ANDROID_CAMERA_FACE_FRONT : AR_VIDEO_ANDROID_CAMERA_FACE_REAR);
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_CAMERA_POSITION:
            // Translate to the AVFoundation equivalent.
            switch (vid->cameraPosition) {
                case AR_VIDEO_POSITION_BACK:
                    *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_REAR;
                    break;
                case AR_VIDEO_POSITION_FRONT:
                    *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT;
                    break;
                case AR_VIDEO_POSITION_UNKNOWN:
                    *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_UNKNOWN;
                    break;
                default:
                    *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_UNSPECIFIED;
                    break;
             }
             break;
        case AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET:
            // Translate to the AVFoundation equivalent.
            if (vid->focal_length <= 0.0) {
                *value = AR_VIDEO_AVFOUNDATION_FOCUS_NONE;
            } else if (vid->focal_length > 6.0) {
                *value = AR_VIDEO_AVFOUNDATION_FOCUS_INF;
            } else if (vid->focal_length < 0.05) {
                *value = AR_VIDEO_AVFOUNDATION_FOCUS_MACRO;
            } else if (vid->focal_length > 0.5) {
                *value = AR_VIDEO_AVFOUNDATION_FOCUS_1_0M;
            } else {
                *value = AR_VIDEO_AVFOUNDATION_FOCUS_0_3M;
            }
            break;
        default:
            return (-1);
    }

    return -1;
}

int ar2VideoSetParamiExternal( AR2VideoParamExternalT *vid, int paramName, int  value )
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET:
            // Translate to metres.
            switch (value) {
                case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                    vid->focal_length = INFINITY;
                    break;
                case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                    vid->focal_length = 1.0f;
                    break;
                case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                    vid->focal_length = 0.01f;
                    break;
                case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                    vid->focal_length = 0.3f;
                    break;
                case AR_VIDEO_AVFOUNDATION_FOCUS_NONE:
                default:
                    vid->focal_length = 0.0f;
                    break;
            }
            break;
       default:
            return (-1);
    }
    return (0);


    return -1;
}

int ar2VideoGetParamdExternal( AR2VideoParamExternalT *vid, int paramName, double *value )
{
    if (!vid || !value) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   *value = (double)vid->focal_length; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdExternal( AR2VideoParamExternalT *vid, int paramName, double  value )
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   vid->focal_length = (float)value; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsExternal( AR2VideoParamExternalT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1); // Sanity check.
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:               *value = strdup(vid->device_id); break;
        default:
            return (-1);
    }
    return (0);
}

#if USE_CPARAM_SEARCH
static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamExternalT *vid = (AR2VideoParamExternalT *)userdata;
    if (!vid) return;

    switch (state) {
        case CPARAM_SEARCH_STATE_INITIAL:
        case CPARAM_SEARCH_STATE_IN_PROGRESS:
            break;
        case CPARAM_SEARCH_STATE_RESULT_NULL:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_OK:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(cparam, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_FAILED_NO_NETWORK:
            ARLOGe("Error during cparamSearch. Internet connection unavailable.\n");
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        default: // Errors.
            ARLOGe("Error %d returned from cparamSearch.\n", (int)state);
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
    }
    if (final) {
        vid->cparamSearchCallback = (void (*)(const ARParam *, void *))nullptr;
        vid->cparamSearchUserdata = nullptr;
    }
}

int ar2VideoGetCParamAsyncAndroid(AR2VideoParamExternalT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1); // Sanity check.
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }

    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;

    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, vid->cameraIndex, vid->width, vid->height, vid->focal_length, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = (void (*)(const ARParam *, void *))nullptr;
        vid->cparamSearchUserdata = nullptr;
        return (-1);
    }

    return (0);
}
#endif

int ar2VideoSetParamsExternal( AR2VideoParamExternalT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            free (vid->device_id);
            if (value) vid->device_id = strdup(value);
            else vid->device_id = NULL;
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoPushInitExternal(AR2VideoParamExternalT *vid, int width, int height, const char *pixelFormat, int cameraIndex, int cameraPosition)
{
    int err;
    int ret = -1;

    ARLOGd("ar2VideoPushInitExternal(): %s camera at %dx%d (%s).\n", (cameraPosition == AR_VIDEO_POSITION_FRONT ? "front" : "back"), width, height, pixelFormat);

    if (!vid || width <= 0 || height <= 0 || !pixelFormat) return (-1); // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));

    if (vid->pushInited) {
        ARLOGe("ar2VideoPushInitAndroid: Error: called while already inited.\n");
        goto done;
    }

    if (strcmp(pixelFormat, "NV21") == 0) {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_NV21;
        vid->pixelFormat = AR_PIXEL_FORMAT_NV21;
    } else if (strcmp(pixelFormat, "NV12") == 0) {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_NV12;
        vid->pixelFormat = AR_PIXEL_FORMAT_420f;
    } else if (strcmp(pixelFormat, "YUV_420_888") == 0) {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_YUV_420_888;
        // We will convert!
        vid->pixelFormat = AR_PIXEL_FORMAT_NV21;
    } else if (strcmp(pixelFormat, "RGBA") == 0)  {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_RGBA;
        vid->pixelFormat = AR_PIXEL_FORMAT_RGBA;
    } else if (strcmp(pixelFormat, "RGB_565") == 0)  {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_RGB_565;
        vid->pixelFormat = AR_PIXEL_FORMAT_RGB_565;
    } else if (strcmp(pixelFormat, "MONO") == 0)  {
        vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_MONO;
        vid->pixelFormat = AR_PIXEL_FORMAT_MONO;
    } else {
        ARLOGe("ar2VideoPushInitAndroid: Error: frames arriving in unsupported pixel format '%s'.\n", pixelFormat);
        goto done;
    }

    // Prepare the vid->buffer structure.
    if (vid->pixelFormat == AR_PIXEL_FORMAT_NV21 || vid->pixelFormat == AR_PIXEL_FORMAT_420f) {
        vid->buffer.bufPlaneCount = 2;
        vid->buffer.bufPlanes = (ARUint8 **)calloc(vid->buffer.bufPlaneCount, sizeof(ARUint8 *));
        vid->buffer.bufPlanes[0] = (ARUint8 *)malloc(width * height);
        vid->buffer.buffLuma = vid->buffer.bufPlanes[0];
        vid->buffer.bufPlanes[1] = (ARUint8 *)malloc(2 * (width / 2 * height / 2));
        if (vid->convertToRGBA) {
            vid->buffer.buff = (ARUint8 *)malloc(width * height * 4);
        } else {
            vid->buffer.buff = vid->buffer.bufPlanes[0];
        }
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_RGBA) {
        vid->buffer.buff = (ARUint8 *)malloc(width * height * 4);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = NULL;
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_RGB_565) {
        vid->buffer.buff = (ARUint8 *)malloc(width * height * 2);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = NULL;
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_MONO) {
        vid->buffer.buff = (ARUint8 *)malloc(width * height);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = vid->buffer.buff;
    } else {
        ARLOGe("Error: unsupported video format %s (%d).\n", arVideoUtilGetPixelFormatName(vid->pixelFormat), vid->pixelFormat);
        goto done;
    }
    vid->width = width;
    vid->height = height;
    vid->cameraIndex = cameraIndex;
    vid->cameraPosition = cameraPosition;
    vid->pushInited = true;
    ret = 0;

done:
    pthread_mutex_unlock(&(vid->frameLock));
    if ((err = pthread_cond_signal(&(vid->pushInitedCond))) != 0) {
        ARLOGe("ar2VideoPushInitAndroid(): pthread_cond_signal error %s (%d).\n", strerror(err), err);
    }
    return (ret);

}

int ar2VideoPushExternal(AR2VideoParamExternalT *vid,
                         ARUint8 *buf0p, int buf0Size, int buf0PixelStride, int buf0RowStride,
                         ARUint8 *buf1p, int buf1Size, int buf1PixelStride, int buf1RowStride,
                         ARUint8 *buf2p, int buf2Size, int buf2PixelStride, int buf2RowStride,
                         ARUint8 *buf3p, int buf3Size, int buf3PixelStride, int buf3RowStride)
{
    int ret = -1;
    if (!vid) return -1; // Sanity check.

    //ARLOGd("ar2VideoPushExternal(buf0p=%p, buf0Size=%d, buf0PixelStride=%d, buf0RowStride=%d, buf1p=%p, buf1Size=%d, buf1PixelStride=%d, buf1RowStride=%d, buf2p=%p, buf2Size=%d, buf2PixelStride=%d, buf2RowStride=%d, buf3p=%p, buf3Size=%d, buf3PixelStride=%d, buf3RowStride=%d)\n", buf0p, buf0Size, buf0PixelStride, buf0RowStride, buf1p, buf1Size, buf1PixelStride, buf1RowStride, buf2p, buf2Size, buf2PixelStride, buf2RowStride, buf3p, buf3Size, buf3PixelStride, buf3RowStride);

    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->pushInited || !vid->capturing) goto done; // Both ar2VideoPushInitExternal AND ar2VideoCapStartExternal must have been called.
    if (!buf0p || buf0Size <= 0) {
        ARLOGe("ar2VideoPushExternal: NULL buffer.\n");
        goto done;
    }

    // Get time of capture as early as possible.
    arUtilTimeSinceEpoch(&vid->buffer.time.sec, &vid->buffer.time.usec);
    vid->buffer.fillFlag = 1;

    // Copy the incoming frame.
    if (vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_NV21 || vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_NV12) {
        if (!buf1p || buf1Size <= 0) {
            ARLOGe("ar2VideoPushExternal: Error: insufficient buffers for format NV21/NV12.\n");
            goto done;
        }
        if ((vid->width * vid->height) != buf0Size || (2 * (vid->width/2 * vid->height/2)) != buf1Size) {
            ARLOGe("ar2VideoPushExternal: Error: unexpected buffer sizes (%d, %d) for format NV21/NV12.\n", buf0Size, buf1Size);
            goto done;
        }
        memcpy(vid->buffer.bufPlanes[0], buf0p, buf0Size);
        memcpy(vid->buffer.bufPlanes[1], buf1p, buf1Size);
        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->width, vid->height, vid->pixelFormat);
        }
    } else if (vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_YUV_420_888) {
        if (!buf1p || buf1Size <= 0 || !buf2p || buf2Size <= 0) {
            ARLOGe("ar2VideoPushExternal: Error: insufficient buffers for format YUV_420_888.\n");
            goto done;
        }

        if ((vid->width * vid->height) != buf0Size) {
            ARLOGe("ar2VideoPushExternal: Error: unexpected buffer size (%d) for format YUV_420_888.\n", buf0Size);
            goto done;
        }

        // Massage into NV21 format.
        // Y plane first. Note that YUV_420_888 guarantees buf0PixelStride == 1.
        if (buf0RowStride == vid->width) {
            memcpy(vid->buffer.bufPlanes[0], buf0p, buf0Size);
        } else {
            unsigned char *p = vid->buffer.bufPlanes[0], *p0 = buf0p;
            for (int i = 0; i < vid->height; i++) {
                memcpy(p, p0, vid->width);
                p += vid->width;
                p0 += buf0RowStride;
            }
        }
        // Next, U (Cb) and V (Cr) planes.
        if ((buf1PixelStride == 2 && buf2PixelStride == 2) && (buf1RowStride == vid->width && buf2RowStride == vid->width) && ((buf1p - 1) == buf2p)) {
            // U and V planes both have pixelstride of 2, rowstride of pixelstride * vid->width/2, and are interleaved by 1 byte, so it's already NV21 and we can do a direct copy.
            memcpy(vid->buffer.bufPlanes[1], buf2p, 2*vid->width/2*vid->height/2);
        } else {
            // Tedious conversion to NV21.
            unsigned char *p = vid->buffer.bufPlanes[1], *p1 = buf1p, *p2 = buf2p;
            for (int i = 0; i < vid->height / 2; i++) {
                for (int j = 0; j < vid->width / 2; j++) {
                    *p++ = p2[j * buf1PixelStride]; // Cr
                    *p++ = p1[j * buf2PixelStride]; // Cb
                }
                p1 += buf1RowStride;
                p2 += buf2RowStride;
            }
        }

        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->width, vid->height, vid->pixelFormat);
        }

    } else if (vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_RGBA) {
        if ((vid->width * vid->height * 4) != buf0Size) {
            ARLOGe("ar2VideoPushExternal: Error: unexpected buffer size (%d) for format RGBA.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_RGB_565) {
        if ((vid->width * vid->height * 2) != buf0Size) {
            ARLOGe("ar2VideoPushExternal: Error: unexpected buffer size (%d) for format RGB_565.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->incomingPixelFormat == ARVideoExternalIncomingPixelFormat_MONO) {
        if ((vid->width * vid->height) != buf0Size) {
            ARLOGe("ar2VideoPushExternal: Error: unexpected buffer size (%d) for format MONO.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    }

    ret = 0;
    vid->pushNewFrameReady = true;

done:
    pthread_mutex_unlock(&(vid->frameLock));

    return (ret);

}

int ar2VideoPushFinalExternal(AR2VideoParamExternalT *vid)
{
    ARLOGd("ar2VideoPushFinalExternal()\n");

    int ret = -1;

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));

    if (!vid->pushInited) goto done;

    if (vid->buffer.bufPlaneCount > 0) {
        for (int i = 0; i < vid->buffer.bufPlaneCount; i++) {
            free(vid->buffer.bufPlanes[i]);
            vid->buffer.bufPlanes[i] = NULL;
        }
        free(vid->buffer.bufPlanes);
        vid->buffer.bufPlanes = NULL;
        vid->buffer.bufPlaneCount = 0;
    }
    if (vid->convertToRGBA) {
        free(vid->buffer.buff);
    }
    vid->buffer.buff = NULL;
    vid->width = vid->height = 0;
    vid->incomingPixelFormat = ARVideoExternalIncomingPixelFormat_UNKNOWN;
    vid->pushInited = false;
    vid->pushNewFrameReady = false;
    ret = 0;

done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);

}

#endif //  ARVIDEO_INPUT_EXTERNAL
