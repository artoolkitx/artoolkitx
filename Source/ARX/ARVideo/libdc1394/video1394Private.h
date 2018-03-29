/*
 *  video1394Private.h
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

#ifndef AR_VIDEO_1394_PRIVATE_H
#define AR_VIDEO_1394_PRIVATE_H

#include "video1394.h"

#include <dc1394/camera.h>
#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
    AR2VideoBufferT     in;
    AR2VideoBufferT     wait;
    AR2VideoBufferT     out;
    pthread_mutex_t     mutex;
} AR2VideoBuffer1394T;

struct _AR2VideoParam1394T {
    int                    mode;
    int                    rate;
    int                    speed;
    int                    format7;
    int                    debug;

    uint32_t               guid[2];
    int                    width;
    int                    height;
    int                    bits;
    int                    dma_buf_num;
    int                    internal_id;
    int                    status;
    dc1394camera_t        *camera;
    dc1394video_mode_t     int_mode;
    dc1394video_modes_t    modes;
    dc1394framerate_t      int_rate;
    dc1394framerates_t     rates;
    dc1394speed_t          int_speed;
    dc1394operation_mode_t int_opmode;
    dc1394featureset_t     features;
    pthread_t              capture;
    AR2VideoBuffer1394T    buffer;
};

void ar2Video1394FormatConversion(ARUint8 *src, ARUint8 *dst, int mode, int width, int height);

int ar2VideoGetValue1394         (AR2VideoParam1394T *vid, int paramName, int *value);
int ar2VideoSetValue1394         (AR2VideoParam1394T *vid, int paramName, int  value);
int ar2VideoGetAutoOn1394        (AR2VideoParam1394T *vid, int paramName, int *value);
int ar2VideoSetAutoOn1394        (AR2VideoParam1394T *vid, int paramName, int  value);
int ar2VideoGetFeatureOn1394     (AR2VideoParam1394T *vid, int paramName, int *value);
int ar2VideoSetFeatureOn1394     (AR2VideoParam1394T *vid, int paramName, int  value);
int ar2VideoGetMaxValue1394      (AR2VideoParam1394T *vid, int paramName, int *value);
int ar2VideoGetMinValue1394      (AR2VideoParam1394T *vid, int paramName, int *value);

int ar2VideoGetAbsValue1394      (AR2VideoParam1394T *vid, int paramName, ARdouble *value);
int ar2VideoSetAbsValue1394      (AR2VideoParam1394T *vid, int paramName, ARdouble  value);
int ar2VideoGetAbsMaxValue1394   (AR2VideoParam1394T *vid, int paramName, ARdouble *value);
int ar2VideoGetAbsMinValue1394   (AR2VideoParam1394T *vid, int paramName, ARdouble *value);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_1394_PRIVATE_H
