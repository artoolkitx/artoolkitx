/*
 *  glStateCache.c
 *
 *  Created by Philip Lamb on 2009-08-15.
 *  Copyright 2008-2013 Eden Networks Ltd. All rights reserved.
 *
 */

#include <Eden/glStateCache.h>

#if USE_GL_STATE_CACHE

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <float.h>


static char stateEnableDepthTest = CHAR_MAX;

static char stateEnableVertexArray = CHAR_MAX;
static char stateEnableNormalArray = CHAR_MAX;

static GLenum stateClientActiveTexture = UINT_MAX; // In range [0 - GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS-1].
static char stateEnableTexCoordArray[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {CHAR_MAX};

static GLenum stateActiveTex = UINT_MAX; // In range [0 - GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS-1].
static char stateEnableTex2D[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {CHAR_MAX};
static GLuint stateTexName[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};
static GLuint stateTexEnvMode[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};
static GLuint stateTexEnvSrc0[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};
static GLuint stateTexEnvSrc1[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};
static GLuint stateTexEnvCombine[GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS] = {UINT_MAX};

static char stateEnableBlend = CHAR_MAX;
static GLenum stateBlendFunc_sfactor = UINT_MAX;
static GLenum stateBlendFunc_dfactor = UINT_MAX;

static GLboolean stateColorMaskRed = UCHAR_MAX;
static GLboolean stateColorMaskGreen = UCHAR_MAX;
static GLboolean stateColorMaskBlue = UCHAR_MAX;
static GLboolean stateColorMaskAlpha = UCHAR_MAX;
static GLboolean stateDepthMask = UCHAR_MAX;

static char stateEnableLighting = CHAR_MAX;
static GLfloat materialAmbient[4] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
static GLfloat materialDiffuse[4] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
static GLfloat materialSpecular[4] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
static GLfloat materialEmission[4] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
static GLfloat materialShininess = FLT_MAX;

static GLint statePixelStoreUnpackAlignment = UINT_MAX;

void glStateCacheFlush()
{
    int i;
    
    stateEnableDepthTest = CHAR_MAX;
    
    stateEnableVertexArray = CHAR_MAX;
    stateEnableNormalArray = CHAR_MAX;
    
    stateClientActiveTexture = UINT_MAX;
    for (i = 0; i < GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++) {
        stateEnableTexCoordArray[i] = CHAR_MAX;
    }
    
    stateActiveTex = UINT_MAX;
    for (i = 0; i < GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++) {
        stateEnableTex2D[i] = CHAR_MAX;
        stateTexName[i] = UINT_MAX;
        stateTexEnvMode[i] = UINT_MAX;
        stateTexEnvSrc0[i] = UINT_MAX;
        stateTexEnvSrc1[i] = UINT_MAX;
        stateTexEnvCombine[i] = UINT_MAX;
    }
    
    stateEnableBlend = CHAR_MAX;
    stateBlendFunc_sfactor = UINT_MAX;
    stateBlendFunc_dfactor = UINT_MAX;
    
    stateEnableLighting = CHAR_MAX;
    materialAmbient[0] = materialAmbient[1] = materialAmbient[2] = materialAmbient[3] = FLT_MAX;
    materialDiffuse[0] = materialDiffuse[1] = materialDiffuse[2] = materialDiffuse[3] = FLT_MAX;
    materialSpecular[0] = materialSpecular[1] = materialSpecular[2] = materialSpecular[3] = FLT_MAX;
    materialEmission[0] = materialEmission[1] = materialEmission[2] = materialEmission[3] = FLT_MAX;
    materialShininess = FLT_MAX;
    
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

void glStateCacheEnableClientStateVertexArray()
{
    if (stateEnableVertexArray != 1) {
        glEnableClientState(GL_VERTEX_ARRAY);
        stateEnableVertexArray = 1;
    }
}

void glStateCacheDisableClientStateVertexArray()
{
    if (stateEnableVertexArray != 0) {
        glDisableClientState(GL_VERTEX_ARRAY);
        stateEnableVertexArray = 0;
    }
}

void glStateCacheEnableClientStateNormalArray()
{
    if (stateEnableNormalArray != 1) {
        glEnableClientState(GL_NORMAL_ARRAY);
        stateEnableNormalArray = 1;
    }
}

void glStateCacheDisableClientStateNormalArray()
{
    if (stateEnableNormalArray != 0) {
        glDisableClientState(GL_NORMAL_ARRAY);
        stateEnableNormalArray = 0;
    }
}

void glStateCacheClientActiveTexture(GLenum texture)
{
    if (stateClientActiveTexture != texture) {
        glClientActiveTexture(texture);
        stateClientActiveTexture = texture;
    }
}

void glStateCacheEnableClientStateTexCoordArray()
{
    if (stateClientActiveTexture < GL_TEXTURE0 || stateClientActiveTexture >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateEnableTexCoordArray[stateClientActiveTexture - GL_TEXTURE0] != 1) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        stateEnableTexCoordArray[stateClientActiveTexture - GL_TEXTURE0] = 1;
    }
}

void glStateCacheDisableClientStateTexCoordArray()
{
    if (stateClientActiveTexture < GL_TEXTURE0 || stateClientActiveTexture >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateEnableTexCoordArray[stateClientActiveTexture - GL_TEXTURE0] != 0) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        stateEnableTexCoordArray[stateClientActiveTexture - GL_TEXTURE0] = 0;
    }
}

void glStateCacheActiveTexture(GLuint texture)
{
    if (stateActiveTex != texture) {
        glActiveTexture(texture);
        stateActiveTex = texture;
    }
}

void glStateCacheEnableTex2D()
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateEnableTex2D[stateActiveTex - GL_TEXTURE0] != 1) {
        glEnable(GL_TEXTURE_2D);
        stateEnableTex2D[stateActiveTex - GL_TEXTURE0] = 1;
    }
}

void glStateCacheDisableTex2D()
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateEnableTex2D[stateActiveTex - GL_TEXTURE0] != 0) {
        glDisable(GL_TEXTURE_2D);
        stateEnableTex2D[stateActiveTex - GL_TEXTURE0] = 0;
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

void glStateCacheTexEnvMode(GLint mode)
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateTexEnvMode[stateActiveTex - GL_TEXTURE0] != mode) {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
        stateTexEnvMode[stateActiveTex - GL_TEXTURE0] = mode;
    }
}

void glStateCacheTexEnvSrc0(GLint source)
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateTexEnvSrc0[stateActiveTex - GL_TEXTURE0] != source) {
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, source);
        stateTexEnvSrc0[stateActiveTex - GL_TEXTURE0] = source;
    }
}

void glStateCacheTexEnvSrc1(GLint source)
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateTexEnvSrc1[stateActiveTex - GL_TEXTURE0] != source) {
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, source);
        stateTexEnvSrc1[stateActiveTex - GL_TEXTURE0] = source;
    }
}

void glStateCacheTexEnvCombine(GLint combine)
{
    if (stateActiveTex < GL_TEXTURE0 || stateActiveTex >= GL_TEXTURE0 + GLSTATECACHE_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return;
    if (stateTexEnvCombine[stateActiveTex - GL_TEXTURE0] != combine) {
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, combine);
        stateTexEnvCombine[stateActiveTex - GL_TEXTURE0] = combine;
    }
}

void glStateCacheEnableLighting()
{
    if (stateEnableLighting != 1) {
        glEnable(GL_LIGHTING);
        stateEnableLighting = 1;
    }
}

void glStateCacheDisableLighting()
{
    if (stateEnableLighting != 0) {
        glDisable(GL_LIGHTING);
        stateEnableLighting = 0;
    }
}

void glStateCacheMaterialv(GLenum pname, GLfloat *param)
{
    switch (pname) {
        case GL_AMBIENT:
            if (param[0] == materialAmbient[0] && param[1] == materialAmbient[1] && param[2] == materialAmbient[2] && param[3] == materialAmbient[3]) return;
            memcpy(materialAmbient, param, 4*sizeof(GLfloat));
            break;
        case GL_DIFFUSE:
            if (param[0] == materialDiffuse[0] && param[1] == materialDiffuse[1] && param[2] == materialDiffuse[2] && param[3] == materialDiffuse[3]) return;
            memcpy(materialDiffuse, param, 4*sizeof(GLfloat));
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            if (param[0] == materialAmbient[0] && param[0] == materialDiffuse[0] && param[1] == materialAmbient[1] && param[1] == materialDiffuse[1] && param[2] == materialAmbient[2] && param[2] == materialDiffuse[2] && param[3] == materialAmbient[3] && param[3] == materialDiffuse[3]) return;
            memcpy(materialAmbient, param, 4*sizeof(GLfloat));
            memcpy(materialDiffuse, param, 4*sizeof(GLfloat));
            break;
        case GL_SPECULAR:
            if (param[0] == materialSpecular[0] && param[1] == materialSpecular[1] && param[2] == materialSpecular[2] && param[3] == materialSpecular[3]) return;
            memcpy(materialSpecular, param, 4*sizeof(GLfloat));
            break;
        case GL_EMISSION:
            if (param[0] == materialEmission[0] && param[1] == materialEmission[1] && param[2] == materialEmission[2] && param[3] == materialEmission[3]) return;
            memcpy(materialEmission, param, 4*sizeof(GLfloat));
            break;
        default:
            return;
    }
    glMaterialfv(GL_FRONT_AND_BACK, pname, param);
}

void glStateCacheMaterial(GLenum pname, GLfloat param)
{
    switch (pname) {
        case GL_SHININESS:
            if (param == materialShininess) return;
            materialShininess = param;
            break;
        default:
            return;
    }    
    glMaterialf(GL_FRONT_AND_BACK, pname, param);
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

#endif // !DISABLE_GL_STATE_CACHE

