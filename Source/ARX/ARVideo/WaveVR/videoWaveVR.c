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

#include <wvr/wvr_camera.h>

struct _AR2VideoParamWaveVRT {
    AR2VideoBufferT    buffer0;
    AR2VideoBufferT    buffer1;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    uint32_t           bufSize;
    int                stereo;
    int                stereoNextEye;
    ARParam            cparam0;
    ARParam            cparam1;
};

int ar2VideoDispOptionWaveVR(void)
{
    ARPRINT(" -module=WaveVR\n");
    ARPRINT("\n");

    return 0;
}

static void getCPara(int xsize, int ysize, WVR_CameraIntrinsic_t *ci, ARParam *param) 
{
    param->xsize = xsize;
    param->ysize = ysize;
	param->dist_function_version = 5;

    param->mat[0][0] =   ci->focalLength.v[0];
    param->mat[0][1] =   0.0;
    param->mat[0][2] =   ci->principalPoint.v[0];
    param->mat[0][3] =   0.0;
    param->mat[1][0] =   0.0;
    param->mat[1][1] =   ci->focalLength.v[1];
    param->mat[1][2] =   ci->principalPoint.v[1];
    param->mat[1][3] =   0.0;
    param->mat[2][0] =   0.0;
    param->mat[2][1] =   0.0;
    param->mat[2][2] =   1.0;
    param->mat[2][3] =   0.0;

    param->dist_factor[0] = 0.0;           /*  k1  */
    param->dist_factor[1] = 0.0;           /*  k2  */
    param->dist_factor[2] = 0.0;           /*  p1  */
    param->dist_factor[3] = 0.0;           /*  p2  */
    param->dist_factor[4] = 0.0;           /*  k3  */
    param->dist_factor[5] = 0.0;           /*  k4  */
    param->dist_factor[6] = 0.0;           /*  k5  */
    param->dist_factor[7] = 0.0;           /*  k6  */
    param->dist_factor[8] = 0.0;           /*  s1  */
    param->dist_factor[9] = 0.0;           /*  s2  */
    param->dist_factor[10] = 0.0;          /*  s3  */
    param->dist_factor[11] = 0.0;          /*  s4  */
    param->dist_factor[12] = ci->focalLength.v[0];     /*  fx  */
    param->dist_factor[13] = ci->focalLength.v[1];     /*  fy  */
    param->dist_factor[14] = ci->principalPoint.v[0];  /*  cx  */
    param->dist_factor[15] = ci->principalPoint.v[1];  /*  cy  */
    param->dist_factor[16] = 1.0;          /*  Size adjust */
}


AR2VideoParamWaveVRT *ar2VideoOpenWaveVR(const char *config)
{
    AR2VideoParamWaveVRT      *vid;
    const char               *a;
    #define LINE_SIZE ((unsigned int)1024)
    char                      line[LINE_SIZE];
    int ok, err_i = 0;

    arMallocClear(vid, AR2VideoParamWaveVRT, 1);
    vid->format = AR_PIXEL_FORMAT_INVALID;

    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;
            if (strcmp(line, "-module=WaveVR") == 0)    {
            } else {
                err_i = 1;
            }

            if (err_i) {
				ARLOGe("Error with configuration option.\n");
                ar2VideoDispOptionWaveVR();
                goto bail;
			}

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    WVR_CameraInfo_t cameraInfo;
    if (!WVR_StartCamera(&cameraInfo)) {
        ARLOGe("Failed to start WVR camera stream.\n");
        goto bail;
    }
    ARLOGi("WVR_CameraInfo: imgType:WVR_CameraImageType_%s imgFormat:WVR_CameraImageFormat_%s width:%d, height:%d size:%d",
        (cameraInfo.imgType == WVR_CameraImageType_Invalid ? "Invalid" : (cameraInfo.imgType == WVR_CameraImageType_SingleEye ? "SingleEye" : (cameraInfo.imgType == WVR_CameraImageType_DualEye ? "DualEye" : "Unknown"))),
        (cameraInfo.imgFormat == WVR_CameraImageFormat_Invalid ? "Invalid" : (cameraInfo.imgFormat == WVR_CameraImageFormat_Grayscale ? "Grayscale" : (cameraInfo.imgFormat == WVR_CameraImageFormat_YUV_420 ? "YUV_420" : "Unknown"))),
        cameraInfo.width, cameraInfo.height, cameraInfo.size);

    vid->height = cameraInfo.height;
    if (cameraInfo.imgType == WVR_CameraImageType_SingleEye) {
        vid->stereo = 0;
        vid->width = cameraInfo.width;
    } else if (cameraInfo.imgType == WVR_CameraImageType_DualEye) {
        vid->stereo = 1;
        vid->width = cameraInfo.width / 2;
    } else {
        ARLOGe("Unsupported WVR_CameraImageType.\n");
        goto bail1;
    }

    if (cameraInfo.imgFormat == WVR_CameraImageFormat_Grayscale) {
        vid->format = AR_PIXEL_FORMAT_MONO;
    } else if (cameraInfo.imgFormat == WVR_CameraImageFormat_YUV_420) {
        // For now, just use the luma plane only. Sometime in the future, could support AR_PIXEL_FORMAT_NV21.
        vid->format = AR_PIXEL_FORMAT_MONO;
    } else {
        ARLOGe("Unsupported WVR_CameraImageFormat.\n");
        goto bail1;
    }
    
    // Get the camera parameters.
    WVR_CameraIntrinsic_t ci;
    if (!WVR_GetCameraIntrinsic(WVR_CameraPosition_Left, &ci)) {
        ARLOGe("Error retrieving WVR camera intrinsics.\n");
        goto bail1;
    }
    getCPara(vid->width, vid->height, &ci, &vid->cparam0);
    if (vid->stereo) {
        if (!WVR_GetCameraIntrinsic(WVR_CameraPosition_Right, &ci)) {
            ARLOGe("Error retrieving WVR camera intrinsics.\n");
            goto bail1;
        }
        getCPara(vid->width, vid->height, &ci, &vid->cparam1);
    }

    vid->bufSize = cameraInfo.size;
    // If stereo, padd the buffer with one full extra row, so that we can treat
    // the right-hand end of each row as if it was the start of a row, and treat
    // both left and right buffers as if they have a chunk of padding. With this,
    // reading from the last row won't read past the end of the buffer and cause
    // an access violation.
    int bufSizeIncludingAnyPadding = cameraInfo.size;
    if (vid->stereo) bufSizeIncludingAnyPadding += cameraInfo.size / cameraInfo.height; 
    vid->buffer0.buff = calloc(1, bufSizeIncludingAnyPadding);
    vid->buffer0.buffLuma = vid->buffer0.buff;
    vid->buffer0.bufPlanes = NULL;
    vid->buffer0.bufPlaneCount = 0;
    vid->buffer0.fillFlag = 0;
    if (vid->stereo) {
        vid->buffer1.buff = vid->buffer0.buff + vid->width;
        vid->buffer1.buffLuma = vid->buffer1.buff;
        vid->buffer1.bufPlanes = NULL;
        vid->buffer1.bufPlaneCount = 0;
        vid->buffer1.fillFlag = 0;
    }

    return vid;

bail1:
    WVR_StopCamera();
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseWaveVR(AR2VideoParamWaveVRT *vid)
{
    if (!vid) return (-1); // Sanity check.
    
    WVR_StopCamera();
    
    free(vid);

    return 0;
}

int ar2VideoCapStartWaveVR(AR2VideoParamWaveVRT *vid)
{
    return 0;
}

int ar2VideoCapStopWaveVR(AR2VideoParamWaveVRT *vid)
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWaveVR(AR2VideoParamWaveVRT *vid)
{
    if (!vid) return (NULL); // Sanity check.

    if (!WVR_GetCameraFrameBuffer(vid->buffer0.buff, vid->bufSize)) {
        ARLOGe("Error in WVR_GetCameraFrameBuffer.\n");
        return (NULL);
    }
    if (!vid->stereo || !vid->stereoNextEye) {
        vid->buffer0.fillFlag = 1;
        return &(vid->buffer0);
    } else {
        vid->buffer1.fillFlag = 1;
        return &(vid->buffer1);
    }
}

int ar2VideoGetSizeWaveVR(AR2VideoParamWaveVRT *vid, int *x, int *y)
{
    if (!vid) return (-1); // Sanity check.

    if (x) *x = vid->width;
    if (y) *y = vid->height;

    return 0;
}

int ar2VideoGetBufferSizeWaveVR(AR2VideoParamWaveVRT *vid, int *x, int *y)
{
    if (!vid) return (-1); // Sanity check.

    if (x) *x = (vid->stereo ? vid->width * 2 : vid->width);
    if (y) *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWaveVR(AR2VideoParamWaveVRT *vid)
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoGetIdWaveVR(AR2VideoParamWaveVRT *vid, ARUint32 *id0, ARUint32 *id1)
{
    return -1;
}

int ar2VideoGetParamiWaveVR(AR2VideoParamWaveVRT *vid, int paramName, int *value)
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    } else if (paramName == AR_VIDEO_PARAM_DEVICE_FLAGS) {
        int val = AR_VIDEO_POSITION_BACK;
        if (vid->stereo) {
            val |= AR_VIDEO_STEREO_MODE_SIDE_BY_SIDE;
        }
        *value = val;
        return 0;
    } else if (paramName == AR_VIDEO_PARAM_STEREO_NEXTEYE) {
        *value = vid->stereoNextEye;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiWaveVR(AR2VideoParamWaveVRT *vid, int paramName, int value)
{
    if (paramName == AR_VIDEO_PARAM_STEREO_NEXTEYE) {
        vid->stereoNextEye = value;
        return 0;
    }
    return -1;
}

int ar2VideoGetParamdWaveVR(AR2VideoParamWaveVRT *vid, int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdWaveVR(AR2VideoParamWaveVRT *vid, int paramName, double  value)
{
    return -1;
}

int ar2VideoGetParamsWaveVR(AR2VideoParamWaveVRT *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsWaveVR(AR2VideoParamWaveVRT *vid, const int paramName, const char *value)
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
    if (!vid) return (-1);

    if (!vid->stereo || !vid->stereoNextEye) {
        *cparam = vid->cparam0;
    } else {
        *cparam = vid->cparam1;
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_WAVEVR
