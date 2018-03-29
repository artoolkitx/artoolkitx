/* readtex.h */
/* 
 * Read an SGI .rgb image file and generate a mipmap texture set.
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

/*!
    @header readtex
    Texture loading from file.
        readtex forms one part of the Eden library.
    @copyright 2001-2013 Philip Lamb
 */

#ifndef READTEX_H
#define READTEX_H

#ifdef __cplusplus
extern "C" {
#endif
	
#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#include <stdio.h>
    
#ifdef EDEN_HAVE_LIBJPEG     
/*!
    @brief Read a JPEG file and return it as a raw buffer.
    @details Reads a JPEG image from a file into a raw buffer. JPEGs in mono and RGB format
    are supported.
    @param fp File pointer to open jpeg file.
    @param w Pointer to location which will be filled with the width of the JPEG image in pixels,
        or NULL if this is not required.
    @param h Pointer to location which will be filled with the width of the JPEG image in pixels,
        or NULL if this is not required.
    @param nc Pointer to location which will be filled with the number of components in
        the JPEG image in pixels, or NULL if this is not required.
    @param dpi Pointer to location which will be filled with the resolution of the jpeg in
        dots-per-inch, or NULL if this is not required.
    @result A raw buffer holding the jpeg data, beginning with the first component of the
        leftmost pixel of the uppermost scanline, and continuing by component, pixel, and line.
        This buffer must be free()d when finished with.
 */
unsigned char *jpgread (FILE *fp, int *w, int *h, int *nc, float *dpi);
#endif // EDEN_HAVE_LIBJPEG
    
/*!
    @brief Read an image file and return it as a raw buffer.
    @param imageFile name of image to read. At present, images in SGI format (.sgi or .rgb)
        and JPEG format (.jpg or .jpeg) are supported.
    @param w Pointer to location which will be filled with the width of the image in pixels,
        or NULL if this is not required.
    @param h Pointer to location which will be filled with the width of the image in pixels,
        or NULL if this is not required.
    @param nc Pointer to location which will be filled with the number of components in
        the image in pixels, or NULL if this is not required.
    @result A raw buffer holding the image data, beginning with the first component of the
        leftmost pixel of the uppermost scanline, and continuing by component, pixel, and line.
        This buffer must be free()d when finished with.
 */
extern unsigned char *ReadTex(const char *imageFile, int *w, int *h, int *nc);

#ifdef __cplusplus
}
#endif

#endif	/* !READTEX_H */
