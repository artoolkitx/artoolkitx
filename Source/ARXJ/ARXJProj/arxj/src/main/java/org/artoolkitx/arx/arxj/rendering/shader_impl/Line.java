/*
 *  Line.java
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

import android.opengl.GLES10;

import org.artoolkitx.arx.arxj.rendering.ARDrawable;
import org.artoolkitx.arx.arxj.rendering.RenderUtils;
import org.artoolkitx.arx.arxj.rendering.ShaderProgram;

import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

public class Line implements ARDrawable {

    private final int vertexLength = 3; //We only work with position vectors with three elements
    private float[] start = new float[3];
    private float[] end = new float[3];
    private float width;
    private float[] color = {1, 0, 0, 1};
    private FloatBuffer mVertexBuffer;
    private FloatBuffer mColorBuffer;


    private ShaderProgram shaderProgram;

    /**
     * @param width Width of the line
     */
    @SuppressWarnings("WeakerAccess")
    public Line(float width) {
        shaderProgram = null;
        this.setWidth(width);
    }

    public Line(float width, ShaderProgram shaderProgram) {
        this(width);
        this.shaderProgram = shaderProgram;
    }

    private void setArrays() {

        float[] vertices = new float[vertexLength * 2];

        for (int i = 0; i < vertexLength; i++) {
            vertices[i] = start[i];
            vertices[i + vertexLength] = end[i];
        }

        mVertexBuffer = RenderUtils.buildFloatBuffer(vertices);
        mColorBuffer = RenderUtils.buildFloatBuffer(color);
    }

    @SuppressWarnings("unused")
    public void draw(GL10 gl) {
        gl.glVertexPointer(vertexLength, GLES10.GL_FLOAT, 0, mVertexBuffer);

        gl.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
        gl.glColor4f(1, 0, 0, 1); // Red
        gl.glLineWidth(this.width);
        gl.glDrawArrays(GLES10.GL_LINES, 0, 2);
        gl.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
    }

    public float getWidth() {
        return width;
    }

    @SuppressWarnings("WeakerAccess")
    public void setWidth(float width) {
        this.width = width;
    }

    @SuppressWarnings("WeakerAccess")
    public FloatBuffer getMVertexBuffer() {
        return this.mVertexBuffer;
    }

    public float[] getStart() {
        return start;
    }

    public void setStart(float[] start) {
        if (start.length > vertexLength) {
            this.start[0] = start[0];
            this.start[1] = start[1];
            this.start[2] = start[2];
        } else {
            this.start = start;
        }
    }

    public float[] getEnd() {
        return end;
    }

    public void setEnd(float[] end) {
        if (end.length > vertexLength) {
            this.end[0] = end[0];
            this.end[1] = end[1];
            this.end[2] = end[2];
        } else {
            this.end = end;
        }
    }


    public float[] getColor() {
        return color;
    }

    public void setColor(float[] color) {
        this.color = color;
    }

    @SuppressWarnings("WeakerAccess")
    public FloatBuffer getColorBuffer() {
        return mColorBuffer;
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

        this.setArrays();
        shaderProgram.render(this.getMVertexBuffer(), this.getColorBuffer(), null);

    }

    @Override
    /*
     * Sets the shader program used by this geometry.
     */
    public void setShaderProgram(ShaderProgram program) {
        this.shaderProgram = program;
    }
}
