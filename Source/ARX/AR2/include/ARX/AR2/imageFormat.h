/*
 *  AR2/imageFormat.h
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR2_IMAGE_FORMAT_H
#define AR2_IMAGE_FORMAT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <ARX/AR2/config.h>

#define   AR2_DEFAULT_JPEG_IMAGE_QUALITY   80


typedef struct {
    ARUint8       *image;
    int           nc;
    int           xsize;
    int           ysize;
    float         dpi;
} AR2JpegImageT;

/*   jpeg.c   */

AR2_EXTERN AR2JpegImageT *ar2ReadJpegImage  ( const char *filename, const char *ext );
AR2_EXTERN AR2JpegImageT *ar2ReadJpegImage2 ( FILE *fp );
AR2_EXTERN int            ar2WriteJpegImage ( const char *filename, const char *ext, AR2JpegImageT *jpegImage, int quality );
AR2_EXTERN int            ar2WriteJpegImage2( FILE *fp, AR2JpegImageT *jpegImage, int quality );
AR2_EXTERN int            ar2FreeJpegImage  ( AR2JpegImageT **jpegImage );

#ifdef __cplusplus
}
#endif
#endif
