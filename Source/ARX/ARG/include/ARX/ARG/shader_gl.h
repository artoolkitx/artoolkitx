/*
 *  shader_gl.h
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

#ifndef __shader_gl_h__
#define __shader_gl_h__

// ============================================================================
//	Public includes.
// ============================================================================

#include <ARX/AR/config.h>

#if (HAVE_GLES2 || HAVE_GL3)

#if HAVE_GLES2
#  include <ARX/ARG/glStateCache2.h>
#elif HAVE_GL3
#  ifdef __APPLE__
#    include <OpenGL/gl3.h>
#  else
#  ifndef _WIN32
#    define GL_GLEXT_PROTOTYPES
#   endif
#    include <GL/glcorearb.h>
#  endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef ARG_STATIC
#    define SHADER_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define SHADER_EXTERN __declspec(dllexport)
#    else
#      define SHADER_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define SHADER_EXTERN
#endif

// ============================================================================
//	Public types and definitions.
// ============================================================================


// ============================================================================
//	Public functions.
// ============================================================================


/* Shader Utilities */
SHADER_EXTERN GLint arglGLCompileShaderFromFile(GLuint *shader, GLenum type, const char *pathname);
SHADER_EXTERN GLint arglGLCompileShaderFromString(GLuint *shader, GLenum type, const char *s);
SHADER_EXTERN GLint arglGLLinkProgram(GLuint prog);
SHADER_EXTERN GLint arglGLValidateProgram(GLuint prog);
SHADER_EXTERN void arglGLDestroyShaders(GLuint vertShader, GLuint fragShader, GLuint prog);

#ifdef __cplusplus
}
#endif

#endif // HAVE_GLES2 || HAVE_GL3

#endif /* !__shader_gl_h__ */
