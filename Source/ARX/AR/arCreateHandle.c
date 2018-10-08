/*
 *  arCreateHandle.c
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
 * Revision: 5.1
 * Date: 03/08/13
 *
 *******************************************************/

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <math.h>

ARHandle *arCreateHandle(ARParamLT *paramLT)
{
    ARHandle   *handle;

    arMalloc(handle, ARHandle, 1);

    handle->arDebug                 = AR_DEBUG_DISABLE;
#if !AR_DISABLE_LABELING_DEBUG_MODE
    handle->labelInfo.bwImage       = NULL;
#endif
    handle->arImageProcInfo         = NULL;
    handle->arPixelFormat           = AR_PIXEL_FORMAT_INVALID;
    handle->arPixelSize             = 0;
    handle->arLabelingMode          = AR_DEFAULT_LABELING_MODE;
    handle->arLabelingThresh        = AR_DEFAULT_LABELING_THRESH;
    handle->arImageProcMode         = AR_DEFAULT_IMAGE_PROC_MODE;
    handle->arPatternDetectionMode  = AR_DEFAULT_PATTERN_DETECTION_MODE;
    handle->arMarkerExtractionMode  = AR_DEFAULT_MARKER_EXTRACTION_MODE;
    handle->pattRatio               = AR_PATT_RATIO;
    handle->matrixCodeType          = AR_MATRIX_CODE_TYPE_DEFAULT;
    handle->arCornerRefinementMode  = AR_DEFAULT_CORNER_REFINEMENT_MODE;
    handle->areaMax                 = AR_AREA_MAX;
    handle->areaMin                 = AR_AREA_MIN;
    handle->squareFitThresh         = AR_SQUARE_FIT_THRESH;

    handle->arParamLT           = paramLT;
    handle->xsize               = paramLT->param.xsize;
    handle->ysize               = paramLT->param.ysize;

    handle->marker_num          = 0;
    handle->marker2_num         = 0;
    handle->labelInfo.label_num = 0;
    handle->history_num         = 0;

    arMalloc(handle->labelInfo.labelImage, AR_LABELING_LABEL_TYPE, handle->xsize*handle->ysize);
    
    handle->pattHandle = NULL;
    
    arSetDebugMode(handle, AR_DEFAULT_DEBUG_MODE);
    
    handle->arLabelingThreshMode = -1;
    handle->arLabelingThreshAutoAdaptiveKernelSize = AR_LABELING_THRESH_ADAPTIVE_KERNEL_SIZE_DEFAULT;
    handle->arLabelingThreshAutoAdaptiveBias = AR_LABELING_THRESH_ADAPTIVE_BIAS_DEFAULT;
    arSetLabelingThreshMode(handle, AR_LABELING_THRESH_MODE_DEFAULT);
    arSetLabelingThreshModeAutoInterval(handle, AR_LABELING_THRESH_AUTO_INTERVAL_DEFAULT);
    
    return handle;
}

int arDeleteHandle(ARHandle *handle)
{
    if (!handle) return -1;

    if (handle->arImageProcInfo) {
        arImageProcFinal(handle->arImageProcInfo);
        handle->arImageProcInfo = NULL;
    }
    
    //if(handle->arParamLT != NULL) arParamLTFree(&handle->arParamLT);
    free(handle->labelInfo.labelImage);
#if !AR_DISABLE_LABELING_DEBUG_MODE
    if (handle->labelInfo.bwImage) free(handle->labelInfo.bwImage);
#endif
    free(handle);

    return 0;
}

void arSetDebugMode(ARHandle *handle, int mode)
{
    if (!handle) return;

    if (handle->arDebug != mode) {
        handle->arDebug = mode;
#if !AR_DISABLE_LABELING_DEBUG_MODE
        if (mode == AR_DEBUG_DISABLE) {
            free(handle->labelInfo.bwImage);
            handle->labelInfo.bwImage = NULL;
        } else {
            arMalloc(handle->labelInfo.bwImage, ARUint8, handle->xsize * handle->ysize);
        }
#endif
    }
}

int arGetDebugMode(ARHandle *handle)
{
    if (!handle) return (AR_DEBUG_DISABLE);

    return (handle->arDebug);
}

void arSetLabelingMode(ARHandle *handle, int mode)
{
    if (!handle) return;

    switch (mode) {
        case AR_LABELING_WHITE_REGION:
        case AR_LABELING_BLACK_REGION:
            break;
        default:
            return;
    }

    handle->arLabelingMode = mode;
}

int arGetLabelingMode(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_LABELING_MODE);

    return (handle->arLabelingMode);
}

void arSetLabelingThresh(ARHandle *handle, int thresh)
{
    if (!handle) return;

    if (thresh < 0 || thresh > 255) return;

    handle->arLabelingThresh = thresh;
}

int arGetLabelingThresh(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_LABELING_THRESH);

    return (handle->arLabelingThresh);
}

void arSetLabelingThreshMode(ARHandle *handle, const AR_LABELING_THRESH_MODE mode)
{
    AR_LABELING_THRESH_MODE mode1;

	if (!handle) return;
    if (handle->arLabelingThreshMode != mode) {
        if (handle->arImageProcInfo) {
            arImageProcFinal(handle->arImageProcInfo);
            handle->arImageProcInfo = NULL;
        }

        mode1 = mode;
        switch (mode) {
            case AR_LABELING_THRESH_MODE_AUTO_MEDIAN:
            case AR_LABELING_THRESH_MODE_AUTO_OTSU:
            case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE:
                handle->arImageProcInfo = arImageProcInit(handle->xsize, handle->ysize);
                break;
            case AR_LABELING_THRESH_MODE_AUTO_BRACKETING:
                handle->arLabelingThreshAutoBracketOver = handle->arLabelingThreshAutoBracketUnder = 1;
                break;
            case AR_LABELING_THRESH_MODE_MANUAL:
                break; // Do nothing.
            default:
                ARLOGe("Unknown or unsupported labeling threshold mode requested. Set to manual.\n");
                mode1 = AR_LABELING_THRESH_MODE_MANUAL;
        }
        handle->arLabelingThreshMode = mode1;
        if (handle->arDebug == AR_DEBUG_ENABLE) {
            const char *modeDescs[] = {
                "MANUAL",
                "AUTO_MEDIAN",
                "AUTO_OTSU",
                "AUTO_ADAPTIVE",
                "AUTO_BRACKETING"
            };
            ARLOGe("Labeling threshold mode set to %s.\n", modeDescs[mode1]);
        }
    }
}

AR_LABELING_THRESH_MODE arGetLabelingThreshMode(const ARHandle *handle)
{
    if (!handle) return (AR_LABELING_THRESH_MODE_DEFAULT);

    return (handle->arLabelingThreshMode);
}

void arSetLabelingThreshModeAutoInterval(ARHandle *handle, const int interval)
{
    if (!handle) return;

    handle->arLabelingThreshAutoInterval = interval;
    handle->arLabelingThreshAutoIntervalTTL = 0;
}

void arSetLabelingThreshAutoAdaptiveKernelSize(ARHandle *handle, const int labelingThreshAutoAdaptiveKernelSize)
{
    if (!handle) return;

    if (labelingThreshAutoAdaptiveKernelSize < 3 || (labelingThreshAutoAdaptiveKernelSize % 2) != 1) return;

    handle->arLabelingThreshAutoAdaptiveKernelSize = labelingThreshAutoAdaptiveKernelSize;
}

int arGetLabelingThreshAutoAdaptiveKernelSize(ARHandle *handle)
{
    if (!handle) return (AR_LABELING_THRESH_ADAPTIVE_KERNEL_SIZE_DEFAULT);
    
    return (handle->arLabelingThreshAutoAdaptiveKernelSize);
}

void arSetLabelingThreshAutoAdaptiveBias(ARHandle *handle, const int labelingThreshAutoAdaptiveBias)
{
    if (!handle) return;

    handle->arLabelingThreshAutoAdaptiveBias = labelingThreshAutoAdaptiveBias;
}

int arGetLabelingThreshAutoAdaptiveBias(ARHandle *handle)
{
    if (!handle) return (AR_LABELING_THRESH_ADAPTIVE_BIAS_DEFAULT);
    
    return (handle->arLabelingThreshAutoAdaptiveBias);
}

int arGetLabelingThreshModeAutoInterval(const ARHandle *handle)
{
    if (!handle) return (AR_LABELING_THRESH_AUTO_INTERVAL_DEFAULT);

    return (handle->arLabelingThreshAutoInterval);
}

void arSetImageProcMode(ARHandle *handle, int mode)
{
    if (!handle) return;

    switch (mode) {
        case AR_IMAGE_PROC_FRAME_IMAGE:
        case AR_IMAGE_PROC_FIELD_IMAGE:
            break;
        default:
            return;
    }

    handle->arImageProcMode = mode;
}

int arGetImageProcMode(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_IMAGE_PROC_MODE);

    return (handle->arImageProcMode);
}

void arSetPatternDetectionMode(ARHandle *handle, int mode)
{
    if (!handle) return;

    switch (mode) {
        case AR_TEMPLATE_MATCHING_COLOR:
        case AR_TEMPLATE_MATCHING_MONO:
        case AR_MATRIX_CODE_DETECTION:
        case AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX:
        case AR_TEMPLATE_MATCHING_MONO_AND_MATRIX:
            break;
        default:
            return;
    }

    handle->arPatternDetectionMode = mode;
}
 
void arSetMatrixCodeType(ARHandle *handle, const AR_MATRIX_CODE_TYPE type)
{
    if (!handle) return;

    handle->matrixCodeType = type;
}

AR_MATRIX_CODE_TYPE arGetMatrixCodeType(ARHandle *handle)
{
    if (!handle ) return (AR_MATRIX_CODE_TYPE_DEFAULT);

    return (handle->matrixCodeType);
}

int arGetPatternDetectionMode(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_PATTERN_DETECTION_MODE);

    return (handle->arPatternDetectionMode);
}

void arSetMarkerExtractionMode(ARHandle *handle, int mode)
{
    if (!handle) return;

    switch (mode) {
        case AR_USE_TRACKING_HISTORY:
        case AR_NOUSE_TRACKING_HISTORY:
        case AR_USE_TRACKING_HISTORY_V2:
            break;
        default:
            return;
    }

    handle->arMarkerExtractionMode = mode;
}

int arGetMarkerExtractionMode(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_MARKER_EXTRACTION_MODE);

    return (handle->arMarkerExtractionMode);
}

void arSetBorderSize(ARHandle *handle, const ARdouble borderSize)
{
    if (!handle) return;
    if (borderSize <= 0.0 || borderSize >= 0.5) return;
    
    handle->pattRatio = 1.0 - 2.0*borderSize;
}

ARdouble arGetBorderSize(ARHandle *handle)
{
    if (!handle) return ((1.0 - AR_PATT_RATIO)*0.5);

    return ((1.0 - handle->pattRatio)*0.5);
}

void arSetPattRatio(ARHandle *handle, const ARdouble pattRatio)
{
    if (!handle) return;
    if (pattRatio <= 0.0 || pattRatio >= 1.0) return;
    
    handle->pattRatio = pattRatio;
}

ARdouble arGetPattRatio(ARHandle *handle)
{
    if (!handle) return (AR_PATT_RATIO);

    return (handle->pattRatio);
}

void arSetPixelFormat(ARHandle *handle, AR_PIXEL_FORMAT pixFormat)
{
    int monoFormat;
    
    if (!handle) return;
    if (pixFormat == handle->arPixelFormat) return;

    switch( pixFormat ) {
        case AR_PIXEL_FORMAT_RGB:
        case AR_PIXEL_FORMAT_BGR:
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_ABGR:
        case AR_PIXEL_FORMAT_ARGB:
        case AR_PIXEL_FORMAT_2vuy:
        case AR_PIXEL_FORMAT_yuvs:
        case AR_PIXEL_FORMAT_RGB_565:
        case AR_PIXEL_FORMAT_RGBA_5551:
        case AR_PIXEL_FORMAT_RGBA_4444:
            monoFormat = FALSE;
            break;
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_NV21:
            monoFormat = TRUE;
            break;
        default:
            ARLOGe("Error: Unsupported pixel format (%d) requested.\n", pixFormat);
            return;
    }

    handle->arPixelFormat = pixFormat;
    handle->arPixelSize   = arUtilGetPixelSize(handle->arPixelFormat);
    
    // Update handle settings that depend on pixel format.
    // None.
    
    // If template matching, automatically switch to these most suitable colour template matching mode.
    if (monoFormat) {
        if (handle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR) handle->arPatternDetectionMode = AR_TEMPLATE_MATCHING_MONO;
        else if (handle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX) handle->arPatternDetectionMode = AR_TEMPLATE_MATCHING_MONO_AND_MATRIX;
    } else {
        if (handle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO) handle->arPatternDetectionMode = AR_TEMPLATE_MATCHING_MONO_AND_MATRIX;
        else if (handle->arPatternDetectionMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX) handle->arPatternDetectionMode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
    }
}

int arGetPixelFormat(ARHandle *handle)
{
    if (!handle) return (AR_PIXEL_FORMAT_INVALID);

    return (handle->arPixelFormat);
}

void arSetCornerRefinementMode(ARHandle *handle, int mode)
{
    if (!handle) return;
    
    switch (mode) {
        case AR_CORNER_REFINEMENT_DISABLE:
        case AR_CORNER_REFINEMENT_ENABLE:
            break;
        default:
            return;
    }
    
    handle->arCornerRefinementMode = mode;
}

void arSetAreaMax(ARHandle *handle, const ARdouble areaMax)
{
    if (!handle) return;
    if (areaMax <= 0.0) return;
    
    handle->areaMax = areaMax;
}

ARdouble arGetAreaMax(ARHandle *handle)
{
    if (!handle) return (AR_AREA_MAX);
    
    return (handle->areaMax);
}

void arSetAreaMin(ARHandle *handle, const ARdouble areaMin)
{
    if (!handle) return;
    if (areaMin <= 0.0) return;
    
    handle->areaMin = areaMin;
}

ARdouble arGetAreaMin(ARHandle *handle)
{
    if (!handle) return (AR_AREA_MIN);
    
    return (handle->areaMin);
}

void arSetSquareFitThresh(ARHandle *handle, const ARdouble squareFitThresh)
{
    if (!handle) return;
    if (squareFitThresh <= 0.0) return;
    
    handle->squareFitThresh = squareFitThresh;
}

ARdouble arGetSquareFitThresh(ARHandle *handle)
{
    if (!handle) return (AR_SQUARE_FIT_THRESH);
    
    return (handle->squareFitThresh);
}

int arGetCornerRefinementMode(ARHandle *handle)
{
    if (!handle) return (AR_DEFAULT_CORNER_REFINEMENT_MODE);
    
    return (handle->arCornerRefinementMode);
}

int arGetMarkerNum(ARHandle *handle)
{
    if (!handle) return -1;

    return (handle->marker_num);
}

ARMarkerInfo *arGetMarker(ARHandle *handle)
{
    if (!handle) return NULL;
    if (handle->marker_num <= 0) return NULL;

    return (handle->markerInfo);
}
