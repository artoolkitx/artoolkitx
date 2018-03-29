//
//  drawOSG.cpp
//  artoolkitX Square Tracking Example
//
//  Copyright 2018 Realmax, Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#include "draw.h"
#include <ARX/ARController.h>

#if HAVE_GLES2 || HAVE_GL3
#  include <ARX/ARG/mtx.h>
#  if HAVE_GLES2
#    if ARX_TARGET_PLATFORM_IOS
#      include <OpenGLES/ES2/gl.h>
#    else
#      include <GLES2/gl2.h>
#    endif
#  else
#    if ARX_TARGET_PLATFORM_MACOS
#      include <OpenGL/gl3.h>
#    else
#      define GL_GLEXT_PROTOTYPES
#      include <GL/glcorearb.h>
#    endif
#  endif

#include "arosg.h"

static ARG_API drawAPI = ARG_API_None;
static bool rotate90 = false;
static bool flipH = false;
static bool flipV = false;

static AROSG *arOSG = NULL;

void drawSetup(ARG_API drawAPI_in, bool rotate90_in, bool flipH_in, bool flipV_in)
{
    drawAPI = drawAPI_in;
    rotate90 = rotate90_in;
    flipH = flipH_in;
    flipV = flipV_in;
    
    if (arOSG) return;
    
    arOSG = arOSGInit();
    if (!arOSG) {
        ARLOGe("Unable to init OSG.\n");
        return;
    }
    arOSGSetFrontFace(arOSG, (int)(flipH != flipV)); // Change winding if either flipH or flipV but not both.

    return;
}

void drawCleanup()
{
    if (!arOSG) return;
    
    arOSGFinal(arOSG); arOSG = NULL;
    
    return;
}

int drawLoadModel(const char *path)
{
    return arOSGLoadModel(arOSG, path);
}

void drawSetViewport(int32_t viewport[4])
{
    arOSGHandleReshape2(arOSG, viewport[0], viewport[1], viewport[2], viewport[3]);
}

void drawSetCamera(float projection[16], float pose[16])
{
    if (projection) {
        if (flipH || flipV) {
            float p[16];
            mtxLoadIdentityf(p);
            mtxScalef(p, flipV ? -1.0f : 1.0f,  flipH ? -1.0f : 1.0f, 1.0f);
            mtxMultMatrixf(p, projection);
            arOSGSetProjectionf(arOSG, p);
        } else {
            arOSGSetProjectionf(arOSG, projection);
        }
    }
    if (pose) {
        arOSGSetViewf(arOSG, pose);
    }
}

void drawSetModel(int modelIndex, bool visible, float pose[16])
{
    arOSGSetModelVisibility(arOSG, modelIndex, (int)visible);
    if (visible) arOSGSetModelPosef(arOSG, modelIndex, pose);
}

void draw()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

    arOSGDraw(arOSG);
}

#endif // HAVE_GLES2 || HAVE_GL3
