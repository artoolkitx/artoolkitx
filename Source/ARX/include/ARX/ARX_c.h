/*
 *  ARX_c.h
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb, Julian Looser.
 *
 */

#ifndef ARX_C_H
#define ARX_C_H

#include <ARX/Platform.h>
#include <ARX/Error.h>
#include <stdint.h>

/**
 * \file ARToolKitWrapperExportedAPI.h
 * Defines functions that provide a C-compatible API. These functions are accessible to
 * other C applications, as well as languages like C#.
 */
#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * Registers a callback function to use when a message is logged.
     * If the callback is to become invalid, be sure to call this function with NULL
     * first so that the callback is unregistered.
	 */
	ARX_EXTERN void arwRegisterLogCallback(PFN_LOGCALLBACK callback);

    ARX_EXTERN void arwSetLogLevel(const int logLevel);

    // ----------------------------------------------------------------------------------------------------
#pragma mark  artoolkitX lifecycle functions
    // ----------------------------------------------------------------------------------------------------
	/**
	 * Initialises the artoolkitX.
     * For any square template (pattern) marker trackables, the number of rows and columns in the template defaults to AR_PATT_SIZE1 and the maximum number of markers that may be loaded for a single matching pass defaults to AR_PATT_NUM_MAX.
	 * @return			true if successful, false if an error occurred
	 * @see				arwShutdownAR()
	 */
	ARX_EXTERN bool arwInitialiseAR();

	/**
	 * Gets the artoolkitX version as a string, such as "10.0.0".
	 * Must not be called prior to arwInitialiseAR().
	 * @param buffer	The character buffer to populate
	 * @param length	The maximum number of characters to set in buffer
	 * @return			true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwGetARToolKitVersion(char *buffer, int length);

    /**
     * Return error information
     * Returns the value of the error flag.  Each detectable error
     * is assigned a numeric code and symbolic name.  When  an  error  occurs,
     * the  error  flag  is set to the appropriate error code value.  No other
     * errors are recorded until arwGetError  is  called,  the  error  code  is
     * returned,  and  the  flag  is  reset  to  AR_ERROR_NONE.   If  a  call to
     * arwGetError returns AR_ERROR_NONE, there  has  been  no  detectable  error
     * since the last call to arwGetError, or since the the library was initialized.
     *
     * To  allow  for  distributed implementations, there may be several error
     * flags.  If any single error flag has recorded an error,  the  value  of
     * that  flag  is  returned  and  that  flag  is reset to AR_ERROR_NONE when
     * arwGetError is called.  If more than one flag  has  recorded  an  error,
     * arwGetError  returns  and  clears  an arbitrary error flag value.  Thus,
     * arwGetError should  always  be  called  in  a  loop,  until  it  returns
     * AR_ERROR_NONE, if all error flags are to be reset.
     *
     * Initially, all error flags are set to AR_ERROR_NONE.
     * @return			enum with error code.
     */
    ARX_EXTERN int arwGetError();

	/**
	 * Changes the working directory to the resources directory used by artoolkitX.
     * Normally, this would be called immediately after arwInitialiseAR()
	 * @return			true if successful, false if an error occurred
	 * @see				arwInitialiseAR()
	 */
	ARX_EXTERN bool arwChangeToResourcesDir(const char *resourcesDirectoryPath);

	/**
	 * Initialises and starts video capture.
	 * @param vconf		The video configuration string
	 * @param cparaName	The camera parameter file, which is used to form the projection matrix
	 * @return			true if successful, false if an error occurred
	 * @see				arwStopRunning()
	 */
	ARX_EXTERN bool arwStartRunning(const char *vconf, const char *cparaName);

	/**
	 * Initialises and starts video capture.
	 * @param vconf		The video configuration string
	 * @param cparaBuff	A string containing the contents of a camera parameter file, which is used to form the projection matrix.
	 * @param cparaBuffLen	Number of characters in cparaBuff.
	 * @return			true if successful, false if an error occurred
	 * @see				arwStopRunning()
	 */
	ARX_EXTERN bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen);

    ARX_EXTERN bool arwStartRunningStereo(const char *vconfL, const char *cparaNameL, const char *vconfR, const char *cparaNameR, const char *transL2RName);

    ARX_EXTERN bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen);

	/**
	 * Returns true if artoolkitX is running, i.e. detecting trackables.
	 * @return			true when running, otherwise false
	 */
	ARX_EXTERN bool arwIsRunning();

	/**
	 * Returns true if artoolkitX is initialized and read for adding trackables.
	 * @return			true when running, otherwise false
	 */
	ARX_EXTERN bool arwIsInited();

	/**
	 * Stops video capture and frees video capture resources.
	 * @return			true if successful, false if an error occurred
	 * @see				arwStartRunning()
	 */
	ARX_EXTERN bool arwStopRunning();

	/**
	 * Shuts down the artoolkitX and frees all resources.
     * N.B.: If this is being called from the destructor of the same module which
     * supplied the log callback, be sure to call arwRegisterLogCallback(NULL)
     * prior to calling this function.
	 * @return			true if successful, false if an error occurred
	 * @see				arwInitialiseAR()
	 */
	ARX_EXTERN bool arwShutdownAR();

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream management
    // ----------------------------------------------------------------------------------------------------

    /**
     * Specifies desired horizontal alignment of video frames in drawing graphics context.
     */
    enum {
        ARW_H_ALIGN_LEFT,       ///< Align the left edge of the video frame with the left edge of the context.
        ARW_H_ALIGN_CENTRE,     ///< Align the centre of the video frame with the centre of the context.
        ARW_H_ALIGN_RIGHT       ///< Align the right edge of the video frame with the right edge of the context.
    };

    /**
     * Specifies desired vertical alignment of video frames in drawing graphics context.
     */
    enum {
        ARW_V_ALIGN_TOP,        ///< Align the top edge of the video frame with the top edge of the context.
        ARW_V_ALIGN_CENTRE,     ///< Align the centre of the video frame with the centre of the context.
        ARW_V_ALIGN_BOTTOM      ///< Align the bottom edge of the video frame with the bottom edge of the context.
    };

    /**
     * Specifies desired scaling of video frames to drawing graphics context.
     */
    enum {
        ARW_SCALE_MODE_STRETCH, ///< Scale the video frame non-proportionally up or down so that it matches exactly the size of the graphics context. If the frame and the context have different aspect ratios, the frame will appear stretched or squashed.
        ARW_SCALE_MODE_FIT,     ///< Scale the video frame proportionally up or down so that it fits visible in its entirety in the graphics context. When the graphics context is wider than the frame, it will be pillarboxed. When the graphics context is taller than the frame, it will be letterboxed.
        ARW_SCALE_MODE_FILL,    ///< Scale the video frame proportionally up or down so that it fills the entire in the graphics context. When the graphics context is wider than the frame, the frame will be cropped top and/or bottom. When the graphics context is taller than the frame, the frame will be cropped left and/or right.
        ARW_SCALE_MODE_1_TO_1   ///< Do not scale the video frame. One pixel of the video frame will be represented by one pixel of the graphics context.
    };

	/**
	 * Populates the given float array with the projection matrix computed from camera parameters for the video source.
     * @param nearPlane Near plane distance for projection matrix calculation.
     * @param farPlane  Far plane distance for projection matrix calculation.
	 * @param p         Float array to populate with OpenGL compatible projection matrix.
	 * @return          true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwGetProjectionMatrix(const float nearPlane, const float farPlane, float p[16]);

	/**
	 * Populates the given float arrays with the projection matrices computed from camera parameters for each of the stereo video sources.
     * @param nearPlane Near plane distance for projection matrix calculation.
     * @param farPlane  Far plane distance for projection matrix calculation.
	 * @param pL        Float array to populate with OpenGL compatible projection matrix for the left camera of the stereo video pair.
	 * @param pR        Float array to populate with OpenGL compatible projection matrix for the right camera of the stereo video pair.
	 * @return          true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwGetProjectionMatrixStereo(const float nearPlane, const float farPlane, float pL[16], float pR[16]);

    ARX_EXTERN bool arwGetProjectionMatrixForViewportSizeAndFittingMode(const int width, const int height, const int scaleMode, const int hAlign, const int vAlign, const float nearPlane, const float farPlane, float p[16]);

    ARX_EXTERN bool arwGetProjectionMatrixForViewportSizeAndFittingModeStereo(const int width, const int height, const int scaleMode, const int hAlign, const int vAlign, const float nearPlane, const float farPlane, float pL[16], float pR[16]);

	/**
	 * Returns the parameters of the video source frame.
     * @param width Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param height Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param pixelSize Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame.
     * @param pixelFormatStringBuffer Pointer to a buffer which will be filled with the symolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferLen Length (in bytes) of pixelFormatStringBuffer, or 0 if this information is not required.
	 * @return			True if the values were returned OK, false if there is currently no video source or an error int[] .
	 * @see				arwGetVideoParamsStereo
	 */
	ARX_EXTERN bool arwGetVideoParams(int *width, int *height, int *pixelSize, char *pixelFormatStringBuffer, int pixelFormatStringBufferLen);

	/**
	 * Returns the parameters of the video source frames.
     * @param widthL Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param widthR Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param heightL Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param heightR Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param pixelSizeL Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame, or NULL if this information is not required.
     * @param pixelSizeR Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame, or NULL if this information is not required.
     * @param pixelFormatStringBufferL Pointer to a buffer which will be filled with the symbolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferR Pointer to a buffer which will be filled with the symbolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferLenL Length (in bytes) of pixelFormatStringBufferL, or 0 if this information is not required.
     * @param pixelFormatStringBufferLenR Length (in bytes) of pixelFormatStringBufferR, or 0 if this information is not required.
	 * @return			True if the values were returned OK, false if there is currently no stereo video source or an error int[] .
	 * @see				arwGetVideoParams
	 */
	ARX_EXTERN bool arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, char *pixelFormatStringBufferL, int pixelFormatStringBufferLenL, int *widthR, int *heightR, int *pixelSizeR, char *pixelFormatStringBufferR, int pixelFormatStringBufferLenR);

	/**
	 * Captures a newest frame from the video source.
	 * @return			true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwCapture();

	/**
	 * Performs tracking and trackable updates. The newest frame from the video source is retrieved and
	 * analysed. All loaded trackables are updated.
	 * @return			true if successful, false if an error occurred
	 */
    ARX_EXTERN bool arwUpdateAR();

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Video push interface.
    // ----------------------------------------------------------------------------------------------------
    ARX_EXTERN int arwVideoPushInit(int videoSourceIndex, int width, int height, const char *pixelFormat, int cameraIndex, int cameraPosition);
    ARX_EXTERN int arwVideoPush(int videoSourceIndex,
                               uint8_t *buf0p, int buf0Size, int buf0PixelStride, int buf0RowStride,
                               uint8_t *buf1p, int buf1Size, int buf1PixelStride, int buf1RowStride,
                               uint8_t *buf2p, int buf2Size, int buf2PixelStride, int buf2RowStride,
                               uint8_t *buf3p, int buf3Size, int buf3PixelStride, int buf3RowStride);
    ARX_EXTERN int arwVideoPushFinal(int videoSourceIndex);

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream retrieval and/or drawing.
    // ----------------------------------------------------------------------------------------------------

    /**
     * Asks the video source to push the most recent frame into the passed-in buffer.
     * @param buffer Pointer to a buffer of pixels (of type 'uint32_t') to be filled with video.
     *      It is the caller's responsibility to ensure that the buffer is of sufficient size,
     *      e.g. using arwGetVideoParams.
     *      The pixels are RGBA in little-endian systems, or ABGR in big-endian systems.
     * @return            true if successful, false if an error occurred
     */
    ARX_EXTERN bool arwUpdateTexture32(uint32_t *buffer);

    /**
     * Asks the video source to push the most recent stereo frame into the passed-in buffer.
     * @param bufferL Pointer to a buffer of pixels (of type 'uint32_t') to be filled with video
     *      from the left camera. It is the caller's responsibility to ensure that the buffer is
     *      of sufficient size, e.g. using arwGetVideoParamsStereo.
     *      The pixels are RGBA in little-endian systems, or ABGR in big-endian systems.
     * @param bufferR Pointer to a buffer of pixels (of type 'uint32_t') to be filled with video
     *      from the right camera. It is the caller's responsibility to ensure that the buffer is
     *      of sufficient size, e.g. using arwGetVideoParamsStereo.
     *      The pixels are RGBA in little-endian systems, or ABGR in big-endian systems.
     * @return            true if successful, false if an error occurred
     */
    ARX_EXTERN bool arwUpdateTexture32Stereo(uint32_t *bufferL, uint32_t *bufferR);

    /**
     * Initialise drawing of video frames in a graphics context.
     *
     * If drawing of video frames into a graphics context is desired,
     * this function must be called from the rendering thread to initialise
     * graphics library structures for future drawing of video frames.
     *
     * This function must be called only with a valid graphics context
     * active (typically from the rendering thread) and only when the
     * function arwIsRunning() returns true.
     *
     * When drawing of video frames is no longer required, the function
     * arwDrawVideoFinal must be called to clean up structures allocated
     * by this call.
     *
     * @param videoSourceIndex The 0-based index of the video source which
     *     will supply frames for drawing.  Normally 0, but for the second camera in a stereo pair, 1.
     * @return true if successful, false if an error occurred.
     * @see arwIsRunning
     * @see arwDrawVideoFinal
     */
    ARX_EXTERN bool arwDrawVideoInit(const int videoSourceIndex);

    /**
     * Specify the layout of the graphics context in which drawing of video frames will occur.
     *
     * As the layout of the graphics context (e.g. size, orientation) may
     * differ widely from the format of the video frames which are to be
     * drawn, this function specifies the layout of the graphics context
     * and the desired scaling and positioning of the video frames within
     * this context. Optionally, a calculated OpenGL-style viewport can be
     * returned to the caller.
     *
     * This function must only be called with a graphics context active
     * (i.e. typically called only from a rendering thread) and only while
     * arwIsRunning is true and only between calls to arwDrawVideoInit and
     * arwDrawVideoFinal.
     *
     * @param videoSourceIndex The 0-based index of the video source which
     *     is supplying frames for drawing. Normally 0, but for the second camera in a stereo pair, 1.
     * @param width The width in pixels of the graphics context.
     * @param height The height in pixels of the graphics context.
     * @param rotate90 If true, content should be rendered in the graphics
     *     context rotated 90-degrees.
     * @param flipH If true, content should be rendered in the graphics
     *     context mirrored (flipped) in the horizontal dimension.
     * @param flipV If true, content should be rendered in the graphics
     *     context mirrored (flipped) in the vertical dimension.
     * @param hAlign An enum ARW_H_ALIGN_* specifying the desired horizontal
     *     alignment of video frames in the graphics context.
     *     If unsure, pass ARW_H_ALIGN_CENTRE.
     * @param vAlign An enum ARW_V_ALIGN_* specifying the desired vertical
     *     alignment of video frames in the graphics context.
     *     If unsure, pass ARW_V_ALIGN_CENTRE.
     * @param scalingMode An enum ARW_SCALE_MODE_* specifying the desired
     *     scaling of the video frames to the graphics context.
     *     If unsure, pass ARW_SCALE_MODE_FIT.
     * @param viewport If non-null, must be an array of 4 32-bit signed
     *     integers, in which the calculated OpenGL-style viewport parameters will
     *     be returned. The order of the parameters is: x-coordinate of the left
     *     edge of the viewport (may be negative), the y-coordinate of the bottom
     *     edge of the viewport (may be negative), the width of the viewport in the
     *     x-axis in pixels, and the height of the viewport in the y-axis in pixels.
     * @return true if successful, false if an error occurred.
     * @see arwIsRunning
     * @see arwDrawVideoInit
     * @see arwDrawVideoFinal
     */
    ARX_EXTERN bool arwDrawVideoSettings(int videoSourceIndex, int width, int height, bool rotate90, bool flipH, bool flipV, int hAlign, int vAlign, int scalingMode, int32_t viewport[4]);

    /**
     * Draws the latest frame from the video source in the active graphics context.
     *
     * This function performs actual drawing of the latest video frame.
     *
     * This function must only be called with a graphics context active
     * (typically from the rendering thread) and only while arwIsRunning is true
     * and only between calls to arwDrawVideoInit and arwDrawVideoFinal,
     * and after at least one call to arwDrawVideoSettings.
     *
     * @param videoSourceIndex The 0-based index of the video source which
     *     is supplying frames for drawing.
     * @return          true if successful, false if an error occurred.
     * @see arwIsRunning
     * @see arwDrawVideoInit
     * @see arwDrawVideoFinal
     * @see arwDrawVideoSettings
     */
    ARX_EXTERN bool arwDrawVideo(const int videoSourceIndex);

    /**
     * Finalise drawing of video frames in a graphics context.
     *
     * When drawing of video frames is no longer required, this function
     * must be called to clean up structures allocated by the call to
     * arwDrawVideoInit.
     *
     * This function must only be called with a graphics context active
     * (typically from the rendering thread).
     *
     * @param videoSourceIndex The 0-based index of the video source which
     *     supplied frames for drawing.
     * @return true if successful, false if an error occurred.
     * @see arwDrawVideoInit
     */
    ARX_EXTERN bool arwDrawVideoFinal(const int videoSourceIndex);

	// ----------------------------------------------------------------------------------------------------
#pragma mark  Tracking configuration
    // ----------------------------------------------------------------------------------------------------
    /**
     * Constants for use with tracker option setters/getters.
     */
    enum {
        ARW_TRACKER_OPTION_NFT_MULTIMODE = 0,                          ///< int.
        ARW_TRACKER_OPTION_SQUARE_THRESHOLD = 1,                       ///< Threshold value used for image binarization. int in range [0-255].
        ARW_TRACKER_OPTION_SQUARE_THRESHOLD_MODE = 2,                  ///< Threshold mode used for image binarization. int.
        ARW_TRACKER_OPTION_SQUARE_LABELING_MODE = 3,                   ///< int.
        ARW_TRACKER_OPTION_SQUARE_PATTERN_DETECTION_MODE = 4,          ///< int.
        ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE = 5,                     ///< float in range (0-0.5).
        ARW_TRACKER_OPTION_SQUARE_MATRIX_CODE_TYPE = 6,                ///< int.
        ARW_TRACKER_OPTION_SQUARE_IMAGE_PROC_MODE = 7,                 ///< int.
        ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE = 8,                      ///< Enables or disable state of debug mode in the tracker. When enabled, a black and white debug image is generated during marker detection. The debug image is useful for visualising the binarization process and choosing a threshold value. bool.
        ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE = 9,                    ///< Number of rows and columns in square template (pattern) markers. Defaults to AR_PATT_SIZE1, which is 16 in all versions of ARToolKit prior to 5.3. int.
        ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX = 10,              ///< Maximum number of square template (pattern) markers that may be loaded at once. Defaults to AR_PATT_NUM_MAX, which is at least 25 in all versions of ARToolKit prior to 5.3. int.
        ARW_TRACKER_OPTION_2D_TRACKER_FEATURE_TYPE = 11,               ///< Feature detector type used in the 2d Tracker - 0 AKAZE, 1 ORB, 2 BRISK, 3 KAZE
        ARW_TRACKER_OPTION_2D_MAXIMUM_MARKERS_TO_TRACK = 12,           ///< Maximum number of markers able to be tracked simultaneously. Defaults to 1. Should not be set higher than the number of 2D markers loaded.
        ARW_TRACKER_OPTION_SQUARE_MATRIX_MODE_AUTOCREATE_NEW_TRACKABLES = 13, ///< If true, when the square tracker is detecting matrix (barcode) markers, new trackables will be created for unmatched markers. Defaults to false. bool.
        ARW_TRACKER_OPTION_SQUARE_MATRIX_MODE_AUTOCREATE_NEW_TRACKABLES_DEFAULT_WIDTH = 14, ///< If ARW_TRACKER_OPTION_SQUARE_MATRIX_MODE_AUTOCREATE_NEW_TRACKABLES is true, this value will be used for the initial width of new trackables for unmatched markers. Defaults to 80.0f. float.
        ARW_TRACKER_OPTION_2D_THREADED = 15,                           ///< bool, If false, 2D tracking updates synchronously, and arwUpdateAR will not return until 2D tracking is complete. If true, 2D tracking updates asychronously on a secondary thread, and arwUpdateAR will not block if the track is busy. Defaults to true.
    };

    /**
     * Set boolean options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackerOptionBool(int option, bool value);

    /**
     * Set integer options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackerOptionInt(int option, int value);

    /**
     * Set floating-point options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackerOptionFloat(int option, float value);

    /**
     * Get boolean options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to get.
     * @return true if option is set, false if option is not set or an error occurred.
     */
    ARX_EXTERN bool arwGetTrackerOptionBool(int option);

    /**
     * Get integer options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to get.
     * @return integer value of option, or INT_MAX if an error occurred.
     */
    ARX_EXTERN int arwGetTrackerOptionInt(int option);

    /**
     * Get floating-point options associated with a tracker.
     * @param option Symbolic constant identifying tracker option to get.
     * @return floating-point value of option, or NAN if an error occurred.
     */
    ARX_EXTERN float arwGetTrackerOptionFloat(int option);

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Trackable management
    // ----------------------------------------------------------------------------------------------------

    /**
	 * Adds a trackable as specified in the given configuration string. The format of the string can be
	 * one of:
     * - Square marker from pattern file: "single;pattern_file;pattern_width", e.g. "single;data/hiro.patt;80"
     * - Square marker from pattern passed in config: "single_buffer;pattern_width;buffer=[]", e.g. "single_buffer;80;buffer=234 221 237..."
     * - Square barcode marker: "single_barcode;barcode_id;pattern_width", e.g. "single_barcode;0;80"
     * - Multi-square marker: "multi;config_file", e.g. "multi;data/multi/marker.dat"
     * - Multi-square auto marker: "multi;origin_barcode_id;pattern_width", e.g. "multi;0;80.0"
     * - NFT marker: "nft;nft_dataset_pathname", e.g. "nft;gibraltar"
     * - 2D textured surface: "2d;image_pathname;image_width" e.g. "2d;pinball.jpg;188.0"
	 * @param cfg		The configuration string
	 * @return			The unique identifier (UID) of the trackable instantiated based on the configuration string, or -1 if an error occurred
	 */
	ARX_EXTERN int arwAddTrackable(const char *cfg);

#pragma pack(push, 4)
    typedef struct {
        int uid;
        bool visible;
        float matrix[16];
        float matrixR[16]; // For stereo.
    } ARWTrackableStatus;
#pragma pack(pop)

    /**
     * Gets current number of trackables.
     * @return          The number of trackables currently loaded, or -1 in case of error.
     */
    ARX_EXTERN int arwGetTrackableCount(void);

    /**
     * Gets status of trackables.
     * @param statuses Pointer to the first element of an array of trackable statuses. This array should be allocated by the caller.
     *  Use the function arwGetTrackableCount to determine an appropriate number of elements for this array.
     * @param statusesCount The number of elements in the statuses array. If this is more than the number of trackables loaded, excess
     *  elements will not be modified. If this number is fewer than the number of trackables loaded, only the first `statuses` trackables will be reported.
     * @return          true if the function proceeded without error, false if an error occurred
     */
    ARX_EXTERN bool arwGetTrackableStatuses(ARWTrackableStatus *statuses, int statusesCount);

    /**
	 * Removes the trackable with the given unique identifier (UID).
	 * @param trackableUID	The unique identifier (UID) of the trackable to remove
	 * @return			true if the trackable was removed, false if an error occurred
	 */
	ARX_EXTERN bool arwRemoveTrackable(int trackableUID);

	/**
	 * Clears the collection of trackables.
	 * @return			The number of trackables removed
	 */
	ARX_EXTERN int arwRemoveAllTrackables();

	/**
	 * Returns the visibility and pose of the specified trackable.
	 *
	 * After a call to arwUpdate, all trackable information will be current. Any trackable can
	 * then be checked for visibility in the current frame, and if visible, additionally
	 * queried for its pose.
	 * @param trackableUID	The unique identifier (UID) of the trackable to query.
	 * @param matrix	A float array to populate with an OpenGL-compatible transformation matrix.
	 * @return			true if the specified trackable is visible, false if not, or an error occurred.
	 */
	ARX_EXTERN bool arwQueryTrackableVisibilityAndTransformation(int trackableUID, float matrix[16]);

	/**
	 * Returns the visibility and stereo pose of the specified trackable.
	 *
	 * After a call to arwUpdate, all trackable information will be current. Any trackable can
	 * then be checked for visibility in the current frame, and if visible, additionally
	 * queried for its pose.
	 * @param trackableUID	The unique identifier (UID) of the trackable to query
	 * @param matrixL	The float array to populate with an OpenGL-compatible transformation matrix for the left camera.
	 * @param matrixR	The float array to populate with an OpenGL-compatible transformation matrix for the right camera.
	 * @return			true if the specified trackable is visible, false if not, or an error occurred
	 */
	ARX_EXTERN bool arwQueryTrackableVisibilityAndTransformationStereo(int trackableUID, float matrixL[16], float matrixR[16]);

	/**
	 * Returns the number of pattern images associated with the specified trackable. A single square marker trackable has
     * one pattern image. A multi-square marker trackable has one or more pattern images.
     * Images of NFT marker trackables are not currently supported, so at present this function will return 0 for NFT trackables.
	 * @param trackableUID	The unique identifier (UID) of the trackable
	 * @return			The number of pattern images.
	 */
	ARX_EXTERN int arwGetTrackablePatternCount(int trackableUID);

	/**
	 * Gets configuration of a pattern associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
	 * @param patternID	The id of the pattern within the trackable, in range from 0 to arwGetTrackablePatternCount() - 1, inclusive. Ignored for single square markers and NFT markers (i.e. 0 assumed).
	 * @param matrix	The float array to populate with the 4x4 transformation matrix of the pattern (column-major order).
	 * @param width		Float value to set to the width of the pattern
	 * @param height	Float value to set to the height of the pattern.
	 * @param imageSizeX Int value to set to the width of the pattern image (in pixels).
	 * @param imageSizeY Int value to set to the height of the pattern image (in pixels).
	 * @return			true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwGetTrackablePatternConfig(int trackableUID, int patternID, float matrix[16], float *width, float *height, int *imageSizeX, int *imageSizeY);

	/**
	 * Gets a pattern image associated with a trackable. The provided color buffer is populated with the image of the
	 * pattern for the specified trackable. If the trackable is a multi-square marker, then the pattern image specified
     * by patternID is used.
     * Images of NFT marker trackables are not currently supported, so at present this function will return no image for NFT trackables.
	 * @param trackableUID	The unique identifier (UID) of the trackable
	 * @param patternID	The id for the pattern within that trackable. Ignored for single square marker and NFT marker trackables.
     * @param buffer    Pointer to a buffer of pixels (of type 'uint32_t') to be filled with pattern image.
     *      It is the caller's responsibility to ensure that the buffer is of sufficient size.
     *      Use arwGetTrackablePatternConfig to get the required size of this array (imageSizeX * imageSizeY elements).
     *      The pixels are RGBA in little-endian systems, or ABGR in big-endian systems.
	 * @return			true if successful, false if an error occurred
	 */
	ARX_EXTERN bool arwGetTrackablePatternImage(int trackableUID, int patternID, uint32_t *buffer);

    /**
     * Constants for use with trackable option setters/getters.
     */
    enum {
        ARW_TRACKABLE_OPTION_TYPE = 0,                             ///< readonly int enum, trackable type as per ARW_TRACKABLE_TYPE_* enum .
        ARW_TRACKABLE_OPTION_FILTERED = 1,                         ///< bool, true for filtering enabled.
        ARW_TRACKABLE_OPTION_FILTER_SAMPLE_RATE = 2,               ///< float, sample rate for filter calculations.
        ARW_TRACKABLE_OPTION_FILTER_CUTOFF_FREQ = 3,               ///< float, cutoff frequency of filter.
        ARW_TRACKABLE_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION = 4,  ///< bool, true to use continuous pose estimate.
        ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE = 5,                ///< float, confidence value of most recent marker match
        ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE_CUTOFF = 6,         ///< float, minimum allowable confidence value used in marker matching.
        ARW_TRACKABLE_OPTION_NFT_SCALE = 7,                        ///< float, scale factor applied to NFT marker size.
        ARW_TRACKABLE_OPTION_MULTI_MIN_SUBMARKERS = 8,             ///< int, minimum number of submarkers for tracking to be valid.
        ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_MATRIX = 9,            ///< float, minimum confidence value for submarker matrix tracking to be valid.
        ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_PATTERN = 10,          ///< float, minimum confidence value for submarker pattern tracking to be valid.
        ARW_TRACKABLE_OPTION_MULTI_MIN_INLIER_PROB = 11,           ///< float, minimum inlier probability value for robust multimarker pose estimation (range 1.0 - 0.0).
        ARW_TRACKABLE_OPTION_SQUARE_WIDTH = 12,                    ///< float, square marker width
        ARW_TRACKABLE_OPTION_2D_SCALE = 13,                        ///< float, 2D trackable scale (i.e. width).
    };

    enum
    {
        ARW_TRACKABLE_TYPE_Unknown = -1,       ///< Type not known, e.g. autocreated trackable.
        ARW_TRACKABLE_TYPE_Square = 0,         ///< A square template (pattern) marker.
        ARW_TRACKABLE_TYPE_SquareBarcode = 1,  ///< A square matrix (2D barcode) marker.
        ARW_TRACKABLE_TYPE_Multimarker = 2,    ///< Multiple square markers treated as a single marker.
        ARW_TRACKABLE_TYPE_NFT = 3,            ///< A legacy NFT marker.
        ARW_TRACKABLE_TYPE_TwoD = 4,           ///< An artoolkitX 2D textured trackable.
    };

	/**
	 * Set boolean options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackableOptionBool(int trackableUID, int option, bool value);

	/**
	 * Set integer options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackableOptionInt(int trackableUID, int option, int value);

	/**
	 * Set floating-point options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to set.
     * @param value The value to set it to.
     */
    ARX_EXTERN void arwSetTrackableOptionFloat(int trackableUID, int option, float value);

	/**
	 * Get boolean options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to get.
	 * @return true if option is set, false if option is not set or an error occurred.
     */
    ARX_EXTERN bool arwGetTrackableOptionBool(int trackableUID, int option);

	/**
	 * Get integer options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to get.
	 * @return integer value of option, or INT_MIN if an error occurred.
     */
    ARX_EXTERN int arwGetTrackableOptionInt(int trackableUID, int option);

	/**
	 * Get floating-point options associated with a trackable.
	 * @param trackableUID	The unique identifier (UID) of the trackable
     * @param option Symbolic constant identifying trackable option to get.
	 * @return floating-point value of option, or NAN if an error occurred.
     */
    ARX_EXTERN float arwGetTrackableOptionFloat(int trackableUID, int option);

    enum {
        ARW_TRACKABLE_EVENT_TYPE_NONE = 0,
        ARW_TRACKABLE_EVENT_TYPE_AUTOCREATED = 1,
        ARW_TRACKABLE_EVENT_TYPE_AUTOREMOVED = 2,
    };

    ARX_EXTERN void arwRegisterTrackableEventCallback(PFN_TRACKABLEEVENTCALLBACK callback);

    /**
     * Load a 2D trackable set from a trackable database.
     * @param databaseFileName Path of the file to load.
     * @return true if load succeeded, false if an error occurred.
     */
    ARX_EXTERN bool arwLoad2dTrackableDatabase(const char *databaseFileName);

    /**
     * Save the currently loaded 2D trackable set as a trackable database.
     * @param databaseFileName Path of the file to save.
     * @return true if save succeeded, false if an error occurred.
     */
    ARX_EXTERN bool arwSave2dTrackableDatabase(const char *databaseFileName);

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
    // ----------------------------------------------------------------------------------------------------
    /**
	 * Loads an optical parameters structure from file or from buffer.
     *
     * @param optical_param_name If supplied, points to a buffer specifying the path
     *      to the optical parameters file (as generated by the calib_optical utility.)
     * @param optical_param_buff If optical_param_name is NULL, the contents of this
     *      buffer will be interpreted as containing the contents of an optical
     *      parameters file.
     * @param optical_param_buffLen Length of the buffer specified in optical_param_buff.
     *      Ignored if optical_param_buff is NULL.
     * @param projectionNearPlane If p is non-NULL, near plane distance for projection matrix calculation.
     * @param projectionFarPlane If p is non-NULL, far plane distance for projection matrix calculation.
     * @param fovy_p Pointer to a float, which will be filled with the
     *      field-of-view (Y axis) component of the optical parameters, in degrees.
     * @param aspect_p Pointer to a float, which will be filled with the
     *      aspect ratio (width / height) component of the optical parameters.
     * @param m Pointer to an array of 16 floats, which will be filled with the
     *      transformation matrix component of the optical parameters.
     * @param p (Optional) May be NULL, or a pointer to an array of 16 floats,
     *      which will be filled with the perspective matrix calculated from fovy and aspect
     *      combined with the near and far projection values supplied in projectionNearPlane and
     *      projectionFarPlane.
     */
    ARX_EXTERN bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, const float projectionNearPlane, const float projectionFarPlane, float *fovy_p, float *aspect_p, float m[16], float p[16]);

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Video source info list management
    // ----------------------------------------------------------------------------------------------------

    /**
     * Create a video source info list using the supplied video configuration string.
     *
     * arwInitialiseAR does NOT need to be called before calling this function.
     * A single video source info list can exist at any one time.
     * The source info list is created by the default video module, or the module selected by
     * passing "-module=" in the configuration string. See
     * https://github.com/artoolkitx/artoolkitx/wiki/artoolkitX-video-module-configuration-reference#video-modules-and-selection
     *
     * Entries in the list can be queried by calling arwGetVideoSourceInfoListEntry.
     * @param config    The video configuration string.
     * @return The number of entries in the video source info list, or 0 if no video source is available or an error occured.
     *  If a non-zero value is returned, the list must be deleted with arwDeleteVideoSourceInfoList once the user is
     *  finished.
     */
    ARX_EXTERN int arwCreateVideoSourceInfoList(char *config);

    /**
     * Get an entry from the video source info list describing the name, model, UID, flags, and more for the video source info..
     *
     * Provided a video source info list has been created by calling arwCreateVideoSourceInfoList, this function
     * gets an entry from the list.
     * @param nameBuf Pointer to a buffer which will be filled with the user-readable name of the video input (as a nul-terminated C-string) or NULL if this information is not required.
     *  It is recommended that at least 256 bytes are allocated, and if the string is longer, it will be truncated to fit into the buffer (including the string nul-terminator).
     * @param nameBufLen Length (in bytes) of nameBuf, or 0 if the string is not required.
     * @param modelBuf Pointer to a buffer which will be filled with the user-readable model of the video input (as a nul-terminated C-string) or NULL if this information is not required.
     *  It is recommended that at least 256 bytes are allocated, and if the string is longer, it will be truncated to fit into the buffer (including the string nul-terminator).
     * @param modelBufLen Length (in bytes) of modelBuf, or 0 if the string is not required.
     * @param UIDBuf Pointer to a buffer which will be filled with the unique ID of the video input (as a nul-terminated C-string) or NULL if this information is not required.
     *  It is recommended that at least 256 bytes are allocated, and if the string is longer, it will be truncated to fit into the buffer (including the string nul-terminator).
     * @param UIDBufLen Length (in bytes) of UIDBuf, or 0 if the string is not required.
     * @param flags_p Pointer to an unsigned 32-bit integer, which if non-NULL will be filled with the flags applicable to this video input.
     * @param openTokenBuf Pointer to a buffer which will be filled with the machine-readable token which should be passed to the ar*VideoOpen function's
     *  configuration string to select this video input, (as a nul-terminated C-string) or NULL if this information is not required.
     *  It is recommended that at least 256 bytes are allocated, and if the string is longer, it will be truncated to fit into the buffer (including the string nul-terminator).
     * @param openTokenBufLen Length (in bytes) of openTokenBuf, or 0 if the string is not required.
     * @return true if option is set, false if option is not set or an error occurred.
     */
    ARX_EXTERN bool arwGetVideoSourceInfoListEntry(int index, char *nameBuf, int nameBufLen, char *modelBuf, int modelBufLen, char *UIDBuf, int UIDBufLen, uint32_t *flags_p, char *openTokenBuf, int openTokenBufLen);

    /**
     * Delete a video source info list.
     */
    ARX_EXTERN void arwDeleteVideoSourceInfoList(void);

#ifdef __cplusplus
}
#endif
#endif // !ARX_C_H
