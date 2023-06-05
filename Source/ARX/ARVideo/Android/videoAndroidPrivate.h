/*
 *  videoAndroidPrivate.h
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
 *  along with  artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef AR_VIDEO_ANDROID_PRIVATE_H
#define AR_VIDEO_ANDROID_PRIVATE_H

#include <ARX/ARVideo/video.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadataTags.h>
#include <media/NdkImageReader.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    ARVideoAndroidCameraCaptureSessionState_READY = 0,  // session is ready
    ARVideoAndroidCameraCaptureSessionState_ACTIVE,     // session is busy
    ARVideoAndroidCameraCaptureSessionState_CLOSED,     // session is closed(by itself or a new session evicts)
    ARVideoAndroidCameraCaptureSessionState_MAX_STATE
} ARVideoAndroidCameraCaptureSessionState;

struct _AR2VideoParamAndroidT {
    char              *device_id;
    int                camera_index; // 0 = first camera, 1 = second etc.
    int                width;
    int                height;
    AR_PIXEL_FORMAT    pixelFormat;
    int                convertToRGBA;
    float              focalLength; // metres.
    void             (*cparamSearchCallback)(const ARParam *, void *);
    void              *cparamSearchUserdata;
    AR2VideoBufferT    buffer;
    bool               native;
    bool               capturing; // Between capStart and capStop.
    pthread_mutex_t    frameLock;  // Protects: buffer, capturing.
    void             (*callback)(void *);
    void              *userdata;
    int                position;
    volatile bool      cameraReady_;
    bool               cameraAvailable_;
    ACameraManager*    cameraMgr_;
    ACameraDevice*     cameraDevice_;
    ACameraManager_AvailabilityCallbacks cameraAvailabilityCallbacks;
    int64_t            exposureTime_;
    int64_t            exposureRangeMin_;
    int64_t            exposureRangeMax_;
    int32_t            sensitivity_;
    int32_t            sensitivityRangeMin_;
    int32_t            sensitivityRangeMax_;
    int32_t            sensorRotation_;
    AImageReader*      imageReader_;
    ANativeWindow*     imageReaderNativeWindow_;
    AImage*            imageCheckedOutDownstream;
    AImage*            imageReady;
    ACaptureSessionOutput* captureSessionOutput_;
    ACameraOutputTarget* captureRequestTarget_;
    ACaptureRequest*   captureRequest_;
    ACaptureSessionOutputContainer* captureSessionOutputContainer_;
    ACameraCaptureSession* captureSession_;
    ARVideoAndroidCameraCaptureSessionState cameraCaptureSessionState;
    int                captureRequestSessionSequenceId_;
};

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_ANDROID_PRIVATE_H
