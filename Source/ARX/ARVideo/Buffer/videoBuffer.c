/*
 *  videoBuffer.c
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
 *   Buffer video capture subrutine
 *   author: Christian Thurow
 *
 *   Revision: 1.0   Date: 2018/05/30
 */

#include "videoBuffer.h"

#ifdef ARVIDEO_INPUT_BUFFER

#include <string.h> // memset()

#define AR_VIDEO_BUFFER_XSIZE   640
#define AR_VIDEO_BUFFER_YSIZE   480
#define ARVIDEO_INPUT_BUFFER_DEFAULT_PIXEL_FORMAT   AR_PIXEL_FORMAT_RGB

int ar2VideoDispOptionBuffer( void )
{
	ARPRINT(" -module=Buffer\n");
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

AR2VideoParamBufferT *ar2VideoOpenBuffer( const char *config )
{
    AR2VideoParamBufferT *vid;
    const char *a;
#   define B_SIZE ((unsigned int)256)
    char b[B_SIZE];
    int bufSizeX;
    int bufSizeY;
    char bufferpow2 = 0;

    arMalloc(vid, AR2VideoParamBufferT, 1);
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = AR_VIDEO_BUFFER_XSIZE;
    vid->height = AR_VIDEO_BUFFER_YSIZE;
    vid->format = ARVIDEO_INPUT_BUFFER_DEFAULT_PIXEL_FORMAT;

    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-width=", 7) == 0) {
                if (sscanf(&b[7], "%d", &vid->width) == 0) {
                    ar2VideoDispOptionBuffer();
                    goto bail;
                }
            }
            else if (strncmp(b, "-height=", 8) == 0) {
                if (sscanf(&b[8], "%d", &vid->height) == 0 ) {
                    ar2VideoDispOptionBuffer();
                    goto bail;
                }
            }
            else if (strncmp(b, "-format=", 8 ) == 0) {
                if (strcmp(b+8, "RGBA_5551") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_5551;
                    ARLOGi("Buffer video will return red-green-blue bars in RGBA_5551.\n");
                } else if (strcmp(b+8, "RGBA_4444") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_4444;
                    ARLOGi("Buffer video will return red-green-blue bars in RGBA_4444.\n");
                } else if (strcmp(b+8, "RGBA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA;
                    ARLOGi("Buffer video will return red-green-blue bars in RGBA.\n");
                } else if (strcmp(b+8, "RGB_565") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB_565;
                    ARLOGi("Buffer video will return red-green-blue bars in RGB_565.\n");
                } else if (strcmp(b+8, "RGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB;
                    ARLOGi("Buffer video will return red-green-blue bars in RGB.\n");
                } else if (strcmp(b+8, "ARGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_ARGB;
                    ARLOGi("Buffer video will return red-green-blue bars in ARGB.\n");
                } else if (strcmp(b+8, "ABGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_ABGR;
                    ARLOGi("Buffer video will return red-green-blue bars in ABGR.\n");
                } else if (strcmp(b+8, "BGRA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGRA;
                    ARLOGi("Buffer video will return red-green-blue bars in BGRA.\n");
                } else if (strcmp(b+8, "BGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGR;
                    ARLOGi("Buffer video will return red-green-blue bars in BGR.\n");
                } else if (strcmp(b+8, "420f") == 0) {
                    vid->format = AR_PIXEL_FORMAT_420f;
                    ARLOGi("Buffer video will return red-green-blue bars in 420f.\n");
                } else if (strcmp(b+8, "NV21") == 0) {
                    vid->format = AR_PIXEL_FORMAT_NV21;
                    ARLOGi("Buffer video will return red-green-blue bars in NV21.\n");
                }
            }
            else if (strcmp(b, "-bufferpow2") == 0)    {
                bufferpow2 = 1;
            }
            else if (strcmp(b, "-module=Buffer") == 0)    {
            }
            else {
                ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                ar2VideoDispOptionBuffer();
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
    if (!bufSizeX || !bufSizeY || ar2VideoSetBufferSizeBuffer(vid, bufSizeX, bufSizeY) != 0) {
        goto bail;
    }

    ARLOGi("Buffer video size %dx%d@%dBpp.\n", vid->width, vid->height, arVideoUtilGetPixelSize(vid->format));

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseBuffer( AR2VideoParamBufferT *vid )
{
    if (!vid) return (-1); // Sanity check.

    ar2VideoSetBufferSizeBuffer(vid, 0, 0);
    free( vid );

    return 0;
}

int ar2VideoCapStartBuffer( AR2VideoParamBufferT *vid )
{
    return 0;
}

int ar2VideoCapStopBuffer( AR2VideoParamBufferT *vid )
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageBuffer( AR2VideoParamBufferT *vid )
{
    if (!vid) return (NULL); // Sanity check.

	vid->buffer.buff = vid->userBufferPointer;

    vid->buffer.fillFlag  = 1;
    vid->buffer.time.sec  = 0;
    vid->buffer.time.usec = 0;
	arUtilTimeSinceEpoch(&vid->buffer.time.sec, &vid->buffer.time.usec);

    vid->buffer.bufPlanes = NULL; //TOOD, change depending on pix format
    vid->buffer.bufPlaneCount = 0; //TOOD, change depending on pix format
    vid->buffer.buffLuma = NULL; //TOOD, change depending on pix format

    // Do a conversion to luma
    if (!vid->buffer.buffLuma) {
        AR_PIXEL_FORMAT pixFormat;
        pixFormat = ar2VideoGetPixelFormat(vid);
        if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
            ARLOGe("ar2VideoGetBuffer unable to get pixel format.\n");
            return (NULL);
        }

		AR2VideoParamT* param = NULL;

        if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21) {
            vid->buffer.buffLuma = vid->buffer.buff;
        } else {
            if (!param->lumaInfo) {
                int xsize, ysize;
                if (ar2VideoGetSize(vid, &xsize, &ysize) < 0) {
                    ARLOGe("ar2VideoGetBuffer unable to get size.\n");
                    return (NULL);
                }
                param->lumaInfo = arVideoLumaInit(xsize, ysize, pixFormat);
                if (!param->lumaInfo) {
                    ARLOGe("ar2VideoGetBuffer unable to initialise luma conversion.\n");
                    return (NULL);
                }
            }
            vid->buffer.buffLuma = arVideoLuma(param->lumaInfo,  vid->buffer.buff);
        }
    }

    return &(vid->buffer);
}

int ar2VideoGetSizeBuffer(AR2VideoParamBufferT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatBuffer( AR2VideoParamBufferT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoSetBufferSizeBuffer(AR2VideoParamBufferT *vid, const int width, const int height)
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

        if (ar2VideoGetPixelFormatBuffer(vid) == AR_PIXEL_FORMAT_420f || ar2VideoGetPixelFormatBuffer(vid) == AR_PIXEL_FORMAT_NV21) {
            arMallocClear(vid->buffer.bufPlanes, ARUint8 *, 2);
            arMalloc(vid->buffer.bufPlanes[0], ARUint8, width*height);
            arMalloc(vid->buffer.bufPlanes[1], ARUint8, width*height/2);
            vid->buffer.bufPlaneCount = 2;
            vid->buffer.buff = vid->buffer.buffLuma = vid->buffer.bufPlanes[0];
        } else {
            rowBytes = width * arVideoUtilGetPixelSize(ar2VideoGetPixelFormatBuffer(vid));
            arMalloc(vid->buffer.buff, ARUint8, height * rowBytes);
            if (ar2VideoGetPixelFormatBuffer(vid) == AR_PIXEL_FORMAT_MONO) {
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

int ar2VideoGetBufferSizeBuffer(AR2VideoParamBufferT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (width) *width = vid->bufWidth;
    if (height) *height = vid->bufHeight;
    return (0);
}

int ar2VideoGetIdBuffer( AR2VideoParamBufferT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiBuffer( AR2VideoParamBufferT *vid, int paramName, int *value )
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiBuffer( AR2VideoParamBufferT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdBuffer( AR2VideoParamBufferT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdBuffer( AR2VideoParamBufferT *vid, int paramName, double  value )
{
    return -1;
}

int ar2VideoGetParamsBuffer( AR2VideoParamBufferT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsBuffer( AR2VideoParamBufferT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_BUFFER
