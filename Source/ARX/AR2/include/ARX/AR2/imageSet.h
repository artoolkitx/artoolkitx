/*
 *  AR2/imageSet.h
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

#ifndef AR2_IMAGE_SET_H
#define AR2_IMAGE_SET_H
#include <ARX/AR/ar.h>
#include <ARX/AR2/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
#define    AR2_BLUR_IMAGE_MAX  5
#endif


typedef struct {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    ARUint8      *imgBWBlur[AR2_BLUR_IMAGE_MAX];
#else
    ARUint8      *imgBW;
#endif
    int           xsize;
    int           ysize;
    float         dpi;
} AR2ImageT;

typedef struct {
    AR2ImageT   **scale;
    int32_t       num;
} AR2ImageSetT;

/*   image.c   */
AR2_EXTERN AR2ImageSetT   *ar2GenImageSet   ( ARUint8 *image, int xsize, int ysize, int nc, float dpi, float dpi_list[], int dpi_num );
AR2_EXTERN AR2ImageSetT   *ar2ReadImageSet  ( char *filename );
AR2_EXTERN int             ar2WriteImageSet ( char *filename, AR2ImageSetT *imageSet );
AR2_EXTERN int             ar2FreeImageSet  ( AR2ImageSetT **imageSet );

#ifdef __cplusplus
}
#endif
#endif
