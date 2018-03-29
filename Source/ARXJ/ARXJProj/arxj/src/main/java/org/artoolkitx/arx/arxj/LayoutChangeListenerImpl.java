package org.artoolkitx.arx.arxj;

import android.app.Activity;
import android.util.Log;
import android.view.View;

import org.artoolkitx.arx.arxj.camera.CameraAccessHandler;
import org.artoolkitx.arx.arxj.camera.CameraSurface;

/*
 *  LayoutChangeListenerImpl.java
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

class LayoutChangeListenerImpl implements View.OnLayoutChangeListener {
    private final static String TAG = LayoutChangeListenerImpl.class.getSimpleName();
    private final Activity activity;
    private final CameraAccessHandler cameraAccessHandler;

    public LayoutChangeListenerImpl(Activity activity, CameraAccessHandler cameraAccessHandler) {
        this.activity = activity;
        this.cameraAccessHandler = cameraAccessHandler;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
        View decorView = activity.getWindow().getDecorView();
        CameraSurface cameraSurface = cameraAccessHandler.getCameraSurfaceView();
        if (AndroidUtils.VIEW_VISIBILITY == decorView.getSystemUiVisibility()) {
            if (!cameraAccessHandler.getCameraAccessPermissions()) {

                if (!cameraSurface.isImageReaderCreated()) {
                    cameraSurface.surfaceCreated();
                }
                if (!cameraSurface.isCamera2DeviceOpen())
                    cameraSurface.surfaceChanged();
            }
        } else{
            Log.v(TAG,"Not in fullscreen.");
        }
    }
}
