/*
 *	videoWaveVR.c
 *  artoolkitX
 *
 *  Video capture module utilising the WaveVR interfaces
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
 *  Copyright 2020 Mozilla
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoWaveVR.h"

#ifdef ARVIDEO_INPUT_WAVEVR

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

struct _AR2VideoParamWaveVRT {
    AR2VideoBufferT    buffer;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                bufWidth;
    int                bufHeight;
};

int ar2VideoDispOptionWaveVR( void )
{
    ARPRINT(" -module=WaveVR\n");
    ARPRINT("\n");

    return 0;
}

AR2VideoParamWaveVRT *ar2VideoOpenWaveVR( const char *config )
{
    AR2VideoParamWaveVRT      *vid;
    const char               *a;
    #define LINE_SIZE ((unsigned int)1024)
    char                      line[LINE_SIZE];
    int ok, err_i = 0;

    arMalloc( vid, AR2VideoParamWaveVRT, 1 );
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = 0;
    vid->height = 0;
    vid->format = AR_PIXEL_FORMAT_INVALID;

    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;

            if (sscanf(a, "%s", line) == 0) break;
            if( strcmp( line, "-module=WaveVR" ) == 0 )    {
            } else {
                err_i = 1;
            }

            if (err_i) {
				ARLOGe("Error with configuration option.\n");
                ar2VideoDispOptionWaveVR();
                goto bail;
			}

            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    vid->width = 0;
    vid->height = 0;
    vid->format = AR_PIXEL_FORMAT_INVALID;

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseWaveVR( AR2VideoParamWaveVRT *vid )
{
    if (!vid) return (-1); // Sanity check.
    free( vid );

    return 0;
}

int ar2VideoCapStartWaveVR( AR2VideoParamWaveVRT *vid )
{
    return 0;
}

int ar2VideoCapStopWaveVR( AR2VideoParamWaveVRT *vid )
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWaveVR( AR2VideoParamWaveVRT *vid )
{
    if (!vid) return (NULL); // Sanity check.

    return &(vid->buffer);
}

int ar2VideoGetSizeWaveVR(AR2VideoParamWaveVRT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWaveVR( AR2VideoParamWaveVRT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoGetIdWaveVR( AR2VideoParamWaveVRT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiWaveVR( AR2VideoParamWaveVRT *vid, int paramName, int *value )
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiWaveVR( AR2VideoParamWaveVRT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdWaveVR( AR2VideoParamWaveVRT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdWaveVR( AR2VideoParamWaveVRT *vid, int paramName, double  value )
{
    return -1;
}

int ar2VideoGetParamsWaveVR( AR2VideoParamWaveVRT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsWaveVR( AR2VideoParamWaveVRT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetCParamWaveVR(AR2VideoParamWaveVRT *vid, ARParam *cparam)
{
    return (0);
}

#endif //  ARVIDEO_INPUT_WAVEVR
