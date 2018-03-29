/*
 *  ARRenderer.java
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

package org.artoolkitx.arx.arxj.rendering;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import org.artoolkitx.arx.arxj.ARActivity;
import org.artoolkitx.arx.arxj.ARController;
import org.artoolkitx.arx.arxj.ARX_jni;
import org.artoolkitx.arx.arxj.rendering.shader_impl.SimpleFragmentShader;
import org.artoolkitx.arx.arxj.rendering.shader_impl.SimpleShaderProgram;
import org.artoolkitx.arx.arxj.rendering.shader_impl.SimpleVertexShader;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Base renderer which should be subclassed in the main application and provided
 * to the ARActivity using its {@link ARActivity#supplyRenderer() supplyRenderer} method.
 * <p/>
 * Subclasses should override {@link #configureARScene() configureARScene}, which will be called by
 * the Activity when AR initialisation is complete. The Renderer can use this method
 * to add markers to the scene, and perform other scene initialisation.
 * <p/>
 * The {@link #draw()}  render} method should also be overridden to perform actual rendering. This is
 * in preference to directly overriding {@link #onDrawFrame(GL10) onDrawFrame}, because ARRenderer will check
 * that the ARToolKitX is running before calling render.
 */
public abstract class ARRenderer implements GLSurfaceView.Renderer {

    private SimpleShaderProgram simpleShaderProgram;
    private int width, height, cameraIndex;
    private int[] viewport = new int[4];
    private boolean firstRun = true;

    private final static String TAG = ARRenderer.class.getName();


    /**
     * Allows subclasses to load markers and prepare the scene. This is called after
     * initialisation is complete.
     */
    public boolean configureARScene() {
        return true;
    }

    public void onSurfaceCreated(GL10 unused, EGLConfig config) {

        // Transparent background
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.f);
        this.simpleShaderProgram = new SimpleShaderProgram(new SimpleVertexShader(), new SimpleFragmentShader());
        GLES20.glUseProgram(simpleShaderProgram.getShaderProgramHandle());
    }

    public void onSurfaceChanged(GL10 unused, int w, int h) {
        this.width = w;
        this.height = h;
        if(ARController.getInstance().isRunning()) {
            //Update the frame settings for native rendering
            ARController.getInstance().drawVideoSettings(cameraIndex, w, h, false, false, false, ARX_jni.ARW_H_ALIGN_CENTRE, ARX_jni.ARW_V_ALIGN_CENTRE, ARX_jni.ARW_SCALE_MODE_FIT, viewport);
        }
    }

    public void onDrawFrame(GL10 unused) {
        if (ARController.getInstance().isRunning()) {
            // Initialize artoolkitX video background rendering.
            if (firstRun) {
                boolean isDisplayFrameInited = ARController.getInstance().drawVideoInit(cameraIndex);
                if (!isDisplayFrameInited) {
                    Log.e(TAG, "Display Frame not inited");
                }

                if (!ARController.getInstance().drawVideoSettings(cameraIndex, this.width, this.height, false, false,
                        false, ARX_jni.ARW_H_ALIGN_CENTRE, ARX_jni.ARW_V_ALIGN_CENTRE,
                        ARX_jni.ARW_SCALE_MODE_FIT, viewport)) {
                    Log.e(TAG, "Error during call of displayFrameSettings.");
                } else {
                    Log.i(TAG, "Viewport {" + viewport[0] + ", " + viewport[1] + ", " + viewport[2] + ", " + viewport[3] + "}.");
                }

                firstRun = false;
            }
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
            if (!ARController.getInstance().drawVideoSettings(cameraIndex)) {
                Log.e(TAG, "Error during call of displayFrame.");
            }
            draw();
        }
    }

    /**
     * Should be overridden in subclasses and used to perform rendering.
     */
    public void draw() {
        GLES20.glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        //TODO: Check how to refactor near and far plane
        simpleShaderProgram.setProjectionMatrix(ARController.getInstance().getProjectionMatrix(10.0f, 10000.0f));
        float[] camPosition = {1f, 1f, 1f};
        simpleShaderProgram.render(camPosition);
    }

    @SuppressWarnings("unused")
    public ShaderProgram getSimpleShaderProgram() {
        return simpleShaderProgram;
    }

    public void setCameraIndex(int cameraIndex) {
        this.cameraIndex = cameraIndex;
    }
}
