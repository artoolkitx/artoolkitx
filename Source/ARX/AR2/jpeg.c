/*
 *  AR2/jpeg.c
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
#include "jpeglib.h"
#include <setjmp.h>
#include <ARX/AR2/imageFormat.h>

struct my_error_mgr {
    struct jpeg_error_mgr pub;	/* "public" fields */    
    jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

static unsigned char *jpgread  (FILE *fp, int *w, int *h, int *nc, float *dpi);
static int            jpgwrite (FILE *fp, unsigned char *image, int w, int h, int nc, float dpi, int quality);

int ar2WriteJpegImage( const char *filename, const char *ext, AR2JpegImageT *jpegImage, int quality )
{
    FILE           *fp;
    int             ret;
    size_t          len;
    char           *buf1;

    len = strlen(filename) + strlen(ext) + 1;
    arMalloc(buf1, char, len + 1); // +1 for nul terminator.
    sprintf(buf1, "%s.%s", filename, ext);
    fp = fopen(buf1, "wb");
    if( fp == NULL ) {
        ARLOGe("Error: Unable to open file '%s' for writing.\n", buf1);
        free(buf1);
        return -1;
    }
    free(buf1);

    ret = ar2WriteJpegImage2( fp, jpegImage, quality );

    fclose(fp);
    return ret;
}

int ar2WriteJpegImage2( FILE *fp, AR2JpegImageT *jpegImage, int quality )
{
    return jpgwrite(fp, jpegImage->image, jpegImage->xsize, jpegImage->ysize, jpegImage->nc, jpegImage->dpi, quality);
}

AR2JpegImageT *ar2ReadJpegImage( const char *filename, const char *ext )
{
    FILE           *fp;
    AR2JpegImageT  *jpegImage;
    size_t          len;
    char           *buf1;
    
    
    len = strlen(filename) + strlen(ext) + 1;
    arMalloc(buf1, char, len + 1); // +1 for nul terminator.
    sprintf(buf1, "%s.%s", filename, ext);
    fp = fopen(buf1, "rb");
    if( fp == NULL ) {
        ARLOGe("Error: Unable to open file '%s' for reading.\n", buf1);
        free(buf1);
        return (NULL);
    }
    free(buf1);

    jpegImage = ar2ReadJpegImage2(fp);

    fclose(fp);
    return jpegImage;
}

AR2JpegImageT *ar2ReadJpegImage2( FILE *fp )
{
    AR2JpegImageT  *jpegImage;
    
    arMalloc( jpegImage, AR2JpegImageT, 1 );
    jpegImage->image = jpgread(fp, &(jpegImage->xsize), &(jpegImage->ysize), &(jpegImage->nc), &(jpegImage->dpi));

    if( jpegImage->image == NULL ) {
        free( jpegImage );
        return NULL;
    }

    return jpegImage;
}

int ar2FreeJpegImage( AR2JpegImageT **jpegImage )
{
    if( jpegImage == NULL ) return -1;
    if( *jpegImage == NULL ) return -1;

    free( (*jpegImage)->image );
    free( (*jpegImage) );

    *jpegImage = NULL;

    return 0;
}

#define BUFFER_HEIGHT 5

static char jpegLastErrorMsg[JMSG_LENGTH_MAX] = "";
static void my_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    
    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    //(*cinfo->err->output_message) (cinfo);

    /* Create the message */
    (*(cinfo->err->format_message)) (cinfo, jpegLastErrorMsg);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

static unsigned char *jpgread (FILE *fp, int *w, int *h, int *nc, float *dpi)
{
    struct jpeg_decompress_struct    cinfo;
    struct my_error_mgr              jerr;
    unsigned char                    *pixels;
    unsigned char                    *buffer[BUFFER_HEIGHT];
    int                              bytes_per_line;
    int                              row;
    int                              i;
    int                              ret;

    /* Initialize the JPEG decompression object with default error handling. */
    memset(&cinfo, 0, sizeof(cinfo));

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);
        ARLOGe("Error reading JPEG file: %s\n", jpegLastErrorMsg);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);

    /* Specify data source for decompression */
    jpeg_stdio_src(&cinfo, fp);

    /* Read file header, set default decompression parameters */
    ret = jpeg_read_header(&cinfo, TRUE);
    if( ret != 1 ) {
        ARLOGe("Error reading JPEG file header.\n");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }

    /* Start decompressor */
    (void) jpeg_start_decompress(&cinfo);

    /* Allocate image buffer */
    bytes_per_line = cinfo.num_components * cinfo.image_width;
    pixels = (unsigned char *)malloc(bytes_per_line  * cinfo.image_height);
    if (!pixels) {
        ARLOGe("Out of memory!!\n");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }

    row = 0;

    /* Process data */
    while (cinfo.output_scanline < cinfo.output_height) {
        for (i=0; i<BUFFER_HEIGHT; ++i) {
            /* read in "upside down" because opengl says the
             * texture origin is lower left 
             */
            //int rrow = cinfo.output_height - row - 1;
            //buffer[i] = &pixels[bytes_per_line * (rrow - i)];
            buffer[i] = &pixels[bytes_per_line * (row + i)];
        }
        row += jpeg_read_scanlines(&cinfo, buffer, BUFFER_HEIGHT);
    }

    (void) jpeg_finish_decompress(&cinfo);

    if (w) *w = cinfo.image_width;
    if (h) *h = cinfo.image_height;
    if (nc) *nc = cinfo.num_components;
    if (dpi) {
        if( cinfo.density_unit == 1 && cinfo.X_density == cinfo.Y_density ) {
            *dpi = (float)cinfo.X_density;
        } else if( cinfo.density_unit == 2 && cinfo.X_density == cinfo.Y_density ) {
            *dpi = (float)cinfo.X_density * 2.54f;
        } else if (cinfo.density_unit > 2 && cinfo.X_density == 0 && cinfo.Y_density == 0) { // Handle the case with some libjpeg versions where density in DPI is returned in the density_unit field.
            *dpi = (float)(cinfo.density_unit);
        } else {
            *dpi = 0.0f;
        }
    }
    
    jpeg_destroy_decompress(&cinfo);

    return pixels;
}

static int jpgwrite (FILE *fp, unsigned char *image, int w, int h, int nc, float dpi, int quality)
{
    struct jpeg_compress_struct    cinfo;
    struct jpeg_error_mgr          jerr;
    unsigned char  *p;
    int i, j;
    JSAMPARRAY img;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width    = w;
    cinfo.image_height   = h;
    if( nc == 1 ) {
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    }
    else if( nc == 3 ) {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }
    else return -1;
    jpeg_set_defaults(&cinfo);
    cinfo.density_unit   = 1;
    cinfo.X_density      = (UINT16)dpi;
    cinfo.Y_density      = (UINT16)dpi;
    cinfo.write_JFIF_header = 1;

    if( quality <   0 ) quality = 0;
    if( quality > 100 ) quality = 100;
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    p = image;
    img = (JSAMPARRAY) malloc(sizeof(JSAMPROW) * h);
    for (i = 0; i < h; i++) {
        img[i] = (JSAMPROW) malloc(sizeof(JSAMPLE) * nc * w);
        if( nc == 1 ) {
            for (j = 0; j < w; j++) {
                img[i][j] = *(p++);
            }
        }
        else if( nc == 3 ) {
            for (j = 0; j < w; j++) {
                img[i][j*3+0] = *(p++);
                img[i][j*3+1] = *(p++);
                img[i][j*3+2] = *(p++);
            }
        }
    }
    jpeg_write_scanlines(&cinfo, img, h);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    for (i = 0; i < h; i++) free(img[i]);
    free(img);

    return 0;
}
