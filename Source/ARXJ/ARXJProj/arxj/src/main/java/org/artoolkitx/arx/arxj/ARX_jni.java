/*
 *  ARX_jni.java
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

package org.artoolkitx.arx.arxj;

import android.util.Log;

import java.nio.ByteBuffer;

/**
 * The ARX_jni class contains the JNI function signatures for
 * native artoolkitX functions. These functions should be accessed via
 * the {@link ARController} class rather than called directly.
 */
@SuppressWarnings({"JniMissingFunction", "UnusedReturnValue", "unused"})
public class ARX_jni {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = "ARX_jni";
    /**
     * The name of the native artoolkitX library.
     */
    private static final String LIBRARY_NAME = "ARX";

    /**
     * Attempts to load the native library so that native functions can be called.
     *
     * @return true if the library was successfully loaded, otherwise false.
     */
    static boolean loadNativeLibrary() {

        try {

            Log.i(TAG, "Attempting to load library " + LIBRARY_NAME + ".");

            System.loadLibrary("c++_shared");
            System.loadLibrary(LIBRARY_NAME);

        } catch(UnsatisfiedLinkError e) {
            Log.e(TAG, "Cannot load native library " + LIBRARY_NAME + ".");
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Error loading native library " + LIBRARY_NAME + ":" + e.toString());
            return false;
        }

        return true;
    }

    /**
     * Gets the version of the underlying artoolkitX library.
     *
     * @return artoolkitX version
     */
    public static native String arwGetARToolKitVersion();

    @SuppressWarnings({"WeakerAccess", "unused"})
	public static final int AR_LOG_LEVEL_DEBUG = 0,
            AR_LOG_LEVEL_INFO = 1,
            AR_LOG_LEVEL_WARN = 2,
            AR_LOG_LEVEL_ERROR = 3,
            AR_LOG_LEVEL_REL_INFO = 4;

    /**
     * Sets the severity level. Log messages below the set severity level are not logged.
     * All calls to artoolkitX's logging facility include a "log level" parameter, which specifies
     * the severity of the log message. (The severities are defined in %lt;ARX/AR/config.h&gt;.)
     * Setting this allows for filtering of log messages. All log messages lower than
     * the set level will not be logged.
     * Note that debug log messages created using the native ARLOGd() macro will be logged only in
     * debug builds, irrespective of the log level.
     * @param logLevel The log level below which log messages should be ignored.
     */
    public static native void arwSetLogLevel(int logLevel);

    /**
     * Initialises the basic artoolkitX functions. After this function has
     * been successfully called, markers can be added and removed, but marker
     * detection is not yet running.
     *
     * @return true on success, false if an error occurred
     * @see	{@link #arwShutdownAR()}
     */
    public static native boolean arwInitialiseAR();

    /**
     * Changes the working directory to the resources directory used by artoolkitX.
     * Normally, this would be called immediately after arwInitialiseAR()
     *
     * @return true if successful, false if an error occurred
     * @see {@link #arwInitialiseAR()}
     */
    public static native boolean arwChangeToResourcesDir(String resourcesDirectoryPath);

    /**
     * Initialises video capture. The native library will start to expect video
     * frames.
     * @param vconf			The video configuration string. Can be left empty.
     * @param cparaName	    Either: null to search for camera parameters specific to the device,
	 *            			or a path (in the filesystem) to a camera parameter file. The path may be an
	 *            			absolute path, or relative to the resourcesDirectoryPath set with arwChangeToResourcesDir.
     * @return				true on success, false if an error occurred.
     */
    public static native boolean arwStartRunning(String vconf, String cparaName);

    /**
     * Initialises stereo video capture. The native library will start to expect video
     * frames.
     *
     * @param vconfL       The video configuration string for the left camera. Can be left empty.
     * @param cparaNameL   The camera parameter file to load for the left camera.
     * @param vconfR       The video configuration string for the right camera. Can be left empty.
     * @param cparaNameR   The camera parameter file to load for the right camera.
     * @param transL2RName The stereo calibration file to load.
     * @return true on success, false if an error occurred
     */
    public static native boolean arwStartRunningStereo(String vconfL, String cparaNameL, String vconfR, String cparaNameR, String transL2RName);

	/**
	 * Queries whether artoolkitX is initialized. This will be true
	 * after a call to {@link #arwInitialiseAR()}. At
	 * this point {@link #arwStartRunning(String, String, float, float)} can be called.
	 *
	 * @return true artoolkitX has been initialized
	 */
    public static native boolean arwIsInited();

	/**
     * Queries whether tracking is up and running. This will be true
     * after a call to arwStartRunning, and frames are being sent through. At
     * this point, marker visibility and transformations can be queried.
     *
     * @return true if tracking is running, false if not
     */
    public static native boolean arwIsRunning();

    /**
     * Stops tracking and closes the video source.
     *
     * @return true on success, false if an error occurred
     */
    public static native boolean arwStopRunning();

    /**
     * Shuts down the basic artoolkitX functions.
     *
     * @return true on success, false if an error occurred
     */
    public static native boolean arwShutdownAR();

    /**
     * Retrieves the artoolkitX projection matrix.
     *
     * The projection will not be available until arwIsRunning() is true.
     * @param nearPlane    The value to use for the near OpenGL clipping plane.
     * @param farPlane     The value to use for the far OpenGL clipping plane.
     * @return A float array containing the OpenGL compatible projection matrix, or null if an error occurred.
     */
    public static native float[] arwGetProjectionMatrix(float nearPlane, float farPlane);

    /**
     * Retrieves the artoolkitX projection matrix for the cameras of a stereo camera pair.
     *
     * The projections will not be available until arwIsRunning() is true.
     * @param nearPlane    The value to use for the near OpenGL clipping plane.
     * @param farPlane     The value to use for the far OpenGL clipping plane.
     * @param projL A float array, which will be filled with the OpenGL compatible projection matrix for the left camera.
     * @param projR An float array, which will be filled with the OpenGL compatible projection matrix for the right camera.
     * @return true if the projections were successfully retrieved, or false if an error occurred.
     */
    public static native boolean arwGetProjectionMatrixStereo(float nearPlane, float farPlane, float[] projL, float[] projR);

    /**
     * Returns the parameters of the video source frame.
     * <br>
     * Usage example:
     * int[] width = new int[1];
     * int[] height = new int[1];
     * int[] pixelSize = new int[1];
     * String[] pixelFormatString = new String[1];
     * boolean ok = ARX_jni.arwGetVideoParams(width, height, pixelSize, pixelFormatString);
     *
     * @return True if the values were returned OK, false if there is currently no video source or an error int[] .
     * @param width An int array, the first element of which will be filled with the width (in pixels) of the video frame, or null if this information is not required.
     * @param height An int array, the first element of which will be filled with the height (in pixels) of the video frame, or null if this information is not required.
     * @param pixelSize An int array, the first element of which will be filled with the numbers of bytes per pixel of the source frame, or null if this information is not required.
     * @param pixelFormatStringBuffer A String array, the first element of which will be filled with the symbolic name of the pixel format of the video frame, or null if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @see {@link #arwGetVideoParamsStereo}
     */
    public static native boolean arwGetVideoParams(int[] width, int[] height, int[] pixelSize, String[] pixelFormatStringBuffer);

    /**
     * Returns the parameters of the video source frames.
     * <br>
     * Usage example:
     * int[] widthL = new int[1];
     * int[] heightL = new int[1];
     * int[] pixelSizeL = new int[1];
     * String[] pixelFormatStringL = new String[1];
     * int[] widthR = new int[1];
     * int[] heightR = new int[1];
     * int[] pixelSizeR = new int[1];
     * String[] pixelFormatStringR = new String[1];
     * boolean ok = ARX_jni.arwGetVideoParams(widthL, heightL, pixelSizeL, pixelFormatStringL, widthR, heightR, pixelSizeR, pixelFormatStringR);
     *
     * @return True if the values were returned OK, false if there is currently no stereo video source or an error occurred.
     * @param widthL An int array, the first element of which will be filled with the width (in pixels) of the video frame, or null if this information is not required.
     * @param widthR An int array, the first element of which will be filled with the width (in pixels) of the video frame, or null if this information is not required.
     * @param heightL An int array, the first element of which will be filled with the height (in pixels) of the video frame, or null if this information is not required.
     * @param heightR An int array, the first element of which will be filled with the height (in pixels) of the video frame, or null if this information is not required.
     * @param pixelSizeL An int array, the first element of which will be filled with the numbers of bytes per pixel of the source frame, or null if this information is not required.
     * @param pixelSizeR An int array, the first element of which will be filled with the numbers of bytes per pixel of the source frame, or null if this information is not required.
     * @param pixelFormatStringL A String array, the first element of which will be filled with the symbolic name of the pixel format of the video frame, or null if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringR A String array, the first element of which will be filled with the symbolic name of the pixel format of the video frame, or null if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @see {@link #arwGetVideoParams(int[], int[], int[], String[])}
     */
    public static native boolean arwGetVideoParamsStereo(int[] widthL, int[] heightL, int[] pixelSizeL, String[] pixelFormatStringL, int[] widthR, int[] heightR, int[] pixelSizeR, String[] pixelFormatStringR);

    /**
     * Checks if a new video frame is available.
     *
     * @return true if a new frame is available.
     */
    public static native boolean arwCapture();

    /**
     * Performs an update, runs tracking if in the running state.
     *
     * @return true if no error occurred, otherwise false
     */
    public static native boolean arwUpdateAR();

    public static native boolean arwUpdateTexture32(byte[] image);

    public static native boolean arwUpdateTextureStereo32(byte[] imageL, byte[] imageR);

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
	 * @see {@link #arwIsRunning()}
	 * @see {@link #arwDrawVideoFinal(int)}
	 */
	public static native boolean arwDrawVideoInit(int videoSourceIndex);

	/**
	 * Specifies desired horizontal alignment of video frames in drawing graphics context.
	 */
	public static final int ARW_H_ALIGN_LEFT = 0,       ///< Align the left edge of the video frame with the left edge of the context.
		ARW_H_ALIGN_CENTRE = 1,     ///< Align the centre of the video frame with the centre of the context.
		ARW_H_ALIGN_RIGHT = 2;      ///< Align the right edge of the video frame with the right edge of the context.

	/**
	 * Specifies desired vertical alignment of video frames in drawing graphics context.
	 */
	public static final int ARW_V_ALIGN_TOP = 0,        ///< Align the top edge of the video frame with the top edge of the context.
		ARW_V_ALIGN_CENTRE = 1,     ///< Align the centre of the video frame with the centre of the context.
		ARW_V_ALIGN_BOTTOM = 2;     ///< Align the bottom edge of the video frame with the bottom edge of the context.

	/**
	 * Specifies desired scaling of video frames to drawing graphics context.
	 */
	public static final int ARW_SCALE_MODE_FIT = 0,     ///< Scale the video frame proportionally up or down so that it fits visible in its entirety in the graphics context. When the graphics context is wider than the frame, it will be pillarboxed. When the graphics context is taller than the frame, it will be letterboxed.
		ARW_SCALE_MODE_FILL = 1,    ///< Scale the video frame proportionally up or down so that it fills the entire in the graphics context. When the graphics context is wider than the frame, it will be cropped top and/or bottom. When the graphics context is taller than the frame, it will be cropped left and/or right.
		ARW_SCALE_MODE_STRETCH = 2, ///< Scale the video frame un-proportionally up or down so that it matches exactly the size of the graphics context.
		ARW_SCALE_MODE_1_TO_1 = 3;  ///< Do not scale the video frame. One pixel of the video frame will be represented by one pixel of the graphics context.

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
	 * (typically from the rendering thread) and only while arwIsRunning is true
	 * and only between calls to arwDrawVideoInit and arwDrawVideoFinal.
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
	 * @see {@link #arwIsRunning}
	 * @see {@link #arwDrawVideoInit}
	 * @see {@link #arwDrawVideoFinal}
	 */
	public static native boolean arwDrawVideoSettings(int videoSourceIndex, int width, int height, boolean rotate90, boolean flipH, boolean flipV, int hAlign, int vAlign, int scalingMode, int[] viewport);

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
	 * @see {@link #arwIsRunning}
	 * @see {@link #arwDrawVideoInit}
	 * @see {@link #arwDrawVideoFinal}
	 * @see {@link #arwDrawVideoSettings}
	 */
	public static native boolean arwDrawVideo(int videoSourceIndex);

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
	 * @see {@link #arwDrawVideoInit(int)}
	 */
	public static native boolean arwDrawVideoFinal(int videoSourceIndex);

    /**
     * Adds a marker to be detected.
     *
     * @param cfg Marker configuration string
     * @return A unique identifier (UID) of the new marker, or -1 if the marker was not added due to an error.
     */
    public static native int arwAddTrackable(String cfg);

    /**
     * Removes the specified marker.
     *
     * @param trackableUID The unique identifier (UID) of the trackable to remove
     * @return true if the marker was removed, otherwise false
     */
    public static native boolean arwRemoveTrackable(int trackableUID);

    /**
     * Removes all loaded markers.
     *
     * @return The number of markers removed
     */
    public static native int arwRemoveAllTrackables();

    /**
     * Returns the visibility and pose of the specified marker.
     *
     * @param trackableUID The unique identifier (UID) of the trackable to check
     * @param matrix A float array to populate with an OpenGL-compatible transformation matrix.
     * @return true if the marker is currently visible, otherwise false
     */
    public static native boolean arwQueryTrackableVisibilityAndTransformation(int trackableUID, float[] matrix);

    /**
     * Returns the visibility and stereo pose of the specified marker.
     *
     * @param trackableUID The unique identifier (UID) of the trackable to check
     * @param matrixL   A float array to populate with an OpenGL-compatible transformation matrix for the left camera.
     * @param matrixR   A float array to populate with an OpenGL-compatible transformation matrix for the right camera.
     * @return true if the marker is currently visible, otherwise false.
     */
    public static native boolean arwQueryTrackableVisibilityAndTransformationStereo(int trackableUID, float[] matrixL, float[] matrixR);

    public static final int ARW_TRACKER_OPTION_NFT_MULTIMODE = 0,                          ///< bool.
    						ARW_TRACKER_OPTION_SQUARE_THRESHOLD = 1,                       ///< Threshold value used for image binarization. int in range [0-255].
    						ARW_TRACKER_OPTION_SQUARE_THRESHOLD_MODE = 2,                  ///< Threshold mode used for image binarization. int.
    						ARW_TRACKER_OPTION_SQUARE_LABELING_MODE = 3,                   ///< int.
    						ARW_TRACKER_OPTION_SQUARE_PATTERN_DETECTION_MODE = 4,          ///< int.
    						ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE = 5,                     ///< float in range (0-0.5).
    						ARW_TRACKER_OPTION_SQUARE_MATRIX_CODE_TYPE = 6,                ///< int.
    						ARW_TRACKER_OPTION_SQUARE_IMAGE_PROC_MODE = 7,                 ///< int.
    						ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE = 8,                      ///< Enables or disable state of debug mode in the tracker. When enabled, a black and white debug image is generated during marker detection. The debug image is useful for visualising the binarization process and choosing a threshold value. bool.
                            ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE = 9,                    ///< Number of rows and columns in square template (pattern) markers. Defaults to AR_PATT_SIZE1, which is 16 in all versions of ARToolKit prior to 5.3. int.
                            ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX = 10;              ///< Maximum number of square template (pattern) markers that may be loaded at once. Defaults to AR_PATT_NUM_MAX, which is at least 25 in all versions of ARToolKit prior to 5.3. int.

    // ARW_TRACKER_OPTION_SQUARE_THRESHOLD_MODE
    public static final int AR_LABELING_THRESH_MODE_MANUAL = 0,
    	    				AR_LABELING_THRESH_MODE_AUTO_MEDIAN = 1,
    	    				AR_LABELING_THRESH_MODE_AUTO_OTSU = 2,
    	    				AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE = 3,
							AR_LABELING_THRESH_MODE_AUTO_BRACKETING = 4;

    // ARW_TRACKER_OPTION_SQUARE_LABELING_MODE
	public static final int AR_LABELING_WHITE_REGION = 0,
    						AR_LABELING_BLACK_REGION = 1;

    // ARW_TRACKER_OPTION_SQUARE_PATTERN_DETECTION_MODE
	public static final int AR_TEMPLATE_MATCHING_COLOR               = 0,
    						AR_TEMPLATE_MATCHING_MONO                = 1,
    						AR_MATRIX_CODE_DETECTION                 = 2,
    						AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX    = 3,
    						AR_TEMPLATE_MATCHING_MONO_AND_MATRIX     = 4;

    // ARW_TRACKER_OPTION_SQUARE_MATRIX_CODE_TYPE
	public static final int AR_MATRIX_CODE_3x3 = 0x03,                                                  // Matrix code in range 0-63.
    						AR_MATRIX_CODE_3x3_PARITY65 = 0x103,                                        // Matrix code in range 0-31.
    						AR_MATRIX_CODE_3x3_HAMMING63 = 0x203,                                       // Matrix code in range 0-7.
    						AR_MATRIX_CODE_4x4 = 0x04,                                                  // Matrix code in range 0-8191.
    						AR_MATRIX_CODE_4x4_BCH_13_9_3 = 0x304,                                      // Matrix code in range 0-511.
    						AR_MATRIX_CODE_4x4_BCH_13_5_5 = 0x404,                                      // Matrix code in range 0-31.
    						AR_MATRIX_CODE_5x5_BCH_22_12_5 = 0x405,                                     // Matrix code in range 0-4095.
    						AR_MATRIX_CODE_5x5_BCH_22_7_7 = 0x505,                                      // Matrix code in range 0-127.
    						AR_MATRIX_CODE_5x5 = 0x05,                                                  // Matrix code in range 0-4194303.
    						AR_MATRIX_CODE_6x6 = 0x06,                                                  // Matrix code in range 0-8589934591.
    						AR_MATRIX_CODE_GLOBAL_ID = 0xb0e;

	public static final int AR_IMAGE_PROC_FRAME_IMAGE = 0,
    						AR_IMAGE_PROC_FIELD_IMAGE = 1;

    public static native void arwSetTrackerOptionBool(int option, boolean value);

    public static native void arwSetTrackerOptionInt(int option, int value);

    public static native void arwSetTrackerOptionFloat(int option, float value);

    public static native boolean arwGetTrackerOptionBool(int option);

    public static native int arwGetTrackerOptionInt(int option);

    public static native float arwGetTrackerOptionFloat(int option);

    public static final int ARW_TRACKABLE_OPTION_FILTERED = 1,
    						ARW_TRACKABLE_OPTION_FILTER_SAMPLE_RATE = 2,
    						ARW_TRACKABLE_OPTION_FILTER_CUTOFF_FREQ = 3,
    						ARW_TRACKABLE_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION = 4,
    						ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE = 5,
    						ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE_CUTOFF = 6,
                            ARW_TRACKABLE_OPTION_NFT_SCALE = 7,
                            ARW_TRACKABLE_OPTION_MULTI_MIN_SUBMARKERS = 8,
                            ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_MATRIX = 9,
                            ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_PATTERN = 10;

    public static native void arwSetTrackableOptionBool(int trackableUID, int option, boolean value);

    public static native void arwSetTrackableOptionInt(int trackableUID, int option, int value);

    public static native void arwSetTrackableOptionFloat(int trackableUID, int option, float value);

    public static native boolean arwGetTrackableOptionBool(int trackableUID, int option);

    public static native int arwGetTrackableOptionInt(int trackableUID, int option);

    public static native float arwGetTrackableOptionFloat(int trackableUID, int option);

    public static final int AR_PIXEL_FORMAT_INVALID = -1,
                            AR_PIXEL_FORMAT_RGB = 0,
                            AR_PIXEL_FORMAT_BGR = 1,
                            AR_PIXEL_FORMAT_RGBA = 2,
                            AR_PIXEL_FORMAT_BGRA = 3,
                            AR_PIXEL_FORMAT_ABGR = 4,
                            AR_PIXEL_FORMAT_MONO = 5,
                            AR_PIXEL_FORMAT_ARGB = 6,
                            AR_PIXEL_FORMAT_2vuy = 7,
                            AR_PIXEL_FORMAT_yuvs = 8,
                            AR_PIXEL_FORMAT_RGB_565 = 9,
                            AR_PIXEL_FORMAT_RGBA_5551 = 10,
                            AR_PIXEL_FORMAT_RGBA_4444 = 11,
                            AR_PIXEL_FORMAT_420v = 12,
                            AR_PIXEL_FORMAT_420f = 13,
                            AR_PIXEL_FORMAT_NV21 = 14;

    /**
     * Tells the native library the source and size and format in which video frames will be pushed.
     * N.B.: Pushing of video from Java to native is no longer required since artoolkitX v1.1.10 introduced native video capture.
     * This amended API was added in artoolkitX v1.2 to support tracking from external video sources.
     * This call may only be made after a call to arwStartRunning or arwStartRunningStereo.
     * @param videoSourceIndex Zero-based index of the video source which is being initialized for pushing. Normally 0, but for the second camera in a stereo pair, 1.
     * @param width			Width of the video frame in pixels.
     * @param height		Height of the video frame in pixels.
     * @param pixelFormat   string with format in which buffers will be pushed. Supported values include "NV21", "NV12", "YUV_420_888", "RGBA", "RGB_565", and "MONO".
     * @param camera_index	Zero-based index into the devices's list of cameras. If only one camera is present on the device, will be 0.
     * @param camera_face   0 if camera is rear-facing (the default) or 1 if camera is facing toward the user.
     * @return				0 if no error occurred, otherwise an error value less than 0.
     */
    public static native int arwVideoPushInit(int videoSourceIndex, int width, int height, String pixelFormat, int camera_index, int camera_face);

    /**
     * Pushes a video frame to the native library.
     * N.B.: Pushing of video from Java to native is no longer required since artoolkitX v1.1.10 introduced native video capture.
     * This amended API was added in artoolkitX v1.2 to support tracking from external video sources.
     * May only be made after calling arwVideoPushInit and may not be made after a call to arwVideoPushFinal.
     * @param videoSourceIndex Zero-based index of the video source which is being pushed. Normally 0, but for the second camera in a stereo pair, 1.
     * @param buf0			For interleaved formats (e.g. RGBA), reference to a byte buffer holding the frame data. For interleaved formats this will be the only plane. For planar formats, reference to a byte buffer holding plane 0 of the frame. For planar NV21 and YUV_420_888 formats, this will be the luma plane.
	 * @param buf0PixelStride The number of bytes between pixels in adjacent columns in buf0
	 * @param buf0RowStride The number of bytes between pixels in adjacent rows in buf0
	 * @param buf1			For planar formats consisting of 2 or more planes, reference to a byte buffer holding plane 1 of the frame. For planar NV21 image format, this will be the chroma plane. For planar YUV_420_888 format, this will be the Cb chroma plane.
	 * @param buf1PixelStride The number of bytes between pixels in adjacent columns in buf1
	 * @param buf1RowStride The number of bytes between pixels in adjacent rows in buf1
     * @param buf2			For planar formats consisting 3 or more planes, reference to a byte buffer holding plane 2 of the frame. For planar YUV_420_888 format, this will be the Cr chroma plane.
	 * @param buf2PixelStride The number of bytes between pixels in adjacent columns in buf2
	 * @param buf2RowStride The number of bytes between pixels in adjacent rows in buf2
     * @param buf3			For planar formats consisting of 4 planes, reference to a byte buffer holding plane 3 of the frame.
	 * @param buf3PixelStride The number of bytes between pixels in adjacent columns in buf3
	 * @param buf3RowStride The number of bytes between pixels in adjacent rows in buf3
     * @return				0 if no error occurred, otherwise an error value less than 0.
     */
    public static native int arwVideoPush(int videoSourceIndex,
										  ByteBuffer buf0, int buf0PixelStride, int buf0RowStride,
										  ByteBuffer buf1, int buf1PixelStride, int buf1RowStride,
										  ByteBuffer buf2, int buf2PixelStride, int buf2RowStride,
										  ByteBuffer buf3, int buf3PixelStride, int buf3RowStride);

    /**
     * Tells the native library that no further frames will be pushed.
     * N.B.: Pushing of video from Java to native is no longer required since artoolkitX v1.1.10 introduced native video capture.
     * This amended API was added in artoolkitX v1.2 to support tracking from external video sources.
     * This call may only be made before a call to arwStopRunning.
     * @param videoSourceIndex Zero-based index of the video source which is being finalized for pushing. Normally 0, but for the second camera in a stereo pair, 1.
     * @return				0 if no error occurred, otherwise an error value less than 0.
     */
    public static native int arwVideoPushFinal(int videoSourceIndex);

}
