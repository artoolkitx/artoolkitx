/*
 *  ARVideoSource.h
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 */

#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H

#include <ARX/Platform.h>

#include <ARX/AR/ar.h>
#include <ARX/ARVideo/video.h>

#include <mutex>
#include <pthread.h>

/**
 * A video source provides video frames to the artoolkitX tracking module. Video sources
 * contain information about the video, such as size and pixel format, camera parameters
 * for distortion compensation, as well as the raw video data itself.
 */
class ARX_EXTERN ARVideoSource {

private:

    typedef enum {
        DEVICE_CLOSED,                  ///< Device is closed.
        DEVICE_OPEN,                    ///< Device is open.
        DEVICE_GETTING_READY,           ///< Device is moving from open to running.
        DEVICE_RUNNING                  ///< Device is open and able to be queried.
    } DeviceState;
    
    DeviceState deviceState;            ///< Current state of the video device

    AR2VideoParamT *m_vid;
    bool m_noCpara;
    char* cameraParam;                  ///< Camera parameter filename
    char* cameraParamBuffer;
    size_t cameraParamBufferLen;
    ARParamLT *cparamLT;                ///< Camera paramaters

    char* videoConfiguration;           ///< Video configuration string

    int videoWidth;                     ///< Width of the video frame in pixels
    int videoHeight;                    ///< Height of the video frame in pixels

    AR_PIXEL_FORMAT pixelFormat;        ///< Pixel format from artoolkitX enumeration.
    
    int m_captureFrameWaitCount;        ///< Number of frames captureFrame waited.

    AR2VideoBufferT *m_frameBuffer;     ///< Pointer to latest frame.
    
    AR2VideoTimestampT m_getFrameTextureTime; ///< Time at which last call to getFrameTexture was made.
    
    int m_error;
    void setError(int error);
    
    pthread_rwlock_t m_frameBufferLock;

    static void openCallback(void *userData);
    bool open2();
    static void open2CparamCallback(const ARParam *cparam_p, void *userdata);
    bool open3(const ARParam *cparam_p);
    
public:

    ARVideoSource();
    
    ~ARVideoSource();

    int getError();

    /**
        @brief Returns true if the video source is open
        @return    true if the video source is open
     */
    bool isOpen()  const;

    /**
        @brief Returns true if the video source is open and ready to be queried.
        @return    true if the video source is open and frame details are known
     */
    bool isRunning()  const;
    
    /**
        @brief Sets initial parameters which will be used when the video source is opened.
        @param vconf Either NULL (the default), to use the default video configuration, or a C-string containing the video configuration to apply.
        @param noCpara If false (the default), the default behaviour to determine the camera lens parameters for the video source will be followed. If true, the video source is treated as uncalibrated and attempts to retrieve the camera lens parameters will return NULL.
        @param cparaName Either NULL (the default), to use the default method for camera lens parameter discovery, or a C-string containing the filesystem path to a camera lens parameter file.
        @param cparaBuff Either NULL (the default), to use the default method for camera lens parameter discovery, or a pointer to an in-memory buffer containing the contents of a camera lens parameter file. If non-NULL, then parameter cparaBuffLen must hold the length of this buffer.
        @param cparaBuffLen If parameter cparaBuff is NULL, this value is ignored. If parameter cparaBuff is non-NULL, this value must hold the length of the buffer pointed to by cparaBuff.
     */
    void configure(const char* vconf, bool noCpara, const char* cparaName, const char* cparaBuff, size_t cparaBuffLen);

    /**
        @brief Returns the camera parameters for the video source.
        @return  The camera parameters, if some are available, or NULL if no parameters are available.
     */
    ARParamLT* getCameraParameters() const;

    /**
        @brief Returns the width of the video in pixels.
        @return        Width of the video in pixels
     */
    int getVideoWidth() const;
    
    /**
        @brief Returns the height of the video in pixels.
        @return        Height of the video in pixels
     */
    int getVideoHeight() const;

    /**
        @brief Returns the pixel format of the video.
        @return        Pixel format of the video
     */
    AR_PIXEL_FORMAT getPixelFormat() const;

    /**
        @brief Opens the video source.
        @return        true if the video source was opened successfully, false if a fatal error occured.
     */
    bool open();
    
    /**
        @brief Closes the video source.
        @return        true if the video source was closed successfully, otherwise false.
     */
    bool close();

    /**
        @brief Asks the video source to capture a frame.
        @return        true if the video source captured a frame, otherwise false
     */
    bool captureFrame();

    /**
        @brief Checkout a locked video frame if the frame's timestamp is newer than 'time'.
        @details
            This function returns a pointer to the current video frame buffer, but only if the
            frame's timestamp is newer than the time passed in parameter 'time'. If the return
            value is non-NULL, the caller has non-exclusive read access to the frame buffer
            until the next call to checkinFrame(). If the return value is NULL, no further action
            is required. I.e. each call to this function which returns non-NULL MUST be balanced
            with a call to checkinFrame() on the same thread.
            Multiple callers may simultaneously checkout frames, but the next frame will not be
            made available until all callers have called checkinFrame() on the current frame.
        @param         time Timestamp of frame to compare. Passing a timestamp of {0, 0} will ensure that the timestamp test always passes.
        @return        Pointer to the buffer containing the current video frame, if frame's timestamp is newer and a frame is available.
        @see checkinFrame
     */
    AR2VideoBufferT* checkoutFrameIfNewerThan(const AR2VideoTimestampT time);

    /**
        @brief Checkin a locked video frame.
        @details
            Each call to checkoutFrameIfNewerThan() which returns non-NULL MUST be balanced with a call to this function on the same thread.
        @see checkoutFrameIfNewerThan
     */
     void checkinFrame(void);

    /**
        @brief Get the underlying AR2VideoParamT settings structure.
        @details
            In some advanced circumstances, users might wish to make direct queries on the
            underlying AR2VideoParamT structure.
        @return A pointer to the structure. No guarantee about the lifetime or validity of the structure is made.
     */
     AR2VideoParamT *getAR2VideoParam(void);
    
    /**
        @brief Populates the provided color buffer with the current video frame.
        @param buffer    The color buffer to populate with frame data
        @return          true if the buffer was updated successfully, otherwise false
     */
    bool getFrameTextureRGBA32(uint32_t *buffer);
    
#if ARX_TARGET_PLATFORM_ANDROID
    jint androidVideoPushInit(JNIEnv *env, jobject obj, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face);
    jint androidVideoPush1(JNIEnv *env, jobject obj, jbyteArray buf, jint bufSize);
    jint androidVideoPush2(JNIEnv *env, jobject obj,
                           jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                           jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                           jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                           jobject buf3, jint buf3PixelStride, jint buf3RowStride);
    jint androidVideoPushFinal(JNIEnv *env, jobject obj);
#endif

};

#endif // !VIDEOSOURCE_H
