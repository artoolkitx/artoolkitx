/*
 *  arLabelingPrivate.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR_LABELING_PRIVATE_H
#define AR_LABELING_PRIVATE_H

#include <ARX/AR/config.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	Function naming convention:
	(E|D) - DEBUG_ENABLE|!DEBUG_ENABLE
	(W|B) - WHITE_REGION|!WHITE_REGION
    (Z| ) - ADAPTIVE|!ADAPTIVE
    (R|I) - FRAME_IMAGE|!FRAME_IMAGE
 */

int arLabelingSubDBIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubDBRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubDWIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubDWRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
#if !AR_DISABLE_LABELING_DEBUG_MODE
int arLabelingSubEBIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubEBRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubEWIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
int arLabelingSubEWRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo );
#endif

/*  Adaptive */

int arLabelingSubDBZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo );
int arLabelingSubDWZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo );
int arLabelingSubEBZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo );
int arLabelingSubEWZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo );

#ifdef __cplusplus
}
#endif
#endif
