/* readtex.c */

/* 
 * Read an SGI .rgb image file and generate a mipmap texture set.
 * Much of this code was borrowed from SGI's tk OpenGL toolkit.
 *
 *	By Mark Kilgard (MK) mkilgard@nvidia.com
 *	Revised by Geoff Stahl (GGS) gstahl@apple.com
 *	Revised by Philip Lamb (PRL) phil@eden.net.nz
 *
 *	Rev		Date		Who		Changes
 *	1.0.1	19990420	GGS		Fixed memory leaks.
 *	1.0.2	20011111	PRL		Improved commenting.
 *	1.0.3	20011219	PRL		Fixed more memory leaks.
 *	1.0.4	20020103	PRL		Fixed more memory leaks in case of error and improved error handling.
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


#include <Eden/readtex.h>

#if EDEN_USE_GL
#  ifdef EDEN_MACOSX
#    include <OpenGL/glu.h>
#  else
#    ifdef _WIN32
#      include <windows.h>
#    endif
#    include <GL/glu.h>
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <Eden/EdenUtil.h> // EdenGetFileExtensionFromPath()

#ifdef EDEN_HAVE_LIBJPEG
#  include <jpeglib.h>
#endif

// Core in OpenGL 1.4.
#ifndef GL_SGIS_generate_mipmap
#define GENERATE_MIPMAP_SGIS            0x8191
#endif

/******************************************************************************/

/* RGB Image Structure */
// 108 byte header.
typedef struct {
    uint16_t imagic;				// Filled in with bytes 0-1 of file. IRIS image file magic number.
    uint8_t  storage;               // Filled in with byte 2 of file. Storage format.
    uint8_t  bpc;                   // Filled in with byte 3 of file. Number of bytes per pixel channel.
    uint16_t dim;					// Filled in with bytes 4-5 of file. Number of dimensions
    uint16_t sizeX, sizeY, sizeZ;	// Filled in with bytes 6-11 of file. X, Y, size in pixels, number of channels.
    uint32_t min, max;              // Filled in with bytes 12-19 of file. Minimum, maximum pixel values.
    uint32_t wasteBytes;            // Filled in with bytes 20-23 of file. Ignored.
    char name[80];                  // Filled in with bytes 24-103 of file. Image name.
    uint32_t colorMap;              // Filled in with bytes 104-107 of file. Colormap ID.
} rawImageHeader;

/******************************************************************************/

static void ConvertShort(uint16_t *array, size_t length)
{
    uint16_t b1, b2;
    unsigned char *ptr;
    
    ptr = (unsigned char *) array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        *array++ = (b1 << 8) | (b2);
    }
}

static void ConvertInt(uint32_t *array, size_t length)
{
    uint32_t b1, b2, b3, b4;
    unsigned char *ptr;
    
    ptr = (unsigned char *) array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        b3 = *ptr++;
        b4 = *ptr++;
        *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}

// Load an SGI .rgb file.
// Supports arbitrary number of channels.
static unsigned char *RawImageRead(const char *imageFile, int *w, int *h, int *nc)
{
    int i, j, k, count;
    FILE *fp = NULL;
    long fileLen;
    unsigned char *fileContents = NULL;
    rawImageHeader *header = NULL;
    uint32_t tableLength;
    uint32_t *rowOffsetsTable = NULL;
    uint32_t *rowLengthsTable = NULL;
    unsigned char **rows = NULL;
    unsigned char *data = NULL;
    unsigned char *dataPtr = NULL;
    unsigned char *oPtr = NULL;
    unsigned char *iPtr = NULL;
    unsigned char pixel;
    int outCount;
    
    if ((fp = fopen(imageFile, "rb")) == NULL) {
        EDEN_LOGe("RawImageRead(): Can't open file '%s' for reading.\n", imageFile);
        EDEN_LOGperror(NULL);
        return (NULL);
    }

    // Determine length of file.
    if (fseek(fp, 0L, SEEK_END) != 0) {
        EDEN_LOGe("RawImageRead(): Error finding length of file '%s'.\n", imageFile);
        EDEN_LOGperror(NULL);
        fclose(fp);
        return (NULL);
    }
    fileLen = ftell(fp);
    rewind(fp);
    
    // Read in entire file in one go.
    fileContents = (unsigned char *)malloc(fileLen);
    if (!fileContents) {
        EDEN_LOGe("Unable to allocate memory to read %ld byte image file.\n", fileLen);
        fclose(fp);
        return (NULL);
    }
    
    if (fread(fileContents, fileLen, 1, fp) < 1) {
        if (ferror(fp)) {
            EDEN_LOGperror("Error reading file");
            fclose(fp);
            goto bail;
        }
    }
    fclose(fp);
    
    // Interpret file header.
    header = (rawImageHeader *)fileContents; // First 108 bytes of file are header.
#ifndef EDEN_BIGENDIAN
    ConvertShort(&(header->imagic), 1);
#endif
    
    if (header->imagic != 474) { // Convert imagic to native endianness.
        EDEN_LOGe("File does not appear to be a raw (SGI) format image.\n");
        goto bail;
    }
#ifndef EDEN_BIGENDIAN
    ConvertShort(&(header->dim), 4); // Convert dim, sizeX, sizeY, sizeZ to native endianness.
    ConvertInt(&(header->min), 2);  // Convert min, max to native endianness.
#endif
    if (header->bpc > 1) {
        EDEN_LOGe("Request to open %d bytes-per-component image, but max. 1 supported.\n", header->bpc);
        goto bail;
    }
    
	if (header->storage == 1) {
        // For RLE-encoded, we'll need to read the table of row start
        // positions, and row sizes. There are sizeY * sizeZ entries,
        // each an unsigned 32-bit int.
		tableLength = header->sizeY * header->sizeZ * sizeof(uint32_t);
        rowOffsetsTable = (uint32_t *)(fileContents + 512);
        rowLengthsTable = (uint32_t *)(fileContents + 512 + tableLength);
#ifndef EDEN_BIGENDIAN
        ConvertInt(rowOffsetsTable, header->sizeY*header->sizeZ);
        ConvertInt(rowLengthsTable, header->sizeY*header->sizeZ);
#endif
	}
    
    if (w) *w = header->sizeX;
    if (h) *h = header->sizeY;
    if (nc) *nc = header->sizeZ;
    
    data = (unsigned char *)malloc(header->sizeX * header->sizeY * header->sizeZ);
    if (!data) {
        EDEN_LOGe("Unable to allocate memory to for image.\n");
        goto bail;
    }
    
    // Set up structures used to unpack the data.
    rows = (unsigned char **)calloc(header->sizeZ, sizeof(unsigned char *));
    for (j = 0; j < header->sizeZ; j++) {
        if (!(rows[j] = (unsigned char *)malloc(header->sizeX))) {
            EDEN_LOGe("Out of memory!\n");
            goto bailRows;
        }
    }
    
    // Now unpack.
    dataPtr = data;
    for (i = 0; i < header->sizeY; i++) {
        // Unpack all planes for a row.
        for (j = 0; j < header->sizeZ; j++) {
            if (header->storage == 1) {
                // Unpack RLE-encoded pixels.
                iPtr = fileContents + rowOffsetsTable[i + j*header->sizeY];
                oPtr = rows[j];
                outCount = 0;
                for (;;) {
                    if (outCount == header->sizeX) break;
                    pixel = *(iPtr++);
                    count = (int)(pixel & 0x7F);
                    if (!count) break; // End of line.
                    if (pixel & 0x80) {
                        while (outCount < header->sizeX && count--) {
                            *(oPtr++) = *(iPtr++);
                            outCount++;
                        }
                    } else {
                        pixel = *(iPtr++);
                        while (outCount < header->sizeX && count--) {
                            *(oPtr++) = pixel;
                            outCount++;
                        }
                    }
                }
            } else {
                // Read pixels directly.
                memcpy(rows[j], fileContents + 512 + header->sizeX * (i + j*header->sizeY), header->sizeX);
            }
        }
        // Copy the row, interleaving.
        for (k = 0; k < header->sizeX; k++) {
            for (j = 0; j < header->sizeZ; j++) {
                *(dataPtr++) = rows[j][k];
            }
        }
    }
    
    // Clean up work structures.
    for (j = 0; j < header->sizeZ; j++) if (rows[j]) free(rows[j]);
    free(rows);
    free(fileContents);
    
    return (data);
	
	// When bailing out, do the correct free()s.
bailRows:
    for (j = 0; j < header->sizeZ; j++) if (rows[j]) free(rows[j]);
    free(rows);
    free(data);
bail:
    free(fileContents);
    return (NULL);
}

#ifdef EDEN_HAVE_LIBJPEG
#define BUFFER_HEIGHT 5

unsigned char *jpgread (FILE *fp, int *w, int *h, int *nc, float *dpi)
{
    struct jpeg_decompress_struct    cinfo;
    struct jpeg_error_mgr            jerr;
    unsigned char                    *pixels;
    unsigned char                    *buffer[BUFFER_HEIGHT];
    int                              bytes_per_line;
    int                              row;
    int                              i;
    int                              ret;
    
    /* Initialize the JPEG decompression object with default error handling. */
    memset(&cinfo, 0, sizeof(cinfo));
    
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr);
    /* jerr.pub.error_exit = my_error_exit; */
    
    jpeg_create_decompress(&cinfo);
    
    /* Specify data source for decompression */
    jpeg_stdio_src(&cinfo, fp);
    
    /* Read file header, set default decompression parameters */
    ret = jpeg_read_header(&cinfo, TRUE);
    if( ret != 1 ) {
        EDEN_LOGe("Error reading JPEG file header.\n");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }
    
    /* Start decompressor */
    (void) jpeg_start_decompress(&cinfo);
    
    /* Allocate image buffer */
    bytes_per_line = cinfo.num_components * cinfo.image_width;
    pixels = (unsigned char *)malloc(bytes_per_line  * cinfo.image_height);
    if (!pixels) {
        EDEN_LOGe("Out of memory!!\n");
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
#endif // EDEN_HAVE_LIBJPEG

unsigned char *ReadTex(const char *imageFile, int *w, int *h, int *nc)
{
#ifdef EDEN_HAVE_LIBJPEG
    FILE *infile;
#endif
    unsigned char *ret;
    
    // Check the filename extension.
    char *imageFileExtension = EdenGetFileExtensionFromPath(imageFile, TRUE);
    if (strcmp(imageFileExtension, "sgi") == 0 || strcmp(imageFileExtension, "rgb") == 0 || strcmp(imageFileExtension, "rgba") == 0 || strcmp(imageFileExtension, "bw") == 0) {
        
        ret = RawImageRead(imageFile, w, h, nc);
        if (!ret) {
            EDEN_LOGe("ReadTex(): Can't read data from file '%s'.\n", imageFile);
            free(imageFileExtension);
            return (NULL);
        }
        
    } else if (strncmp(imageFileExtension, "jpg", 3) == 0 || strncmp(imageFileExtension, "jpeg", 4) == 0) {
        
#ifdef EDEN_HAVE_LIBJPEG        
        if ((infile = fopen(imageFile, "rb")) == NULL) {
            EDEN_LOGe("ReadTex(): Can't open file '%s' for reading.\n", imageFile);
            EDEN_LOGperror(NULL);
            free(imageFileExtension);
            return (NULL);
        }
        ret = jpgread(infile, w, h, nc, NULL);
        fclose(infile);
        
        if (!ret) {
            EDEN_LOGe("ReadTex(): Can't read data from file '%s'.\n", imageFile);
            free(imageFileExtension);
            return (NULL);
        }
#else
        EDEN_LOGe("ReadTex(): Error: unable to read JPEG image file; libjpeg not available.\n");
        free(imageFileExtension);
        return (NULL);
#endif
    } else {
        EDEN_LOGe("ReadTex(): Error: unsupported image file of type '%s'.\n", imageFileExtension);
        free(imageFileExtension);
        return (NULL);
    }
    
    free(imageFileExtension);
    return (ret);
}
