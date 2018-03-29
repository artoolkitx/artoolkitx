package org.artoolkitx.arx.arxj.camera;

import android.Manifest;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/*
 *  CameraSurfaceImpl.java
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
 *  Author(s): Philip Lamb, Thorsten Bux, John Wolf
 *
 */
class CameraSurfaceImpl implements CameraSurface {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = CameraSurfaceImpl.class.getSimpleName();
    private CameraDevice mCameraDevice;
    private ImageReader mImageReader;
    private Size mImageReaderVideoSize;
    private final Context mAppContext;

    private final CameraDevice.StateCallback mCamera2DeviceStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(@NonNull CameraDevice camera2DeviceInstance) {
            mCameraDevice = camera2DeviceInstance;
            startCaptureAndForwardFramesSession();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera2DeviceInstance) {
            camera2DeviceInstance.close();
            mCameraDevice = null;
        }

        @Override
        public void onError(@NonNull CameraDevice camera2DeviceInstance, int error) {
            camera2DeviceInstance.close();
            mCameraDevice = null;
        }
    };

    /**
     * Listener to inform of camera related events: start, frame, and stop.
     */
    private final CameraEventListener mCameraEventListener;
    /**
     * Tracks if SurfaceView instance was created.
     */
    private boolean mImageReaderCreated;

    public CameraSurfaceImpl(CameraEventListener cameraEventListener, Context appContext){
        this.mCameraEventListener = cameraEventListener;
        this.mAppContext = appContext;
    }


    private final ImageReader.OnImageAvailableListener mImageAvailableAndProcessHandler = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader)
        {

            Image imageInstance = reader.acquireLatestImage();
            if (imageInstance == null) {
                //Note: This seems to happen quite often.
                Log.v(TAG, "onImageAvailable(): unable to acquire new image");
                return;
            }

            // Get a ByteBuffer for each plane.
            final Image.Plane[] imagePlanes = imageInstance.getPlanes();
            final int imagePlaneCount = Math.min(4, imagePlanes.length); // We can handle up to 4 planes max.
            final ByteBuffer[] imageBuffers = new ByteBuffer[imagePlaneCount];
            final int[] imageBufferPixelStrides = new int[imagePlaneCount];
            final int[] imageBufferRowStrides = new int[imagePlaneCount];
            for (int i = 0; i < imagePlaneCount; i++) {
                imageBuffers[i] = imagePlanes[i].getBuffer();
                // For ImageFormat.YUV_420_888 the order of planes in the array returned by Image.getPlanes()
                // is guaranteed such that plane #0 is always Y, plane #1 is always U (Cb), and plane #2 is always V (Cr).
                // The Y-plane is guaranteed not to be interleaved with the U/V planes (in particular, pixel stride is
                // always 1 in yPlane.getPixelStride()). The U/V planes are guaranteed to have the same row stride and
                // pixel stride (in particular, uPlane.getRowStride() == vPlane.getRowStride() and uPlane.getPixelStride() == vPlane.getPixelStride(); ).
                imageBufferPixelStrides[i] = imagePlanes[i].getPixelStride();
                imageBufferRowStrides[i] = imagePlanes[i].getRowStride();
            }

            if (mCameraEventListener != null) {
                mCameraEventListener.cameraStreamFrame(imageBuffers, imageBufferPixelStrides, imageBufferRowStrides);
            }

            imageInstance.close();
        }
    };

    @Override
    public void surfaceCreated() {
        Log.i(TAG, "surfaceCreated(): called");

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mAppContext);
        int defaultCameraIndexId = mAppContext.getResources().getIdentifier("pref_defaultValue_cameraIndex","string", mAppContext.getPackageName());
        mCamera2DeviceID = Integer.parseInt(prefs.getString("pref_cameraIndex", mAppContext.getResources().getString(defaultCameraIndexId)));
        Log.i(TAG, "surfaceCreated(): will attempt to open camera \"" + mCamera2DeviceID +
                "\", set orientation, set preview surface");

        /*
        Set the resolution from the settings as size for the glView. Because the video stream capture
        is requested based on this size.

        WARNING: While coding the preferences are taken from the res/xml/preferences.xml!!!
        When building for Unity the actual used preferences are taken from the UnityARPlayer project!!!
        */
        int defaultCameraValueId = mAppContext.getResources().getIdentifier("pref_defaultValue_cameraResolution","string",mAppContext.getPackageName());
        String camResolution = prefs.getString("pref_cameraResolution", mAppContext.getResources(). getString(defaultCameraValueId));
        String[] dims = camResolution.split("x", 2);
        mImageReaderVideoSize =  new Size(Integer.parseInt(dims[0]),Integer.parseInt(dims[1]));

        // Note that maxImages should be at least 2 for acquireLatestImage() to be any different than acquireNextImage() -
        // discarding all-but-the-newest Image requires temporarily acquiring two Images at once. Or more generally,
        // calling acquireLatestImage() with less than two images of margin, that is (maxImages - currentAcquiredImages < 2)
        // will not discard as expected.
        mImageReader = ImageReader.newInstance(mImageReaderVideoSize.getWidth(),mImageReaderVideoSize.getHeight(), ImageFormat.YUV_420_888, /* The maximum number of images the user will want to access simultaneously:*/ 2 );
        mImageReader.setOnImageAvailableListener(mImageAvailableAndProcessHandler, null);

        mImageReaderCreated = true;

    } // end: public void surfaceCreated(SurfaceHolder holder)

    /* Interface implemented by this SurfaceView subclass
       holder: SurfaceHolder instance associated with SurfaceView instance that changed
       format: pixel format of the surface
       width: of the SurfaceView instance
       height: of the SurfaceView instance
    */
    @Override
    public void surfaceChanged() {
        Log.i(TAG, "surfaceChanged(): called");

        // This is where to calculate the optimal size of the display and set the aspect ratio
        // of the surface view (probably the service holder). Also where to Create transformation
        // matrix to scale and then rotate surface view, if the app is going to handle orientation
        // changes.
        if (!mImageReaderCreated) {
            surfaceCreated();
        }
        if (!isCamera2DeviceOpen()) {
            openCamera2(mCamera2DeviceID);
        }
        if (isCamera2DeviceOpen() && (null == mYUV_CaptureAndSendSession)) {
            startCaptureAndForwardFramesSession();
        }
    }

    private void openCamera2(int camera2DeviceID) {
        Log.i(TAG, "openCamera2(): called");
        CameraManager camera2DeviceMgr = (CameraManager)mAppContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            if (PackageManager.PERMISSION_GRANTED == ContextCompat.checkSelfPermission(mAppContext, Manifest.permission.CAMERA)) {
                camera2DeviceMgr.openCamera(Integer.toString(camera2DeviceID), mCamera2DeviceStateCallback, null);
                return;
            }
        } catch (CameraAccessException ex) {
            Log.e(TAG, "openCamera2(): CameraAccessException caught, " + ex.getMessage());
        } catch (Exception ex) {
            Log.e(TAG, "openCamera2(): exception caught, " + ex.getMessage());
        }
        if (null == camera2DeviceMgr) {
            Log.e(TAG, "openCamera2(): Camera2 DeviceMgr not set");
        }
        Log.e(TAG, "openCamera2(): abnormal exit");
    }

    private int mCamera2DeviceID = -1;
    private CaptureRequest.Builder mCaptureRequestBuilder;
    private CameraCaptureSession mYUV_CaptureAndSendSession;

    private void startCaptureAndForwardFramesSession() {

        if ((null == mCameraDevice) || (!mImageReaderCreated) /*|| (null == mPreviewSize)*/) {
            return;
        }

        closeYUV_CaptureAndForwardSession();

        try {
            mCaptureRequestBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            List<Surface> surfaces = new ArrayList<>();

            Surface surfaceInstance;
            surfaceInstance = mImageReader.getSurface();
            surfaces.add(surfaceInstance);
            mCaptureRequestBuilder.addTarget(surfaceInstance);

            mCameraDevice.createCaptureSession(
                    surfaces, // Output surfaces
                    new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession session) {
                            try {
                                if (mCameraEventListener != null) {
                                    mCameraEventListener.cameraStreamStarted(mImageReaderVideoSize.getWidth(), mImageReaderVideoSize.getHeight(), "YUV_420_888", mCamera2DeviceID, false);
                                }
                                mYUV_CaptureAndSendSession = session;
                                // Session to repeat request to update passed in camSensorSurface
                                mYUV_CaptureAndSendSession.setRepeatingRequest(mCaptureRequestBuilder.build(), /* CameraCaptureSession.CaptureCallback cameraEventListener: */null, /* Background thread: */ null);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                            Toast.makeText(mAppContext, "Unable to setup camera sensor capture session", Toast.LENGTH_SHORT).show();
                        }
                    }, // Callback for capture session state updates
                    null); // Secondary thread message queue
        } catch (CameraAccessException ex) {
            ex.printStackTrace();
        }
    }

    @Override
    public void closeCameraDevice() {
        closeYUV_CaptureAndForwardSession();
        if (null != mCameraDevice) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
        if (null != mImageReader) {
            mImageReader.close();
            mImageReader = null;
        }
        if (mCameraEventListener != null) {
            mCameraEventListener.cameraStreamStopped();
        }
        mImageReaderCreated = false;
    }

    private void closeYUV_CaptureAndForwardSession() {
        if (mYUV_CaptureAndSendSession != null) {
            mYUV_CaptureAndSendSession.close();
            mYUV_CaptureAndSendSession = null;
        }
    }

    /**
     * Indicates whether or not camera2 device instance is available, opened, enabled.
     */
    @Override
    public boolean isCamera2DeviceOpen() {
        return (null != mCameraDevice);
    }

    @Override
    public boolean isImageReaderCreated() {
        return mImageReaderCreated;
    }
}
