/*
 *	videoGStreamer.h
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
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2008-2015 ARToolworks, Inc.
 *  Copyright 2003-2008 Hartmut Seichter <http://www.technotecture.com>
 *
 *  Author(s): Hartmut Seichter, Philip Lamb
 *
 */

#ifndef AR_VIDEO_GSTREAMER_H
#define AR_VIDEO_GSTREAMER_H

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamGStreamerT AR2VideoParamGStreamerT;

int                    ar2VideoDispOptionGStreamer     ( void );
AR2VideoParamGStreamerT *ar2VideoOpenGStreamer         ( const char *config_in );
int                    ar2VideoCloseGStreamer          ( AR2VideoParamGStreamerT *vid );
int                    ar2VideoGetIdGStreamer          ( AR2VideoParamGStreamerT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeGStreamer        ( AR2VideoParamGStreamerT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatGStreamer ( AR2VideoParamGStreamerT *vid );
AR2VideoBufferT       *ar2VideoGetImageGStreamer       ( AR2VideoParamGStreamerT *vid );
int                    ar2VideoCapStartGStreamer       ( AR2VideoParamGStreamerT *vid );
int                    ar2VideoCapStopGStreamer        ( AR2VideoParamGStreamerT *vid );

int                    ar2VideoGetParamiGStreamer      ( AR2VideoParamGStreamerT *vid, int paramName, int *value );
int                    ar2VideoSetParamiGStreamer      ( AR2VideoParamGStreamerT *vid, int paramName, int value );
int                    ar2VideoGetParamdGStreamer      ( AR2VideoParamGStreamerT *vid, int paramName, double *value );
int                    ar2VideoSetParamdGStreamer      ( AR2VideoParamGStreamerT *vid, int paramName, double value );
int                    ar2VideoGetParamsGStreamer      ( AR2VideoParamGStreamerT *vid, const int paramName, char **value );
int                    ar2VideoSetParamsGStreamer      ( AR2VideoParamGStreamerT *vid, const int paramName, const char *value );


#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_GSTREAMER_H
