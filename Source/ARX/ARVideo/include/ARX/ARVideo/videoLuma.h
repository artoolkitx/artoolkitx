/*
 *  videoLuma.h
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
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef AR_VIDEO_LUMA_H
#define AR_VIDEO_LUMA_H

#include <ARX/ARVideo/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _ARVideoLumaInfo ARVideoLumaInfo;

ARVIDEO_EXTERN ARVideoLumaInfo *arVideoLumaInit(int xsize, int ysize, AR_PIXEL_FORMAT pixFormat);

ARVIDEO_EXTERN ARUint8 *__restrict arVideoLuma(ARVideoLumaInfo *vli, const ARUint8 *__restrict dataPtr);

ARVIDEO_EXTERN int arVideoLumaFinal(ARVideoLumaInfo **vli_p);

#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_LUMA_H
