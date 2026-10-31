/*
 *  ARController.java
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb, Thorsten Bux
 *
 */

package org.artoolkitx.arx.arxj;

import android.opengl.Matrix;
import android.util.Log;

import java.nio.ByteBuffer;

/**
 * The ARController class is a singleton which manages access to the underlying
 * native functions defined in the {@link ARX_jni} class. Native calls
 * should be made only from within this class so that adequate error checking
 * and correct type conversion can take place.
 */
@SuppressWarnings("unused")
public class ARController {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = "ARController";
    /**
     * Set to true only once the native library has been loaded.
     */
    private static boolean loadedNative = false;
    private static boolean initedNative = false;
    /**
     * Single instance of the ARController class.
     */
    private static ARController instance = null;

    static {
        loadedNative = ARX_jni.loadNativeLibrary();
        if (!loadedNative) Log.e(TAG, "Loading native library failed!");
        else Log.i(TAG, "Loaded native library.");
    }

    /**
     * Private constructor as required by the singleton pattern.
     */
    private ARController() {
        Log.i(TAG, "ARController(): ARController constructor");
    }

    /**
     * Implementation of the singleton pattern to provide a sole instance of the ARController class.
     *
     * @return The single instance of ARController.
     */
    public static ARController getInstance() {
        if (instance == null) instance = new ARController();
        return instance;
    }

    /**
     * Initialises the native code library if it is available.
     *
     * @param resourcesDirectoryPath The full path (in the filesystem) to the directory to be used by the
     *                               native routines as the base for relative references.
     *                               e.g. Activity.getContext().getCacheDir().getAbsolutePath()
     *                               or Activity.getContext().getFilesDir().getAbsolutePath()
     * @return true if the library was found and successfully initialised.
     */
    public boolean initialiseNative(String resourcesDirectoryPath) {
        if (!loadedNative) return false;
        ARX_jni.arwSetLogLevel(ARX_jni.AR_LOG_LEVEL_DEBUG);
        if (!ARX_jni.arwInitialiseAR()) {
            Log.e(TAG, "Error initialising native library!");
            return false;
        }
        Log.i(TAG, "artoolkitX v" + ARX_jni.arwGetARToolKitVersion());
        if (!ARX_jni.arwChangeToResourcesDir(resourcesDirectoryPath)) {
            Log.i(TAG, "Error while attempting to change working directory to resources directory.");
        }
        initedNative = true;
        return true;
    }

    public String getARToolKitXVersion() {
        return ARX_jni.arwGetARToolKitVersion();
    }

    /**
     * Initialises the ARController.
     *
     * @param videoWidth     The width of the video image in pixels.
     * @param videoHeight    The height of the video image in pixels.
     * @param pixelFormat    string with format in which buffers will be pushed. Supported values include "NV21", "NV12", "YUV_420_888", "RGBA", "RGB_565", and "MONO".
	 * @param cameraParaPath Either: null to search for camera parameters specific to the device,
	 *            or a path (in the filesystem) to a camera parameter file. The path may be an
	 *            absolute path, or relative to the resourcesDirectoryPath set in initialiseNative().
     * @param cameraIsFrontFacing false to search for a rear-facing camera, or true to search for a camera facing toward the user.
	 * @param cameraIndex    Zero-based index of the camera in use, i.e. if 0, the first camera facing in the requested direction will be used, if 1 then the second etc.
     * @return true if initialisation was successful.
     */
    public boolean start(int videoWidth, int videoHeight, String pixelFormat, String cameraParaPath, int cameraIndex, boolean cameraIsFrontFacing) {

        if (!initedNative) {
            Log.e(TAG, "start(): Cannot start because native interface not inited.");
            return false;
        }

        // Not currently using cameraIsFrontFacing. String would be " -position=" + (cameraIsFrontFacing ? "front" : "back")
        if (!ARX_jni.arwStartRunning("-native -width=" + videoWidth + " -height=" + videoHeight + " -prefer=closestpixelcount -source=" + cameraIndex, cameraParaPath)) {
            Log.e(TAG, "start(): Error starting");
            return false;
        }

        return true;
    }

    /**
     * Returns whether the square tracker debug video image is enabled.
     *
     * @return Whether the square tracker debug video image is enabled.
     */
    public boolean getDebugMode() {
        if (!initedNative) return false;
        return ARX_jni.arwGetTrackerOptionBool(ARX_jni.ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE);
    }

    /**
     * Enables or disables the debug video image in the square tracker.
     *
     * @param debug Whether or not to enable the square tracker debug video image.
     */
    public void setDebugMode(boolean debug) {
        if (!initedNative) return;
        ARX_jni.arwSetTrackerOptionBool(ARX_jni.ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE, debug);
    }

    /**
     * Returns the threshold used to binarize the video image for square marker detection.
     *
     * @return The current threshold value in the range 0 to 255, or -1 if the
     * threshold could not be retrieved.
     */
    public int getThreshold() {
        if (!initedNative) return -1;
        return ARX_jni.arwGetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_THRESHOLD);
    }

    /**
     * Sets the threshold used to binarize the video image for square marker detection.
     *
     * @param threshold The new threshold value in the range 0 to 255.
     */
    public void setThreshold(int threshold) {
        if (!initedNative) return;
        ARX_jni.arwSetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_THRESHOLD, threshold);
    }

    public float getBorderSize() {
        if (!initedNative) return 0.0f;
        return ARX_jni.arwGetTrackerOptionFloat(ARX_jni.ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE);
    }

    public void setBorderSize(float size) {
        if (!initedNative) return;
        ARX_jni.arwSetTrackerOptionFloat(ARX_jni.ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE, size);
    }

    public int getPatternSize() {
        if (!initedNative) return 0;
        return ARX_jni.arwGetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE);
    }

    public void setPatternSize(int size) {
        if (!initedNative) return;
        ARX_jni.arwSetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE, size);
    }

    public int getPatternCountMax() {
        if (!initedNative) return 0;
        return ARX_jni.arwGetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX);
    }

    public void setPatternCountMax(int count) {
        if (!initedNative) return;
        ARX_jni.arwSetTrackerOptionInt(ARX_jni.ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX, count);
    }


    /**
     * Returns the projection matrix calculated from camera parameters.
     *
     * @param nearPlane The value to use for the near OpenGL clipping plane.
     * @param farPlane The value to use for the far OpenGL clipping plane.
     * @return A float array containing the OpenGL compatible projection matrix, or null if an error occurred.
     */
    public float[] getProjectionMatrix(float nearPlane, float farPlane) {
        if (!initedNative) return null;
        return ARX_jni.arwGetProjectionMatrix(nearPlane, farPlane);
    }

    /**
     * Adds a new single marker to the set of currently active markers.
     *
     * @param cfg The settings string for the used marker. Possible configurations look like this:
     *            `single;data/hiro.patt;80`
     *            `single_buffer;80;buffer=234 221 237...`
     *            `single_barcode;0;80`
     *            `multi;data/multi/marker.dat`
     *            `nft;data/nft/pinball`
     * @return The unique identifier (UID) of the new marker, or -1 on error.
     */
    public int addTrackable(String cfg) {
        if (!initedNative) return -1;
        return ARX_jni.arwAddTrackable(cfg);
    }

    /**
     * Returns whether the marker with the specified ID is currently visible, and if visible the trackable transformation.
     *
     * @param trackableUID The unique identifier (UID) of the trackable to query.
     * @return true if the marker is visible and tracked in the current video frame.
     */
    public boolean queryTrackableVisibilityAndTransformation(int trackableUID, float[] matrix) {
        if (!initedNative) return false;
        return ARX_jni.arwQueryTrackableVisibilityAndTransformation(trackableUID, matrix);
    }

	/**
	 * Queries whether artoolkitX is initialized. This will be true
	 * after a call to {@link #initialiseNative(String)}. At
	 * this point arwStartRunning can be called.
	 *
	 * @return true artoolkitX has been initialized
	 */
    public boolean isInited() {
        if (!initedNative) {
            initedNative = ARX_jni.arwIsInited();
        };
        return initedNative;
    }

    /**
     * Returns true when video and tracking are running.
     *
     * @return true when video and tracking are running, otherwise false
     */
    public boolean isRunning() {
        if (!initedNative) return false;
        return ARX_jni.arwIsRunning();
    }

    /**
     * Checks for a new frame available from the camera and captures it if so.
     *
     * @return true if frame was captured, otherwise false.
     */
    public boolean capture() {

        if (!initedNative) {
            return false;
        }
        return ARX_jni.arwCapture();
    }

    /**
     * Takes an incoming frame from the Android camera and passes it to native
     * code for conversion and tracking.
     *
     * @return true if successful, otherwise false.
     */
    public boolean update() {

        if (!initedNative) {
            return false;
        }
        return ARX_jni.arwUpdateAR();
    }

    /**
     * Stop and final.
     */
    public void stopAndFinal() {

        if (!initedNative) return;

        ARX_jni.arwStopRunning();
        ARX_jni.arwShutdownAR();

        initedNative = false;
    }

    /**
     * Calculates the reference matrix for the given markers. First marker is the base.
     *
     * @param idMarkerBase Reference base
     * @param idMarker2    Marker that will be depending on that base
     * @return Matrix that contains the transformation from @idMarkerBase to @idMarker2
     */
    @SuppressWarnings("WeakerAccess")
    public float[] calculateReferenceMatrix(int idMarkerBase, int idMarker2) {
        float[] referenceMarkerTranslationMatrix = new float[16];
        float[] secondMarkerTranslationMatrix = new float[16];
        if (this.queryTrackableVisibilityAndTransformation(idMarkerBase, referenceMarkerTranslationMatrix) && this.queryTrackableVisibilityAndTransformation(idMarker2, secondMarkerTranslationMatrix)) {
            float[] invertedMatrixOfReferenceMarker = new float[16];

            Matrix.invertM(invertedMatrixOfReferenceMarker, 0, referenceMarkerTranslationMatrix, 0);

            float[] transformationFromMarker1ToMarker2 = new float[16];
            Matrix.multiplyMM(transformationFromMarker1ToMarker2, 0, invertedMatrixOfReferenceMarker, 0, secondMarkerTranslationMatrix, 0);

            return transformationFromMarker1ToMarker2;
        } else {
            //It seems like ARToolkit might be faster with updating then the Android part. Because of that
            //it can happen that, even though one ensured in there Android-App that both markers are visible,
            //ARToolkit might not return a transformation matrix for both markers. In that case this RuntimeException is thrown.
            Log.e(TAG, "calculateReferenceMatrix(): Currently there are no two markers visible at the same time");
            return null;
        }
    }

    /**
     * Calculated the distance between two markers
     *
     * @param referenceMarker Reference base. Marker from which the distance is calculated
     * @param markerId2       Marker to which the distance is calculated
     * @return distance
     */
    public float distance(int referenceMarker, int markerId2) {

        float[] referenceMatrix = calculateReferenceMatrix(referenceMarker, markerId2);

        if (referenceMatrix != null) {
            float distanceX = referenceMatrix[12];
            float distanceY = referenceMatrix[13];
            float distanceZ = referenceMatrix[14];

            Log.d(TAG, "distance(): Marker distance: x: " + distanceX + " y: " + distanceY + " z: " + distanceZ);
            float length = Matrix.length(distanceX, distanceY, distanceZ);
            Log.d(TAG, "distance(): Absolute distance: " + length);

            return length;
        }
        return 0;
    }

    /**
     * Calculates the position depending on the referenceMarker
     *
     * @param referenceMarkerId           Reference marker id
     * @param markerIdToGetThePositionFor Id of the marker for which the position is calculated
     * @return Position vector with length 4 x,y,z,1
     */
    public float[] retrievePosition(int referenceMarkerId, int markerIdToGetThePositionFor) {
        float[] initialVector = {1f, 1f, 1f, 1f};
        float[] positionVector = new float[4];

        float[] transformationMatrix = calculateReferenceMatrix(referenceMarkerId, markerIdToGetThePositionFor);
        if (transformationMatrix != null) {
            Matrix.multiplyMV(positionVector, 0, transformationMatrix, 0, initialVector, 0);
            return positionVector;
        }
        return null;
    }

    /**
     * Perform required initialisation for future drawing of video frames.
     * Must be called from a rendering thread with an active rendering context.
     * @param videoSourceIndex 0, or a video source with multiple inputs, the 0-based index of the active video source.
     * @return true if successful
     */
    public boolean drawVideoInit(int videoSourceIndex){
        return ARX_jni.arwDrawVideoInit(videoSourceIndex);
    }

    /**
     * Set parameters of the rendering context for future drawing of video frames.
     * Must be called from a rendering thread with an active rendering context.
     * @param videoSourceIndex 0, or a video source with multiple inputs, the 0-based index of the active video source.
     * @return true if successful
     */
    public boolean drawVideoSettings(int videoSourceIndex, int width, int height, boolean rotate90, boolean flipH, boolean flipV, int hAlign, int vAlign, int scalingMode, int[] viewport) {
        return ARX_jni.arwDrawVideoSettings(videoSourceIndex, width, height, rotate90, flipH, flipV, hAlign, vAlign, scalingMode, viewport);
    }

    /**
     * Draw a video frame.
     * Must be called from a rendering thread with an active rendering context.
     * @param videoSourceIndex 0, or a video source with multiple inputs, the 0-based index of the active video source.
     * @return true if successful
     */
    @SuppressWarnings("WeakerAccess")
    public boolean drawVideo(int videoSourceIndex) {
        return ARX_jni.arwDrawVideo(videoSourceIndex);
    }

    /**
     * Perform required cleanup after drawing of video frames.
     * Must be called from a rendering thread with an active rendering context.
     * @param videoSourceIndex 0, or a video source with multiple inputs, the 0-based index of the active video source.
     * @return true if successful
     */
    @SuppressWarnings("WeakerAccess")
    public boolean drawVideoFinal(int videoSourceIndex) {
        return ARX_jni.arwDrawVideoFinal(videoSourceIndex);
    }
}
