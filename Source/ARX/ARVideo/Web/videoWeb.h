/*
 *	videoWeb.h
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
 *
 *  Author(s): Thorsten Bux
 *
 */

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamWebT AR2VideoParamWebT;

#define AR_VIDEO_WEB_FOCAL_LENGTH_DEFAULT 0.3 // Metres.

int                    ar2VideoDispOptionWeb     ( void );
AR2VideoParamWebT *ar2VideoOpenAsyncWeb      ( const char *config, void (*callback)(void *), void *userdata );
int                    ar2VideoCloseWeb         ( AR2VideoParamWebT *vid );
int                    ar2VideoGetIdWeb          ( AR2VideoParamWebT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeWeb        ( AR2VideoParamWebT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatWeb ( AR2VideoParamWebT *vid );
AR2VideoBufferT       *ar2VideoGetImageWeb       ( AR2VideoParamWebT *vid );
int                    ar2VideoCapStartWeb       ( AR2VideoParamWebT *vid );
int                    ar2VideoCapStopWeb        ( AR2VideoParamWebT *vid );

int ar2VideoPushInitWeb(AR2VideoParamWebT *vid, int width, int height, const char *pixelFormat, int camera_index, int camera_face);
#ifdef  __cplusplus
}
#endif
