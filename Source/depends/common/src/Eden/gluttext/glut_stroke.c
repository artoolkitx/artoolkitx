#include <Eden/gluttext.h>
#if GLUTTEXT_STROKE_ENABLE
#if EDEN_USE_GL
#  define USE_GL_STATE_CACHE 0
#  include <Eden/glStateCache.h>
#elif EDEN_USE_GLES2
#  include <stdlib.h>
#  include <ARX/ARG/glStateCache2.h>
#endif

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

//#include "glutint.h"
#include "glutstroke.h"

#if EDEN_USE_GL
void glutStrokeCharacter(GLUTstrokeFont font, int c)
#elif EDEN_USE_GLES2
void glutStrokeCharacter(GLUTstrokeFont font, int c, uint32_t vertexAttribIndex, float *translateX)
#endif
{
    const StrokeCharRec *ch;
    const StrokeRec *stroke;
    StrokeFontPtr fontinfo;
    int i;
    
    fontinfo = (StrokeFontPtr) font;
    
    if (c < 0 || c >= fontinfo->num_chars)
        return;
    ch = &(fontinfo->ch[c]);
    if (ch) {
#if EDEN_USE_GL
        for (i = ch->num_strokes, stroke = ch->stroke;
             i > 0; i--, stroke++) {
            // Essential setup for glDrawArrays();
            glVertexPointer(2, GL_FLOAT, 0, stroke->coord);
            glStateCacheEnableClientStateVertexArray();
            glStateCacheClientActiveTexture(GL_TEXTURE0);
            glStateCacheDisableClientStateTexCoordArray();
            glStateCacheDisableClientStateNormalArray();
            
            glDrawArrays(GL_LINE_STRIP, 0, stroke->num_coords);
        }
        glTranslatef(ch->right, 0.0f, 0.0f);
#elif EDEN_USE_GLES2
        for (i = ch->num_strokes, stroke = ch->stroke;
             i > 0; i--, stroke++) {
            // Essential setup for glDrawArrays();
            glVertexAttribPointer(vertexAttribIndex, 2, GL_FLOAT, GL_FALSE, 0, stroke->coord);
            glEnableVertexAttribArray(vertexAttribIndex);
            glDrawArrays(GL_LINE_STRIP, 0, stroke->num_coords);
        }
        if (translateX) *translateX = ch->right;
#endif
    }
}

#endif
