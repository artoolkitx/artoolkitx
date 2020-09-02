/*
 *  videoEmscripten.c
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
 *  Copyright 2020-2020 Philip Lamb
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoEmscripten.h"

#ifdef ARVIDEO_INPUT_EMSCRIPTEN

struct _AR2VideoParamEmscriptenT {
    AR2VideoBufferT    buffer;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                bufWidth;
    int                bufHeight;
};

int ar2VideoDispOptionEmscripten(void)
{
    ARPRINT(" -module=Emscripten\n");
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
    ARPRINT("\n");

    return 0;
}

AR2VideoParamEmscriptenT *ar2VideoOpenEmscripten(const char *config)
{
    AR2VideoParamEmscriptenT *vid;
    const char               *a;
    #define LINE_SIZE ((unsigned int)1024)
    char                      line[LINE_SIZE];
    int w, h;
    int err_i = 0;

    arMalloc(vid, AR2VideoParamEmscriptenT, 1);
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = 0;
    vid->height = 0;
    vid->format = AR_PIXEL_FORMAT_INVALID;

    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;
            if (strncmp(line, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->width) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(line, "-height=", 8) == 0) {
                if (sscanf(&line[8], "%d", &vid->height) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(line, "-format=", 8) == 0) {
                if (strcmp(line+8, "RGB") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB;
                } else if (strcmp(line+8, "RGBA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA;
                } else if (strcmp(line+8, "MONO") == 0) {
                    vid->format = AR_PIXEL_FORMAT_MONO;
                } else {
                    ARLOGe("Request for unsupported pixel format conversion to '%s'. Ignoring.\n", line+8);
                }
            } else if (strcmp(line, "-module=Emscripten") == 0)    {
            } else {
                err_i = 1;
            }

            if (err_i) {
				ARLOGe("Error with configuration option.\n");
                ar2VideoDispOptionEmscripten();
                goto bail;
			}

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    ARLOGi("Emscripten video size %dx%d@%dBpp.\n", vid->width, vid->height, arVideoUtilGetPixelSize(vid->format));

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseEmscripten(AR2VideoParamEmscriptenT *vid)
{
    if (!vid) return (-1); // Sanity check.

    ar2VideoSetBufferSizeEmscripten(vid, 0, 0);
    free(vid);

    return 0;
}

int ar2VideoCapStartEmscripten(AR2VideoParamEmscriptenT *vid)
{
    return 0;
}

int ar2VideoCapStopEmscripten(AR2VideoParamEmscriptenT *vid)
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageEmscripten(AR2VideoParamEmscriptenT *vid)
{
    if (!vid) return (NULL); // Sanity check.

    vid->buffer.fillFlag  = 1;
    vid->buffer.time.sec  = 0;
    vid->buffer.time.usec = 0;

    return &(vid->buffer);
}

int ar2VideoGetSizeEmscripten(AR2VideoParamEmscriptenT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatEmscripten(AR2VideoParamEmscriptenT *vid)
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoSetBufferSizeEmscripten(AR2VideoParamEmscriptenT *vid, const int width, const int height)
{
    if (!vid) return (-1);
    vid->bufWidth = width;
    vid->bufHeight = height;
    return (0);
}

int ar2VideoGetBufferSizeEmscripten(AR2VideoParamEmscriptenT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (width) *width = vid->bufWidth;
    if (height) *height = vid->bufHeight;
    return (0);
}

int ar2VideoGetIdEmscripten(AR2VideoParamEmscriptenT *vid, ARUint32 *id0, ARUint32 *id1)
{
    return -1;
}

int ar2VideoGetParamiEmscripten(AR2VideoParamEmscriptenT *vid, int paramName, int *value)
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiEmscripten(AR2VideoParamEmscriptenT *vid, int paramName, int  value)
{
    return -1;
}

int ar2VideoGetParamdEmscripten(AR2VideoParamEmscriptenT *vid, int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdEmscripten(AR2VideoParamEmscriptenT *vid, int paramName, double  value)
{
    return -1;
}

int ar2VideoGetParamsEmscripten(AR2VideoParamEmscriptenT *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsEmscripten(AR2VideoParamEmscriptenT *vid, const int paramName, const char  *value)
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_EMSCRIPTEN
