package org.artoolkitx.arx.arxj.camera;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.util.Log;

import org.artoolkitx.arx.arxj.rendering.ARRenderer;

/*
 *  FrameListenerImpl.java
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
 *  Author(s): Thorsten Bux, John Wolf
 *
 */
public final class FrameListenerImpl implements FrameListener {

    private static final String TAG = FrameListenerImpl.class.getSimpleName();
    private final ARRenderer renderer;
    private final Activity activity;
    private final GLSurfaceView glSurfaceView;

    public FrameListenerImpl(ARRenderer renderer, Activity activity, GLSurfaceView glSurfaceView){
        this.renderer = renderer;
        this.activity = activity;
        this.glSurfaceView = glSurfaceView;
    }

    @Override
    public void firstFrame(int cameraIndex) {
        // ARToolKit has been initialised. The renderer can now add markers, etc...
        if(renderer != null) {
            renderer.setCameraIndex(cameraIndex);
            if (renderer.configureARScene()) {
                Log.i(TAG, "firstFrame(): Scene configured successfully");
            } else {
                // Error
                Log.e(TAG, "firstFrame(): Error configuring scene. Cannot continue.");
                activity.finish();
            }
        }
    }

    @Override
    public void onFrameProcessed() {
        // Update the renderer as the frame has changed
        if (glSurfaceView != null) {
            glSurfaceView.requestRender();
        }
    }
}
