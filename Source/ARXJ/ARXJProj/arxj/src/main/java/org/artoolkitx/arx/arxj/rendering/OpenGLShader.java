/*
 *  OpenGLShader.java
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
 *  Author(s): Philip Lamb, Thorsten Bux
 *
 */
package org.artoolkitx.arx.arxj.rendering;

/**
 * Created by Thorsten Bux on 21.01.2016.
 * <br>
 * Provides an interface that ensures the basic shader methods and information in a shader implementation
 * are provided.
 */
public interface OpenGLShader {

    //These properties are used to make the connection between the code and the shader. We use them
    //to link the projection and model matrix to the shader and to pass these matrices to the shader
    //from the AR application.
    String projectionMatrixString = "u_projection";
    String modelViewMatrixString = "u_modelView";
    //Also used to provide a link to the shader program. In this case we pass in the position vectors from the
    //AR application to the shader.
    String positionVectorString = "a_Position";

    int configureShader();

    void setShaderSource(String source);

}
