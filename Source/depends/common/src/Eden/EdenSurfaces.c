/*
 *  EdenSurfaces.c
 *
 *	Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2001-11-11	PRL     Initial version for The SRMS simulator.
 *  1.1.0   2008-07-18  PRL     Repurposed for OpenGL ES.
 *  1.2.0   2010-09-22  PRL     Remove redundant static material handling.
 */

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

// ============================================================================
//	Private includes
// ============================================================================
#include <Eden/EdenSurfaces.h>

#include <stdio.h>
#include <string.h>				// strcmp()
#include <stdlib.h>				// malloc(), calloc(), free()
#include <Eden/readtex.h>			// ReadTex()
#if EDEN_USE_GL
#  define USE_GL_STATE_CACHE 0
#  include <Eden/glStateCache.h>
#elif EDEN_USE_GLES2
#  include <ARX/ARG/glStateCache2.h>
#  include <ARX/ARG/shader_gl.h>
#  include <ARX/ARG/mtx.h>
#endif
#ifndef GL_GENERATE_MIPMAP
#  define GL_GENERATE_MIPMAP 0x8191
#endif

// ============================================================================
//	Private types
// ============================================================================
//#define SURFACES_DEBUG			// Uncomment to build version that outputs debug info to stderr.

typedef struct {
	GLuint		name;
	GLint		env_mode;
	//GLfloat	env_color[4];
    GLfloat     maxS; // maxS = (GLfloat)contentWidth / (GLfloat)width;
    GLfloat     maxT; // maxT = (GLfloat)contentHeight / (GLfloat)height;
    GLuint      contentWidth;
    GLuint      contentHeight;
} TEXTURE_t;

// ============================================================================
//	Global variables
// ============================================================================
static TEXTURE_t *gTextures = NULL;	// For our texture heaps (one per context.)
static unsigned int *gTextureIndexPtr = NULL; // For pointer into each heap.
static unsigned int gTextureIndexMax = 0;
static int gSurfacesContextsActiveCount = 0;


// ============================================================================
//	Public functions
// ============================================================================

EDEN_BOOL EdenSurfacesInit(const int contextsActiveCount, const int textureIndexMax)
{
    if (gSurfacesContextsActiveCount) return (FALSE);
    
	gTextures = (TEXTURE_t *)calloc(textureIndexMax * contextsActiveCount, sizeof(TEXTURE_t));
	gTextureIndexPtr = (unsigned int *)calloc(contextsActiveCount, sizeof(unsigned int));
	gTextureIndexMax = textureIndexMax;

	gSurfacesContextsActiveCount = contextsActiveCount;

	return (TRUE);
}

EDEN_BOOL EdenSurfacesFinal(void)
{
    if (!gSurfacesContextsActiveCount) return (FALSE);
    
	gTextureIndexMax = 0;
	free(gTextureIndexPtr); gTextureIndexPtr = NULL;
	free(gTextures); gTextures = NULL;
	
	gSurfacesContextsActiveCount = 0;
	return (TRUE);
}

EDEN_BOOL EdenSurfacesTextureLoad(const int contextIndex, const int numTextures, const TEXTURE_INFO_t *textureInfo, TEXTURE_INDEX_t *textureIndices, char *hasAlpha_out)
{
    return (EdenSurfacesTextureLoad2(contextIndex, numTextures, textureInfo, textureIndices, hasAlpha_out, FALSE, FALSE));
}

EDEN_BOOL EdenSurfacesTextureLoad2(const int contextIndex, const int numTextures, const TEXTURE_INFO_t *textureInfo, TEXTURE_INDEX_t *textureIndices, char *hasAlpha_out, const EDEN_BOOL flipH, const EDEN_BOOL flipV)
{
    GLint textureSizeMax;
    GLboolean haveLimitedNPOT, haveNPOT;
	int i;
	unsigned int ptr;
	unsigned int offset;
	EDEN_BOOL ok = TRUE;
    unsigned char *data = NULL;
    int nc, sizeX, sizeY, textureSizeX, textureSizeY;
	GLenum format;

	if (contextIndex < 0 || contextIndex >= gSurfacesContextsActiveCount) return (FALSE); // Sanity check.

	glStateCachePixelStoreUnpackAlignment(1);	// ReadTex returns tightly packed texture data.
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureSizeMax);
#if EDEN_USE_GLES2
    haveLimitedNPOT = TRUE;
    haveNPOT = EdenGLCapabilityCheck(0u, (const unsigned char *)"GL_OES_texture_npot");
#else
    haveLimitedNPOT = FALSE;
    haveNPOT = EdenGLCapabilityCheck(0x200, (const unsigned char *)"GL_ARB_texture_non_power_of_two");
#endif
    if (hasAlpha_out) *hasAlpha_out = 0; // Will be set if any textures have alpha.

	for (i = 0; i < numTextures; i++) {
        
        // Assign no name to begin with. If all OK, we'll assign a valid name.
        textureIndices[i] = 0;
        
		// Find a free slot in the texture array.
		ptr =  gTextureIndexPtr[contextIndex];
		while (gTextures[contextIndex * gTextureIndexMax + ptr].name != 0) {
			ptr++;
			if (ptr == gTextureIndexMax) ptr = 0; // Loop around.
			if (ptr == gTextureIndexPtr[contextIndex]) break; // Quit if we're back where we started.
		}
        if (gTextures[contextIndex * gTextureIndexMax + ptr].name != 0) {
            EDEN_LOGe("EdenSurfacesTextureLoad(): Error, maximum number of textures already loaded.\n");
            ok = FALSE;
            continue;
        }
		gTextureIndexPtr[contextIndex] = ptr; // Speed up next search by saving index.

#if EDEN_USE_GL
        // OpenGL 1.4 is required for automatic mipmap generation.
        if (textureInfo[i].mipmaps) {
            if (!EdenGLCapabilityCheck(0x0140, NULL)) {
                EDEN_LOGe("ReadTex(): Error: OpenGL implementation does not support automatic mipmap generation.\n");
                ok = FALSE;
                continue;
            }
        }
#endif
        
		offset = contextIndex * gTextureIndexMax + ptr;
		glGenTextures(1, &(gTextures[offset].name));
		gTextures[offset].env_mode = textureInfo[i].env_mode;

		// Bind the texture object and set its initial state.
        glStateCacheActiveTexture(GL_TEXTURE0);
		glStateCacheBindTexture2D(gTextures[offset].name);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureInfo[i].min_filter); 	// Subsampling when minifying.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureInfo[i].mag_filter); 	// Interpolation when magnifying.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureInfo[i].wrap_s); 		// What to do with pixels outside [0,1].
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureInfo[i].wrap_t); 		// What to do with pixels outside [0,1].
#if EDEN_USE_GL
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, textureInfo[i].priority);
#endif
        
		// Read and send the actual texture data to GL.
#ifdef SURFACES_DEBUG
		EDEN_LOGe("EdenSurfacesTextureLoad(): loading file '%s'\n", textureInfo[i].pathname);
#endif
		if (!(data = ReadTex(textureInfo[i].pathname, &sizeX, &sizeY, &nc))) {	// Load image.
			EDEN_LOGe("EdenSurfacesTextureLoad(): Unable to read file '%s'.\n", textureInfo[i].pathname);
			ok = FALSE;
            continue;
		} else {
            
#ifdef SURFACES_DEBUG
            EDEN_LOGe("EdenSurfacesTextureLoad(): image is %dx%d (%d-channel)\n", sizeX, sizeY, nc);
#endif
            // Do an in-place image flip if requested.
            if (flipH || flipV) {
                
                unsigned char *p0, *p1;
                int c, x, y;
                
                // Work out pointers.
                p0 = p1 = data;
                if (flipV) p1 += (sizeY - 1)*sizeX*nc; // Move to last row.
                if (flipH) p1 += (sizeX - 1)*nc; // Move to last column.

                if (flipV && flipH) {
                    while (p0 < p1) {
                        for (c = 0; c < nc; c++) {
                            unsigned char t = p0[c];
                            p0[c] = p1[c];
                            p1[c] = t;
                        }
                        p0 += nc;
                        p1 -= nc;
                    }
                } else if (flipV) {
                    while (p0 < p1) {
                        for (x = 0; x < sizeX; x++) {
                            for (c = 0; c < nc; c++) {
                                unsigned char t = p0[x*nc + c];
                                p0[x*nc + c] = p1[x*nc + c];
                                p1[x*nc + c] = t;
                            }
                        }
                        p0 += sizeX*nc;
                        p1 -= sizeX*nc;
                    }
                } else /* flipH */ {
                    while (p0 < p1) {
                        for (y = 0; y < sizeY; y++) {
                            for (c = 0; c < nc; c++) {
                                unsigned char t = p0[y*sizeX*nc + c];
                                p0[y*sizeX*nc + c] = p1[y*sizeX*nc + c];
                                p1[y*sizeX*nc + c] = t;
                            }
                        }
                        p0 += nc;
                        p1 -= nc;
                    }
                }
            }
            
            if (nc == 3) {
                format = GL_RGB;
            } else if (nc == 4) {
                format = GL_RGBA;
            } else if (nc == 1) {
                if (textureInfo[i].internalformat == GL_ALPHA) format = GL_ALPHA;
                else format = GL_LUMINANCE;
            } else {
                /* not implemented */
                EDEN_LOGe("EdenSurfacesTextureLoad(): %d-component images not supported.\n", nc);
                free(data);
                ok = FALSE;
                continue;
            }
            
            // Check texturing capabilities can accomodate the image.
            if (sizeX > textureSizeMax || sizeY > textureSizeMax) {
                EDEN_LOGe("EdenSurfacesTextureLoad(): Image exceeds maximum texture size (%d).\n", (int)(textureSizeMax));
                free(data);
                ok = FALSE;
                continue;
            }
            
            if (haveNPOT || (!textureInfo[i].mipmaps && haveLimitedNPOT)) {
                textureSizeX = sizeX;
                textureSizeY = sizeY;
            } else {
                // Work out how big power-of-two textures needs to be.
                textureSizeX = textureSizeY = 1;
                while (textureSizeX < sizeX) textureSizeX <<= 1;
                while (textureSizeY < sizeY) textureSizeY <<= 1;
            }
           
            // Upload data.
            if (textureInfo[i].mipmaps) glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            if (sizeX == textureSizeX && sizeY == textureSizeY) {
                glTexImage2D(GL_TEXTURE_2D, 0, format, textureSizeX, textureSizeY, 0, format, GL_UNSIGNED_BYTE, data);
            } else {
                // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
                glTexImage2D(GL_TEXTURE_2D, 0, format, textureSizeX, textureSizeY, 0, format, GL_UNSIGNED_BYTE, NULL);
                // Then send the NPOT-data as a subimage.
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeX, sizeY, format, GL_UNSIGNED_BYTE, data);
            }
            
            free(data);
            
            // Stash info for later.
            gTextures[offset].contentWidth = sizeX;
            gTextures[offset].contentHeight = sizeY;
            gTextures[offset].maxS = (GLfloat)sizeX / (GLfloat)textureSizeX;
            gTextures[offset].maxT = (GLfloat)sizeY / (GLfloat)textureSizeY;
            
            // Info we need to return.
			textureIndices[i] = ptr + 1;
            if (hasAlpha_out) *hasAlpha_out = (format == GL_RGBA || format == GL_ALPHA);
		}
	}

	return (ok);
}

void EdenSurfacesTextureSet(const int contextIndex, TEXTURE_INDEX_t textureIndex)
{
	unsigned int offset;

	if (textureIndex > 0 && textureIndex <= gTextureIndexMax) {
		offset = contextIndex * gTextureIndexMax + textureIndex - 1;
        glStateCacheActiveTexture(GL_TEXTURE0);
		glStateCacheBindTexture2D(gTextures[offset].name);
#if EDEN_USE_GL
		glStateCacheTexEnvMode(gTextures[offset].env_mode);	// Environment mode specific to this texture.
		//glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, gTextures[offset].env_color);	// Environment colour specific to this texture.
#endif
	}
}

EDEN_BOOL EdenSurfacesTextureUnload(const int contextIndex, const int numTextures, TEXTURE_INDEX_t *textureIndices)
{
	EDEN_BOOL ok = TRUE;
	int i;

	if (contextIndex < 0 || contextIndex >= gSurfacesContextsActiveCount) return (FALSE); // Sanity check.

    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(0);
	for (i = 0; i < numTextures; i++) {
		glDeleteTextures(1, &(gTextures[contextIndex * gTextureIndexMax + textureIndices[i] - 1].name));
		gTextures[contextIndex * gTextureIndexMax + textureIndices[i] - 1].name = 0;
		textureIndices[i] = 0;
	}
	return (ok);
}

#if EDEN_USE_GL
EDEN_BOOL EdenSurfacesDraw(const int contextIndex, const TEXTURE_INDEX_t textureIndex, const int width, const int height, const EDEN_TEXTURE_SCALING_MODE scaleMode, const EDEN_TEXTURE_ALIGNMENT_MODE alignMode)
{
	unsigned int offset;
    float aspectRatio, contentAspectRatio, S, T, maxS, maxT, S0, S1, T0, T1;
    
	if (contextIndex < 0 || contextIndex >= gSurfacesContextsActiveCount) return (FALSE); // Sanity check.
	if (textureIndex < 1 || textureIndex > gTextureIndexMax) return (FALSE);

    offset = contextIndex * gTextureIndexMax + textureIndex - 1;
    
    // Calculate S and T as proportion of valid texture to be used in S and T dimensions, e.g. 1.0f == use 100% of texture.
    if (scaleMode == STRETCH) {
        S = 1.0f;
        T = 1.0f;
    } else if (scaleMode == FILL || scaleMode == FIT) {
        // For FILL mode: if aspectRatio > contentAspectRatio, content is too tall and will be cropped top and bottom. If aspectRatio < contentAspectRatio, content is too wide and will be cropped left and right.
        // For FIT mode: if aspectRatio > contentAspectRatio, content is too narrow and will be placed inside black bars left and right. If aspectRatio < contentAspectRatio, content is too short and will be placed inside black bars top and bottom.
        aspectRatio = (float)width / (float)height;
        contentAspectRatio = (float)gTextures[offset].contentWidth / (float)gTextures[offset].contentHeight;
        if (scaleMode == FILL) {
            S = (aspectRatio > contentAspectRatio ? 1.0f                             : aspectRatio / contentAspectRatio);
            T = (aspectRatio > contentAspectRatio ? contentAspectRatio / aspectRatio : 1.0f                            );
        } else /*if (scaleMode == FIT)*/ {
            S = (aspectRatio > contentAspectRatio ? aspectRatio / contentAspectRatio : 1.0f                            ); // S >= 1.
            T = (aspectRatio > contentAspectRatio ? 1.0f                             : contentAspectRatio / aspectRatio); // T >= 1.
        }
    } else { // UNITY
        S = width / (float)gTextures[offset].contentWidth; // Could also be > 1.
        T = height / (float)gTextures[offset].contentHeight; // Could also be > 1.
    }
    // Now convert into actual texture coordinate units.
    maxS = gTextures[offset].maxS;
    maxT = gTextures[offset].maxT;
    S *= maxS;
    T *= maxT;
    
    if (alignMode == TOP_LEFT || alignMode == LEFT || alignMode == BOTTOM_LEFT) {S0 = 0.0f ; S1 = S;}
    else if (alignMode == TOP_RIGHT || alignMode == RIGHT || alignMode == BOTTOM_RIGHT) {S0 = maxS - S; S1 = maxS;}
    else {S0 = maxS*0.5f - S*0.5f; S1 = maxS*0.5f + S*0.5f;}
    
    if (alignMode == BOTTOM_LEFT || alignMode == BOTTOM || alignMode == BOTTOM_RIGHT) {T0 = 0.0f ; T1 = T;}
    else if (alignMode == TOP_LEFT || alignMode == TOP || alignMode == TOP_RIGHT) {T0 = maxT - T; T1 = maxT;}
    else {T0 = maxT*0.5f - T*0.5f; T1 = maxT*0.5f + T*0.5f;}
    
    return (EdenSurfacesDraw2(contextIndex, textureIndex, width, height, S0, S1, T0, T1));
}

EDEN_BOOL EdenSurfacesDraw2(const int contextIndex, const TEXTURE_INDEX_t textureIndex, const int width, const int height, const float S0, const float S1, const float T0, const float T1)
{
	unsigned int offset;
    GLfloat vertices[4][2] = { {0.0f, 0.0f}, {width, 0.0f}, {width, height}, {0.0f, height} };
    GLfloat normals[4][3] = { {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} };
    GLfloat texcoords[4][2];
    
	if (contextIndex < 0 || contextIndex >= gSurfacesContextsActiveCount) return (FALSE); // Sanity check.
	if (textureIndex < 1 || textureIndex > gTextureIndexMax) return (FALSE);
    
    offset = contextIndex * gTextureIndexMax + textureIndex - 1;
    
    texcoords[0][0] = S0; texcoords[0][1] = T1; // Flip incoming image in Y axis.
    texcoords[1][0] = S1; texcoords[1][1] = T1;
    texcoords[2][0] = S1; texcoords[2][1] = T0;
    texcoords[3][0] = S0; texcoords[3][1] = T0;
    
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glStateCacheEnableClientStateVertexArray();
    glNormalPointer(GL_FLOAT, 0, normals);
    glStateCacheEnableClientStateNormalArray();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    glStateCacheEnableClientStateTexCoordArray();
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(gTextures[offset].name);
    glStateCacheTexEnvMode(gTextures[offset].env_mode);	// Environment mode specific to this texture.
    glStateCacheEnableTex2D();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    return (TRUE);
}

#endif // EDEN_USE_GL

GLboolean EdenGluCheckExtension(const GLubyte* extName, const GLubyte *extString)
{
    const GLubyte *start;
    GLubyte *where, *terminator;
    
    // Extension names should not have spaces.
    where = (GLubyte *)strchr((const char *)extName, ' ');
    if (where || *extName == '\0')
        return GL_FALSE;
    // It takes a bit of care to be fool-proof about parsing the
    //    OpenGL extensions string. Don't be fooled by sub-strings, etc.
    start = extString;
    for (;;) {
        where = (GLubyte *) strstr((const char *)start, (const char *)extName);
        if (!where)
            break;
        terminator = where + strlen((const char *)extName);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return GL_TRUE;
        start = terminator;
    }
    return GL_FALSE;
}

int EdenGLCapabilityCheck(const unsigned short minVersion, const unsigned char *extension)
{
    const GLubyte *strVersion;
    const GLubyte *strExtensions;
    short j, shiftVal;
    unsigned short version = 0; // binary-coded decimal gl version (ie. 1.4 is 0x0140).
    
    if (minVersion > 0) {
        strVersion = glGetString(GL_VERSION);
        if (!strVersion) return (FALSE);
#if EDEN_USE_GLES2
        j = 10; // Of the form "OpenGL ES 2.0" etc.
#else
        j = 0;
#endif
        shiftVal = 8;
        // Construct BCD version.
        while (((strVersion[j] <= '9') && (strVersion[j] >= '0')) || (strVersion[j] == '.')) { // Get only basic version info (until first non-digit or non-.)
            if ((strVersion[j] <= '9') && (strVersion[j] >= '0')) {
                version += (strVersion[j] - '0') << shiftVal;
                shiftVal -= 4;
            }
            j++;
        }
        if (version >= minVersion) return (TRUE);
    }
    
    if (extension) {
        strExtensions = glGetString(GL_EXTENSIONS);
        if (EdenGluCheckExtension(extension, strExtensions)) return (TRUE);
    }
    
    return (FALSE);
}
