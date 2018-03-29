/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

#ifndef __glutbitmap_h__
#define __glutbitmap_h__

#if EDEN_USE_GL3
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl3.h>
#  else
#    include <GL3/gl3.h>
#  endif
#elif EDEN_USE_GLES2
#  ifndef EDEN_IPHONEOS
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  else
#    include <OpenGLES/ES2/gl.h>
#    include <OpenGLES/ES2/glext.h>
#  endif
#else
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

typedef struct {
    const GLsizei width;
    const GLsizei height;
    const GLfloat xorig;
    const GLfloat yorig;
    const GLfloat advance;
    const GLubyte *bitmap;
} BitmapCharRec, *BitmapCharPtr;

typedef struct _BitmapFontRec {
    const char *name;
    const int num_chars;
    const int first;
    const BitmapCharRec * const *ch;
} BitmapFontRec, *BitmapFontPtr;

typedef void *GLUTbitmapFont;

#endif /* __glutbitmap_h__ */
