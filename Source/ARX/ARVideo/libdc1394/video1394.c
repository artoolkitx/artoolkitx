/*
 *  video1394.c
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
 *  Author(s): Kiyoshi Kiyokawa, Hirokazu Kato, Philip Lamb
 *
 */
/* 
 *   Video capture subrutine for Linux/libdc1394 devices
 *   author: Hirokazu Kato (kato@sys.im.hiroshima-cu.ac.jp)
 *
 *   Revision: 3.0   Date: 2004/01/01
 */

#include "video1394.h"

#ifdef ARVIDEO_INPUT_LIBDC1394
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#ifndef __APPLE__
#  include <linux/types.h>
#endif
#include <dc1394/dc1394.h>
//#include <dc1394/utils.h>
#include <dc1394/control.h>
#include <pthread.h>

#include <ARX/ARUtil/time.h>

#include "video1394Private.h"


#define   AR2VIDEO_1394_STATUS_IDLE    0
#define   AR2VIDEO_1394_STATUS_RUN     1
#define   AR2VIDEO_1394_STATUS_STOP    2

typedef struct __arVideo1394 {
    dc1394_t               *dc1394;
    dc1394camera_list_t    *list;
    int                    *activeFlag;
} ARVideo1394;

static ARVideo1394   arV1394;
static int           initFlag = 0;

static int  ar2VideoInit1394(int debug);
static int  ar2VideGetCameraGUID1394(int internal_id, uint32_t euid[2]);
static void ar2VideoBufferInit1394(AR2VideoBuffer1394T *buffer, int size);
static void ar2VideoCapture1394(AR2VideoParam1394T *vid);

int ar2VideoDispOption1394(void)
{
    ARPRINT(" -module=1394\n");
    ARPRINT("\n");
    //ARPRINT(" -port=N\n");
    //ARPRINT("    specifies a FireWire adaptor port (-1: Any).\n");
    ARPRINT(" -guid=N\n");
    ARPRINT("    specifies EUID of a FireWire camera (-1: Any).\n");
    ARPRINT(" -mode=[320x240_YUV422|640x480_YUV422|\n");
    ARPRINT("        640x480_YUV411|640x480_YUV411_HALF|\n");
    ARPRINT("        640x480_RGB|640x480_MONO_COLOR|640x480_MONO_COLOR2|640x480_MONO_COLOR3|640x480_MONO|\n");
    ARPRINT("        640x480_MONO_COLOR_HALF|640x480_MONO_COLOR_HALF2|640x480_MONO_COLOR_HALF3|\n");
    ARPRINT("        800x600_RGB|800x600_MONO_COLOR|800x600_MONO_COLOR2|800x600_MONO_COLOR3|800x600_MONO|\n");
    ARPRINT("        800x600_MONO_COLOR_HALF|800x600_MONO_COLOR_HALF2|800x600_MONO_COLOR_HALF3|\n");
    ARPRINT("        1024x768_RGB|1024x768_MONO_COLOR|1024x768_MONO_COLOR2|1024x768_MONO_COLOR3|1024x768_MONO|\n");
    ARPRINT("        1280x720_RGB|1280x720_MONO_COLOR|1280x720_MONO_COLOR2|1280x720_MONO_COLOR3|1280x720_MONO|\n");
    ARPRINT("        1280x1024_RGB|1280x1024_MONO_COLOR|1280x1024_MONO_COLOR2|1280x1024_MONO_COLOR3|1280x1024_MONO|\n");
    ARPRINT("        1600x900_RGB|1600x900_MONO_COLOR|1600x900_MONO_COLOR2|1600x900_MONO_COLOR3|1600x900_MONO|\n");
    ARPRINT("        1600x1200_RGB|1600x1200_MONO_COLOR|1600x1200_MONO_COLOR2|1600x1200_MONO_COLOR3|1600x1200_MONO]\n");
    ARPRINT("    specifies input image format.\n");
    ARPRINT(" -rate=N\n");
    ARPRINT("    specifies desired input framerate. \n");
    ARPRINT("    (1.875, 3.75, 7.5, 15, 30, 60, 120)\n");
    ARPRINT(" -speed=[400|800]\n");
    ARPRINT("    specifies interface speed.\n");
    ARPRINT(" -format7\n");
    ARPRINT(" -resetCamera\n");
    ARPRINT(" -resetBus\n");
    ARPRINT(" -listCamera\n");
    ARPRINT("\n");
    
    return 0;
}

AR2VideoParam1394T *ar2VideoOpen1394(const char *config)
{
    AR2VideoParam1394T       *vid;
    dc1394format7mode_t       f7_mode;
    dc1394color_coding_t      color_coding;
    uint32_t                  guid[2];
    int                       resetCameraFlag;
    int                       resetBusFlag;
    int                       listCameraFlag;
    const char               *a;
    char                      b[256];
    int                       err;
    int                       j;
    
    arMalloc(vid, AR2VideoParam1394T, 1);
    vid->guid[0]      = 0;
    vid->guid[1]      = 0;
    vid->mode         = AR_VIDEO_1394_DEFAULT_MODE;
    vid->rate         = AR_VIDEO_1394_DEFAULT_FRAME_RATE;
    vid->speed        = AR_VIDEO_1394_DEFAULT_SPEED;
    vid->dma_buf_num  = 3;
    vid->format7      = 0;
    vid->debug        = 0;
    resetCameraFlag   = 0;
    resetBusFlag      = 0;
    listCameraFlag    = 0;
    
    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;
            
            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-mode=", 6) == 0) {
                if (strcmp(&b[6], "320x240_YUV422") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_320x240_YUV422;
                } else if (strcmp(&b[6], "640x480_YUV422") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV422;
                } else if (strcmp(&b[6], "640x480_YUV411_HALF") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV411_HALF;
                } else if (strcmp(&b[6], "640x480_YUV411") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV411;
                } else if (strcmp(&b[6], "640x480_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_RGB;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR2;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR3;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR_HALF") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR_HALF2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2;
                } else if (strcmp(&b[6], "640x480_MONO_COLOR_HALF3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3;
                } else if (strcmp(&b[6], "640x480_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO;
                } else if (strcmp(&b[6], "800x600_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_RGB;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR2;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR3;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR_HALF") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR_HALF2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF2;
                } else if (strcmp(&b[6], "800x600_MONO_COLOR_HALF3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF3;
                } else if (strcmp(&b[6], "800x600_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_800x600_MONO;
                } else if (strcmp(&b[6], "1024x768_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_RGB;
                } else if (strcmp(&b[6], "1024x768_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO;
                } else if (strcmp(&b[6], "1024x768_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR;
                } else if (strcmp(&b[6], "1024x768_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2;
                } else if (strcmp(&b[6], "1024x768_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3;
                } else if (strcmp(&b[6], "1280x720_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x720_RGB;
                } else if (strcmp(&b[6], "1280x720_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x720_MONO;
                } else if (strcmp(&b[6], "1280x720_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x720_MONO_COLOR;
                } else if (strcmp(&b[6], "1280x720_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2;
                } else if (strcmp(&b[6], "1280x720_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3;
                } else if (strcmp(&b[6], "1280x960_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x960_RGB;
                } else if (strcmp(&b[6], "1280x960_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x960_MONO;
                } else if (strcmp(&b[6], "1280x960_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x960_MONO_COLOR;
                } else if (strcmp(&b[6], "1280x960_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2;
                } else if (strcmp(&b[6], "1280x960_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3;
                } else if (strcmp(&b[6], "1280x1024_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x1024_RGB;
                } else if (strcmp(&b[6], "1280x1024_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x1024_MONO;
                } else if (strcmp(&b[6], "1280x1024_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR;
                } else if (strcmp(&b[6], "1280x1024_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2;
                } else if (strcmp(&b[6], "1280x1024_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3;
                } else if (strcmp(&b[6], "1600x900_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_RGB;
                } else if (strcmp(&b[6], "1600x900_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_MONO;
                } else if (strcmp(&b[6], "1600x900_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_MONO_COLOR;
                } else if (strcmp(&b[6], "1600x900_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2;
                } else if (strcmp(&b[6], "1600x900_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3;
                } else if (strcmp(&b[6], "1600x1200_RGB") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_RGB;
                } else if (strcmp(&b[6], "1600x1200_MONO") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x900_MONO;
                } else if (strcmp(&b[6], "1600x1200_MONO_COLOR") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR;
                } else if (strcmp(&b[6], "1600x1200_MONO_COLOR2") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2;
                } else if (strcmp(&b[6], "1600x1200_MONO_COLOR3") == 0) {
                    vid->mode = AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3;
                } else {
                    ARLOGe("Error: unrecognised mode \"%s\".\n", &b[6]);
                    ar2VideoDispOption1394();
                    goto bail;
                }
            } else if (strncmp(b, "-guid=", 6) == 0) {
                if (sscanf(&b[6], "%08x%08x", &vid->guid[1], &vid->guid[0]) != 2) {
                    ar2VideoDispOption1394();
                    goto bail;
                }
            } else if (strncmp(b, "-rate=", 6) == 0) {
                if (strcmp(&b[6], "1.875") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_1_875;
                } else if (strcmp(&b[6], "3.75") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_3_75;
                } else if (strcmp(&b[6], "7.5") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_7_5;
                } else if (strcmp(&b[6], "15") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_15;
                } else if (strcmp(&b[6], "30") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_30;
                } else if (strcmp(&b[6], "60") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_60;
                } else if (strcmp(&b[6], "120") == 0) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_120;
                } else {
                    ARLOGe("Error: unrecognised rate \"%s\".\n", &b[6]);
                    ar2VideoDispOption1394();
                    goto bail;
                }
            } else if (strncmp(b, "-speed=", 7) == 0) {
                if (strcmp(&b[7], "400") == 0) {
                    vid->speed = AR_VIDEO_1394_SPEED_400;
                } else if (strcmp(&b[7], "800") == 0) {
                    vid->speed = AR_VIDEO_1394_SPEED_800;
                } else {
                    ARLOGe("Error: unrecognised speed \"%s\".\n", &b[6]);
                    ar2VideoDispOption1394();
                    goto bail;
                }
            } else if (strcmp(b, "-format7") == 0) {
                vid->format7 = 1;
            } else if (strcmp(b, "-debug") == 0) {
                vid->debug = 1;
            } else if (strcmp(b, "-resetCamera") == 0) {
                resetCameraFlag = 1;
            } else if (strcmp(b, "-resetBus") == 0) {
                resetBusFlag = 1;
            } else if (strcmp(b, "-listCamera") == 0) {
                listCameraFlag = 1;
            } else if (strcmp(b, "-module=1394") == 0)    {
            } else {
                ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                ar2VideoDispOption1394();
                goto bail;
            }
            
            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    if (initFlag == 0) {
        if (ar2VideoInit1394(vid->debug) < 0) {
            goto bail;
        }
        initFlag = 1;
    }
    
    if (listCameraFlag) {
        dc1394camera_t *camera;
        ARPRINT("%d camera(s) found.\n", arV1394.list->num);
        for (j=0; j<arV1394.list->num; j++) {
            camera = dc1394_camera_new(arV1394.dc1394, arV1394.list->ids[j].guid);
            dc1394_camera_print_info(camera, stdout);
            dc1394_camera_free(camera);
            ARPRINT("----------------------------------------------------\n");
        }
        goto bail;
    }
    
    for (j = 0; j < arV1394.list->num; j++) {
        if (vid->guid[0] != 0 || vid->guid[1] != 0) {
            if (ar2VideGetCameraGUID1394(j, guid) < 0) continue;
            if (vid->guid[0] != guid[0] || vid->guid[1] != guid[1]) continue;
            if (arV1394.activeFlag[j] > 0) {
                ARLOGe("The camera guid=%08x%08x is already in use.\n", guid[1], guid[0]);
                goto bail;
            }
            if (vid->debug) {
                ARLOGi("Using the camera guid=%08x%08x.\n", guid[1], guid[0]);
            }
            vid->internal_id = j;
            vid->guid[0]     = guid[0];
            vid->guid[1]     = guid[1];
            vid->camera = dc1394_camera_new(arV1394.dc1394, arV1394.list->ids[j].guid);
            break;
        } else {
            if (arV1394.activeFlag[j] > 0) continue;
            if (ar2VideGetCameraGUID1394(j, guid) < 0) continue;
            if (vid->debug) {
                ARLOGi("Using the camera guid=%08x%08x.\n", guid[1], guid[0]);
            }
            vid->internal_id = j;
            vid->guid[0]     = guid[0];
            vid->guid[1]     = guid[1];
            vid->camera = dc1394_camera_new(arV1394.dc1394, arV1394.list->ids[j].guid);
            break;
        }
    }
    if (j == arV1394.list->num) {
        if (vid->debug) ARLOGe("Couldn't find the specified camera.\n");
        goto bail;
    }
    
    if (resetBusFlag) {
        err = dc1394_reset_bus(vid->camera);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to reset bus\n");
            goto bail1;
        }
    }
    
    if (resetCameraFlag) {
        err = dc1394_camera_reset(vid->camera);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to reset camera\n");
            goto bail1;
        }
    }
    
    switch (vid->speed) {
        case AR_VIDEO_1394_SPEED_400:
            vid->int_speed = DC1394_ISO_SPEED_400;
            vid->int_opmode = DC1394_OPERATION_MODE_LEGACY;
            break;
        case AR_VIDEO_1394_SPEED_800:
            vid->int_speed = DC1394_ISO_SPEED_800;
            vid->int_opmode = DC1394_OPERATION_MODE_1394B;
            break;
        default:
            ARLOGe("Sorry, Unsupported Video Speed for IEEE1394 Camera.\n");
            goto bail1;
    }
    err = dc1394_video_set_operation_mode(vid->camera, vid->int_opmode);
    if (err != DC1394_SUCCESS) {
        DC1394_WRN(err, "Unsupported operation mode for the camera\n");
        goto bail1;
    }
    err = dc1394_video_set_iso_speed(vid->camera, vid->int_speed);
    if (err != DC1394_SUCCESS) {
        DC1394_WRN(err, "Unsupported speed for the camera\n");
        goto bail1;
    }
    
    switch (vid->mode) {
        case AR_VIDEO_1394_MODE_320x240_YUV422:
            vid->int_mode = DC1394_VIDEO_MODE_320x240_YUV422;
            break;
        case AR_VIDEO_1394_MODE_640x480_YUV422:
            vid->int_mode = DC1394_VIDEO_MODE_640x480_YUV422;
            break;
        case AR_VIDEO_1394_MODE_640x480_YUV411:
        case AR_VIDEO_1394_MODE_640x480_YUV411_HALF:
            vid->int_mode = DC1394_VIDEO_MODE_640x480_YUV411;
            break;
        case AR_VIDEO_1394_MODE_640x480_RGB:
            vid->int_mode = DC1394_VIDEO_MODE_640x480_RGB8;
            break;
        case AR_VIDEO_1394_MODE_640x480_MONO:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR3:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3:
            vid->int_mode = DC1394_VIDEO_MODE_640x480_MONO8;
            break;
        case AR_VIDEO_1394_MODE_800x600_RGB:
            vid->int_mode = DC1394_VIDEO_MODE_800x600_RGB8;
            break;
        case AR_VIDEO_1394_MODE_800x600_MONO:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR3:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF2:
        case AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF3:
            vid->int_mode = DC1394_VIDEO_MODE_800x600_MONO8;
            break;
        case AR_VIDEO_1394_MODE_1024x768_RGB:
            vid->int_mode = DC1394_VIDEO_MODE_1024x768_RGB8;
            break;
        case AR_VIDEO_1394_MODE_1024x768_MONO:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3:
            vid->int_mode = DC1394_VIDEO_MODE_1024x768_MONO8;
            break;
        case AR_VIDEO_1394_MODE_1280x960_RGB:
            vid->int_mode = DC1394_VIDEO_MODE_1280x960_RGB8;
            break;
        case AR_VIDEO_1394_MODE_1280x960_MONO:
        case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3:
            vid->int_mode = DC1394_VIDEO_MODE_1280x960_MONO8;
            break;
        case AR_VIDEO_1394_MODE_1600x1200_RGB:
            vid->int_mode = DC1394_VIDEO_MODE_1600x1200_RGB8;
            break;
        case AR_VIDEO_1394_MODE_1600x1200_MONO:
        case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3:
            vid->int_mode = DC1394_VIDEO_MODE_1600x1200_MONO8;
            break;
        case AR_VIDEO_1394_MODE_1280x720_RGB:
        case AR_VIDEO_1394_MODE_1280x720_MONO:
        case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3:
        case AR_VIDEO_1394_MODE_1280x1024_RGB:
        case AR_VIDEO_1394_MODE_1280x1024_MONO:
        case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3:
        case AR_VIDEO_1394_MODE_1600x900_RGB:
        case AR_VIDEO_1394_MODE_1600x900_MONO:
        case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3:
            if (vid->format7 == 0) {
                ARLOGe("Sorry. This mode can work only for format7. You need to specify -format7 as well.\n");
                goto bail1;
            }
            vid->int_mode = DC1394_VIDEO_MODE_1600x1200_MONO8;
            break;
        default:
            ARLOGe("Sorry, Unsupported Video Format for IEEE1394 Camera.\n");
            goto bail1;
    }
    if (vid->format7) {
        vid->int_mode = DC1394_VIDEO_MODE_FORMAT7_0;
    }
    
    switch (vid->rate) {
        case AR_VIDEO_1394_FRAME_RATE_1_875:
            vid->int_rate = DC1394_FRAMERATE_1_875;
            break;
        case AR_VIDEO_1394_FRAME_RATE_3_75:
            vid->int_rate = DC1394_FRAMERATE_3_75;
            break;
        case AR_VIDEO_1394_FRAME_RATE_7_5:
            vid->int_rate = DC1394_FRAMERATE_7_5;
            break;
        case AR_VIDEO_1394_FRAME_RATE_15:
            vid->int_rate = DC1394_FRAMERATE_15;
            break;
        case AR_VIDEO_1394_FRAME_RATE_30:
            vid->int_rate = DC1394_FRAMERATE_30;
            break;
        case AR_VIDEO_1394_FRAME_RATE_60:
            vid->int_rate = DC1394_FRAMERATE_60;
            break;
        case AR_VIDEO_1394_FRAME_RATE_120:
            vid->int_rate = DC1394_FRAMERATE_120;
            break;
        default:
            ARLOGe("Sorry, Unsupported Frame Rate for IEEE1394 Camera.\n");
            goto bail1;
    }
    
    /*-----------------------------------------------------------------------*/
    /*  check parameters                                                     */
    /*-----------------------------------------------------------------------*/
    err = dc1394_video_get_supported_modes(vid->camera, &(vid->modes));
    if (err != DC1394_SUCCESS) {
        DC1394_WRN(err, "unable to get supported_modes set\n");
        goto bail1;
    }
    for (j = 0; j < vid->modes.num; j++) {
        if (vid->int_mode == vid->modes.modes[j]) break;
    }
    if (j == vid->modes.num) {
        ARLOGe("Unsupported Mode for the specified camera.\n");
        ar2VideoDispOption1394();
        goto bail1;
    }
    
    err = dc1394_video_set_mode(vid->camera, vid->int_mode);
    if (err != DC1394_SUCCESS) {
        DC1394_WRN(err, "unable to set mode\n");
        goto bail1;
    }
    
    if (vid->format7 == 0) {
        err = dc1394_video_get_supported_framerates(vid->camera, vid->int_mode, &(vid->rates));
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to get supported_framerates\n");
            goto bail1;
        }
        for (j = 0; j < vid->rates.num; j++) {
            if (vid->int_rate == vid->rates.framerates[j]) break;
        }
        if (j == vid->rates.num) {
            ARLOGe("Unsupported framerate for the specified camera.\n");
            ar2VideoDispOption1394();
            goto bail1;
        }
        err = dc1394_video_set_framerate(vid->camera, vid->int_rate);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to set mode\n");
            goto bail1;
        }
    } else {
        int pos_x, pos_y, size_x, size_y;
        unsigned int packet_size;
        unsigned int unit_bytes, max_bytes;
        err = dc1394_format7_get_mode_info(vid->camera, vid->int_mode, &f7_mode);
        switch (vid->mode) {
            case AR_VIDEO_1394_MODE_320x240_YUV422:
                size_x = 320;
                size_y = 240;
                break;
            case AR_VIDEO_1394_MODE_640x480_YUV422:
            case AR_VIDEO_1394_MODE_640x480_YUV411:
            case AR_VIDEO_1394_MODE_640x480_YUV411_HALF:
            case AR_VIDEO_1394_MODE_640x480_RGB:
            case AR_VIDEO_1394_MODE_640x480_MONO:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3:
                size_x = 640;
                size_y = 480;
                break;
            case AR_VIDEO_1394_MODE_1024x768_RGB:
            case AR_VIDEO_1394_MODE_1024x768_MONO:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3:
                size_x = 1024;
                size_y = 768;
                break;
            case AR_VIDEO_1394_MODE_1280x720_RGB:
            case AR_VIDEO_1394_MODE_1280x720_MONO:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3:
                size_x = 1280;
                size_y = 720;
                break;
            case AR_VIDEO_1394_MODE_1280x960_RGB:
            case AR_VIDEO_1394_MODE_1280x960_MONO:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3:
                size_x = 1280;
                size_y = 960;
                break;
            case AR_VIDEO_1394_MODE_1280x1024_RGB:
            case AR_VIDEO_1394_MODE_1280x1024_MONO:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3:
                size_x = 1280;
                size_y = 1024;
                break;
            case AR_VIDEO_1394_MODE_1600x900_RGB:
            case AR_VIDEO_1394_MODE_1600x900_MONO:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3:
                size_x = 1600;
                size_y = 900;
                break;
            case AR_VIDEO_1394_MODE_1600x1200_RGB:
            case AR_VIDEO_1394_MODE_1600x1200_MONO:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3:
                size_x = 1600;
                size_y = 1200;
                break;
            default:
                ARLOGe("Sorry, Unsupported Video Format for IEEE1394 Camera.\n");
                goto bail1;
        }
        switch (vid->mode) {
            case AR_VIDEO_1394_MODE_320x240_YUV422:
            case AR_VIDEO_1394_MODE_640x480_YUV422:
                color_coding = DC1394_COLOR_CODING_YUV422;
                break;
            case AR_VIDEO_1394_MODE_640x480_YUV411:
            case AR_VIDEO_1394_MODE_640x480_YUV411_HALF:
                color_coding = DC1394_COLOR_CODING_YUV411;
                break;
            case AR_VIDEO_1394_MODE_640x480_MONO:
            case AR_VIDEO_1394_MODE_1024x768_MONO:
            case AR_VIDEO_1394_MODE_1280x720_MONO:
            case AR_VIDEO_1394_MODE_1280x960_MONO:
            case AR_VIDEO_1394_MODE_1280x1024_MONO:
            case AR_VIDEO_1394_MODE_1600x900_MONO:
            case AR_VIDEO_1394_MODE_1600x1200_MONO:
                color_coding = DC1394_COLOR_CODING_MONO8;
                break;
            case AR_VIDEO_1394_MODE_640x480_RGB:
            case AR_VIDEO_1394_MODE_1024x768_RGB:
            case AR_VIDEO_1394_MODE_1280x720_RGB:
            case AR_VIDEO_1394_MODE_1280x960_RGB:
            case AR_VIDEO_1394_MODE_1280x1024_RGB:
            case AR_VIDEO_1394_MODE_1600x900_RGB:
            case AR_VIDEO_1394_MODE_1600x1200_RGB:
                color_coding = DC1394_COLOR_CODING_RGB8;
                break;
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2:
            case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2:
            case AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3:
                color_coding = DC1394_COLOR_CODING_RAW8;
                break;
            default:
                ARLOGe("Sorry, Unsupported Video Format for IEEE1394 Camera.\n");
                goto bail1;
        }
        pos_x = (f7_mode.max_size_x - size_x)/2;
        pos_y = (f7_mode.max_size_y - size_y)/2;
        if (pos_x < 0 || pos_y < 0) {
            DC1394_WRN(err, "unable to use format7\n");
            goto bail1;
        }
        
        err = dc1394_format7_set_image_position(vid->camera, vid->int_mode, 0, 0);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7 image_position\n");
            goto bail1;
        }
        err = dc1394_format7_set_image_size(vid->camera, vid->int_mode, size_x, size_y);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7 image_size\n");
            goto bail1;
        }
        err = dc1394_format7_set_image_position(vid->camera, vid->int_mode, pos_x, pos_y);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7 image_position\n");
            goto bail1;
        }
        err = dc1394_format7_set_color_coding(vid->camera, vid->int_mode, color_coding);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7 color_coding\n");
            goto bail1;
        }
        
        err = dc1394_format7_get_packet_parameters(vid->camera, vid->int_mode, &unit_bytes, &max_bytes);
        err = dc1394_format7_get_recommended_packet_size(vid->camera, vid->int_mode, &packet_size);
        ARLOGe("%d %d %d\n", unit_bytes, max_bytes, packet_size);
        err = dc1394_format7_set_packet_size(vid->camera, vid->int_mode, max_bytes);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7 color_coding\n");
            goto bail1;
        }
#if 0
        err = dc1394_format7_set_roi(vid->camera, vid->int_mode, color_coding,
                                     packet_size, pos_x, pos_y, size_x, size_y);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to setup for format7\n");
            goto bail1;
        }
#endif
    }
    
    /*-----------------------------------------------------------------------*/
    /*  setup capture                                                        */
    /*-----------------------------------------------------------------------*/
    err = dc1394_capture_setup(vid->camera, vid->dma_buf_num, DC1394_CAPTURE_FLAGS_DEFAULT);
    if (err != DC1394_SUCCESS) {
        ARLOGe("unable to setup camera.\n");
        goto bail2;
    }
#if 0
    /* set trigger mode */
    err = dc1394_external_trigger_set_mode(vid->camera, DC1394_TRIGGER_MODE_0);
    if (err != DC1394_SUCCESS) {
        ARLOGe("unable to set trigger mode.\n");
        goto bail2;
    }
#endif
    
    /*-----------------------------------------------------------------------*/
    /*  report camera's features                                             */
    /*-----------------------------------------------------------------------*/
    err = dc1394_feature_get_all(vid->camera, &(vid->features));
    if (err != DC1394_SUCCESS) {
        DC1394_WRN(err, "unable to get feature set\n");
        goto bail2;
    } else if (vid->debug) {
        err = dc1394_feature_print_all(&(vid->features), stdout);
        if (err != DC1394_SUCCESS) {
            DC1394_WRN(err, "unable to print feature set\n");
        }
    }
    
    arV1394.activeFlag[vid->internal_id] = 1;
    
    dc1394_get_image_size_from_video_mode(vid->camera, vid->int_mode, (uint32_t *)&(vid->width), (uint32_t *)&(vid->height));
    dc1394_get_color_coding_from_video_mode(vid->camera, vid->int_mode, &color_coding);
    dc1394_get_color_coding_bit_size(color_coding, (uint32_t *)&(vid->bits));
    
    if (vid->mode == AR_VIDEO_1394_MODE_640x480_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x720_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x960_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x1024_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1600x900_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1600x1200_MONO) {
        ar2VideoBufferInit1394(&(vid->buffer), vid->width * vid->height);
    } else {
        ar2VideoBufferInit1394(&(vid->buffer), vid->width * vid->height * 3);
    }
    vid->status = AR2VIDEO_1394_STATUS_IDLE;
    pthread_create(&(vid->capture), NULL, (void * (*)(void *))ar2VideoCapture1394, vid);
    
    return vid;

bail2:
    dc1394_video_set_transmission(vid->camera, DC1394_OFF);
    dc1394_capture_stop(vid->camera);
bail1:
    dc1394_camera_free(vid->camera);
bail:
    free(vid);
    return NULL;
}


int ar2VideoClose1394(AR2VideoParam1394T *vid)
{
    vid->status = AR2VIDEO_1394_STATUS_STOP;
    
    pthread_join(vid->capture, NULL);
    
    dc1394_video_set_transmission(vid->camera, DC1394_OFF);
    dc1394_capture_stop(vid->camera);
    dc1394_camera_free(vid->camera);
    arV1394.activeFlag[vid->internal_id] = 0;
    free(vid->buffer.in.buff);
    free(vid->buffer.wait.buff);
    free(vid->buffer.out.buff);
    free(vid);
    
    arUtilSleep(100);
    
    return 0;
}

int ar2VideoGetId1394(AR2VideoParam1394T *vid, ARUint32 *id0, ARUint32 *id1)
{
    if (vid == NULL) return -1;
    
    *id0 = (ARUint32)vid->guid[0];
    *id1 = (ARUint32)vid->guid[1];
    
    return 0;
}

int ar2VideoGetSize1394(AR2VideoParam1394T *vid, int *x,int *y)
{
    if (vid == NULL) return -1;
    
    *x = vid->width;
    *y = vid->height;
    
    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormat1394(AR2VideoParam1394T *vid)
{
    if (vid == NULL) return AR_PIXEL_FORMAT_INVALID;
    
    if (vid->mode == AR_VIDEO_1394_MODE_640x480_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x720_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x960_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1280x1024_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1600x900_MONO
        || vid->mode == AR_VIDEO_1394_MODE_1600x1200_MONO) {
        return AR_PIXEL_FORMAT_MONO;
    } else {
        return AR_PIXEL_FORMAT_RGB;
    }
}


int ar2VideoCapStart1394(AR2VideoParam1394T *vid)
{
    if (vid->status == AR2VIDEO_1394_STATUS_RUN){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }
    
    vid->status = AR2VIDEO_1394_STATUS_RUN;
    
    return 0;
}


int ar2VideoCapStop1394(AR2VideoParam1394T *vid)
{
    if (vid->status != AR2VIDEO_1394_STATUS_RUN) return -1;
    vid->status = AR2VIDEO_1394_STATUS_IDLE;
    arUtilSleep(100);
    
    return 0;
}

AR2VideoBufferT *ar2VideoGetImage1394(AR2VideoParam1394T *vid)
{
    AR2VideoBufferT   tmp;
    
    pthread_mutex_lock(&(vid->buffer.mutex));
    tmp = vid->buffer.wait;
    vid->buffer.wait = vid->buffer.out;
    vid->buffer.out = tmp;
    
    vid->buffer.wait.fillFlag = 0;
    pthread_mutex_unlock(&(vid->buffer.mutex));
    
    return &(vid->buffer.out);
}

static void ar2VideoCapture1394(AR2VideoParam1394T *vid)
{
    AR2VideoBufferT   tmp;
    int               startFlag = 0;
    int               i;
    
    while (vid->status != AR2VIDEO_1394_STATUS_STOP) {
        if (vid->status == AR2VIDEO_1394_STATUS_RUN && startFlag == 0) {
            dc1394switch_t status = DC1394_OFF;
            if (dc1394_video_set_transmission(vid->camera, DC1394_ON) != DC1394_SUCCESS) {
                ARLOGe("error: unable to start camera iso transmission\n");
                return;
            }
            i = 0;
            while (status == DC1394_OFF && i++ < 5) {
                usleep(500000);
#if 0
                if (dc1394_video_get_transmission(vid->camera, &status) != DC1394_SUCCESS) {
                    ARLOGe("unable to get transmision status\n");
                    return;
                }
#else
                dc1394_video_get_transmission(vid->camera, &status);
#endif
            }
            if (i == 5) {
                ARLOGe("Camera doesn't seem to want to turn on!\n");
                return;
            }
            startFlag = 1;
        }
        
        if (vid->status == AR2VIDEO_1394_STATUS_IDLE && startFlag == 1) {
            if (dc1394_video_set_transmission(vid->camera, DC1394_OFF) !=DC1394_SUCCESS) {
                ARLOGe("error: couldn't stop the camera?\n");
                return;
            }
            startFlag = 0;
        }
        
        if (vid->status == AR2VIDEO_1394_STATUS_IDLE) {
            usleep(100);
            continue;
        }
        
        if (startFlag) {
            dc1394video_frame_t *frame=NULL;
            if (dc1394_capture_dequeue(vid->camera, DC1394_CAPTURE_POLICY_WAIT, &frame) != DC1394_SUCCESS) {
                ARLOGe("error: unable to capture a frame\n");
                return;
            }
            
            vid->buffer.in.time.sec  = frame->timestamp / 1000000;
            vid->buffer.in.time.usec = frame->timestamp % 1000000;
            vid->buffer.in.fillFlag = 1;
            vid->buffer.in.buffLuma = NULL;
            
            ar2Video1394FormatConversion((ARUint8 *)frame->image, vid->buffer.in.buff, vid->mode, vid->width, vid->height);
            
            dc1394_capture_enqueue(vid->camera, frame);
            
            pthread_mutex_lock(&(vid->buffer.mutex));
            tmp = vid->buffer.wait;
            vid->buffer.wait = vid->buffer.in;
            vid->buffer.in = tmp;
            pthread_mutex_unlock(&(vid->buffer.mutex));
        }
    }
    
    if (startFlag == 1) {
        if (dc1394_video_set_transmission(vid->camera, DC1394_OFF) !=DC1394_SUCCESS) {
            ARLOGe("error: couldn't stop the camera?\n");
            return;
        }
        startFlag = 0;
    }
    
    return;
}


static int ar2VideoInit1394(int debug)
{
    dc1394camera_t   *camera;
    int               err;
    int               i;
    
    arV1394.dc1394 = dc1394_new();
    if (arV1394.dc1394 == NULL) {
        ARLOGe("error on dc1394_new.\n");
        return -1;
    }
    
    err = dc1394_camera_enumerate(arV1394.dc1394, &(arV1394.list));
    if (err!=DC1394_SUCCESS) {
        //if (err!=DC1394_SUCCESS && err != DC1394_NO_CAMERA) {
        ARLOGe("Unable to look for an IIDC camera. error code = %d\n", err);
        dc1394_free(arV1394.dc1394);
        return -1;
    }
    
    if (arV1394.list->num < 1) {
        ARLOGe("no cameras found.\n");
        dc1394_camera_free_list(arV1394.list);
        dc1394_free(arV1394.dc1394);
        return -1;
    }
    
    if (debug) {
        ARLOGe("%d camera(s) found.\n", arV1394.list->num);
        for (i = 0; i < arV1394.list->num; i++) {
            camera = dc1394_camera_new(arV1394.dc1394, arV1394.list->ids[i].guid);
            dc1394_camera_print_info(camera, stdout);
            dc1394_camera_free(camera);
            ARPRINT("----------------------------------------------------\n");
        }
    }
    
    arMalloc(arV1394.activeFlag, int, arV1394.list->num);
    for (i = 0; i < arV1394.list->num; i++) {
        arV1394.activeFlag[i] = 0;
    }
    
    return 0;
}

static int ar2VideGetCameraGUID1394(int internal_id, uint32_t euid[2])
{
    euid[0]=  arV1394.list->ids[internal_id].guid       & 0xffffffff;
    euid[1]= (arV1394.list->ids[internal_id].guid >>32) & 0xffffffff;
    
    return 0;
}

static void ar2VideoBufferInit1394(AR2VideoBuffer1394T *buffer, int size)
{
    arMalloc(buffer->in.buff,   ARUint8, size);
    arMalloc(buffer->wait.buff, ARUint8, size);
    arMalloc(buffer->out.buff,  ARUint8, size);
    buffer->in.fillFlag   = 0;
    buffer->wait.fillFlag = 0;
    buffer->out.fillFlag  = 0;
    buffer->in.buffLuma   = NULL;
    buffer->wait.buffLuma = NULL;
    buffer->out.buffLuma  = NULL;
    pthread_mutex_init(&(buffer->mutex), NULL);
    
    return;
}

int ar2VideoGetParami1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    if (paramName == AR_VIDEO_1394_BRIGHTNESS) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_GAIN) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_UB) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_VR) {
        return ar2VideoGetValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON) {
        return ar2VideoGetFeatureOn1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON) {
        return ar2VideoGetAutoOn1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_MAX_VAL) {
        return ar2VideoGetMaxValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_MIN_VAL) {
        return ar2VideoGetMinValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    }
    
    return -1;
}

int ar2VideoSetParami1394(AR2VideoParam1394T *vid, int paramName, int value)
{
    if (paramName == AR_VIDEO_1394_BRIGHTNESS) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_BRIGHTNESS_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_EXPOSURE_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_EXPOSURE, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
    } else if (paramName == AR_VIDEO_1394_GAIN) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_GAIN_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_GAIN, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_FOCUS_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_FOCUS, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_UB) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_VR) {
        return ar2VideoSetValue1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON) {
        return ar2VideoSetFeatureOn1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    } else if (paramName == AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON) {
        return ar2VideoSetAutoOn1394(vid, AR_VIDEO_1394_WHITE_BALANCE, value);
    }
    
    return -1;
}

int ar2VideoGetParamd1394(AR2VideoParam1394T *vid, int paramName, double *value)
{
    if (paramName == AR_VIDEO_1394_GAMMA) {
        return ar2VideoGetAbsValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_MAX_VAL) {
        return ar2VideoGetAbsMaxValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    } else if (paramName == AR_VIDEO_1394_GAMMA_MIN_VAL) {
        return ar2VideoGetAbsMinValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    }
    return -1;
}

int ar2VideoSetParamd1394(AR2VideoParam1394T *vid, int paramName, double value)
{
    if (paramName == AR_VIDEO_1394_GAMMA) {
        return ar2VideoSetAbsValue1394(vid, AR_VIDEO_1394_GAMMA, value);
    }
    return -1;
}

int ar2VideoGetParams1394(AR2VideoParam1394T *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParams1394(AR2VideoParam1394T *vid, const int paramName, const char *value)
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}


#endif // ARVIDEO_INPUT_LIBDC1394
