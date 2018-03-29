/*
 *  shader_gl.c
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
 *  Author(s): Philip Lamb
 *
 */

// ============================================================================
//    Private includes.
// ============================================================================
#include <ARX/ARG/shader_gl.h>

#if HAVE_GLES2 || HAVE_GL3

#include <stdio.h>         // fprintf(), stderr
#include <string.h>        // strchr(), strstr(), strlen()
#include <stdlib.h>        // malloc()
#include <ARX/ARUtil/log.h>
#include <ARX/ARUtil/file_utils.h> // cat()

// On Windows, all OpenGL v3 and later API must be dynamically resolved against the actual driver
#if defined(_WIN32)
    # define ARGL_GET_PROC_ADDRESS wglGetProcAddress
    PFNGLCOMPILESHADERPROC glCompileShader = NULL; // (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffersARB");
    PFNGLCREATESHADERPROC glCreateShader = NULL;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
    PFNGLDELETESHADERPROC glDeleteShader = NULL;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
    PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
    PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
    PFNGLSHADERSOURCEPROC glShaderSource = NULL;
    PFNGLVALIDATEPROGRAMPROC glValidateProgram = NULL;
#endif

// ============================================================================
//    Private types and defines.
// ============================================================================


// ============================================================================
//    Public globals.
// ============================================================================


// ============================================================================
//    Private globals.
// ============================================================================


#pragma mark -
// ============================================================================
//    Private functions.
// ============================================================================


// ============================================================================
//    Public functions.
// ============================================================================

/* Create and compile a shader from the provided source(s) */
GLint arglGLCompileShaderFromFile(GLuint *shader, GLenum type, const char *pathname)
{
    GLint status = GL_FALSE;
    char *s;
    
    s = cat(pathname, NULL);
    if (!s) {
        ARLOGe("Unable to read shader source file '%s'.\n", pathname);
        ARLOGperror(NULL);
    } else {
        status = arglGLCompileShaderFromString(shader, type, s);
        free(s);
    }
    return (status);
}

GLint arglGLCompileShaderFromString(GLuint *shader, GLenum type, const char *s)
{
	GLint status;
	
	if (!shader || !s) return (GL_FALSE);

    #if defined(_WIN32)
        if (!glCompileShader) glCompileShader = (PFNGLCOMPILESHADERPROC)ARGL_GET_PROC_ADDRESS("glCompileShader");
        if (!glCreateShader) glCreateShader = (PFNGLCREATESHADERPROC) ARGL_GET_PROC_ADDRESS("glCreateShader");
        if (!glDeleteProgram) glDeleteProgram = (PFNGLDELETEPROGRAMPROC) ARGL_GET_PROC_ADDRESS("glDeleteProgram");
        if (!glDeleteShader) glDeleteShader = (PFNGLDELETESHADERPROC) ARGL_GET_PROC_ADDRESS("glDeleteShader");
        if (!glGetProgramiv) glGetProgramiv = (PFNGLGETPROGRAMIVPROC) ARGL_GET_PROC_ADDRESS("glGetProgramiv");

        if (!glGetProgramInfoLog) glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) ARGL_GET_PROC_ADDRESS("glGetProgramInfoLog");
        if (!glGetShaderiv) glGetShaderiv = (PFNGLGETSHADERIVPROC) ARGL_GET_PROC_ADDRESS("glGetShaderiv");
        if (!glGetShaderInfoLog) glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) ARGL_GET_PROC_ADDRESS("glGetShaderInfoLog");
        if (!glLinkProgram) glLinkProgram = (PFNGLLINKPROGRAMPROC) ARGL_GET_PROC_ADDRESS("glLinkProgram");
        if (!glShaderSource) glShaderSource = (PFNGLSHADERSOURCEPROC) ARGL_GET_PROC_ADDRESS("glShaderSource");
        if (!glValidateProgram) glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) ARGL_GET_PROC_ADDRESS("glValidateProgram");

        if (!glCompileShader || !glCreateShader || !glDeleteProgram || !glDeleteShader || 
            !glGetProgramiv || !glGetProgramInfoLog || !glGetShaderiv || !glGetShaderInfoLog || 
            !glLinkProgram || !glShaderSource || !glValidateProgram) {
                ARLOGe("Error: a required OpenGL function counld not be bound.\n");
                return (FALSE);
        }
	#endif
	
    *shader = glCreateShader(type);				// create shader
    glShaderSource(*shader, 1, &s, NULL);	// set source code in the shader
    glCompileShader(*shader);					// compile shader
	
#ifdef DEBUG
	GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        ARLOGd("Shader compile log:\n%s\n", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
		ARLOGe("Failed to compile shader:\n%s", s);
	}
	
	return status;
}

/* Link a program with all currently attached shaders */
GLint arglGLLinkProgram(GLuint prog)
{
	GLint status;
	
	glLinkProgram(prog);
	
#ifdef DEBUG
	GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ARLOGd("Program link log:\n%s\n", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
		ARLOGe("Failed to link program %d.\n", prog);
	
	return status;
}


/* Validate a program (for i.e. inconsistent samplers) */
GLint arglGLValidateProgram(GLuint prog)
{
	GLint logLength, status;
	
	glValidateProgram(prog);
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
		ARLOGe("Failed to validate program %d.\n", prog);
    }
	
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ARLOGi("Program validate log:\n%s\n", log);
        free(log);
    }

	return status;
}

/* delete shader resources */
void arglGLDestroyShaders(GLuint vertShader, GLuint fragShader, GLuint prog)
{
	if (vertShader) {
		glDeleteShader(vertShader);
	}
	if (fragShader) {
		glDeleteShader(fragShader);
	}
	if (prog) {
		glDeleteProgram(prog);
	}
}

#endif // HAVE_GLES2 || HAVE_GL3
