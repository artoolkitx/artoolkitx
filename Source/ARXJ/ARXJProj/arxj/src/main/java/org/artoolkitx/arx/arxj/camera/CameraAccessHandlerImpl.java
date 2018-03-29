package org.artoolkitx.arx.arxj.camera;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.widget.Toast;

import org.artoolkitx.arx.arxj.ARActivity;

/*
 *  CameraAccessHandlerImpl.java
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
public class CameraAccessHandlerImpl implements CameraAccessHandler {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = CameraAccessHandlerImpl.class.getSimpleName();
    private final CameraSurfaceImpl mCameraSurface;
    private boolean mAskPermissionFirst = false;

    public CameraAccessHandlerImpl(Activity activity, CameraEventListener cameraEventListener) {
        Log.i(TAG, "CameraAccessHandlerImpl(): ctor called");
        Context mAppContext = activity.getApplicationContext();
        this.mCameraSurface = new CameraSurfaceImpl(cameraEventListener, mAppContext);

        try {
             if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                 if (PackageManager.PERMISSION_GRANTED != ContextCompat.checkSelfPermission(activity, Manifest.permission.CAMERA)) {
                     mAskPermissionFirst = true;
                     if (activity.shouldShowRequestPermissionRationale(Manifest.permission.CAMERA)) {
                         // Will drop in here if user denied permissions access camera before.
                         // Or no uses-permission CAMERA element is in the
                         // manifest file. Must explain to the end user why the app wants
                         // permissions to the camera devices.
                         Toast.makeText(activity.getApplicationContext(),
                                 "App requires access to camera to be granted",
                                 Toast.LENGTH_SHORT).show();
                     }

                     // Request permission from the user to access the camera.
                     Log.i(TAG, "CameraAccessHandler(): ask for camera access permission");
                     activity.requestPermissions(new String[] { Manifest.permission.CAMERA }, ARActivity.REQUEST_CAMERA_PERMISSION_RESULT);
                 }
             }
         } catch (Exception ex) {
             Log.e(TAG, "CameraAccessHandler(): exception , " + ex.getMessage());
         }
     }

    @Override
    public void resetCameraAccessPermissionsFromUser() {
        mAskPermissionFirst = false;
    }

    @Override
    public boolean getCameraAccessPermissions() {
        return this.mAskPermissionFirst;
    }

    @Override
    public void closeCamera() {
        this.getCameraSurfaceView().closeCameraDevice();
    }

    @Override
    public CameraSurface getCameraSurfaceView() {
        return this.mCameraSurface;
    }
}
