/*
 *  videoAndroidJNI.c
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

#include "videoAndroidPrivate.h"
#include <sys/time.h>
#include <pthread.h>
#include <ARX/ARVideo/videoRGBA.h>
#include <unistd.h>

jint ar2VideoPushInitAndroid(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face)
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
    vid->camera_index = camera_index;
    vid->camera_face = camera_face;
    vid->pushInited = true;
    ret = 0;

done: 
    pthread_mutex_unlock(&(vid->frameLock));
    if ((err = pthread_cond_signal(&(vid->pushInitedCond))) != 0) {
        ARLOGe("ar2VideoPushInitAndroid(): pthread_cond_signal error %d.\n", err);
    }
    return (ret);
}

jint ar2VideoPushAndroid1(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj, jbyteArray buf, jint bufSize)
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
        if (vid->width * vid->height + 2 * (vid->width/2 * vid->height/2) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format NV21/NV12.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, vid->width * vid->height, (jbyte *)vid->buffer.bufPlanes[0]);
        (*env)->GetByteArrayRegion(env, buf, vid->width * vid->height, 2 * (vid->width/2 * vid->height/2), (jbyte *)vid->buffer.bufPlanes[1]);
        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->width, vid->height, vid->pixelFormat);
        }
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGBA) {
        if ((vid->width * vid->height * 4) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format RGBA.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGB_565) {
        if ((vid->width * vid->height * 4) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format RGB_565.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_MONO) {
        if ((vid->width * vid->height) != bufSize) {
            ARLOGe("ar2VideoPushAndroid1: Error: unexpected buffer size (%d) for format MONO.\n", bufSize);
            goto done;
        }
        (*env)->GetByteArrayRegion(env, buf, 0, bufSize, (jbyte *)vid->buffer.buff);
    }
    
    ret = 0;
    vid->pushNewFrameReady = true;
    
done:
    pthread_mutex_unlock(&(vid->frameLock));

    return (ret);
}

jint ar2VideoPushAndroid2(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj,
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
        if ((vid->width * vid->height) != buf0Size || (2 * (vid->width/2 * vid->height/2)) != buf1Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer sizes (%d, %d) for format NV21/NV12.\n", buf0Size, buf1Size);
            goto done;
        }
        memcpy(vid->buffer.bufPlanes[0], buf0p, buf0Size);
        memcpy(vid->buffer.bufPlanes[1], buf1p, buf1Size);
        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->width, vid->height, vid->pixelFormat);
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
        
        if ((vid->width * vid->height) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format YUV_420_888.\n", buf0Size);
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

    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGBA) {
        if ((vid->width * vid->height * 4) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format RGBA.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_RGB_565) {
        if ((vid->width * vid->height * 2) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format RGB_565.\n", buf0Size);
            goto done;
        }
        memcpy(vid->buffer.buff, buf0p, buf0Size);
    } else if (vid->androidIncomingPixelFormat == ARVideoAndroidIncomingPixelFormat_MONO) {
        if ((vid->width * vid->height) != buf0Size) {
            ARLOGe("ar2VideoPushAndroid2: Error: unexpected buffer size (%d) for format MONO.\n", buf0Size);
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

jint ar2VideoPushFinalAndroid(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj)
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
    vid->width = vid->height = 0;
    vid->androidIncomingPixelFormat = ARVideoAndroidIncomingPixelFormat_UNKNOWN;
    vid->pushInited = false;    
    vid->pushNewFrameReady = false;
    ret = 0;
    
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

#endif // ARVIDEO_INPUT_ANDROID
