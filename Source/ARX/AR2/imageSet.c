/*
 *  AR2/imageSet.c
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

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#  define lroundf(x) ((x)>=0.0f?(long)((x)+0.5f):(long)((x)-0.5f))
#endif
#include <ARX/AR2/imageFormat.h>
#include <ARX/AR2/imageSet.h>

static AR2ImageT *ar2GenImageLayer1 ( ARUint8 *image, int xsize, int ysize, int nc, float srcdpi, float dstdpi );
static AR2ImageT *ar2GenImageLayer2 ( AR2ImageT *src, float dstdpi );
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static void       defocus_image     ( ARUint8 *img, int xsize, int ysize, int n );
#endif
static AR2ImageSetT *ar2ReadImageSetOld( FILE *fp );

AR2ImageSetT *ar2GenImageSet( ARUint8 *image, int xsize, int ysize, int nc, float dpi, float dpi_list[], int dpi_num )
{
    AR2ImageSetT   *imageSet;
    int             i;

    if( nc != 1 && nc != 3 )    return NULL;
    if( dpi_num <= 0 )          return NULL;
    if( dpi_list[0] > dpi )     return NULL;
    for( i = 1; i < dpi_num; i++ ) {
        if( dpi_list[i] > dpi_list[0] ) return NULL;
    }

    arMalloc( imageSet, AR2ImageSetT, 1 );
    imageSet->num = dpi_num;
    arMalloc( imageSet->scale,  AR2ImageT*,  imageSet->num );

    imageSet->scale[0] = ar2GenImageLayer1( image, xsize, ysize, nc, dpi, dpi_list[0] );
    for( i = 1; i < dpi_num; i++ ) {
        imageSet->scale[i] = ar2GenImageLayer2( imageSet->scale[0], dpi_list[i] );
    }

    return imageSet;
}

AR2ImageSetT *ar2ReadImageSet( char *filename )
{
    FILE          *fp;
    AR2JpegImageT *jpgImage;
    AR2ImageSetT  *imageSet;
    float          dpi;
    int            i, k1;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    int            j, k2;
    ARUint        *p1, *p2;
#endif
    size_t         len;
    const char     ext[] = ".iset";
    char          *buf;
    
    len = strlen(filename) + strlen(ext) + 1; // +1 for nul terminator.
    arMalloc(buf, char, len);
    sprintf(buf, "%s%s", filename, ext);
    fp = fopen(buf, "rb");
    free(buf);
    if (!fp) {
        ARLOGe("Error: unable to open file '%s%s' for reading.\n", filename, ext);
        return (NULL);
    }

    arMalloc( imageSet, AR2ImageSetT, 1 );

    if( fread(&(imageSet->num), sizeof(imageSet->num), 1, fp) != 1 || imageSet->num <= 0) {
        ARLOGe("Error reading imageSet.\n");
        goto bail;
    }
    ARLOGi("Imageset contains %d images.\n", imageSet->num);
    arMalloc( imageSet->scale, AR2ImageT*, imageSet->num );

    arMalloc( imageSet->scale[0], AR2ImageT, 1 );
    jpgImage = ar2ReadJpegImage2(fp); // Caller must free result.
    if( jpgImage == NULL || jpgImage->nc != 1 ) {
        ARLOGw("Falling back to reading '%s%s' in ARToolKit v4.x format.\n", filename, ext);
        free(imageSet->scale[0]);
        free(imageSet->scale);
        free(imageSet);
        
        if( jpgImage == NULL ) {
            rewind(fp);
            return ar2ReadImageSetOld(fp);
        }
        free(jpgImage); //COVHI10396
        fclose(fp);
        return NULL;
    }
    imageSet->scale[0]->xsize = jpgImage->xsize;
    imageSet->scale[0]->ysize = jpgImage->ysize;
    imageSet->scale[0]->dpi   = jpgImage->dpi; // The dpi value is not read correctly by jpeglib embedded in OpenCV 2.2.x.
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    imageSet->scale[0]->imgBWBlur[0] = jpgImage->image;
    // Create the blurred images.
    for( j = 1; j < AR2_BLUR_IMAGE_MAX; j++ ) {
        arMalloc( imageSet->scale[0]->imgBWBlur[j], ARUint8, imageSet->scale[0]->xsize * imageSet->scale[0]->ysize);
        p1 = dst->imgBWBlur[0];
        p2 = dst->imgBWBlur[i];
        for( k1 = 0; k1 < imageSet->scale[0]->xsize * imageSet->scale[0]->ysize; k1++ ) *(p2++) = *(p1++);
        defocus_image( imageSet->scale[0]->imgBWBlur[j], imageSet->scale[0]->xsize, imageSet->scale[0]->ysize, 3 );
    }
#else
    imageSet->scale[0]->imgBW = jpgImage->image;
#endif
    free(jpgImage);

    // Minify for the other scales.
    // First, find the list of scales we wrote into the file.
    fseek(fp, (long)(-(int)sizeof(dpi)*(imageSet->num - 1)), SEEK_END);
    for( i = 1; i < imageSet->num; i++ ) {
        
        if( fread(&dpi, sizeof(dpi), 1, fp) != 1 ) {
            for( k1 = 0; k1 < i; k1++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                for( k2 = 0; k2 < AR2_BLUR_IMAGE_MAX; k2++ ) free(imageSet->scale[k1]->imgBWBlur[k2]);
#else
                free(imageSet->scale[k1]->imgBW);
#endif
                free(imageSet->scale[k1]);
            }
            goto bail1;
        }
        
        imageSet->scale[i] = ar2GenImageLayer2( imageSet->scale[0], dpi );
        if( imageSet->scale[i] == NULL ) {
            for( k1 = 0; k1 < i; k1++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                for( k2 = 0; k2 < AR2_BLUR_IMAGE_MAX; k2++ ) free(imageSet->scale[k1]->imgBWBlur[k2]);
#else
                free(imageSet->scale[k1]->imgBW);
#endif
                free(imageSet->scale[k1]);
            }
            goto bail1;
        }
    }

    fclose(fp);

    return imageSet;
    
    
bail1:
    free(imageSet->scale);
bail:
    free(imageSet);
    fclose(fp);
    return NULL;
}

int ar2WriteImageSet( char *filename, AR2ImageSetT *imageSet )
{
    FILE          *fp;
    AR2JpegImageT  jpegImage;
    int            i;
    size_t         len;
    const char     ext[] = ".iset";
    char          *buf;
    
    len = strlen(filename) + strlen(ext) + 1; // +1 for nul terminator.
    arMalloc(buf, char, len);
    sprintf(buf, "%s%s", filename, ext);
    if( (fp=fopen(buf, "wb")) == NULL ) {
        ARLOGe("Error: unable to open file '%s' for writing.\n", buf);
        free(buf);
        return (-1);
    }
    free(buf);

    if( fwrite(&(imageSet->num), sizeof(imageSet->num), 1, fp) != 1 ) goto bailBadWrite;

    jpegImage.xsize = imageSet->scale[0]->xsize;
    jpegImage.ysize = imageSet->scale[0]->ysize;
    jpegImage.dpi   = imageSet->scale[0]->dpi;
    jpegImage.nc    = 1;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    jpegImage.image = imageSet->scale[0]->imgBWBlur[0];
#else
    jpegImage.image = imageSet->scale[0]->imgBW;
#endif

    if( ar2WriteJpegImage2(fp, &jpegImage, AR2_DEFAULT_JPEG_IMAGE_QUALITY) < 0 ) goto bailBadWrite;

    for( i = 1; i < imageSet->num; i++ ) {
        if( fwrite(&(imageSet->scale[i]->dpi), sizeof(imageSet->scale[i]->dpi), 1, fp) != 1 ) goto bailBadWrite;
    }

    fclose(fp);
    return 0;
    
bailBadWrite:
    ARLOGe("Error saving image set: error writing data.\n");
    fclose(fp);
    return (-1);
}

int ar2FreeImageSet( AR2ImageSetT **imageSet )
{
    int    i;

    if(  imageSet == NULL ) return -1;
    if( *imageSet == NULL ) return -1;

    for( i = 0; i < (*imageSet)->num; i++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        for( int j = 0; j < AR2_BLUR_IMAGE_MAX; j++ ) {
            free( (*imageSet)->scale[i]->imgBWBlur[j] );
        }
#else
        free( (*imageSet)->scale[i]->imgBW  );
#endif
        free( (*imageSet)->scale[i] );
    }
    free( (*imageSet)->scale );
    free( *imageSet );
    *imageSet = NULL;

    return 0;
}

static AR2ImageT *ar2GenImageLayer1( ARUint8 *image, int xsize, int ysize, int nc, float srcdpi, float dstdpi )
{
    AR2ImageT   *dst;
    ARUint8     *p1, *p2;
    int          wx, wy;
    int          sx, sy, ex, ey;
    int          ii, jj, iii, jjj;
    int          co, value;

    wx = (int)lroundf(xsize * dstdpi / srcdpi);
    wy = (int)lroundf(ysize * dstdpi / srcdpi);

    arMalloc( dst, AR2ImageT, 1 );
    dst->xsize = wx;
    dst->ysize = wy;
    dst->dpi   = dstdpi;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    for( int i = 0; i < AR2_BLUR_IMAGE_MAX; i++ ) {
        arMalloc( dst->imgWBlur[i], ARUint8, wx*wy );
    }
    p2 = dst->imgBWBlur[0];
#else
    arMalloc( dst->imgBW, ARUint8, wx*wy );
    p2 = dst->imgBW;
#endif

    // Scale down, nearest neighbour.
    for( jj = 0; jj < wy; jj++ ) {
        sy = (int)lroundf( jj    * srcdpi / dstdpi);
        ey = (int)lroundf((jj+1) * srcdpi / dstdpi) - 1;
        if( ey >= ysize ) ey = ysize - 1;
        for( ii = 0; ii < wx; ii++ ) {
            sx = (int)lroundf( ii    * srcdpi / dstdpi);
            ex = (int)lroundf((ii+1) * srcdpi / dstdpi) - 1;
            if( ex >= xsize ) ex = xsize - 1;

            co = value = 0;
            if( nc == 1 ) {
                for( jjj = sy; jjj <= ey; jjj++ ) {
                    p1 = &(image[(jjj*xsize+sx)*nc]);
                    for( iii = sx; iii <= ex; iii++ ) {
                        value += *(p1++);
                        co++;
                    }
                }
            }
            else {
                for( jjj = sy; jjj <= ey; jjj++ ) {
                    p1 = &(image[(jjj*xsize+sx)*nc]);
                    for( iii = sx; iii <= ex; iii++ ) {
                        value += *(p1++);
                        value += *(p1++);
                        value += *(p1++);
                        co+=3;
                    }
                }
            }
            *(p2++) = value / co;
        }
    }

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    for( int i = 1; i < AR2_BLUR_IMAGE_MAX; i++ ) {
        p1 = dst->imgBWBlue[0];
        p2 = dst->imgBWBlue[i];
        for( int j = 0; j < wx*wy; j++ ) *(p2++) = *(p1++);
        defocus_image( dst->imgBWBlur[i], wx, wy, 2 );
    }
#else
    //defocus_image( dst->imgBW, wx, wy, 3 );
#endif

    return dst;
}

static AR2ImageT *ar2GenImageLayer2( AR2ImageT *src, float dpi )
{
    AR2ImageT   *dst;
    ARUint8     *p1, *p2;
    int          wx, wy;
    int          sx, sy, ex, ey;
    int          ii, jj, iii, jjj;
    int          co, value;

    wx = (int)lroundf(src->xsize * dpi / src->dpi);
    wy = (int)lroundf(src->ysize * dpi / src->dpi);

    arMalloc( dst, AR2ImageT, 1 );
    dst->xsize = wx;
    dst->ysize = wy;
    dst->dpi   = dpi;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    for( int i = 0; i < AR2_BLUR_IMAGE_MAX; i++ ) {
        arMalloc( dst->imgBWBlur[i], ARUint8, wx*wy );
    }
    p2 = dst->imgBWBlue[0];
#else
    arMalloc( dst->imgBW, ARUint8, wx*wy );
    p2 = dst->imgBW;
#endif

    for( jj = 0; jj < wy; jj++ ) {
        sy = (int)lroundf( jj    * src->dpi / dpi);
        ey = (int)lroundf((jj+1) * src->dpi / dpi) - 1;
        if( ey >= src->ysize ) ey = src->ysize - 1;
        for( ii = 0; ii < wx; ii++ ) {
            sx = (int)lroundf( ii    * src->dpi / dpi);
            ex = (int)lroundf((ii+1) * src->dpi / dpi) - 1;
            if( ex >= src->xsize ) ex = src->xsize - 1;

            co = value = 0;
            for( jjj = sy; jjj <= ey; jjj++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                p1 = &(src->imgBWBlur[0][jjj*src->xsize+sx]);
#else
                p1 = &(src->imgBW[jjj*src->xsize+sx]);
#endif
                for( iii = sx; iii <= ex; iii++ ) {
                    value += *(p1++);
                    co++;
                }
            }
            *(p2++) = value / co;
        }
    }

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    defocus_image( dst->imageBWBlur[0], wx, wy, 3 );
    for( int i = 1; i < AR2_BLUR_IMAGE_MAX; i++ ) {
        p1 = dst->imgBWBlue[0];
        p2 = dst->imgBWBlue[i];
        for( int j = 0; j < wx*wy; j++ ) *(p2++) = *(p1++);
        defocus_image( dst->imgBWBlur[i], wx, wy, 2 );
    }
#else
    //defocus_image( dst->imgBW, wx, wy, 3 );
#endif

    return dst;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static void defocus_image( ARUint8 *img, int xsize, int ysize, int n )
{
    ARUint8        *wimg;
    int             isize;
    ARUint8         *p1, *p2;
    int             i, j, k, w;

    isize = xsize * ysize;
    arMalloc( wimg, ARUint8, xsize*ysize );

    for( k = 0; k < n; k++ ) {
        if( k%2 == 0 ) {
            p1 = img;
            p2 = wimg;
        }
        else {
            p1 = wimg;
            p2 = img;
        }
        for( j = 0; j < ysize; j++ ) {
            for( i = 0; i < xsize; i++ ) {
                if( i == 0 || j == 0 || i == xsize-1 || j == ysize-1 ) {
                    *(p2++) = *(p1++);
                    continue;
                }

                w = *(p1-xsize-1) + *(p1-xsize) + *(p1-xsize+1)
                 +  *(p1-1)       + *(p1)       + *(p1+1)
                 +  *(p1+xsize-1) + *(p1+xsize) + *(p1+xsize+1);
                *(p2++) = w / 9;
                p1++;
            }
        }
    }

    if( n%2 == 1 ) {
        p1 = wimg;
        p2 = img;
        for( i = 0; i < xsize*ysize; i++ ) *(p2++) = *(p1++);
    }

    free(wimg);

    return;
}
#endif

static AR2ImageSetT *ar2ReadImageSetOld( FILE *fp )
{
    AR2ImageSetT  *imageSet;
    int            i, k;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    int            j, l;
#endif

    arMalloc( imageSet, AR2ImageSetT, 1 );
    
    if( fread(&(imageSet->num), sizeof(imageSet->num), 1, fp) != 1 || imageSet->num <= 0) {
        ARLOGe("Error reading imageSet.\n");
        goto bail;
    }

    arMalloc( imageSet->scale, AR2ImageT*, imageSet->num );
    for( i = 0; i < imageSet->num; i++ ) {
        arMalloc( imageSet->scale[i], AR2ImageT, 1 );
    }
    
    for( i = 0; i < imageSet->num; i++ ) {
        if( fread(&(imageSet->scale[i]->xsize), sizeof(imageSet->scale[i]->xsize), 1, fp) != 1 ) {
            for( k = 0; k < i; k++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                for( l = 0; l < AR2_BLUR_IMAGE_MAX; l++) free(imageSet->scale[k]->imgBWBlur[l]);
#else
                free(imageSet->scale[k]->imgBW);
#endif
            }
            for( k = 0; k < imageSet->num; k++ ) free(imageSet->scale[k]);
            goto bail1;
        }
        if( fread(&(imageSet->scale[i]->ysize), sizeof(imageSet->scale[i]->ysize), 1, fp) != 1 ) {
            for( k = 0; k < i; k++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                for( l = 0; l < AR2_BLUR_IMAGE_MAX; l++) free(imageSet->scale[k]->imgBWBlur[l]);
#else
                free(imageSet->scale[k]->imgBW);
#endif
            }
            for( k = 0; k < imageSet->num; k++ ) free(imageSet->scale[k]);
            goto bail1;
        }
        if( fread(&(imageSet->scale[i]->dpi), sizeof(imageSet->scale[i]->dpi), 1, fp) != 1 ) {
            for( k = 0; k < i; k++ ) {
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                for( l = 0; l < AR2_BLUR_IMAGE_MAX; l++) free(imageSet->scale[k]->imgBWBlur[l]);
#else
                free(imageSet->scale[k]->imgBW);
#endif
            }
            for( k = 0; k < imageSet->num; k++ ) free(imageSet->scale[k]);
            goto bail1;
        }
        
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        for( j = 0; j < AR2_BLUR_IMAGE_MAX; j++ ) {
            arMalloc( imageSet->scale[i]->imgBWBlur[j], ARUint8, imageSet->scale[i]->xsize * imageSet->scale[i]->ysize);
        }
#else
        arMalloc( imageSet->scale[i]->imgBW,  ARUint8, imageSet->scale[i]->xsize * imageSet->scale[i]->ysize);
#endif
        
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        for( j = 0; j < AR2_BLUR_IMAGE_MAX; j++ ) {
            if( fread(imageSet->scale[i]->imgBWBlur[j], sizeof(ARUint8), imageSet->scale[i]->xsize * imageSet->scale[i]->ysize, fp)
               != imageSet->scale[i]->xsize * imageSet->scale[i]->ysize ) {
                for( k = 0; k <= i; k++ ) {
                    for( l = 0; l < AR2_BLUR_IMAGE_MAX; l++) free(imageSet->scale[k]->imgBWBlur[l]);
                }
                for( k = 0; k < imageSet->num; k++ ) free(imageSet->scale[k]);
                goto bail1;
            }
        }
#else
        if( fread(imageSet->scale[i]->imgBW, sizeof(ARUint8), imageSet->scale[i]->xsize * imageSet->scale[i]->ysize, fp)
           != imageSet->scale[i]->xsize * imageSet->scale[i]->ysize ) {
            for( k = 0; k <= i; k++ ) {
                free(imageSet->scale[k]->imgBW);
            }
            for( k = 0; k < imageSet->num; k++ ) free(imageSet->scale[k]);
            goto bail1;
        }
#endif
    }
    
    fclose(fp);
    return imageSet;
    
bail1:
    free(imageSet->scale);
bail:
    free(imageSet);
    fclose(fp);
    return NULL;
}

