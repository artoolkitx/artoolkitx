/*
 *	videoBuffer.h
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.4
 * Date: 2004/01/01
 *
 *******************************************************/

#ifndef AR_VIDEO_BUFFER_H
#define AR_VIDEO_BUFFER_H


#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
    AR2VideoBufferT    buffer;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                bufWidth;
    int                bufHeight;
	BYTE*              userBufferPointer;
}AR2VideoParamBufferT;

int                    ar2VideoDispOptionBuffer     ( void );
AR2VideoParamBufferT  *ar2VideoOpenBuffer           ( const char *config );
int                    ar2VideoCloseBuffer          ( AR2VideoParamBufferT *vid );
int                    ar2VideoGetIdBuffer          ( AR2VideoParamBufferT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeBuffer        ( AR2VideoParamBufferT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatBuffer ( AR2VideoParamBufferT *vid );
AR2VideoBufferT       *ar2VideoGetImageBuffer       ( AR2VideoParamBufferT *vid );
int                    ar2VideoCapStartBuffer       ( AR2VideoParamBufferT *vid );
int                    ar2VideoCapStopBuffer        ( AR2VideoParamBufferT *vid );

int                    ar2VideoGetParamiBuffer      ( AR2VideoParamBufferT *vid, int paramName, int *value );
int                    ar2VideoSetParamiBuffer      ( AR2VideoParamBufferT *vid, int paramName, int  value );
int                    ar2VideoGetParamdBuffer      ( AR2VideoParamBufferT *vid, int paramName, double *value );
int                    ar2VideoSetParamdBuffer      ( AR2VideoParamBufferT *vid, int paramName, double  value );
int                    ar2VideoGetParamsBuffer      ( AR2VideoParamBufferT *vid, const int paramName, char **value );
int                    ar2VideoSetParamsBuffer      ( AR2VideoParamBufferT *vid, const int paramName, const char  *value );

int                    ar2VideoSetBufferSizeBuffer  (AR2VideoParamBufferT *vid, const int width, const int height);
int                    ar2VideoGetBufferSizeBuffer  (AR2VideoParamBufferT *vid, int *width, int *height);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_BUFFER_H
