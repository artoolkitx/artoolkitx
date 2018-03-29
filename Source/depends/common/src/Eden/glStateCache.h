/*
 *  glStateCache.h
 *
 *  Created by Philip Lamb on 2009-08-15.
 *  Copyright 2008-2013 Eden Networks Ltd. All rights reserved.
 *
 */

// glStateCache optimises OpenGL ES on implementations where
// changes in GL state are expensive, by eliminating redundant
// changes to state.

#ifndef __Eden_glStateCache_h__
#define __Eden_glStateCache_h__

#include <Eden/Eden.h>

#if USE_GL_STATE_CACHE

#ifndef EDEN_IPHONEOS
#  include <GLES/gl.h>
#  include <GLES/glext.h>
#else
#  include <OpenGLES/ES1/gl.h>
#  include <OpenGLES/ES1/glext.h>
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
void glStateCacheFlush();
#endif
#define glStateCacheBeginAgain glStateCacheFlush // Deprecated name.

// Depth testing.
#if !USE_GL_STATE_CACHE
#define glStateCacheEnableDepthTest() glEnable(GL_DEPTH_TEST)
#define glStateCacheDisableDepthTest() glDisable(GL_DEPTH_TEST)
#else
void glStateCacheEnableDepthTest();
void glStateCacheDisableDepthTest();
#endif

// Client-side vertex and normal operations.
#if !USE_GL_STATE_CACHE
#define glStateCacheEnableClientStateVertexArray() glEnableClientState(GL_VERTEX_ARRAY)
#define glStateCacheDisableClientStateVertexArray() glDisableClientState(GL_VERTEX_ARRAY)
#define glStateCacheEnableClientStateNormalArray() glEnableClientState(GL_NORMAL_ARRAY)
#define glStateCacheDisableClientStateNormalArray() glDisableClientState(GL_NORMAL_ARRAY)
#else
void glStateCacheEnableClientStateVertexArray();
void glStateCacheDisableClientStateVertexArray();
void glStateCacheEnableClientStateNormalArray();
void glStateCacheDisableClientStateNormalArray();
#endif
// No longer included in cached state. Definitions retained here for backwards compatibility.
#define glStateCacheVertexPtr(size, type, stride, ptr) glVertexPointer(size, type, stride, ptr)
#define glStateCacheNormalPtr(type, stride, ptr) glNormalPointer(type, stride, ptr)

// Client-side texture operations.
// The glStateCacheEnableClientStateTexCoordArray()/glStateCacheDisableClientStateTexCoordArray()
// operations all operate on the current active texture set with glStateCacheClientActiveTexture().
#if !USE_GL_STATE_CACHE
#ifdef GL_VERSION_1_3
#  define glStateCacheClientActiveTexture(texture) glClientActiveTexture(texture)
#else
#  define glStateCacheClientActiveTexture(texture)
#endif
#define glStateCacheEnableClientStateTexCoordArray() glEnableClientState(GL_TEXTURE_COORD_ARRAY)
#define glStateCacheDisableClientStateTexCoordArray() glDisableClientState(GL_TEXTURE_COORD_ARRAY)
#else
void glStateCacheClientActiveTexture(GLenum texture);
void glStateCacheEnableClientStateTexCoordArray();
void glStateCacheDisableClientStateTexCoordArray();
#endif
// No longer included in cached state. Definitions retained here for backwards compatibility.
#define glStateCacheTexCoordPtr(size, type, stride, ptr) glTexCoordPointer(size, type, stride, ptr)
    
// Server-side texture operations.
// The glStateCacheBindTexture2D()/glStateCacheEnableTex2D()/glStateCacheDisableTex2D() and glStateCacheTexEnv*()
// operations all operate on the current active texture set with glStateCacheActiveTexture().
#if !USE_GL_STATE_CACHE
#ifdef GL_VERSION_1_3
#  define glStateCacheActiveTexture(texture) glActiveTexture(texture)
#else
#  define glStateCacheActiveTexture(texture)
#endif
#define glStateCacheBindTexture2D(name) glBindTexture(GL_TEXTURE_2D, name)
#define glStateCacheEnableTex2D() glEnable(GL_TEXTURE_2D)
#define glStateCacheDisableTex2D() glDisable(GL_TEXTURE_2D)
#define glStateCacheTexEnvMode(mode) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode)
#define glStateCacheTexEnvSrc0(source) glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, source)
#define glStateCacheTexEnvSrc1(source) glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, source)
#define glStateCacheTexEnvCombine(combine) glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, combine)
#else
#define GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS 8 // Minimum value for GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 8
void glStateCacheActiveTexture(GLuint texture);
void glStateCacheBindTexture2D(GLuint name);
void glStateCacheEnableTex2D();
void glStateCacheDisableTex2D();
void glStateCacheTexEnvMode(GLint mode);
void glStateCacheTexEnvSrc0(GLint source);
void glStateCacheTexEnvSrc1(GLint source);
void glStateCacheTexEnvCombine(GLint combine);
#endif

// Materials and lighting.
#if !USE_GL_STATE_CACHE
#define glStateCacheEnableLighting() glEnable(GL_LIGHTING)
#define glStateCacheDisableLighting() glDisable(GL_LIGHTING)
#define glStateCacheMaterialv(pname, param) glMaterialfv(GL_FRONT_AND_BACK, pname, param)
#define glStateCacheMaterial(pname, param) glMaterialf(GL_FRONT_AND_BACK, pname, param)
#else
void glStateCacheEnableLighting();
void glStateCacheDisableLighting();
void glStateCacheMaterialv(GLenum pname, GLfloat *param);
void glStateCacheMaterial(GLenum pname, GLfloat param);
#endif

// Blending.
#if !USE_GL_STATE_CACHE
#define glStateCacheEnableBlend() glEnable(GL_BLEND)
#define glStateCacheDisableBlend() glDisable(GL_BLEND)
#define glStateCacheBlendFunc(sfactor, dfactor) glBlendFunc(sfactor, dfactor)
#else
void glStateCacheEnableBlend();
void glStateCacheDisableBlend();
void glStateCacheBlendFunc(GLenum sfactor, GLenum dfactor);
#endif

// Buffer masking operations.
#if !USE_GL_STATE_CACHE
#define glStateCacheColorMask(red, green, blue, alpha) glColorMask(red, green, blue, alpha)
#define glStateCacheDepthMask(flag) glDepthMask(flag)
#else
void glStateCacheColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glStateCacheDepthMask(GLboolean flag);
#endif
    
// Pixel transfer.
#if !USE_GL_STATE_CACHE
#define glStateCachePixelStoreUnpackAlignment(param) glPixelStorei(GL_UNPACK_ALIGNMENT, param)
#else
void glStateCachePixelStoreUnpackAlignment(GLint param);
#endif

#ifdef __cplusplus
}
#endif
        
#endif // !__Eden_glStateCache_h__
