/*
 *  glStateCache2.c
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

#include <ARX/ARG/glStateCache2.h>

#if USE_GL_STATE_CACHE

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <float.h>

static char stateEnableDepthTest = CHAR_MAX;

static GLenum stateActiveTex = UINT_MAX;  // In range [0 - GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS-1].
static GLuint stateTexName[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};

static char stateEnableBlend = CHAR_MAX;
static GLenum stateBlendFunc_sfactor = UINT_MAX;
static GLenum stateBlendFunc_dfactor = UINT_MAX;

static GLboolean stateColorMaskRed = UCHAR_MAX;
static GLboolean stateColorMaskGreen = UCHAR_MAX;
static GLboolean stateColorMaskBlue = UCHAR_MAX;
static GLboolean stateColorMaskAlpha = UCHAR_MAX;
static GLboolean stateDepthMask = UCHAR_MAX;

static GLint statePixelStoreUnpackAlignment = UINT_MAX;

void glStateCacheFlush()
{
    int i;
    
    stateEnableDepthTest = CHAR_MAX;
    
    stateActiveTex = UINT_MAX;
    for (i = 0; i < GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++) stateTexName[i] = UINT_MAX;

    stateEnableBlend = CHAR_MAX;
    stateBlendFunc_sfactor = UINT_MAX;
    stateBlendFunc_dfactor = UINT_MAX;
    
    stateColorMaskRed = UCHAR_MAX;
    stateColorMaskGreen = UCHAR_MAX;
    stateColorMaskBlue = UCHAR_MAX;
    stateColorMaskAlpha = UCHAR_MAX;
    stateDepthMask = UCHAR_MAX;
    
    statePixelStoreUnpackAlignment = UINT_MAX;
}

void glStateCacheEnableDepthTest()
{
    if (stateEnableDepthTest != 1) {
        glEnable(GL_DEPTH_TEST);
        stateEnableDepthTest = 1;
    }
}

void glStateCacheDisableDepthTest()
{
    if (stateEnableDepthTest != 0) {
        glDisable(GL_DEPTH_TEST);
        stateEnableDepthTest = 0;
    }
}

void glStateCacheActiveTexture(GLuint texture)
{
    if (stateActiveTex != texture) {
        glActiveTexture(texture);
        stateActiveTex = texture;
    }
}

void glStateCacheBindTexture2D(GLuint name)
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateTexName[stateActiveTex - GL_TEXTURE0] != name) {
        glBindTexture(GL_TEXTURE_2D, name);
        stateTexName[stateActiveTex - GL_TEXTURE0] = name;
    }
}

void glStateCacheEnableBlend()
{
    if (stateEnableBlend != 1) {
        glEnable(GL_BLEND);
        stateEnableBlend = 1;
    }
}

void glStateCacheDisableBlend()
{
    if (stateEnableBlend != 0) {
        glDisable(GL_BLEND);
        stateEnableBlend = 0;
    }
}

void glStateCacheBlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (stateBlendFunc_sfactor != sfactor || stateBlendFunc_dfactor != dfactor) {
        glBlendFunc(sfactor, dfactor);
        stateBlendFunc_sfactor = sfactor;
        stateBlendFunc_dfactor = dfactor;
    }
}

void glStateCacheColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    if (stateColorMaskRed != red || stateColorMaskGreen != green || stateColorMaskBlue != blue || stateColorMaskAlpha != alpha) {
        glColorMask(red, green, blue, alpha);
        stateColorMaskRed = red;
        stateColorMaskGreen = green;
        stateColorMaskBlue = blue;
        stateColorMaskAlpha = alpha;
    }
}

void glStateCacheDepthMask(GLboolean flag)
{
    if (stateDepthMask != flag) {
        glDepthMask(flag);
        stateDepthMask = flag;
    }
}

void glStateCachePixelStoreUnpackAlignment(GLint param)
{
    if (statePixelStoreUnpackAlignment != param) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, param);
        statePixelStoreUnpackAlignment = param;
    }
}

#endif // USE_GL_STATE_CACHE

