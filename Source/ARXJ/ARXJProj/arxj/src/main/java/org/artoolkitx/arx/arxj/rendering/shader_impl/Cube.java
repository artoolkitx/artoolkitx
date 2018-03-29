/*
 *  Cube.java
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
 *  Author(s): Thorsten Bux
 *
 */
package org.artoolkitx.arx.arxj.rendering.shader_impl;

import org.artoolkitx.arx.arxj.rendering.ARDrawable;
import org.artoolkitx.arx.arxj.rendering.RenderUtils;
import org.artoolkitx.arx.arxj.rendering.ShaderProgram;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;


@SuppressWarnings("SameParameterValue")
public final class Cube implements ARDrawable {

    private FloatBuffer mVertexBuffer;
    private FloatBuffer mColorBuffer;
    private ByteBuffer mIndexBuffer;
    private ShaderProgram shaderProgram;

    @SuppressWarnings("unused")
    public Cube() {
        this(1.0f);
    }

    public Cube(ShaderProgram shaderProgram) {
        super();
        this.shaderProgram = shaderProgram;
    }

    @SuppressWarnings("WeakerAccess")
    public Cube(float size) {
        this(size, 0.0f, 0.0f, 0.0f);
    }

    public Cube(float size, float x, float y, float z) {
        setArrays(size, x, y, z);
    }

    @SuppressWarnings("WeakerAccess")
    public FloatBuffer getmVertexBuffer() {
        return mVertexBuffer;
    }
    @SuppressWarnings("WeakerAccess")
    public FloatBuffer getmColorBuffer() {
        return mColorBuffer;
    }
    @SuppressWarnings("WeakerAccess")
    public ByteBuffer getmIndexBuffer() {
        return mIndexBuffer;
    }

    private void setArrays(float size, float x, float y, float z) {

        float hs = size / 2.0f;

        /*
        In the marker coordinate system z points from the marker up. x goes to the right and y to the top
         */
        float vertices[] = {
                x - hs, y - hs, z - hs, // 0 --> If you look at the cube from the front, this is the corner
                // in the front on the left of the ground plane.
                x + hs, y - hs, z - hs, // 1 --> That is the one to the right of corner 0
                x + hs, y + hs, z - hs, // 2 --> That is the one to the back right of corner 0
                x - hs, y + hs, z - hs, // 3 --> That is the one to the left of corner 2
                // Or if you imaging (or paint) a 3D cube on paper this is the only corner that is hidden
                x - hs, y - hs, z + hs, // 4 --> That is the top left corner. Directly on top of 0
                x + hs, y - hs, z + hs, // 5 --> That is directly on top of 1
                x + hs, y + hs, z + hs, // 6 --> That is directly on top of 2
                x - hs, y + hs, z + hs, // 7 --> That is directly on top of 3
        };
        float c = 1.0f;
        float colors[] = {
                0, 0, 0, c, // 0 black
                c, 0, 0, c, // 1 red
                c, c, 0, c, // 2 yellow
                0, c, 0, c, // 3 green
                0, 0, c, c, // 4 blue
                c, 0, c, c, // 5 magenta
                c, c, c, c, // 6 white
                0, c, c, c, // 7 cyan
        };

        byte indices[] = {
                // bottom
                1, 0, 2,
                2, 0, 3,
                // right
                1, 2, 5,
                5, 2, 6,
                // top
                4, 5, 7,
                7, 5, 6,
                // left
                0, 4, 3,
                3, 4, 7,
                // back
                7, 6, 3,
                6, 2, 3,
                // front
                0, 1, 4,
                4, 1, 5
        };

        mVertexBuffer = RenderUtils.buildFloatBuffer(vertices);
        mColorBuffer = RenderUtils.buildFloatBuffer(colors);
        mIndexBuffer = RenderUtils.buildByteBuffer(indices);

    }


    @Override
    /*
     * Used to render objects when working with OpenGL ES 2.x
     *
     * @param projectionMatrix The projection matrix obtained from the ARToolkit
     * @param modelViewMatrix  The marker transformation matrix obtained from ARToolkit
     */
    public void draw(float[] projectionMatrix, float[] modelViewMatrix) {

        shaderProgram.setProjectionMatrix(projectionMatrix);
        shaderProgram.setModelViewMatrix(modelViewMatrix);

        shaderProgram.render(this.getmVertexBuffer(), this.getmColorBuffer(), this.getmIndexBuffer());

    }

    @Override
    /*
     * Sets the shader program used by this geometry.
     */
    public void setShaderProgram(ShaderProgram shaderProgram) {
        this.shaderProgram = shaderProgram;
    }
}
