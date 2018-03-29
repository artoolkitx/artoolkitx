#include <Eden/gluttext.h>
#if GLUTTEXT_BITMAP_ENABLE

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

//#include "glutint.h"
#include "glutbitmap.h"

void glutBitmapCharacter(GLUTbitmapFont font, int c)
{
    const BitmapCharRec *ch;
    BitmapFontPtr fontinfo;
#if 0
    GLint swapbytes, lsbfirst, rowlength;
    GLint skiprows, skippixels;
#endif
    GLint alignment;
    
    fontinfo = (BitmapFontPtr) font;
    
    if (c < fontinfo->first ||
        c >= fontinfo->first + fontinfo->num_chars)
        return;
    ch = fontinfo->ch[c - fontinfo->first];
    if (ch) {
        /* Save current modes. */
#if 0
        glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
        glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
#endif
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        /* Little endian machines (DEC Alpha for example) could
         benefit from setting GL_UNPACK_LSB_FIRST to GL_TRUE
         instead of GL_FALSE, but this would require changing the
         generated bitmaps too. */
#if 0
        glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
        glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
#endif
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBitmap(ch->width, ch->height, ch->xorig, ch->yorig,
                 ch->advance, 0, ch->bitmap);
        /* Restore saved modes. */
#if 0
        glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
        glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
#endif
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
}

#endif