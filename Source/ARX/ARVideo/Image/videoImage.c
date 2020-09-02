/*
 *	videoImage.c
 *  artoolkitX
 *
 *  Video capture module utilising the GStreamer pipeline for AR Toolkit
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoImage.h"

#ifdef ARVIDEO_INPUT_IMAGE

#include <string.h> // memset()
#include "jpeglib.h"

#define AR_VIDEO_IMAGE_XSIZE_DEFAULT   640
#define AR_VIDEO_IMAGE_YSIZE_DEFAULT   480
#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

typedef struct _AR2VideoImageRef AR2VideoImageRef;

struct _AR2VideoImageRef {
    AR2VideoImageRef *next;
    char *pathname;
};

struct _AR2VideoParamImageT {
    AR2VideoBufferT    buffer;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                bufWidth;
    int                bufHeight;
    AR2VideoImageRef  *imageList;
    AR2VideoImageRef  *nextImage;
    unsigned long      imageCount;
    int                loop;
};

#define OUTBUF_HEIGHT_MAX 16

static int jpegGetSize(FILE *fp, int *w, int *h, int *nc, float *dpi)
{
    struct jpeg_decompress_struct    cinfo;
    struct jpeg_error_mgr            jerr;

    memset(&cinfo, 0, sizeof(cinfo));
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    (void) jpeg_read_header(&cinfo, TRUE);
    if (w) *w = cinfo.image_width;
    if (h) *h = cinfo.image_height;
    if (nc) *nc = cinfo.num_components;
    if (dpi) {
        if( cinfo.density_unit == 1 && cinfo.X_density == cinfo.Y_density ) {
            *dpi = cinfo.X_density;
        } else if( cinfo.density_unit == 2 && cinfo.X_density == cinfo.Y_density ) {
            *dpi = cinfo.X_density * 2.54f;
        } else {
            *dpi = 0.0f;
        }
    }
    jpeg_destroy_decompress(&cinfo);
    return (TRUE);
}

static int jpegRead(FILE *fp, unsigned char *buf, int bufWidth, int bufHeight, AR_PIXEL_FORMAT bufPixFormat)
{
    struct jpeg_decompress_struct    cinfo;
    struct jpeg_error_mgr            jerr;
    AR_PIXEL_FORMAT                  pixFormat;
    int                              convertPixFormat = FALSE;
    unsigned char                  **decompressBufRowPtrs = NULL;
    unsigned char                   *decompressBuf;
    int                              bufBytesPerRow, decompressBufRows, rowsToRead, height, decompressBufBytesPerRow;
    int                              row, rowsread, columnsToCopy;
    int                              i, j;
    unsigned char *inp, *outp;

    bufBytesPerRow = bufWidth * arVideoUtilGetPixelSize(bufPixFormat);

    memset(&cinfo, 0, sizeof(cinfo));
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    (void) jpeg_read_header(&cinfo, TRUE);

    // Adjust decompression parameters to match requested.
    if (cinfo.num_components == 3) pixFormat = AR_PIXEL_FORMAT_RGB;
    else if (cinfo.num_components == 4) pixFormat = AR_PIXEL_FORMAT_RGBA;
    else if (cinfo.num_components == 1) pixFormat = AR_PIXEL_FORMAT_MONO;
    else {
        ARLOGe("JPEG file has unsupported %d-component pixels\n", cinfo.num_components);
        jpeg_destroy_decompress(&cinfo);
        return (FALSE);
    }
    if (pixFormat != bufPixFormat) { // Pixel format conversion required?
        if (bufPixFormat == AR_PIXEL_FORMAT_MONO) { // If converting to mono, let libjpeg handle it.
            cinfo.out_color_space = JCS_GRAYSCALE;
        } else {
            convertPixFormat = TRUE; // Otherwise, we must handle it. Either: MONO to RGB, MONO to RGBA, RGB to RGBA, RGBA to RGB.
        }
    }

    // Start decompression. This gives us access to the JPEG size.
    (void) jpeg_start_decompress(&cinfo);
    height = MIN(((int)cinfo.output_height), bufHeight); // If jpeg is taller than our buffer, extra rows will be ignored.

    // Decompression requires a bunch of pointers to where to output the decompressed pixels. Create an array to hold the pointers.
    decompressBufRows = cinfo.rec_outbuf_height;
    arMalloc(decompressBufRowPtrs, unsigned char *, decompressBufRows);

    // If output width is > bufWidth or colour conversion required, decompress into an intermediate buffer and copy/convert.
    // Otherwise, decompress straight into user-supplied buffer (i.e. buf)..
    if (convertPixFormat || ((int)cinfo.output_width) > bufWidth) {

        // Create decompressBuf to decompress into, and a set of pointers to its rows;
        decompressBufBytesPerRow = cinfo.output_width * cinfo.num_components;
        arMalloc(decompressBuf, unsigned char, decompressBufBytesPerRow * decompressBufRows);
        for (i = 0; i < decompressBufRows; i++) decompressBufRowPtrs[i] = &(decompressBuf[decompressBufBytesPerRow*i]);

        row = 0;
        while (((int)cinfo.output_scanline) < height) {
            rowsToRead = MIN(height - ((int)cinfo.output_scanline), decompressBufRows);

            // Decompress.
            rowsread = jpeg_read_scanlines(&cinfo, decompressBufRowPtrs, rowsToRead);

            // Copy, and optionally convert.
            if (!convertPixFormat) {
                // Just copying with truncation.
                for (i = 0; i < rowsread; i++) memcpy(&(buf[bufBytesPerRow*(row + i)]), decompressBufRowPtrs[i], bufBytesPerRow);
            } else {
                // Copy with format conversion.
                columnsToCopy = MIN(bufWidth, ((int)cinfo.output_width)); // Possibly also truncate.
                for (i = 0; i < rowsread; i++) {
                    inp = decompressBufRowPtrs[i];
                    outp = &buf[bufBytesPerRow*(row + i)];
                    if (bufPixFormat == AR_PIXEL_FORMAT_RGBA) {
                        if (pixFormat == AR_PIXEL_FORMAT_RGB) { // RGB to RGBA
                            for (j = 0; j < columnsToCopy; j++) {
                                outp[0] = inp[0];
                                outp[1] = inp[1];
                                outp[2] = inp[2];
                                outp[3] = 255;
                                inp += 3;
                                outp += 4;
                            }
                        } else /*if (pixFormat == AR_PIXEL_FORMAT_MONO)*/ { // MONO to RGBA
                            for (j = 0; j < columnsToCopy; j++) {
                                outp[0] = inp[0];
                                outp[1] = inp[0];
                                outp[2] = inp[0];
                                outp[3] = 255;
                                inp++;
                                outp += 4;
                            }
                        }
                    } else /*if (bufPixFormat == AR_PIXEL_FORMAT_RGB)*/ {
                        if (pixFormat == AR_PIXEL_FORMAT_RGBA) { // RGBA to RGB
                            for (j = 0; j < columnsToCopy; j++) {
                                outp[0] = inp[0];
                                outp[1] = inp[1];
                                outp[2] = inp[2];
                                inp += 4;
                                outp += 3;
                            }
                        } else /*if (pixFormat == AR_PIXEL_FORMAT_MONO)*/ { // MONO to RGB
                            for (j = 0; j < columnsToCopy; j++) {
                                outp[0] = inp[0];
                                outp[1] = inp[0];
                                outp[2] = inp[0];
                                inp++;
                                outp += 3;
                            }
                        }
                    }
                }
            }
            row += rowsread;
        }

        free(decompressBuf);

    } else {

        // Decompress directly to buf.
        row = 0;
        while (((int)cinfo.output_scanline) < height) {
            rowsToRead = MIN(height - ((int)cinfo.output_scanline), decompressBufRows);
            // Update the set of pointers to decompress into.
            for (i = 0; i < rowsToRead; i++) decompressBufRowPtrs[i] = &(buf[bufBytesPerRow*(row + i)]);
            // Decompress.
            row += jpeg_read_scanlines(&cinfo, decompressBufRowPtrs, rowsToRead);
        }
    }

    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    free(decompressBufRowPtrs);

    return (TRUE);
}

int ar2VideoDispOptionImage( void )
{
    ARPRINT(" -module=Image\n");
    ARPRINT("\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    specifies width of image.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    specifies height of image.\n");
    ARPRINT(" -format=X\n");
    ARPRINT("    specifies format of image pixels.\n");
    ARPRINT("    Acceptable values for X are:\n");
    ARPRINT("    RGB (24 bpp)\n");
    ARPRINT("    RGBA (32 bpp)\n");
    ARPRINT("    MONO (8 bpp)\n");
    ARPRINT("    Byte ordering in each case is big-endian, e.g. in RGB format\n");
    ARPRINT("    R occupies the lowest byte in memory, then G, then B. \n");
    ARPRINT(" -image=pathnameOrURL.\n");
    ARPRINT(" -image=\"pathname Or URL\".\n");
    ARPRINT("    specifies image to be read from file (or network, if supported).\n");
    ARPRINT("    Multiple images may be specified by repeated use of this token.\n");
    ARPRINT(" -loop\n");
    ARPRINT("    After reading last image, next read will return first image.\n");
    ARPRINT(" -noloop\n");
    ARPRINT("    After reading last image, no further images will be returned.\n");
    ARPRINT("\n");

    return 0;
}

AR2VideoParamImageT *ar2VideoOpenImage( const char *config )
{
    AR2VideoParamImageT      *vid;
    const char               *a;
    #define LINE_SIZE ((unsigned int)1024)
    char                      line[LINE_SIZE];
    int bufSizeX;
    int bufSizeY;
    char bufferpow2 = 0;
    AR2VideoImageRef *imageListTail = NULL, *imageRef;
    FILE *infile;
    int i, w, h, components;
    int ok, err_i = 0;

    arMalloc( vid, AR2VideoParamImageT, 1 );
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = 0;
    vid->height = 0;
    vid->format = AR_PIXEL_FORMAT_INVALID;
    vid->imageList = NULL;
    vid->imageCount = 0ul;
    vid->loop = FALSE;

    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;

            if (sscanf(a, "%s", line) == 0) break;
            if (strncmp( line, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->width) == 0) {
                    err_i = 1;
                }
            } else if (strncmp( line, "-height=", 8) == 0) {
                if (sscanf( &line[8], "%d", &vid->height) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-image=", 7) == 0) {
                // Attempt to read in image pathname or URL, allowing for quoting of whitespace.
                a += 7; // Skip "-image=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) err_i = 1;
                else {
                    // Create a new entry for the imageList.
                    arMallocClear(imageRef, AR2VideoImageRef, 1); // implicit imageRef->next = NULL;
                    imageRef->pathname = strdup(line);
                    // Add it to the tail of the current list.
                    if (!vid->imageList) vid->imageList = imageRef;
                    else imageListTail->next = imageRef;
                    imageListTail = imageRef;
                    vid->imageCount++;
                }
            } else if( strncmp( line, "-format=", 8 ) == 0 ) {
                if (strcmp(line+8, "RGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB;
                } else if (strcmp(line+8, "RGBA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA;
                } else if (strcmp(line+8, "MONO") == 0) {
                    vid->format = AR_PIXEL_FORMAT_MONO;
                } else {
                    ARLOGe("Request for unsupported pixel format conversion to '%s'. Ignoring.\n", line+8);
                }
            } else if( strcmp( line, "-bufferpow2" ) == 0 )    {
                bufferpow2 = 1;
            } else if (strncmp(a, "-loop", 5) == 0) {
                vid->loop = TRUE;
            } else if (strncmp(a, "-noloop", 7) == 0) {
                vid->loop = FALSE;
            } else if( strcmp( line, "-module=Image" ) == 0 )    {
            } else {
                err_i = 1;
            }

            if (err_i) {
				ARLOGe("Error with configuration option.\n");
                ar2VideoDispOptionImage();
                goto bail;
			}

            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    // Unless parameters were set in config string, attempt to determine them from first image.
    if (!vid->width || !vid->height || vid->format == AR_PIXEL_FORMAT_INVALID) {
        if (!vid->imageList) {
            ARLOGe("No default size and/or pixel format specified and no images specified.\n");
            goto bail;
        }
        if ((infile = fopen(vid->imageList->pathname, "rb")) == NULL) {
            ARLOGe("Error: unable to open JPEG file '%s' for reading.\n", vid->imageList->pathname);
            ARLOGperror(NULL);
            goto bail;
        }
        ok = jpegGetSize(infile, &w, &h, &components, NULL);
        fclose(infile);
        if (!ok) {
            ARLOGe("Can't get size of JPEG file '%s'\n", vid->imageList->pathname);
            goto bail;
        }
        if (!vid->width) vid->width = w;
        if (!vid->height) vid->height = h;
        if (vid->format == AR_PIXEL_FORMAT_INVALID) {
            if (components == 3) vid->format = AR_PIXEL_FORMAT_RGB;
            else if (components == 4) vid->format = AR_PIXEL_FORMAT_RGBA;
            else if (components == 1) vid->format = AR_PIXEL_FORMAT_MONO;
            else {
                ARLOGe("JPEG file '%s' has unsupported %d-component pixels\n", vid->imageList->pathname, components);
                goto bail;
            }
        }
    }

    if (bufferpow2) {
        bufSizeX = bufSizeY = 1;
        while (bufSizeX < vid->width) bufSizeX *= 2;
        while (bufSizeY < vid->height) bufSizeY *= 2;
    } else {
        bufSizeX = vid->width;
        bufSizeY = vid->height;
    }
    if (ar2VideoSetBufferSizeImage(vid, bufSizeX, bufSizeY) != 0) {
        goto bail;
    }

    // Point to head of image list.
    vid->nextImage = vid->imageList;

    ARPRINT("Image video size %dx%d@%dBpp.\n", vid->width, vid->height, arVideoUtilGetPixelSize(vid->format));

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseImage( AR2VideoParamImageT *vid )
{
    AR2VideoImageRef *imageRefToFree;

    if (!vid) return (-1); // Sanity check.
    while (vid->imageList) {
        imageRefToFree = vid->imageList;
        vid->imageList = vid->imageList->next;
        if (imageRefToFree->pathname) free (imageRefToFree->pathname);
        free(imageRefToFree);
        vid->imageCount--;
    }
    ar2VideoSetBufferSizeImage(vid, 0, 0);
    free( vid );

    return 0;
}

int ar2VideoCapStartImage( AR2VideoParamImageT *vid )
{
    return 0;
}

int ar2VideoCapStopImage( AR2VideoParamImageT *vid )
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageImage( AR2VideoParamImageT *vid )
{
    FILE *infile;
    int ok;

    if (!vid) return (NULL); // Sanity check.

    if (vid->nextImage) {
        if ((infile = fopen(vid->nextImage->pathname, "rb")) == NULL) {
            ARLOGe("Error: unable to open JPEG file '%s' for reading.\n", vid->nextImage->pathname);
            ARLOGperror(NULL);
        } else {
            ok = jpegRead(infile, vid->buffer.buff, vid->bufWidth, vid->bufHeight, vid->format);
            fclose(infile);
            if (!ok)
                return (NULL);
            vid->buffer.fillFlag  = 1;
            if (vid->format == AR_PIXEL_FORMAT_MONO) {
                vid->buffer.buffLuma = vid->buffer.buff;
            } else {
                vid->buffer.buffLuma = NULL;
            }
            vid->buffer.time.sec  = 0;
            vid->buffer.time.usec = 0;
        }

        vid->nextImage = vid->nextImage->next; // Next item in linked list.
        if (!vid->nextImage && vid->loop) vid->nextImage = vid->imageList; // If we've hit the end of the list and looping requested, go back to head of linked list.

        return &(vid->buffer);
    } else {
        return (NULL);
    }
}

int ar2VideoGetSizeImage(AR2VideoParamImageT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatImage( AR2VideoParamImageT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoSetBufferSizeImage(AR2VideoParamImageT *vid, const int width, const int height)
{
    int rowBytes;

    if (!vid) return (-1);

    if (vid->buffer.buff) {
        free (vid->buffer.buff);
        vid->buffer.buff = vid->buffer.buffLuma = NULL;
    }

    if (width && height) {
        if (width < vid->width || height < vid->height) {
            ARLOGe("Error: Requested buffer size smaller than video size.\n");
            return (-1);
        }
        rowBytes = width * arVideoUtilGetPixelSize(vid->format);
        vid->buffer.buff = (unsigned char *)malloc(height * rowBytes);
        if (!vid->buffer.buff) {
            ARLOGe("Error: Out of memory!\n");
            return (-1);
        }
        vid->buffer.buffLuma = NULL;
    }

    vid->bufWidth = width;
    vid->bufHeight = height;

    return (0);
}

int ar2VideoGetBufferSizeImage(AR2VideoParamImageT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (width) *width = vid->bufWidth;
    if (height) *height = vid->bufHeight;
    return (0);
}

int ar2VideoGetIdImage( AR2VideoParamImageT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiImage( AR2VideoParamImageT *vid, int paramName, int *value )
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiImage( AR2VideoParamImageT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdImage( AR2VideoParamImageT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdImage( AR2VideoParamImageT *vid, int paramName, double  value )
{
    return -1;
}

int ar2VideoGetParamsImage( AR2VideoParamImageT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsImage( AR2VideoParamImageT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_IMAGE
