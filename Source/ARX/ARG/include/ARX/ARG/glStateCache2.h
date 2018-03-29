/*
 *  glStateCache2.h
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
 *  Copyright 2009-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb 
 *
 */

// glStateCache optimises OpenGL ES on implementations where
// changes in GL state are expensive, by eliminating redundant
// changes to state.

#ifndef __glStateCache2_h__
#define __glStateCache2_h__

#include <ARX/AR/config.h>

#if USE_GL_STATE_CACHE

#ifdef __APPLE__
#  include <OpenGLES/ES2/gl.h>
#  include <OpenGLES/ES2/glext.h>
#else
#  include <GLES2/gl2.h>
#  include <GLES2/gl2ext.h>
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Use of the state cache:
//
// Prior to drawing, a given piece of code should set the state
// it prefers using the calls below. All code that uses the state
// cache should refrain from querying state, and refrain from saving
// and restoring state. The state cache maintains track of the current
// state and no GL calls to make state changes will be made if the
// requested state is already set.
//
// One additional note: If you have some code in your application
// which does NOT use the state cache routines, then the state cache's
// record of the state of the GL machine may be erroneous. In this
// case you will have to call glStateCacheFlush() at the beginning
// of the section of your code which DOES cache state.
//

// Tells the state cache that changes to state have been made
// elsewhere, and that the cache should be emptied.
#if !USE_GL_STATE_CACHE
#define glStateCacheFlush()
#else
void glStateCacheFlush(void);
#endif
#define glStateCacheBeginAgain glStateCacheFlush // Deprecated name.

#if !USE_GL_STATE_CACHE
#define glStateCacheEnableDepthTest() glEnable(GL_DEPTH_TEST)
#define glStateCacheDisableDepthTest() glDisable(GL_DEPTH_TEST)
#define glStateCacheEnableBlend() glEnable(GL_BLEND)
#define glStateCacheDisableBlend() glDisable(GL_BLEND)
#else
void glStateCacheEnableDepthTest(void);
void glStateCacheDisableDepthTest(void);
void glStateCacheEnableBlend(void);
void glStateCacheDisableBlend(void);
#endif

#if !USE_GL_STATE_CACHE
#define glStateCacheActiveTexture(texture) glActiveTexture(texture)
#else
#define GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS 8 // Minimum value for GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 8
void glStateCacheActiveTexture(GLuint texture);
#endif

#if !USE_GL_STATE_CACHE
#define glStateCacheBindTexture2D(name) glBindTexture(GL_TEXTURE_2D, name)
#else
void glStateCacheBindTexture2D(GLuint name);
#endif

#if !USE_GL_STATE_CACHE
#define glStateCacheBlendFunc(sfactor, dfactor) glBlendFunc(sfactor, dfactor)
#else
void glStateCacheBlendFunc(GLenum sfactor, GLenum dfactor);
#endif

#if !USE_GL_STATE_CACHE
#define glStateCacheColorMask(red, green, blue, alpha) glColorMask(red, green, blue, alpha)
#else
void glStateCacheColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
#endif
    
#if !USE_GL_STATE_CACHE
#define glStateCacheDepthMask(flag) glDepthMask(flag)
#else
void glStateCacheDepthMask(GLboolean flag);
#endif
    
#if !USE_GL_STATE_CACHE
#define glStateCachePixelStoreUnpackAlignment(param) glPixelStorei(GL_UNPACK_ALIGNMENT, param)
#else
void glStateCachePixelStoreUnpackAlignment(GLint param);
#endif

#ifdef __cplusplus
}
#endif
        
#endif // !__glStateCache2_h__
