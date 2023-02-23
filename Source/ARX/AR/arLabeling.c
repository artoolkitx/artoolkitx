/*
 *  arLabeling.c
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
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.0
 * Date: 03/08/13
 *
 *******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/config.h>
#include "arLabelingSub/arLabelingPrivate.h"

int arLabeling( ARUint8 *imageLuma, int xsize, int ysize,
                int debugMode, int labelingMode, int labelingThresh, int imageProcMode,
                ARLabelInfo *labelInfo, ARUint8 *image_thresh )
{
#if !AR_DISABLE_LABELING_DEBUG_MODE
    if (debugMode == AR_DEBUG_DISABLE) {
#endif
        if (labelingMode == AR_LABELING_BLACK_REGION) {
            if (image_thresh) return arLabelingSubDBZ(imageLuma, xsize, ysize, image_thresh, labelInfo);
            if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) {
                return arLabelingSubDBRC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            } else /* imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE */ {
                return arLabelingSubDBIC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            }
        } else /* labelingMode == AR_LABELING_WHITE_REGION */ {
            if (image_thresh) return arLabelingSubDWZ(imageLuma, xsize, ysize, image_thresh, labelInfo);
            if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) {
                return arLabelingSubDWRC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            } else /* imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE */ {
                return arLabelingSubDWIC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            }
        }
#if !AR_DISABLE_LABELING_DEBUG_MODE
    } else /* debugMode == AR_DEBUG_ENABLE */ {
        if (labelingMode == AR_LABELING_BLACK_REGION) {
            if (image_thresh) return arLabelingSubEBZ(imageLuma, xsize, ysize, image_thresh, labelInfo);
            if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) {
                return arLabelingSubEBRC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            } else /* imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE */ {
                return arLabelingSubEBIC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            }
        } else /* labelingMode == AR_LABELING_WHITE_REGION */ {
            if (image_thresh) return arLabelingSubEWZ(imageLuma, xsize, ysize, image_thresh, labelInfo);
            if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) {
                return arLabelingSubEWRC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            } else /* imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE */ {
                return arLabelingSubEWIC(imageLuma, xsize, ysize, labelingThresh, labelInfo);
            }
        }
    }
#endif
}
