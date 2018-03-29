/*
 *  videoAndroid.c
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
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoAndroid.h"

#ifdef ARVIDEO_INPUT_ANDROID

#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <ARX/ARVideo/videoRGBA.h>
#include <unistd.h>

#include <ARX/ARUtil/android.h>
#include "../cparamSearch.h"

#undef ARVIDEO_ANDROID_NATIVE

typedef enum {
    ARVideoAndroidIncomingPixelFormat_UNKNOWN,
    ARVideoAndroidIncomingPixelFormat_NV21,
    ARVideoAndroidIncomingPixelFormat_NV12,
    ARVideoAndroidIncomingPixelFormat_RGBA,
    ARVideoAndroidIncomingPixelFormat_RGB_565,
    ARVideoAndroidIncomingPixelFormat_YUV_420_888,
    ARVideoAndroidIncomingPixelFormat_MONO // Not official Android.
} ARVideoAndroidIncomingPixelFormat;

struct _AR2VideoParamAndroidT {
    char               device_id[PROP_VALUE_MAX*3+2]; // From <sys/system_properties.h>. 3 properties plus separators.
    int                widthIn;
    int                heightIn;
    ARVideoAndroidIncomingPixelFormat androidIncomingPixelFormat;
    AR_PIXEL_FORMAT    pixelFormat;
    int                convertToRGBA;
    int                camera_index; // 0 = first camera, 1 = second etc.
    int                camera_face; // 0 = camera is rear facing, 1 = camera is front facing.
    float              focal_length; // metres.
    void             (*cparamSearchCallback)(const ARParam *, void *);
    void              *cparamSearchUserdata;
    AR2VideoBufferT    buffer;
    bool               capturing;
    bool               pushInited;
    bool               newFrame;
    pthread_mutex_t    frameLock;
    pthread_cond_t     frameInCond;
    pthread_t          openAsyncThread;
    void             (*callback)(void *);
    void              *userdata;
};

static void *openAsyncThread(void *arg);

int ar2VideoDispOptionAndroid(void)
{
    ARPRINT(" -module=Android\n");
    ARPRINT(" -source=N\n");
    ARPRINT("    Acquire video from connected source device with index N (default = 0).\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    specifies desired width of image.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    specifies desired height of image.\n");
    ARPRINT(" -cachedir=/path/to/cache\n");
    ARPRINT("    Specifies the path in which to look for/store camera parameter cache files.\n");
    ARPRINT("    Default is working directory.\n");
    ARPRINT(" -format=[0|RGBA].\n");
    ARPRINT("    Specifies the pixel format for output images.\n");
    ARPRINT("    0=use system default. RGBA=output RGBA, including conversion if necessary.\n");
    ARPRINT("\n");
    
    return 0;
}

AR2VideoParamAndroidT *ar2VideoOpenAsyncAndroid(const char *config, void (*callback)(void *), void *userdata)
{
    char                     *cacheDir = NULL;
    char                     *cacheInitDir = NULL;
    char                     *csdu = NULL;
    char                     *csat = NULL;
    AR2VideoParamAndroidT    *vid;
    const char               *a;
    char                      line[1024];
    int err_i = 0;
    int i;
#ifdef ARVIDEO_ANDROID_NATIVE
    int width = 0, height = 0;
#endif
    int convertToRGBA = 0;
    
    arMallocClear(vid, AR2VideoParamAndroidT, 1);
    
    a = config;
    if(a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;
            
            if (sscanf(a, "%s", line) == 0) break;
            if (strcmp(line, "-module=Android") == 0) {
            } else if (strncmp(line, "-width=", 7) == 0) {
#ifndef ARVIDEO_ANDROID_NATIVE
                ARLOGw("Warning: videoAndroid cannot change frame size from native code. Configuration option '-width=' will be ignored.\n");
#else
                if (sscanf(&line[7], "%d", &width) == 0) {
                    ARLOGe("Error: Configuration option '-width=' must be followed by width in integer pixels.\n");
                    err_i = 1;
                }
#endif
            } else if (strncmp(line, "-height=", 8) == 0) {
#ifndef ARVIDEO_ANDROID_NATIVE
                ARLOGw("Warning: videoAndroid cannot change frame size from native code. Configuration option '-height=' will be ignored.\n");
#else
                if (sscanf(&line[8], "%d", &height) == 0) {
                    ARLOGe("Error: Configuration option '-height=' must be followed by height in integer pixels.\n");
                    err_i = 1;
                }
#endif
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
            } else if (strncmp(line, "-source=", 8) == 0) {
                if (sscanf(&line[8], "%d", &vid->camera_index) == 0) err_i = 1;
            } else {
                err_i = 1;
            }
            
            if (err_i) {
				ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionAndroid();
                goto bail;
			}
            
            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
	// Initialisation required before cparamSearch can be used.
    if (cparamSearchInit(cacheDir ? cacheDir : "cparam_cache", cacheInitDir ? cacheInitDir : "cparam_cache", false, csdu, csat) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };

    // Initial state.
    vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_UNKNOWN;
    vid->pixelFormat = AR_PIXEL_FORMAT_INVALID;
    vid->convertToRGBA = convertToRGBA;
    if (!vid->focal_length) vid->focal_length = AR_VIDEO_ANDROID_FOCAL_LENGTH_DEFAULT;
    vid->pushInited = false;
    vid->capturing = false;
    vid->newFrame = false;
    
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
    
	pthread_mutex_init(&(vid->frameLock), NULL);
	pthread_cond_init(&(vid->frameInCond), NULL);
	pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1); // Preclude the need to do pthread_join on the thread after it exits.
	vid->callback = callback;
	vid->userdata = userdata;
	err_i = pthread_create(&(vid->openAsyncThread), &attr, openAsyncThread, vid);
	pthread_attr_destroy(&attr);
    if (err_i != 0) {
        ARLOGe("ar2VideoOpenAsyncAndroid(): pthread_create error %d.\n");
        goto bail;
    }

    goto done;

bail:
    free(vid);
    vid = NULL;
done:
    free(cacheDir);
    free(cacheInitDir);
    free(csdu);
    free(csat);
    return (vid);
}

static void *openAsyncThread(void *arg)
{
    int err;

    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)arg; // Cast the thread start arg to the correct type.
    pthread_mutex_lock(&(vid->frameLock));
    while (!vid->pushInited) {
        // Android "Bionic" libc doesn't implement cancelation, so need to let wait expire somewhat regularly.
        struct timeval tv;
        struct timespec ts;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + 2;
        ts.tv_nsec = 0;
        if ((err = pthread_cond_timedwait(&(vid->frameInCond), &(vid->frameLock), &ts)) != 0) {
            ARLOGe("openAsyncThread(): pthread_cond_timedwait error %d.\n", err);
        }
    }
    pthread_mutex_unlock(&(vid->frameLock));
    (vid->callback)(vid->userdata);

    return (NULL);
}

int ar2VideoCloseAndroid(AR2VideoParamAndroidT *vid)
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
    
    if (vid->capturing) ar2VideoCapStopAndroid(vid);
    
	pthread_mutex_destroy(&(vid->frameLock));
	pthread_cond_destroy(&(vid->frameInCond));
    
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
    
    free(vid);
    
    return 0;
} 

int ar2VideoGetIdAndroid(AR2VideoParamAndroidT *vid, ARUint32 *id0, ARUint32 *id1)
{
    return -1;
}

int ar2VideoGetSizeAndroid(AR2VideoParamAndroidT *vid, int *x, int *y)
{
    if (!vid) return -1; // Sanity check.
    
    if (x) *x = vid->widthIn;
    if (y) *y = vid->heightIn;
    
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageAndroid(AR2VideoParamAndroidT *vid)
{
    AR2VideoBufferT *ret = NULL;
    
    if (!vid) return NULL; // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));    
    if (!vid->pushInited || !vid->capturing) goto done; // Both ar2VideoPushInitAndroid AND ar2VideoCapStartAndroid must have been called.
    if (vid->newFrame) {
        vid->newFrame = false;
        ret = &vid->buffer;
    }
done:
    pthread_mutex_unlock(&(vid->frameLock));    
    return (ret);    
}

int ar2VideoCapStartAndroid(AR2VideoParamAndroidT *vid)
{
    int ret = -1;
    
    if (!vid) return -1; // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));
    if (vid->capturing) goto done; // Already capturing.
    vid->capturing = true;
    vid->newFrame = false;
    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

int ar2VideoCapStopAndroid(AR2VideoParamAndroidT *vid)
{
    int ret = -1;
    
    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->capturing) goto done; // Not capturing.
    vid->capturing = false;
    vid->newFrame = false;
    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatAndroid(AR2VideoParamAndroidT *vid)
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID; // Sanity check.
    
    if (vid->convertToRGBA) {
        return (AR_PIXEL_FORMAT_RGBA);
    } else {
        return vid->pixelFormat;
    }
}

int ar2VideoGetParamiAndroid(AR2VideoParamAndroidT *vid, int paramName, int *value)
{
    if (!vid || !value) return (-1); // Sanity check.
    
    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX:   *value = vid->camera_index; break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_FACE:    *value = vid->camera_face; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiAndroid(AR2VideoParamAndroidT *vid, int paramName, int  value)
{
    if (!vid) return (-1); // Sanity check.
    
    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_INTERNET_STATE: return (cparamSearchSetInternetState(value)); break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdAndroid(AR2VideoParamAndroidT *vid, int paramName, double *value)
{
    if (!vid || !value) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   *value = (double)vid->focal_length; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdAndroid(AR2VideoParamAndroidT *vid, int paramName, double  value)
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   vid->focal_length = (float)value; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsAndroid(AR2VideoParamAndroidT *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1); // Sanity check.
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:               *value = strdup(vid->device_id); break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsAndroid(AR2VideoParamAndroidT *vid, const int paramName, const char  *value)
{
    if (!vid) return (-1); // Sanity check.
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)userdata;
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
    if (final) vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
}

int ar2VideoGetCParamAsyncAndroid(AR2VideoParamAndroidT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1); // Sanity check.
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }

    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;
    
    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, vid->camera_index, vid->widthIn, vid->heightIn, vid->focal_length, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
        return (-1);
    }
    
    return (0);
}

jint ar2VideoPushInitAndroid(AR2VideoParamAndroidT *vid, JNIEnv *env, jobject obj, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face)
{
    int err;
    int ret = -1;
 
     ARLOGd("ar2VideoPushInitAndroid(): called with camera %d (%s) at %dx%d (%s).\n", camera_index, (camera_face == 1 ? "front" : "back"), width, height, pixelFormat);

    if (!vid || width <= 0 || height <= 0 || !pixelFormat) return (-1); // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));
    
    if (vid->pushInited) {
        ARLOGe("ar2VideoPushInitAndroid: Error: called while already inited.\n");
        goto done;
    }
    
    if (strcmp(pixelFormat, "NV21") == 0) {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_NV21;
        vid->pixelFormat = AR_PIXEL_FORMAT_NV21;
    } else if (strcmp(pixelFormat, "NV12") == 0) {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_NV12;
        vid->pixelFormat = AR_PIXEL_FORMAT_420f;
    } else if (strcmp(pixelFormat, "YUV_420_888") == 0) {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_YUV_420_888;
        // We will convert!
        vid->pixelFormat = AR_PIXEL_FORMAT_NV21;
    } else if (strcmp(pixelFormat, "RGBA") == 0)  {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_RGBA;
        vid->pixelFormat = AR_PIXEL_FORMAT_RGBA;
    } else if (strcmp(pixelFormat, "RGB_565") == 0)  {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_RGB_565;
        vid->pixelFormat = AR_PIXEL_FORMAT_RGB_565;
    } else if (strcmp(pixelFormat, "MONO") == 0)  {
        vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_MONO;
        vid->pixelFormat = AR_PIXEL_FORMAT_MONO;
    } else {
        ARLOGe("ar2VideoPushInitAndroid: Error: frames arriving in unsupported pixel format '%s'.\n", pixelFormat);
        goto done;
    }
    
    // Prepare the vid->buffer structure.
    if (vid->pixelFormat == AR_PIXEL_FORMAT_NV21 || vid->pixelFormat == AR_PIXEL_FORMAT_420f) {
        vid->buffer.bufPlaneCount = 2;
        vid->buffer.bufPlanes = (ARUint8 **)calloc(vid->buffer.bufPlaneCount, sizeof(ARUint8 *));
        vid->buffer.bufPlanes[0] = malloc(width * height);
        vid->buffer.buffLuma = vid->buffer.bufPlanes[0];
        vid->buffer.bufPlanes[1] = malloc(2 * (width / 2 * height / 2));
        if (vid->convertToRGBA) {
            vid->buffer.buff = malloc(width * height * 4);
        } else {
            vid->buffer.buff = vid->buffer.bufPlanes[0];
        }
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_RGBA) {
        vid->buffer.buff = malloc(width * height * 4);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = NULL;
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_RGB_565) {
        vid->buffer.buff = malloc(width * height * 2);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = NULL;
    } else if (vid->pixelFormat == AR_PIXEL_FORMAT_MONO) {
        vid->buffer.buff = malloc(width * height);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = vid->buffer.buff;
    } else {
        ARLOGe("Error: unsupported video format %s (%d).\n", arVideoUtilGetPixelFormatName(vid->pixelFormat), vid->pixelFormat);
        goto done;
    }
    vid->widthIn = width;
    vid->heightIn = height;
    vid->camera_index = camera_index;
    vid->camera_face = camera_face;
    vid->pushInited = true;
    ret = 0;

done: 
    pthread_mutex_unlock(&(vid->frameLock));
    if ((err = pthread_cond_signal(&(vid->frameInCond))) != 0) {
        ARLOGe("ar2VideoPushInitAndroid(): pthread_cond_signal error %d.\n", err);
    }
    return (ret);
}

jint ar2VideoPushAndroid1(AR2VideoParamAndroidT *vid, JNIEnv *env, jobject obj, jbyteArray buf, jint bufSize)
{
    int ret = -1;
    if (!vid) return -1; // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->pushInited || !vid->capturing) goto done; // Both ar2VideoPushInitAndroid AND ar2VideoCapStartAndroid must have been called.
    if (!buf) {
        ARLOGe("ar2VideoPushAndroid1: NULL buffer.\n");
        goto done;
    }
    
    // Get time of capture.
    struct timeval time;
    gettimeofday(&time, NULL);
    vid->buffer.time.sec = time.tv_sec;
    vid->buffer.time.usec = time.tv_usec;
    vid->buffer.fillFlag = 1;
    
    // Copy the incoming frame.
    if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_NV21 || vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_NV12) {
        if (vid->widthIn * vid->heightIn + 2 * (vid->widthIn/2 * vid->heightIn/2) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format NV21/NV12.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, vid->widthIn * vid->heightIn, (jbyte *)vid->buffer.bufPlanes[0]);
        (*env)->GetByteArrayRegion(env, buf, vid->widthIn * vid->heightIn, 2 * (vid->widthIn/2 * vid->heightIn/2), (jbyte *)vid->buffer.bufPlanes[1]);
        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->widthIn, vid->heightIn, vid->pixelFormat);
        }
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGBA) {
        if ((vid->widthIn * vid->heightIn * 4) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format RGBA.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    } else if (vid->pixelFormat == ARVideoAndroidIncomingPixelFormat_RGB_565) {
        if ((vid->widthIn * vid->heightIn * 4) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format RGB_565.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    } else if (vid->androidIncomingPixelFormat == AR_PIXEL_FORMAT_MONO) {
        if ((vid->widthIn * vid->heightIn) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format MONO.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    }
    
    ret = 0;
    vid->newFrame = true;
    
done:
    pthread_mutex_unlock(&(vid->frameLock));
    if (vid->newFrame) pthread_cond_broadcast(&(vid->frameInCond));

    return (ret);
}

jint ar2VideoPushAndroid2(AR2VideoParamAndroidT *vid, JNIEnv *env, jobject obj,
                          jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                          jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                          jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                          jobject buf3, jint buf3PixelStride, jint buf3RowStride)
{
    int ret = -1;
    if (!vid) return -1; // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->pushInited || !vid->capturing) goto done; // Both ar2VideoPushInitAndroid AND ar2VideoCapStartAndroid must have been called.
    if (!buf0) {
        ARLOGe("ar2VideoPushAndroid2: NULL buffer.\n");
        goto done;
    }
    unsigned char *buf0p = (*env)->GetDirectBufferAddress(env, buf0);
    long buf0Size = (*env)->GetDirectBufferCapacity(env, buf0);
    
    // Get time of capture.
    struct timeval time;
    gettimeofday(&time, NULL);
    vid->buffer.time.sec = time.tv_sec;
    vid->buffer.time.usec = time.tv_usec;
    vid->buffer.fillFlag = 1;
    
    // Copy the incoming frame.
    if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_NV21 || vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_NV12) {
        if (!buf1) {
            ARLOGe("ar2VideoPushAndroid2: Error: insufficient buffers for format NV21/NV12.\n");
            goto done;
        }
        unsigned char *buf1p = (*env)->GetDirectBufferAddress(env, buf1);
        long buf1Size = (*env)->GetDirectBufferCapacity(env, buf1);
        if ((vid->widthIn * vid->heightIn) != buf0Size || (2 * (vid->widthIn/2 * vid->heightIn/2)) != buf1Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer sizes (%d, %d) for format NV21/NV12.\n", buf0Size, buf1Size);
            goto done;
        }
        memcpy(vid->buffer.bufPlanes[0], buf0p, buf0Size);
        memcpy(vid->buffer.bufPlanes[1], buf1p, buf1Size);
        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->widthIn, vid->heightIn, vid->pixelFormat);
        }
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_YUV_420_888) {
        if (!buf1 || !buf2) {
            ARLOGe("ar2VideoPushAndroid2: Error: insufficient buffers for format YUV_420_888.\n");
            goto done;
        }
        unsigned char *buf1p = (*env)->GetDirectBufferAddress(env, buf1); // Guaranteed to be U (Cb)
        long buf1Size = (*env)->GetDirectBufferCapacity(env, buf1);
        unsigned char *buf2p = (*env)->GetDirectBufferAddress(env, buf2); // Guaranteed to be V (Cr)
        long buf2Size = (*env)->GetDirectBufferCapacity(env, buf2);
        
        if ((vid->widthIn * vid->heightIn) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format YUV_420_888.\n", buf0Size);
            goto done;
        }

        // Massage into NV21 format.
        // Y plane first. Note that YUV_420_888 guarantees buf0PixelStride == 1.
        if (buf0RowStride == vid->widthIn) {
            memcpy(vid->buffer.bufPlanes[0], buf0p, buf0Size);
        } else {
            unsigned char *p = vid->buffer.bufPlanes[0], *p0 = buf0p;
            for (int i = 0; i < vid->heightIn; i++) {
                memcpy(p, p0, vid->widthIn);
                p += vid->widthIn;
                p0 += buf0RowStride;
            }
        }
        // Next, U (Cb) and V (Cr) planes.
        if ((buf1PixelStride == 2 && buf2PixelStride == 2) && (buf1RowStride == vid->widthIn && buf2RowStride == vid->widthIn) && ((buf1p - 1) == buf2p)) {
            // U and V planes both have pixelstride of 2, rowstride of pixelstride * vid->widthIn/2, and are interleaved by 1 byte, so it's already NV21 and we can do a direct copy.
            memcpy(vid->buffer.bufPlanes[1], buf2p, 2*vid->widthIn/2*vid->heightIn/2);
        } else {
            // Tedious conversion to NV21.
            unsigned char *p = vid->buffer.bufPlanes[1], *p1 = buf1p, *p2 = buf2p;
            for (int i = 0; i < vid->heightIn / 2; i++) {
                for (int j = 0; j < vid->widthIn / 2; j++) {
                    *p++ = p2[j * buf1PixelStride]; // Cr
                    *p++ = p1[j * buf2PixelStride]; // Cb
                }
                p1 += buf1RowStride;
                p2 += buf2RowStride;
            }
        }

        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->widthIn, vid->heightIn, vid->pixelFormat);
        }

    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGBA) {
        if ((vid->widthIn * vid->heightIn * 4) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format RGBA.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGB_565) {
        if ((vid->widthIn * vid->heightIn * 2) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format RGB_565.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_MONO) {
        if ((vid->widthIn * vid->heightIn) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format MONO.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    }
    
    ret = 0;
    vid->newFrame = true;
    
done:
    pthread_mutex_unlock(&(vid->frameLock));
    if (vid->newFrame) pthread_cond_broadcast(&(vid->frameInCond));

    return (ret);
}

jint ar2VideoPushFinalAndroid(AR2VideoParamAndroidT *vid, JNIEnv *env, jobject obj)
{
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
    vid->widthIn = vid->heightIn = 0;
    vid->androidIncomingPixelFormat = AR_PIXEL_FORMAT_INVALID;
    vid->pushInited = false;    
    vid->newFrame = false;
    ret = 0;
    
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

#endif // ARVIDEO_INPUT_ANDROID
