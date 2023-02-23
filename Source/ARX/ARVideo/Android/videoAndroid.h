/*
 *	videoAndroid.h
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
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_ANDROID_H
#define AR_VIDEO_ANDROID_H

#include <ARX/ARVideo/video.h>
#include <jni.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamAndroidT AR2VideoParamAndroidT;

#define AR_VIDEO_ANDROID_FOCAL_LENGTH_DEFAULT 0.3 // Metres.

//
// At present, videoAndroid does not support native camera access, and depends on use of the
// ar2VideoPush*Android functions to push incoming video frames.
// Most of the video functionality on Android is handled in separate Android-specific classes.
// For Java (JDK) Android apps, the classes are in ARBaseLib.
// For Native (NDK) Android apps, the classes are in the 'CameraSurface' class (part of the example code).
//

int                    ar2VideoDispOptionAndroid     ( void );
AR2VideoParamAndroidT *ar2VideoOpenAsyncAndroid      ( const char *config, void (*callback)(void *), void *userdata );
int                    ar2VideoCloseAndroid          ( AR2VideoParamAndroidT *vid );
int                    ar2VideoGetIdAndroid          ( AR2VideoParamAndroidT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeAndroid        ( AR2VideoParamAndroidT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatAndroid ( AR2VideoParamAndroidT *vid );
AR2VideoBufferT       *ar2VideoGetImageAndroid       ( AR2VideoParamAndroidT *vid );
int                    ar2VideoCapStartAndroid       ( AR2VideoParamAndroidT *vid );
int                    ar2VideoCapStopAndroid        ( AR2VideoParamAndroidT *vid );
int                    ar2VideoGetParamiAndroid      ( AR2VideoParamAndroidT *vid, int paramName, int *value );
int                    ar2VideoSetParamiAndroid      ( AR2VideoParamAndroidT *vid, int paramName, int  value );
int                    ar2VideoGetParamdAndroid      ( AR2VideoParamAndroidT *vid, int paramName, double *value );
int                    ar2VideoSetParamdAndroid      ( AR2VideoParamAndroidT *vid, int paramName, double  value );
int                    ar2VideoGetParamsAndroid      ( AR2VideoParamAndroidT *vid, const int paramName, char **value );
int                    ar2VideoSetParamsAndroid      ( AR2VideoParamAndroidT *vid, const int paramName, const char  *value );
ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListAndroid(const char *config_in);

int                    ar2VideoGetCParamAsyncAndroid (AR2VideoParamAndroidT *vid, void (*callback)(const ARParam *, void *), void *userdata);

// JNI interface.
jint ar2VideoPushInitAndroid(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face);
jint ar2VideoPushAndroid1(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj, jbyteArray buf, jint bufSize);
jint ar2VideoPushAndroid2(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj,
                          jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                          jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                          jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                          jobject buf3, jint buf3PixelStride, jint buf3RowStride);
jint ar2VideoPushFinalAndroid(AR2VideoParamAndroidT *vid, C_JNIEnv *env, jobject obj);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_ANDROID_H
