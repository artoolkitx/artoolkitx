/*
 *  SimpleFragmentShader.java
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

import android.opengl.GLES20;

import org.artoolkitx.arx.arxj.rendering.OpenGLShader;

/**
 * Here you define your fragment shader and what it does with the given color.
 * <br>
 * This class also provides the implementation of the {@link #configureShader()} method. So all you need to do is
 * call this one from your fragment shader implementation.
 */
public class SimpleFragmentShader implements OpenGLShader {

    /**
     * We get the color to apply to the rendered geometry from the vertex shader.
     * We don't do anything with it, just simply pass it to the rendering pipe.
     * Therefor OpenGL 2.0 uses the gl_FragColor variable
     */
    private String fragmentShader =
            "precision mediump float;       \n"     // Set the default precision to medium. We don't need as high of a
                    // precision in the fragment shader.
                    + "varying vec4 v_Color;          \n"     // This is the color from the vertex shader interpolated across the
                    // triangle per fragment.
                    + "void main()                    \n"     // The entry point for our fragment shader.
                    + "{                              \n"
                    + "   gl_FragColor = v_Color;     \n"     // Pass the color directly through the pipeline.
                    + "}                              \n";

    public int configureShader() {

        int fragmentShaderHandle = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
        String fragmentShaderErrorLog = "";

        if (fragmentShaderHandle != 0) {

            //Pass in the shader source
            GLES20.glShaderSource(fragmentShaderHandle, fragmentShader);

            //Compile the shader
            GLES20.glCompileShader(fragmentShaderHandle);

            //Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(fragmentShaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            //If the compilation failed, delete the shader
            if (compileStatus[0] == 0) {
                fragmentShaderErrorLog = GLES20.glGetShaderInfoLog(fragmentShaderHandle);
                GLES20.glDeleteShader(fragmentShaderHandle);
                fragmentShaderHandle = 0;
            }
        }
        if (fragmentShaderHandle == 0) {
            throw new RuntimeException("Error creating fragment shader.\\n" + fragmentShaderErrorLog);
        }
        return fragmentShaderHandle;
    }

    @Override
    public void setShaderSource(String source) {
        this.fragmentShader = source;
    }
}
