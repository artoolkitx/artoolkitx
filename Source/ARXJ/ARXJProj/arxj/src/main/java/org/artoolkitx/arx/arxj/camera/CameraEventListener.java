/*
 *  CameraEventListener.java
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

package org.artoolkitx.arx.arxj.camera;

import java.nio.ByteBuffer;

/**
 * The CameraEventListener interface allows an observer to respond to events
 * from a {@link FrameListener}.
 */
public interface CameraEventListener {

    /**
     * Called when the camera preview is started. The video dimensions and frame rate
     * are passed through, along with information about the camera.
     *
     * @param width               The width of the video image in pixels.
     * @param height              The height of the video image in pixels.
     * @param pixelFormat         A string with format in which buffers will be pushed. Supported values include "NV21", "NV12", "YUV_420_888", "RGBA", "RGB_565", and "MONO".
     * @param cameraIndex         Zero-based index of the camera in use. If only one camera is present, will be 0.
     * @param cameraIsFrontFacing false if camera is rear-facing (the default) or true if camera is facing toward the user.
     */
    void cameraStreamStarted(int width, int height, String pixelFormat, int cameraIndex, @SuppressWarnings("SameParameterValue") boolean cameraIsFrontFacing);

    /**
     * Called when the camera preview has a new frame ready.
     *
     * @param frame A byte array from the camera, in the camera's capture format.
     */
    void cameraStreamFrame(byte[] frame, int frameSize);

    /**
     * Called when the camera preview has a new frame ready.
     *
     * @param framePlanes An array of ByteBuffers from the camera's frame planes, in the camera's capture format.
     */
    void cameraStreamFrame(ByteBuffer[] framePlanes, int[] framePlanePixelStrides, int[] framePlaneRowStrides);

    /**
     * Called when the capture preview is stopped. No new frames will be sent.
     */
    void cameraStreamStopped();

}
