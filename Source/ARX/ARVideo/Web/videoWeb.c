/*
 *  videoWeb.c
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
 *
 *  Author(s): Thorsten Bux
 *
 */
#ifdef ARVIDEO_INPUT_WEB

#include "videoWeb.h"
#include <emscripten.h>

#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <ARX/ARVideo/videoRGBA.h>
#include <unistd.h>

struct _AR2VideoParamWebT {
    int                widthIn;
    int                heightIn;
    AR_PIXEL_FORMAT    pixelFormat;
    int                camera_index; // 0 = first camera, 1 = second etc.
    int                camera_face; // 0 = camera is rear facing, 1 = camera is front facing.
    float              focal_length; // metres.
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

int ar2VideoDispOptionWeb(void)
{
    ARPRINT(" -module=Web\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    specifies desired width of image.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    specifies desired height of image.\n");
    ARPRINT("\n");
    
    return 0;
}

AR2VideoParamWebT *ar2VideoOpenAsyncWeb(const char *config, void (*callback)(void *), void *userdata)
{
    AR2VideoParamWebT    *vid;
    const char           *a;
    char                 line[1024];
    int err_i = 0;
    int i;
    int width = 0, height = 0;
    
    arMallocClear(vid, AR2VideoParamWebT, 1);
    
    ARLOGi("ar2VideoOpenAsyncWeb '%s'.\n", config);

    a = config;
    if(a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;
            
            if (sscanf(a, "%s", line) == 0) break;
            if (strcmp(line, "-module=Web") == 0) {
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
            } else {
                err_i = 1;
            }
            
            if (err_i) {
                ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionWeb();
                goto bail;
            }
            
            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    // Initial state.
    vid->pixelFormat = AR_PIXEL_FORMAT_INVALID;
    if (!vid->focal_length) vid->focal_length = AR_VIDEO_WEB_FOCAL_LENGTH_DEFAULT;
    vid->pushInited = false;
    vid->newFrame = false;

	vid->callback = callback;
	vid->userdata = userdata;

    goto done;

bail:
    free(vid);
    vid = NULL;
done:
    return (vid);
}

int ar2VideoCloseWeb(AR2VideoParamWebT *vid)
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
    
    if (vid->capturing) ar2VideoCapStopWeb(vid);
    
	pthread_mutex_destroy(&(vid->frameLock));
	pthread_cond_destroy(&(vid->frameInCond));     
    free(vid->buffer.buff);
    free(vid->buffer.buffLuma);
    free(vid);
    
    return 0;
} 

int ar2VideoGetIdWeb(AR2VideoParamWebT *vid, ARUint32 *id0, ARUint32 *id1)
{
    return -1;
}

int ar2VideoGetSizeWeb(AR2VideoParamWebT *vid, int *x, int *y)
{
    if (!vid) return -1; // Sanity check.
    
    if (x) *x = vid->widthIn;
    if (y) *y = vid->heightIn;
    
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWeb(AR2VideoParamWebT *vid)
{

    AR2VideoBufferT *ret = NULL;
    
    if (!vid) return NULL; // Sanity check.
    pthread_mutex_lock(&(vid->frameLock));    
    if (!vid->pushInited) goto done; // ar2VideoPushInitWeb must have been called.
    if (vid->newFrame) {
        vid->newFrame = false;
        ret = &vid->buffer;
    }

done:
    pthread_mutex_unlock(&(vid->frameLock));    
    return (ret);    
}

int ar2VideoCapStartWeb(AR2VideoParamWebT *vid)
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

int ar2VideoCapStopWeb(AR2VideoParamWebT *vid)
{
    int ret = -1;
    ARLOGd("Stopping video.\n");

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->capturing) goto done; // Not capturing.
    vid->capturing = false;
    vid->newFrame = false;
    vid->pushInited = false;    
    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWeb(AR2VideoParamWebT *vid)
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID; // Sanity check.
    return vid->pixelFormat;
}

int ar2VideoPushInitWeb(AR2VideoParamWebT *vid, int width, int height, const char *pixelFormat, int camera_index, int camera_face)
{
    int err;
    int ret = -1;
 
    if (!vid || width <= 0 || height <= 0 || !pixelFormat) return (-1); // Sanity check.
    
    pthread_mutex_lock(&(vid->frameLock));
    
    if (vid->pushInited) {
        ARLOGe("ar2VideoPushInitWeb: Error: called while already inited.\n");
        goto done;
    }
    
    if (strcmp(pixelFormat, "RGBA") == 0)  {
        vid->pixelFormat = AR_PIXEL_FORMAT_RGBA;
    } else {
        ARLOGe("ar2VideoPushInitWeb: Error: frames arriving in unsupported pixel format '%s'.\n", pixelFormat);
        goto done;
    }
    
    int videoSize = width * height * 4 * sizeof(ARUint8);
    ARLOGd("videoSize = width %d * height %d * 4 * sizeof(ARUint8) %d= '%d'.\n", width, height, sizeof(ARUint8), videoSize);

    // Prepare the vid->buffer structure.
    if (vid->pixelFormat == AR_PIXEL_FORMAT_RGBA) {
        vid->buffer.buff = (ARUint8*) malloc(videoSize);
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
        vid->buffer.buffLuma = (ARUint8*) malloc(videoSize/4);
    } else {
        ARLOGe("Error: unsupported video format %s (%d).\n", arVideoUtilGetPixelFormatName(vid->pixelFormat), vid->pixelFormat);
        goto done;
    }
    vid->widthIn = width;
    vid->heightIn = height;
    vid->camera_index = camera_index;
    vid->camera_face = camera_face;
    vid->pushInited = true;

    EM_ASM_({
        if (!artoolkitXjs["videoMalloc"]) {
            artoolkitXjs["videoMalloc"] = ({});
        }
        var videoMalloc = artoolkitXjs["videoMalloc"];
        videoMalloc["framepointer"] = $0;
        videoMalloc["framesize"] = $1;
        videoMalloc["lumaFramePointer"] = $2;
        videoMalloc["newFrameBoolPtr"] = $3;
        videoMalloc["fillFlagIntPtr"] = $4;
        videoMalloc["timeSecPtr"] = $5;
        videoMalloc["timeMilliSecPtr"] = $6;
    }, 
        vid->buffer.buff,
        videoSize,
        vid->buffer.buffLuma,
        &vid->newFrame,
        &vid->buffer.fillFlag,
        &vid->buffer.time.sec,
        &vid->buffer.time.usec
    );

    ret = 0;
    (vid->callback)(vid->userdata);

done: 
    pthread_mutex_unlock(&(vid->frameLock));
    if ((err = pthread_cond_signal(&(vid->frameInCond))) != 0) {
        ARLOGe("ar2VideoPushInitWeb(): pthread_cond_signal error %d.\n", err);
    }
    return (ret);
}

#endif
