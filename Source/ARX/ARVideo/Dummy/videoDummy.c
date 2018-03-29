/*
 *  videoDummy.c
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/* 
 *   Dummy video capture subrutine
 *   author: Hirokazu Kato ( kato@sys.im.hiroshima-cu.ac.jp )
 *
 *   Revision: 6.0   Date: 2003/09/29
 */

#include "videoDummy.h"

#ifdef ARVIDEO_INPUT_DUMMY

#include <string.h> // memset()

#define AR_VIDEO_DUMMY_XSIZE   640
#define AR_VIDEO_DUMMY_YSIZE   480
#define ARVIDEO_INPUT_DUMMY_DEFAULT_PIXEL_FORMAT   AR_PIXEL_FORMAT_RGB

int ar2VideoDispOptionDummy( void )
{
    ARPRINT(" -module=Dummy\n");
    ARPRINT("\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    specifies width of image.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    specifies height of image.\n");
    ARPRINT(" -format=X\n");
    ARPRINT("    specifies format of image pixels.\n");
    ARPRINT("    Acceptable values for X are:\n");
    ARPRINT("    RGB (24 bpp)\n");
    ARPRINT("    BGR (24 bpp)\n");
    ARPRINT("    RGBA (32 bpp)\n");
    ARPRINT("    BGRA (32 bpp)\n");
    ARPRINT("    ARGB (32 bpp)\n");
    ARPRINT("    ABGR (32 bpp)\n");
    ARPRINT("    RGBA_5551 (16 bpp)\n");
    ARPRINT("    RGBA_4444 (16 bpp)\n");
    ARPRINT("    RGBA_565 (16 bpp)\n");
    ARPRINT("    Byte ordering in each case is big-endian, e.g. in RGB format\n");
    ARPRINT("    R occupies the lowest byte in memory, then G, then B. \n");
    ARPRINT("\n");

    return 0;
}

AR2VideoParamDummyT *ar2VideoOpenDummy( const char *config )
{
    AR2VideoParamDummyT *vid;
    const char *a;
#   define B_SIZE ((unsigned int)256)
    char b[B_SIZE];
    int bufSizeX;
    int bufSizeY;
    char bufferpow2 = 0;

    arMalloc(vid, AR2VideoParamDummyT, 1);
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = AR_VIDEO_DUMMY_XSIZE;
    vid->height = AR_VIDEO_DUMMY_YSIZE;
    vid->format = ARVIDEO_INPUT_DUMMY_DEFAULT_PIXEL_FORMAT;

    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-width=", 7) == 0) {
                if (sscanf(&b[7], "%d", &vid->width) == 0) {
                    ar2VideoDispOptionDummy();
                    goto bail;
                }
            }
            else if (strncmp(b, "-height=", 8) == 0) {
                if (sscanf(&b[8], "%d", &vid->height) == 0 ) {
                    ar2VideoDispOptionDummy();
                    goto bail;
                }
            }
            else if (strncmp(b, "-format=", 8 ) == 0) {
                if (strcmp(b+8, "RGBA_5551") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_5551;
                    ARLOGi("Dummy video will return red-green-blue bars in RGBA_5551.\n");
                } else if (strcmp(b+8, "RGBA_4444") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_4444;
                    ARLOGi("Dummy video will return red-green-blue bars in RGBA_4444.\n");
                } else if (strcmp(b+8, "RGBA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA;
                    ARLOGi("Dummy video will return red-green-blue bars in RGBA.\n");
                } else if (strcmp(b+8, "RGB_565") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB_565;
                    ARLOGi("Dummy video will return red-green-blue bars in RGB_565.\n");
                } else if (strcmp(b+8, "RGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB;
                    ARLOGi("Dummy video will return red-green-blue bars in RGB.\n");
                } else if (strcmp(b+8, "ARGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_ARGB;
                    ARLOGi("Dummy video will return red-green-blue bars in ARGB.\n");
                } else if (strcmp(b+8, "ABGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_ABGR;
                    ARLOGi("Dummy video will return red-green-blue bars in ABGR.\n");
                } else if (strcmp(b+8, "BGRA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGRA;
                    ARLOGi("Dummy video will return red-green-blue bars in BGRA.\n");
                } else if (strcmp(b+8, "BGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGR;
                    ARLOGi("Dummy video will return red-green-blue bars in BGR.\n");
                } else if (strcmp(b+8, "420f") == 0) {
                    vid->format = AR_PIXEL_FORMAT_420f;
                    ARLOGi("Dummy video will return red-green-blue bars in 420f.\n");
                } else if (strcmp(b+8, "NV21") == 0) {
                    vid->format = AR_PIXEL_FORMAT_NV21;
                    ARLOGi("Dummy video will return red-green-blue bars in NV21.\n");
                }
            }
            else if (strcmp(b, "-bufferpow2") == 0)    {
                bufferpow2 = 1;
            }
            else if (strcmp(b, "-module=Dummy") == 0)    {
            }
            else {
                ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                ar2VideoDispOptionDummy();
                goto bail;
            }

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
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
    if (!bufSizeX || !bufSizeY || ar2VideoSetBufferSizeDummy(vid, bufSizeX, bufSizeY) != 0) {
        goto bail;
    }

    ARLOGi("Dummy video size %dx%d@%dBpp.\n", vid->width, vid->height, arVideoUtilGetPixelSize(vid->format));

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseDummy( AR2VideoParamDummyT *vid )
{
    if (!vid) return (-1); // Sanity check.

    ar2VideoSetBufferSizeDummy(vid, 0, 0);
    free( vid );

    return 0;
}

int ar2VideoCapStartDummy( AR2VideoParamDummyT *vid )
{
    return 0;
}

int ar2VideoCapStopDummy( AR2VideoParamDummyT *vid )
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageDummy( AR2VideoParamDummyT *vid )
{
    static int   k = 0;
    ARUint8     *p, *p1;
    int          i, j;

    if (!vid) return (NULL); // Sanity check.

    // Create 100 x 100 square, oscillating in x dimension.
    k++;
    if( k > 100 ) k = -100;
    if (!vid->buffer.bufPlaneCount) {
        p = vid->buffer.buff;
        p1 = NULL;
    } else {
        p = vid->buffer.bufPlanes[0];
        p1 = vid->buffer.bufPlanes[1];
    }


    // Clear buffer to white.
    memset(p, 255, vid->bufWidth*vid->bufHeight*arVideoUtilGetPixelSize(vid->format));
    if (p1) memset(p1, 128, vid->bufWidth*vid->bufHeight/2);

    for( j = vid->height/2 - 50; j < vid->height/2 - 25; j++ ) {
        for( i = vid->width/2 - 50 + k; i < vid->width/2 + 50 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
                    p[(j*vid->bufWidth+i)*2+0] = p[(j*vid->bufWidth+i)*2+1] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x01;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x01;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                case AR_PIXEL_FORMAT_420f:
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 0;
                    break;
                default:
                    break;
            }
        }
    }
    for( j = vid->height/2 - 25; j < vid->height/2 + 25; j++ ) {
        // Black bar (25 pixels wide).
        for( i = vid->width/2 - 50 + k; i < vid->width/2 - 25 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                    // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
                    p[(j*vid->bufWidth+i)*2+0] = p[(j*vid->bufWidth+i)*2+1] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x01;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x01;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                case AR_PIXEL_FORMAT_420f:
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 0;
                    break;
                default:
                    break;
            }
        }
        // Red bar (17 pixels wide).
        for( i = vid->width/2 - 25 + k; i < vid->width/2 - 8 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                    p[(j*vid->bufWidth+i)*3+0] = 255; p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = 0; p[(j*vid->bufWidth+i)*3+2] = 255;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = 0; p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = 255; p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0xf8; p[(j*vid->bufWidth+i)*2+0] = 0;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0xf8; p[(j*vid->bufWidth+i)*2+1] = 0;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0xf8; p[(j*vid->bufWidth+i)*2+0] = 0x01;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0xf8; p[(j*vid->bufWidth+i)*2+1] = 0x01;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0xf0; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0xf0; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                    p[j*vid->bufWidth+i] = 76;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 255; p1[(j/2)*vid->bufWidth+i+1] = 86;
                    }
                    break;
                case AR_PIXEL_FORMAT_420f:
                    p[j*vid->bufWidth+i] = 76;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 86; p1[(j/2)*vid->bufWidth+i+1] = 255;
                    }
                    break;
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 76;
                    break;
                default:
                    break;
            }
        }
        // Green bar (16 pixels wide).
        for( i = vid->width/2 - 8 + k; i < vid->width/2 + 8 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = 0; p[(j*vid->bufWidth+i)*3+1] = 255; p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = 0; p[(j*vid->bufWidth+i)*4+1] = 255; p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = 0; p[(j*vid->bufWidth+i)*4+2] = 255; p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0x07; p[(j*vid->bufWidth+i)*2+0] = 0xe0;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0x07; p[(j*vid->bufWidth+i)*2+1] = 0xe0;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0x07; p[(j*vid->bufWidth+i)*2+0] = 0xc1;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0x07; p[(j*vid->bufWidth+i)*2+1] = 0xc1;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0x0f; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0x0f; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                case AR_PIXEL_FORMAT_NV21:
                    p[j*vid->bufWidth+i] = 150;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 21; p1[(j/2)*vid->bufWidth+i+1] = 44;
                    }
                    break;
                case AR_PIXEL_FORMAT_420f:
                    p[j*vid->bufWidth+i] = 150;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 44; p1[(j/2)*vid->bufWidth+i+1] = 21;
                    }
                    break;
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 150;
                    break;
                default:
                    break;
            }
        }
        // Blue bar (17 pixels wide).
        for( i = vid->width/2 + 8 + k; i < vid->width/2 + 25 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = 0; p[(j*vid->bufWidth+i)*3+2] = 255;
                    break;
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = 255; p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = 0; p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_BGRA:
                case AR_PIXEL_FORMAT_ARGB:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = 255; p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x1f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x1f;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x3f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x3f;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0xff;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0xff;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                    p[j*vid->bufWidth+i] = 29;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 118; p1[(j/2)*vid->bufWidth+i+1] = 255;
                    }
                    break;
                case AR_PIXEL_FORMAT_420f:
                    p[j*vid->bufWidth+i] = 29;
                    if ((j & 1) == 0 && (i & 1) == 0) {
                        p1[(j/2)*vid->bufWidth+i] = 255; p1[(j/2)*vid->bufWidth+i+1] = 118;
                    }
                    break;
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 29;
                    break;
                default:
                    break;
            }
        }
        // Black bar (25 pixels wide).
        for( i = vid->width/2 + 25 + k; i < vid->width/2 + 50 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                    // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
                    p[(j*vid->bufWidth+i)*2+0] = p[(j*vid->bufWidth+i)*2+1] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x01;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x01;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                case AR_PIXEL_FORMAT_420f:
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 0;
                    break;
                default:
                    break;
            }
        }
    }
    for( j = vid->height/2 + 25; j < vid->height/2 + 50; j++ ) {
        for( i = vid->width/2 - 50 + k; i < vid->width/2 + 50 + k; i++ ) {
            switch (vid->format) {
                case AR_PIXEL_FORMAT_RGB:
                case AR_PIXEL_FORMAT_BGR:
                    p[(j*vid->bufWidth+i)*3+0] = p[(j*vid->bufWidth+i)*3+1] = p[(j*vid->bufWidth+i)*3+2] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                case AR_PIXEL_FORMAT_BGRA:
                    p[(j*vid->bufWidth+i)*4+0] = p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = 0; p[(j*vid->bufWidth+i)*4+3] = 255;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                case AR_PIXEL_FORMAT_ABGR:
                    p[(j*vid->bufWidth+i)*4+0] = 255; p[(j*vid->bufWidth+i)*4+1] = p[(j*vid->bufWidth+i)*4+2] = p[(j*vid->bufWidth+i)*4+3] = 0;
                    break;
                    // Packed pixel formats are endianness-dependent.
                case AR_PIXEL_FORMAT_RGB_565:
                    p[(j*vid->bufWidth+i)*2+0] = p[(j*vid->bufWidth+i)*2+1] = 0;
                    break;
                case AR_PIXEL_FORMAT_RGBA_5551:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x01;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x01;
#endif
                    break;
                case AR_PIXEL_FORMAT_RGBA_4444:
#ifdef AR_LITTLE_ENDIAN
                    p[(j*vid->bufWidth+i)*2+1] = 0; p[(j*vid->bufWidth+i)*2+0] = 0x0f;
#else
                    p[(j*vid->bufWidth+i)*2+0] = 0; p[(j*vid->bufWidth+i)*2+1] = 0x0f;
#endif
                    break;
                case AR_PIXEL_FORMAT_NV21:
                case AR_PIXEL_FORMAT_420f:
                case AR_PIXEL_FORMAT_MONO:
                    p[j*vid->bufWidth+i] = 0;
                    break;
                default:
                    break;
            }
        }
    }

    vid->buffer.fillFlag  = 1;
    vid->buffer.time.sec  = 0;
    vid->buffer.time.usec = 0;

    return &(vid->buffer);
}

int ar2VideoGetSizeDummy(AR2VideoParamDummyT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatDummy( AR2VideoParamDummyT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoSetBufferSizeDummy(AR2VideoParamDummyT *vid, const int width, const int height)
{
    unsigned int i;
    int rowBytes;

    if (!vid) return (-1);

    if (vid->buffer.bufPlaneCount) {
        for (i = 0; i < vid->buffer.bufPlaneCount; i++) {
            free(vid->buffer.bufPlanes[i]);
            vid->buffer.bufPlanes[i] = NULL;
            free(vid->buffer.bufPlanes);
            vid->buffer.bufPlaneCount = 0;
            vid->buffer.buff = vid->buffer.buffLuma = NULL;
        }
    } else {
        if (vid->buffer.buff) {
            free (vid->buffer.buff);
            vid->buffer.buff = vid->buffer.buffLuma = NULL;
        }
    }

    if (width && height) {
        if (width < vid->width || height < vid->height) {
            ARLOGe("Error: Requested buffer size smaller than video size.\n");
            return (-1);
        }

        if (ar2VideoGetPixelFormatDummy(vid) == AR_PIXEL_FORMAT_420f || ar2VideoGetPixelFormatDummy(vid) == AR_PIXEL_FORMAT_NV21) {
            arMallocClear(vid->buffer.bufPlanes, ARUint8 *, 2);
            arMalloc(vid->buffer.bufPlanes[0], ARUint8, width*height);
            arMalloc(vid->buffer.bufPlanes[1], ARUint8, width*height/2);
            vid->buffer.bufPlaneCount = 2;
            vid->buffer.buff = vid->buffer.buffLuma = vid->buffer.bufPlanes[0];
        } else {
            rowBytes = width * arVideoUtilGetPixelSize(ar2VideoGetPixelFormatDummy(vid));
            arMalloc(vid->buffer.buff, ARUint8, height * rowBytes);
            if (ar2VideoGetPixelFormatDummy(vid) == AR_PIXEL_FORMAT_MONO) {
                vid->buffer.buffLuma = vid->buffer.buff;
            } else {
                vid->buffer.buffLuma = NULL;
            }
        }
    }

    vid->bufWidth = width;
    vid->bufHeight = height;

    return (0);
}

int ar2VideoGetBufferSizeDummy(AR2VideoParamDummyT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (width) *width = vid->bufWidth;
    if (height) *height = vid->bufHeight;
    return (0);
}

int ar2VideoGetIdDummy( AR2VideoParamDummyT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiDummy( AR2VideoParamDummyT *vid, int paramName, int *value )
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiDummy( AR2VideoParamDummyT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdDummy( AR2VideoParamDummyT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdDummy( AR2VideoParamDummyT *vid, int paramName, double  value )
{
    return -1;
}

int ar2VideoGetParamsDummy( AR2VideoParamDummyT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsDummy( AR2VideoParamDummyT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_DUMMY
