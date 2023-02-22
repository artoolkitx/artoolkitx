/*
 *  ARActivity.java
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
 *  Author(s): Julian Looser, Philip Lamb, Thorsten Bux, John Wolf
 *
 */

package org.artoolkitx.arx.arxj;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.Choreographer;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.Toast;

import org.artoolkitx.arx.arxj.camera.CameraAccessHandler;
import org.artoolkitx.arx.arxj.camera.CameraAccessHandlerImpl;
import org.artoolkitx.arx.arxj.camera.CameraPreferencesActivity;
import org.artoolkitx.arx.arxj.rendering.ARRenderer;

/**
 * An activity which can be subclassed to create an AR application. ARActivity handles almost all of
 * the required operations to create a simple augmented reality application.
 * <br>
 * ARActivity automatically creates a camera preview surface and an OpenGL surface view, and
 * arranges these correctly in the user interface.The subclass simply needs to provide a FrameLayout
 * object which will be populated with these UI components, using {@link #supplyFrameLayout() supplyFrameLayout}.
 * <br>
 * To create a custom AR experience, the subclass should also provide a custom renderer using
 * {@link #supplyRenderer() Renderer}. This allows the subclass to handle OpenGL drawing calls on its own.
 */

public abstract class ARActivity extends /*AppCompat*/Activity implements View.OnClickListener {

    /**
     * Used to match-up permission user request to user response
     */
    public static final int REQUEST_CAMERA_PERMISSION_RESULT = 0;

    /**
     * Android logging tag for this class.
     */
    private final static String TAG = "ARXJ::ARActivity";

    /**
     * Renderer to use. This is provided by the subclass using {@link #supplyRenderer() Renderer()}.
     */
    private ARRenderer renderer;

    /**
     * Layout that will be filled with the camera preview and GL views. This is provided by the subclass using {@link #supplyFrameLayout() supplyFrameLayout()}.
     */
    private FrameLayout mainLayout;

    private Context mContext;
    private CameraAccessHandler mCameraAccessHandler;
    private ImageButton mConfigButton;
    private GLSurfaceView mGlView;
    private Choreographer.FrameCallback frameCallback = null;

    @SuppressWarnings("unused")
    public Context getAppContext() {
        return mContext;
    }

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    //@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = getApplicationContext();

        // This needs to be done just only the very first time the application is run,
        // or whenever a new preference is added (e.g. after an application upgrade).
        PreferenceManager.setDefaultValues(this, R.xml.preferences, false);


        // This locks the orientation. Hereafter, any API returning display orientation data will
        // return the data representing this orientation no matter the current position of the
        // device.
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        AndroidUtils.reportDisplayInformation(this);
    }

    /**
     * Allows subclasses to supply a custom {@link Renderer}.
     *
     * @return The {@link Renderer} to use.
     */
    protected abstract ARRenderer supplyRenderer();

    /**
     * Allows subclasses to supply a {@link FrameLayout} which will be populated
     * with a camera preview and GL surface view.
     *
     * @return The {@link FrameLayout} to use.
     */
    protected abstract FrameLayout supplyFrameLayout();

    @Override
    protected void onStart() {
        super.onStart();

        Log.i(TAG, "onStart(): called");
        // Use cache directory as root for native path references.
        // The AssetFileTransfer class can help with unpacking from the built .apk to the cache.
        if (!ARController.getInstance().initialiseNative(this.getCacheDir().getAbsolutePath())) {
            notifyFinish("The native ARX library could not be loaded.");
            return;
        }

        mainLayout = this.supplyFrameLayout();
        if (mainLayout == null) {
            Log.e(TAG, "onStart(): Error: supplyFrameLayout did not return a layout.");
            return;
        }

        renderer = supplyRenderer();
        if (renderer == null) {
            Log.e(TAG, "onStart(): Error: supplyRenderer did not return a renderer.");
            notifyFinish("You need to supply a renderer. Create your own renderer class (MyArRenderer) and derive it from ARRenderer.");
        }
    }

    @Override
    public void onResume() {
        Log.i(TAG, "onResume(): called");
        super.onResume();

        // Create the GL view, request an OpenGL ES 2.0 compatible context, and set renderer.
        // In case of using this method from Unity we do not provide a renderer.
        mGlView = new GLSurfaceView(this);
        mGlView.setEGLContextClientVersion(2);
        if (renderer != null) {
            mGlView.setRenderer(renderer);
        }
        mGlView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY); // Only render when we have a frame (must call requestRender()).

        mCameraAccessHandler = new CameraAccessHandlerImpl(this);
        mGlView.addOnLayoutChangeListener(new LayoutChangeListenerImpl(this, mCameraAccessHandler));

        Log.i(TAG, "onResume(): GLSurfaceView created");

        // Add the OpenGL view which will be used to render the video background and the virtual environment.
        mainLayout.addView(mGlView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        Log.i(TAG, "onResume(): Views added to main layout.");
        mGlView.onResume();

        if (mCameraAccessHandler.getCameraAccessPermissions()) {
            //No need to go further, must ask user to allow access to the camera first.
            return;
        }

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
        int defaultCameraIndexId = mContext.getResources().getIdentifier("pref_defaultValue_cameraIndex","string", mContext.getPackageName());
        int cameraIndex = Integer.parseInt(prefs.getString("pref_cameraIndex", mContext.getResources().getString(defaultCameraIndexId)));
        Log.i(TAG, "Will attempt to open camera \"" + cameraIndex + "\".");

        /*
        Set the resolution from the settings as size for the glView. Because the video stream capture
        is requested based on this size.

        WARNING: While coding the preferences are taken from the res/xml/preferences.xml!!!
        When building for Unity the actual used preferences are taken from the UnityARPlayer project!!!
        */
        int defaultCameraResolutionId = mContext.getResources().getIdentifier("pref_defaultValue_cameraResolution","string", mContext.getPackageName());
        String camResolution = prefs.getString("pref_cameraResolution", mContext.getResources(). getString(defaultCameraResolutionId));
        String[] dims = camResolution.split("x", 2);
        int width = Integer.parseInt(dims[0]);
        int height = Integer.parseInt(dims[1]);
        
        if (ARController.getInstance().start(width, height, "YUV_420_888", null, cameraIndex, false)) {
            // Expects Data to be already in the cache dir. This can be done with the AssetUnpacker.
            Log.i(TAG, "Initialised AR.");
        } else {
            // Error
            Log.e(TAG, "Error initialising AR. Cannot continue.");
            this.finish();
        }

        Toast.makeText(this, "Camera settings: " + width + "x" + height, Toast.LENGTH_SHORT).show();

        if (renderer != null) {
            renderer.setCameraIndex(cameraIndex);
            if (renderer.configureARScene()) {
                Log.i(TAG, "Scene configured successfully");
            } else {
                // Error
                Log.e(TAG, "Error configuring scene. Cannot continue.");
                this.finish();
            }
        }

        //Load settings button
        View settingsButtonLayout = this.getLayoutInflater().inflate(R.layout.settings, mainLayout, false);
        mConfigButton = settingsButtonLayout.findViewById(R.id.button_config);
        mainLayout.addView(settingsButtonLayout);
        mConfigButton.setOnClickListener(this);

        // Set up our frame update loop.
        if (frameCallback == null) {
            frameCallback = new Choreographer.FrameCallback() {
                @Override
                public void doFrame(long l) {
                    if (ARController.getInstance().capture()) {
                        if (!ARController.getInstance().update()) {
                            Log.e(TAG, "Error updating AR trackers.");
                        } else {
                            // Update the renderer as the frame has changed
                            if (mGlView != null) {
                                mGlView.requestRender();
                            }
                        }
                    }
                    if (frameCallback != null) {
                        Choreographer.getInstance().postFrameCallback(frameCallback);
                    }
                }
            };
        }
        Choreographer.getInstance().postFrameCallback(frameCallback);
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "onPause(): called");

        if (!mCameraAccessHandler.getCameraAccessPermissions()) {
            if (frameCallback != null) {
                Choreographer.getInstance().removeFrameCallback(frameCallback);
                frameCallback = null;
            }
            ARController.getInstance().stopAndFinal();
        }


        if (mGlView != null) {
            mGlView.onPause();
            mainLayout.removeView(mGlView);
        }

        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "onStop(): Activity stopping.");
        super.onStop();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.options, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.settings) {
            startActivity(new Intent(this, CameraPreferencesActivity.class));
            return true;
        } else {
            return super.onOptionsItemSelected(item);
        }
    }

    @SuppressWarnings("unused")
    protected void showInfo() {

        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);

        dialogBuilder.setMessage("artoolkitX v" + ARX_jni.arwGetARToolKitVersion());

        dialogBuilder.setCancelable(false);
        dialogBuilder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });

        AlertDialog alert = dialogBuilder.create();
        alert.setTitle("artoolkitX");
        alert.show();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        View decorView = getWindow().getDecorView();
        if (hasFocus) {
            // Now can configure view to run  full screen
            decorView.setSystemUiVisibility(AndroidUtils.VIEW_VISIBILITY);
        }
    }

    @Override
    public void onClick(View v) {
        if (v.equals(mConfigButton)) {
            v.getContext().startActivity(new Intent(v.getContext(), CameraPreferencesActivity.class));
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        Log.i(TAG, "onRequestPermissionsResult(): called");

        if (requestCode == REQUEST_CAMERA_PERMISSION_RESULT) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                notifyFinish("Application will not run with camera access denied");
            } else if (1 <= permissions.length) {
                Toast.makeText(getApplicationContext(),
                        String.format("Camera access permission \"%s\" allowed", permissions[0]),
                        Toast.LENGTH_SHORT).show();
            }
            Log.i(TAG, "onRequestPermissionsResult(): reset ask for cam access perm");
            mCameraAccessHandler.resetCameraAccessPermissionsFromUser();
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
        onResume();
    }

    /**
     * Returns the GL surface view.
     *
     * @return The GL surface view.
     */
    @SuppressWarnings("unused")
    public GLSurfaceView getGLView() {
        return mGlView;
    }

    @SuppressWarnings("unused")
    public ARRenderer getRenderer() {
        return renderer;
    }

    private void notifyFinish(String errorMessage) {
        new AlertDialog.Builder(this)
                .setMessage(errorMessage)
                .setTitle("Error")
                .setCancelable(true)
                .setNeutralButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                finish();
                            }
                        })
                .show();
    }
}
