package org.artoolkitx.arx.arxj.camera;

import android.app.Activity;
import android.util.Log;
import android.widget.Toast;

import org.artoolkitx.arx.arxj.ARController;

import java.nio.ByteBuffer;

/*
 *  CameraEventListenerImpl.java
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

public class CameraEventListenerImpl implements CameraEventListener {

    private final Activity arActivity;
    private static final String TAG = CameraEventListenerImpl.class.getName();
    private final FrameListener frameListener;
    private boolean firstUpdate;
    private int cameraIndex;

    public CameraEventListenerImpl(Activity arActivity, FrameListener frameCallbackListener) {
        this.arActivity = arActivity;
        this.frameListener = frameCallbackListener;
    }

    @Override
    public void cameraStreamStarted(int width, int height, String pixelFormat, int cameraIndex, boolean cameraIsFrontFacing) {
        this.cameraIndex = cameraIndex;
        if (ARController.getInstance().startWithPushedVideo(width, height, pixelFormat, null, cameraIndex, cameraIsFrontFacing)) {
            // Expects Data to be already in the cache dir. This can be done with the AssetUnpacker.
            Log.i(TAG, "Initialised AR.");
        } else {
            // Error
            Log.e(TAG, "Error initialising AR. Cannot continue.");
            arActivity.finish();
        }

        Toast.makeText(arActivity, "Camera settings: " + width + "x" + height, Toast.LENGTH_SHORT).show();
        firstUpdate = true;
    }

    @Override
    public void cameraStreamFrame(byte[] frame, int frameSize) {
        if (firstUpdate) {
            frameListener.firstFrame(cameraIndex);
            firstUpdate = false;
        }

        if (ARController.getInstance().convertAndDetect1(frame, frameSize)) {
            frameListener.onFrameProcessed();
        }
    }

    @Override
    public void cameraStreamFrame(ByteBuffer[] framePlanes, int[] framePlanePixelStrides, int[] framePlaneRowStrides) {
        if (firstUpdate) {
            frameListener.firstFrame(cameraIndex);
            firstUpdate = false;
        }

        if (ARController.getInstance().convertAndDetect2(framePlanes, framePlanePixelStrides, framePlaneRowStrides)) {
            frameListener.onFrameProcessed();
        }
    }

    @Override
    public void cameraStreamStopped() {
        ARController.getInstance().stopAndFinal();
    }
}
