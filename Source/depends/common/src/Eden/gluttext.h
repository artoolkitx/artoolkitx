/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

//
//	Updated by Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//
//
// Defines glut stroke and bitmap font functions for OpenGL ES-based platforms.
//

#ifndef __gluttext_h__
#define __gluttext_h__

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLUTTEXT_BITMAP_ENABLE 0
#define GLUTTEXT_STROKE_ENABLE 1

#if GLUTTEXT_STROKE_ENABLE

/* Stroke font opaque addresses (use constants instead in source code). */
extern const struct _StrokeFontRec glutStrokeRoman;
extern const struct _StrokeFontRec glutStrokeMonoRoman;

/* Stroke font constants (use these in GLUT program). */

/*
   A proportionally spaced Roman Simplex font for ASCII characters
   32 through 127. The maximum top character in the font is 119.05
   units; the bottom descends 33.33 units.
 */
#define GLUT_STROKE_ROMAN		((void *)&glutStrokeRoman)

/*
   A mono-spaced spaced Roman Simplex  font  (same  characters  as
   GLUT_STROKE_ROMAN)  for  ASCII  characters  32 through 127. The
   maximum top character in the font is 119.05 units;  the  bottom
   descends 33.33 units. Each character is 104.76 units wide.
 */
#define GLUT_STROKE_MONO_ROMAN		((void *)&glutStrokeMonoRoman)

#if EDEN_USE_GL
extern void glutStrokeCharacter(void *font, int character);
#elif EDEN_USE_GLES2
extern void glutStrokeCharacter(void *font, int character, uint32_t vertexAttribIndex, float *translateX);
#endif
extern int glutStrokeWidth(void *font, int character);
extern int glutStrokeLength(void *font, const unsigned char *string);
#endif // GLUTTEXT_STROKE_ENABLE

#if GLUTTEXT_BITMAP_ENABLE

/* Bitmap font opaque addresses (use constants instead in source code). */
extern const struct _BitmapFontRec glutBitmap9By15;
extern const struct _BitmapFontRec glutBitmap8By13;
extern const struct _BitmapFontRec glutBitmapTimesRoman10;
extern const struct _BitmapFontRec glutBitmapTimesRoman24;
extern const struct _BitmapFontRec glutBitmapHelvetica10;
extern const struct _BitmapFontRec glutBitmapHelvetica12;
extern const struct _BitmapFontRec glutBitmapHelvetica18;

/* Bitmap font constants (use these in GLUT program). */
#define GLUT_BITMAP_9_BY_15		((void *)&glutBitmap9By15)
#define GLUT_BITMAP_8_BY_13		((void *)&glutBitmap8By13)
#define GLUT_BITMAP_TIMES_ROMAN_10	((void *)&glutBitmapTimesRoman10)
#define GLUT_BITMAP_TIMES_ROMAN_24	((void *)&glutBitmapTimesRoman24)
#define GLUT_BITMAP_HELVETICA_10	((void *)&glutBitmapHelvetica10)
#define GLUT_BITMAP_HELVETICA_12	((void *)&glutBitmapHelvetica12)
#define GLUT_BITMAP_HELVETICA_18	((void *)&glutBitmapHelvetica18)
extern void glutBitmapCharacter(void *font, int character);
extern int glutBitmapWidth(void *font, int character);
extern int glutBitmapLength(void *font, const unsigned char *string);
#endif // GLUTTEXT_BITMAP_ENABLE

#ifdef __cplusplus
}
#endif
#endif // !__gluttext_h__
