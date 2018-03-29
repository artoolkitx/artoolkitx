/*
 *  videoSaveImage.c
 *  artoolkitX
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <ARX/ARVideo/video.h>
#if !AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT

#include <stdio.h>
#include <string.h>
#include "jpeglib.h"

#ifndef MAX
#  define MAX(x,y) (x > y ? x : y)
#endif
#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif
#ifndef CLAMP
#  define CLAMP(x,r1,r2) (MIN(MAX(x,r1),r2))
#endif

int arVideoSaveImageJPEG(int w, int h, AR_PIXEL_FORMAT pixFormat, ARUint8 *pixels, const char *filename, const int quality /* 0 to 100 */, const int flipV)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE   *outfile;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if ((outfile = fopen(filename, "wb")) == NULL) {
        ARLOGe("Error: can't open file '%s' for writing.\n", filename);
        ARLOGperror(NULL);
        jpeg_destroy_compress(&cinfo);
        return (-1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = w;      /* image width and height, in pixels */
    cinfo.image_height = h;
    if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21) {
        cinfo.input_components = 1;     /* # of color components per pixel */
        cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
    } else {
        cinfo.input_components = 3;     /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
    }
    jpeg_set_defaults(&cinfo);
    /* Make optional parameter settings here */
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

    jpeg_start_compress(&cinfo, TRUE);

    if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21 || pixFormat == AR_PIXEL_FORMAT_RGB) {
        while (cinfo.next_scanline < cinfo.image_height) {
            JSAMPROW row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * cinfo.input_components];
            jpeg_write_scanlines(&cinfo, &row, 1);
        }
    } else {
        JSAMPROW row = (JSAMPROW)malloc(cinfo.image_width * cinfo.input_components);
        if (!row) {
            ARLOGe("Out of memory.\n");
            fclose(outfile);
            jpeg_destroy_compress(&cinfo);
            return (-1);
        }
        while (cinfo.next_scanline < cinfo.image_height) {
            unsigned char *pixels_row;
            unsigned int i;
            switch (pixFormat) {
                case AR_PIXEL_FORMAT_BGR:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 3];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] = pixels_row[i*3 + 2];
                        row[i*3 + 1] = pixels_row[i*3 + 1];
                        row[i*3 + 2] = pixels_row[i*3 + 0];
                    }
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 4];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] = pixels_row[i*4 + 0];
                        row[i*3 + 1] = pixels_row[i*4 + 1];
                        row[i*3 + 2] = pixels_row[i*4 + 2];
                    }
                    break;
                case AR_PIXEL_FORMAT_BGRA:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 4];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] = pixels_row[i*4 + 2];
                        row[i*3 + 1] = pixels_row[i*4 + 1];
                        row[i*3 + 2] = pixels_row[i*4 + 0];
                    }
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 4];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] = pixels_row[i*4 + 1];
                        row[i*3 + 1] = pixels_row[i*4 + 2];
                        row[i*3 + 2] = pixels_row[i*4 + 3];
                    }
                    break;
                case AR_PIXEL_FORMAT_ABGR:
                    pixels_row = &pixels[cinfo.next_scanline * cinfo.image_width * 4];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] = pixels_row[i*4 + 3];
                        row[i*3 + 1] = pixels_row[i*4 + 2];
                        row[i*3 + 2] = pixels_row[i*4 + 1];
                    }
                    break;
                case AR_PIXEL_FORMAT_2vuy:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 2];
                    for (i = 0; i < cinfo.image_width; i += 2) {
                        unsigned char Cb =      pixels_row[i*2 + 0];
                        unsigned char Yprime0 = pixels_row[i*2 + 1];
                        unsigned char Cr =      pixels_row[i*2 + 2];
                        unsigned char Yprime1 = pixels_row[i*2 + 3];
                        // Conversion from Poynton's color FAQ http://www.poynton.com.
                        row[i*3 + 0] = CLAMP((int)(298.082f*(Yprime0 - 16) +   0.000f*(Cb - 128) + 408.583f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 1] = CLAMP((int)(298.082f*(Yprime0 - 16) - 100.291f*(Cb - 128) - 208.120f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 2] = CLAMP((int)(298.082f*(Yprime0 - 16) + 516.411f*(Cb - 128) +   0.000f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 3] = CLAMP((int)(298.082f*(Yprime1 - 16) +   0.000f*(Cb - 128) + 408.583f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 4] = CLAMP((int)(298.082f*(Yprime1 - 16) - 100.291f*(Cb - 128) - 208.120f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 5] = CLAMP((int)(298.082f*(Yprime1 - 16) + 516.411f*(Cb - 128) +   0.000f*(Cr - 128)) >> 8, 0, 255);
                    }
                    break;
                case AR_PIXEL_FORMAT_yuvs:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 2];
                    for (i = 0; i < cinfo.image_width; i += 2) {
                        unsigned char Yprime0 = pixels_row[i*2 + 0];
                        unsigned char Cb =      pixels_row[i*2 + 1];
                        unsigned char Yprime1 = pixels_row[i*2 + 2];
                        unsigned char Cr =      pixels_row[i*2 + 3];
                        // Conversion from Poynton's color FAQ http://www.poynton.com.
                        row[i*3 + 0] = CLAMP((int)(298.082f*(Yprime0 - 16) +   0.000f*(Cb - 128) + 408.583f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 1] = CLAMP((int)(298.082f*(Yprime0 - 16) - 100.291f*(Cb - 128) - 208.120f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 2] = CLAMP((int)(298.082f*(Yprime0 - 16) + 516.411f*(Cb - 128) +   0.000f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 3] = CLAMP((int)(298.082f*(Yprime1 - 16) +   0.000f*(Cb - 128) + 408.583f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 4] = CLAMP((int)(298.082f*(Yprime1 - 16) - 100.291f*(Cb - 128) - 208.120f*(Cr - 128)) >> 8, 0, 255);
                        row[i*3 + 5] = CLAMP((int)(298.082f*(Yprime1 - 16) + 516.411f*(Cb - 128) +   0.000f*(Cr - 128)) >> 8, 0, 255);
                    }
                    break;
                case AR_PIXEL_FORMAT_RGB_565:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 2];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] =  ((pixels_row[i*2 + 0] & 0xf8) + 0x04);
                        row[i*3 + 1] = (((pixels_row[i*2 + 0] & 0x07) << 5) + ((pixels_row[i*2 + 1] & 0xe0) >> 3) + 0x02);
                        row[i*3 + 2] =                                       (((pixels_row[i*2 + 1] & 0x1f) << 3) + 0x04);
                    }
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 2];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] =  ((pixels_row[i*2 + 0] & 0xf8) + 0x04);
                        row[i*3 + 1] = (((pixels_row[i*2 + 0] & 0x07) << 5) + ((pixels_row[i*2 + 1] & 0xc0) >> 3) + 0x04);
                        row[i*3 + 2] =                                       (((pixels_row[i*2 + 1] & 0x3e) << 2) + 0x04);
                    }
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
                    pixels_row = &pixels[(flipV ? (h - 1 - cinfo.next_scanline) : cinfo.next_scanline) * cinfo.image_width * 2];
                    for (i = 0; i < cinfo.image_width; i++) {
                        row[i*3 + 0] =  ((pixels_row[i*2 + 0] & 0xf0) + 0x08);
                        row[i*3 + 1] = (((pixels_row[i*2 + 0] & 0x0f) << 4) + 0x08);
                        row[i*3 + 2] =  ((pixels_row[i*2 + 1] & 0xf0) + 0x08);
                    }
                    break;
                default:
                    break;
            }
            jpeg_write_scanlines(&cinfo, &row, 1);
        }
        free(row);
    }

    jpeg_finish_compress(&cinfo);

    fclose(outfile);

    jpeg_destroy_compress(&cinfo);

    return (0);

}

#endif // !AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
