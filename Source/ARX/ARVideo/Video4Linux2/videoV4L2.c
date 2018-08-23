/*
 *  videoV4L2.c
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
 *  Author(s): Atsushi Nakazawa, Hirokazu Kato, Philip Lamb, Simon Goodall
 *
 */
/*
 *   Video capture subrutine for Linux/Video4Linux2 devices
 *   Based upon the V4L 1 artoolkit code and v4l2 spec example
 *   at http://v4l2spec.bytesex.org/spec/a13010.htm
 *   Simon Goodall <sg@ecs.soton.ac.uk>
 */

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#define _XOPEN_SOURCE 500 // realpath()

#include "videoV4L2.h"

#ifdef ARVIDEO_INPUT_V4L2

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h> // gettimeofday(), struct timeval
#include <sys/param.h> // MAXPATHLEN
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> // asprintf()
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> // memset()
#include <errno.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <libudev.h>
#include "../cparamSearch.h"
#include <ARX/ARVideo/videoRGBA.h>

#define   AR2VIDEO_V4L2_STATUS_IDLE    0
#define   AR2VIDEO_V4L2_STATUS_RUN     1
#define   AR2VIDEO_V4L2_STATUS_STOP    2

//#define AR2VIDEO_V4L2_NONBLOCKING
//#define AR2VIDEO_V4L2_DEBUG

typedef struct {
    uint8_t  *ptr;
    size_t    length;
} AR2VideoInternalBufferSetV4LT;

struct _AR2VideoParamV4L2T {
    char                   dev[MAXPATHLEN];
    int                    width;
    int                    height;
    int                    channel;
    int                    mode;
    int                    field;
    int                    debug;
    int                    palette;
    int                    saturation;
    int                    exposure;
    int                    gain;
    int                    gamma;
    int                    contrast;
    int                    brightness;
    int                    hue;
    double                 whiteness;
    double                 color;
    int                    frameDurationNumer;
    int                    frameDurationDenom;

    int                    fd;
    int                    status;
    int                    video_cont_num;
    
    AR2VideoInternalBufferSetV4LT *internalBufferSet;
    int                    internalBufferCount;
    AR_PIXEL_FORMAT        format;
    AR2VideoBufferT        buffer;
    AR_PIXEL_FORMAT        formatConverted;
    AR2VideoBufferT        bufferConverted;
    
    void                 (*cparamSearchCallback)(const ARParam *, void *);
    void                  *cparamSearchUserdata;
    char                  *device_id;
    char                  *name;
};

static int xioctl(int fd, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && ((errno == EINTR)
#ifdef AR2VIDEO_V4L2_NONBLOCKING
                         || (errno == EAGAIN)
#endif
                         ));
    if (r == -1) {
        ARLOGperror("ioctl error");
    }
    return (r);
}

#define MAXCHANNEL   10

static void printPalette(const char *tag, int p)
{
    const char *s;

    switch (p) {
        // YUV formats.
        case (V4L2_PIX_FMT_GREY): s = "GREY"; break;
        case (V4L2_PIX_FMT_YUYV): s = "YUYV"; break;
        case (V4L2_PIX_FMT_UYVY): s = "UYVY"; break;
        case (V4L2_PIX_FMT_Y41P): s = "Y41P"; break;
        case (V4L2_PIX_FMT_YVU420): s = "YVU420"; break;
        case (V4L2_PIX_FMT_YVU410): s = "YVU410"; break;
        case (V4L2_PIX_FMT_YUV422P): s = "YUV422P"; break;
        case (V4L2_PIX_FMT_YUV411P): s = "YUV411P"; break;
        case (V4L2_PIX_FMT_NV12): s = "NV12"; break;
        case (V4L2_PIX_FMT_NV21): s = "NV21"; break;
        // RGB formats
        case (V4L2_PIX_FMT_RGB332): s = "RGB332"; break;
        case (V4L2_PIX_FMT_ARGB444): s = "ARGB444"; break;
        case (V4L2_PIX_FMT_XRGB444): s = "XRGB444"; break;
        case (V4L2_PIX_FMT_ARGB555): s = "ARGB555"; break;
        case (V4L2_PIX_FMT_XRGB555): s = "XRGB555"; break;
        case (V4L2_PIX_FMT_RGB565): s = "RGB565"; break;
        case (V4L2_PIX_FMT_ARGB555X): s = "ARGB555X"; break;
        case (V4L2_PIX_FMT_XRGB555X): s = "XRGB555X"; break;
        case (V4L2_PIX_FMT_RGB565X): s = "RGB565X"; break;
        case (V4L2_PIX_FMT_BGR24): s = "BGR24"; break;
        case (V4L2_PIX_FMT_RGB24): s = "RGB24"; break;
        case (V4L2_PIX_FMT_BGR666): s = "BGR666"; break;
        case (V4L2_PIX_FMT_ABGR32): s = "ABGR32"; break;
        case (V4L2_PIX_FMT_XBGR32): s = "XBGR32"; break;
        case (V4L2_PIX_FMT_ARGB32): s = "ARGB32"; break;
        case (V4L2_PIX_FMT_XRGB32): s = "XRGB32"; break;
        // Deprecated formats.
        case (V4L2_PIX_FMT_RGB444): s = "RGB444"; break;
        case (V4L2_PIX_FMT_RGB555): s = "RGB555"; break;
        case (V4L2_PIX_FMT_RGB555X): s = "RGB555X"; break;
        case (V4L2_PIX_FMT_BGR32): s = "BGR32"; break;
        case (V4L2_PIX_FMT_RGB32): s = "RGB32"; break;
        default:  s = "Unknown"; break;
    };
    ARLOGi("%s%s\n", (tag ? tag : ""), s);
}

static int getControl(int fd, int type, int *value)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    
    memset (&queryctrl, 0, sizeof (queryctrl));
    // TODO: Manke sure this is a correct value
    queryctrl.id = type;
    
    if (-1 == xioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
            ARLOGe("Error calling VIDIOC_QUERYCTRL\n");
            return 1;
        } else {
            ARLOGe("Control %d is not supported\n", type);
            return 1;
        }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        ARLOGe("Control %s is not supported\n", queryctrl.name);
        return 1;
    } else {
        memset (&control, 0, sizeof (control));
        control.id = type;
        
        if (-1 == xioctl (fd, VIDIOC_G_CTRL, &control)) {
            ARLOGe("Error getting control %s value\n", queryctrl.name);
            return 1;
        }
        *value = control.value;
    }
    return 0;
}

static int setControl(int fd, int type, int value)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    
    memset (&queryctrl, 0, sizeof (queryctrl));
    // TODO: Manke sure this is a correct value
    queryctrl.id = type;
    
    if (-1 == xioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
            ARLOGe("Error calling VIDIOC_QUERYCTRL\n");
            return 1;
        } else {
            ARLOGe("Control %d is not supported\n", type);
            return 1;
        }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        ARLOGe("Control %s is not supported\n", queryctrl.name);
        return 1;
    } else {
        memset (&control, 0, sizeof (control));
        control.id = type;
        // TODO check min/max range
        // If value is -1, then we use the default value
        control.value = (value == -1) ? (queryctrl.default_value) : (value);
        
        if (-1 == xioctl (fd, VIDIOC_S_CTRL, &control)) {
            ARLOGe("Error setting control %s to %d\n", queryctrl.name, value);
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------*/

int ar2VideoDispOptionV4L2(void)
{
    ARPRINT(" -module=V4L2\n");
    ARPRINT("\n");
    ARPRINT("DEVICE CONTROLS:\n");
    ARPRINT(" -dev=filepath\n");
    ARPRINT("    specifies device file.\n");
    ARPRINT(" -channel=N\n");
    ARPRINT("    specifies source channel.\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    request an image of width N.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    request an image of height N.\n");
    ARPRINT(" -palette=[BGR32|ABGR32|RGB32|ARGB32|BGR24|RGB24|GREY\n");
    ARPRINT("           YUYV|UYV|NV12|NV21|RGB565X]\n");
    ARPRINT("    request an image with the specified palette (i.e. pixel format).\n");
    ARPRINT("    (WARNING: not all options are supported by every camera).\n");
    ARPRINT(" -format=[0|BGRA|RGBA].\n");
    ARPRINT("    Specifies the pixel format to convert output images to.\n");
    ARPRINT("    0=Don't convert.\n");
    ARPRINT(" -frameduration=N/D.\n");
    ARPRINT("    request frames of duration N/D (numerator / denominator) seconds.\n");
    ARPRINT("IMAGE CONTROLS (WARNING: not all options are not supported by every camera):\n");
    ARPRINT(" -brightness=N\n");
    ARPRINT("    specifies brightness. (0.0 <-> 1.0)\n");
    ARPRINT(" -contrast=N\n");
    ARPRINT("    specifies contrast. (0.0 <-> 1.0)\n");
    ARPRINT(" -saturation=N\n");
    ARPRINT("    specifies saturation (color). (0.0 <-> 1.0) (for color camera only)\n");
    ARPRINT(" -hue=N\n");
    ARPRINT("    specifies hue. (0.0 <-> 1.0) (for color camera only)\n");
    ARPRINT("OPTION CONTROLS:\n");
    ARPRINT(" -mode=[PAL|NTSC|SECAM]\n");
    ARPRINT("    specifies TV signal mode (for tv/capture card).\n");
    ARPRINT("\n");
    
    return 0;
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListV4L2(const char *config_in)
{
    struct udev *udev;
    ARVideoSourceInfoListT *sil = NULL;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev, *dev_parent;
	const char udev_err_msg[] = "Unable to query udev for V4L device list.\n"; 
	
	udev = udev_new();
    if (!udev) {
        ARLOGe(udev_err_msg);
        return NULL;
    }
    enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        ARLOGe(udev_err_msg);
        goto bail;
    }
    if (udev_enumerate_add_match_subsystem(enumerate, "video4linux") < 0) {
        ARLOGe(udev_err_msg);
        goto bail1;
    }
	if (udev_enumerate_scan_devices(enumerate) < 0) {
        ARLOGe(udev_err_msg);
        goto bail1;
    }
    devices = udev_enumerate_get_list_entry(enumerate);
    if (!devices) {
        goto bail1;
    }
    
    // Count devices, and create list.
    int count = 0;
    udev_list_entry_foreach(dev_list_entry, devices) {
        count++;
    } 
    arMallocClear(sil, ARVideoSourceInfoListT, 1);
    sil->count = count;
    arMallocClear(sil->info, ARVideoSourceInfoT, count);
    
    int i = 0;
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        
        const char *path = udev_list_entry_get_name(dev_list_entry);
        //ARLOGd("System Path: %s\n", path);                         // e.g. '/sys/devices/pci0000:00/0000:00:1d.7/usb1/1-1/1-1:1.0/video4linux/video0'
        dev = udev_device_new_from_syspath(udev, path);
        //ARLOGd("Device Node: %s\n", udev_device_get_devnode(dev)); // e.g. '/dev/video0'
        //ARLOGd("System Name: %s\n", udev_device_get_sysname(dev)); // e.g. 'video0'
        //ARLOGd("Device Path: %s\n", udev_device_get_devpath(dev)); // e.g. '/devices/pci0000:00/0000:00:1d.7/usb1/1-1/1-1:1.0/video4linux/video0'

        sil->info[i].flags |= AR_VIDEO_POSITION_UNKNOWN;
        //sil->info[i].flags |= AR_VIDEO_SOURCE_INFO_FLAG_OPEN_ASYNC; // If we require async opening.
        sil->info[i].name = strdup(udev_device_get_sysattr_value(dev, "name"));
        //ARLOGd("name: %s\n", sil->info[i].name);                   // e.g. 'Logitech Camera'
        if (asprintf(&sil->info[i].open_token, "-dev=%s", udev_device_get_devnode(dev)) < 0) {
            ARLOGperror(NULL);
            sil->info[i].open_token = NULL;
        }
        
        dev_parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (dev_parent) {
            sil->info[i].UID = strdup(udev_device_get_sysattr_value(dev_parent, "serial"));
            //ARLOGd("UID: %s\n", sil->info[i].UID);
            const char *idVendor = udev_device_get_sysattr_value(dev_parent, "idVendor");
            const char *idProduct = udev_device_get_sysattr_value(dev_parent, "idProduct");
            const char *product = udev_device_get_sysattr_value(dev_parent, "product");
            if (idProduct && idVendor) {
                if (asprintf(&sil->info[i].model, "usb %s:%s%s%s", idVendor, idProduct, (product ? " " : ""), (product ? product : "")) == -1) {
                    ARLOGperror(NULL);
                    sil->info[i].model = NULL;
                }
            }
        }
        
        udev_device_unref(dev);
        
        i++;
    }

bail1:
    udev_enumerate_unref(enumerate);
bail:   
    udev_unref(udev);
    
    return (sil);
}

AR2VideoParamV4L2T *ar2VideoOpenV4L2(const char *config)
{
    char                     *cacheDir = NULL;
    char                     *cacheInitDir = NULL;
    char                     *csdu = NULL;
    char                     *csat = NULL;
    AR2VideoParamV4L2T       *vid;
    struct v4l2_capability   vd;
    struct v4l2_format fmt;
    struct v4l2_input  ipt;
    struct v4l2_requestbuffers req;
    struct v4l2_streamparm parm;
    
    const char *a;
    char line[1024];
    int value;
    int err_i = 0;
    int i;
    
    arMallocClear(vid, AR2VideoParamV4L2T, 1);
    strcpy(vid->dev, AR_VIDEO_V4L2_DEFAULT_DEVICE);
    vid->width      = AR_VIDEO_V4L2_DEFAULT_WIDTH;
    vid->height     = AR_VIDEO_V4L2_DEFAULT_HEIGHT;
    vid->channel    = AR_VIDEO_V4L2_DEFAULT_CHANNEL;
    vid->mode       = AR_VIDEO_V4L2_DEFAULT_MODE;
    vid->field = V4L2_FIELD_ANY;
    vid->palette = V4L2_PIX_FMT_YUYV;     /* palette format */
    vid->contrast   = -1;
    vid->brightness = -1;
    vid->saturation = -1;
    vid->hue        = -1;
    vid->gamma  = -1;
    vid->exposure  = -1;
    vid->gain  = 1;
    //vid->debug      = 0;
    vid->debug      = 1;
    vid->formatConverted = AR_VIDEO_V4L2_DEFAULT_FORMAT_CONVERSION;
    vid->frameDurationNumer = vid->frameDurationDenom = 0;
    
    a = config;
    if (a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;

            if (strncmp(a, "-dev=", 5) == 0) {
                if (sscanf(&line[5], "%s", vid->dev) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-channel=", 9) == 0) {
                if (sscanf(&line[9], "%d", &vid->channel) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->width) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-height=", 8) == 0) {
                if (sscanf(&line[8], "%d", &vid->height) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-palette=", 9) == 0) {
                if (strcmp(&line[9], "BGR32") == 0) { // deprecated
                    vid->palette = V4L2_PIX_FMT_BGR32;
                } else if (strcmp(&line[9], "ABGR32") == 0) {
                    vid->palette = V4L2_PIX_FMT_ABGR32;
                } else if (strcmp(&line[9], "RGB32") == 0) {  // deprecated
                    vid->palette = V4L2_PIX_FMT_RGB32;
                } else if (strcmp(&line[9], "ARGB32") == 0) {
                    vid->palette = V4L2_PIX_FMT_ARGB32;
                } else if (strcmp(&line[9], "BGR24") == 0) {
                    vid->palette = V4L2_PIX_FMT_BGR24;
                } else if (strcmp(&line[9], "RGB24") == 0) {
                    vid->palette = V4L2_PIX_FMT_RGB24;
                } else if (strcmp(&line[9], "GREY") == 0) {
                    vid->palette = V4L2_PIX_FMT_GREY;
                } else if (strcmp(&line[9], "YUYV") == 0) {
                    vid->palette = V4L2_PIX_FMT_YUYV;
                } else if (strcmp(&line[9], "UYVY") == 0) {
                    vid->palette = V4L2_PIX_FMT_UYVY;
                } else if (strcmp(&line[9], "NV12") == 0) {
                    vid->palette = V4L2_PIX_FMT_NV12;
                } else if (strcmp(&line[9], "NV21") == 0) {
                    vid->palette = V4L2_PIX_FMT_NV21;
                } else if (strcmp(&line[9], "RGB565X") == 0) {
                    vid->palette = V4L2_PIX_FMT_RGB565X;
                } else {
                    ARLOGe("Request for a palette format '%s' unsupported by artoolkitX.\n", &line[9]);
                    err_i = 1;
                }
            } else if (strncmp(line, "-format=", 8) == 0) {
                if (strcmp(line+8, "0") == 0) {
                    vid->formatConverted = AR_PIXEL_FORMAT_INVALID;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(line+8, "RGBA") == 0) {
                    vid->formatConverted = AR_PIXEL_FORMAT_RGBA;
                    ARLOGi("Requesting images in RGBA format.\n");
                } else if (strcmp(line+8, "BGRA") == 0) {
                    vid->formatConverted = AR_PIXEL_FORMAT_BGRA;
                    ARLOGi("Requesting images in BGRA format.\n");
                } else {
                    ARLOGe("Ignoring unsupported request for conversion to video format '%s'.\n", line+8);
                }
            } else if (strncmp(a, "-frameduration=", 15) == 0) {
                if (sscanf(&line[15], "%d/%d", &vid->frameDurationNumer,  &vid->frameDurationDenom) != 2) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-contrast=", 10) == 0) {
                if (sscanf(&line[10], "%d", &vid->contrast) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-brightness=", 12) == 0) {
                if (sscanf(&line[12], "%d", &vid->brightness) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-saturation=", 12) == 0) {
                if (sscanf(&line[12], "%d", &vid->saturation) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-hue=", 5) == 0) {
                if (sscanf(&line[5], "%d", &vid->hue) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-gamma=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->gamma) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-exposure=", 10) == 0) {
                if (sscanf(&line[10], "%d", &vid->exposure) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-gain=", 6) == 0) {
                if (sscanf(&line[6], "%d", &vid->gain) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-mode=", 6) == 0) {
                if (strncmp(&a[6], "PAL", 3) == 0)        vid->mode = V4L2_STD_PAL;
                else if (strncmp(&a[6], "NTSC", 4) == 0)  vid->mode = V4L2_STD_NTSC;
                else if (strncmp(&a[6], "SECAM", 5) == 0) vid->mode = V4L2_STD_SECAM;
                else {
                    err_i = 1;
                }
            } else if (strncmp(a, "-field=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->field) == 0) {
                    err_i = 1;
                }
            } else if (strcmp(line, "-debug") == 0) {
                vid->debug = 1;
            } else if (strncmp(a, "-cachedir=", 10) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 10; // Skip "-cachedir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cachedir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheDir);
                    cacheDir = strdup(line);
                }
            } else if (strncmp(a, "-cacheinitdir=", 14) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 14; // Skip "-cacheinitdir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cacheinitdir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheInitDir);
                    cacheInitDir = strdup(line);
                }
            } else if (strncmp(a, "-csdu=", 6) == 0) {
                // Attempt to read in download URL.
                a += 6; // Skip "-csdu=" characters.
                sscanf(a, "%s", line);
                free(csdu);
                if (!strlen(line)) {
                    csdu = NULL;
                } else {
                    csdu = strdup(line);
                }
            } else if (strncmp(a, "-csat=", 6) == 0) {
                // Attempt to read in authentication token, allowing for quoting of whitespace.
                a += 6; // Skip "-csat=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                free(csat);
                if (!strlen(line)) {
                    csat = NULL;
                } else {
                    csat = strdup(line);
                }
            } else if (strcmp(line, "-module=V4L2") == 0)    {
            } else {
                err_i = 1;
            }
            
            if (err_i) {
                ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionV4L2();
                goto bail;
			}
            
            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
#if USE_CPARAM_SEARCH
    // Initialisation required before cparamSearch can be used.
    if (!cacheDir) {
        cacheDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR);
    }
    if (!cacheInitDir) {
        cacheInitDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR);
    }
    if (cparamSearchInit(cacheDir, cacheInitDir, false, csdu, csat) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };
#endif
    free(cacheDir);
    cacheDir = NULL;
    free(cacheInitDir);
    cacheInitDir = NULL;
    free(csdu);
    csdu = NULL;
    free(csat);
    csat = NULL;

    vid->fd = open(vid->dev, O_RDWR
#ifdef AR2VIDEO_V4L2_NONBLOCKING
                   | O_NONBLOCK
#endif
                   );
    if (vid->fd < 0) {
        ARLOGe("video device (%s) open failed\n", vid->dev);
        goto bail;
    }
    
    if (xioctl(vid->fd, VIDIOC_QUERYCAP, &vd) < 0) {
        ARLOGe("xioctl failed\n");
        goto bail1;
    }
    
    if (!(vd.capabilities & V4L2_CAP_STREAMING)) {
        ARLOGe("Device does not support streaming i/o\n");
    }
    
    if (vid->debug) {
        ARLOGi("=== debug info ===\n");
        ARLOGi("  vd.driver        =   %s\n",vd.driver);
        ARLOGi("  vd.card          =   %s\n",vd.card);
        ARLOGi("  vd.bus_info      =   %s\n",vd.bus_info);
        ARLOGi("  vd.version       =   %d\n",vd.version);
        ARLOGi("  vd.capabilities  =   %d\n",vd.capabilities);
    }

    // Get the sysname of the device requested by the user.
    // Since vid->dev might be a symbolic link, it has to be resolved first.
    char *sysname = NULL;
    char *dev_real = realpath(vid->dev, NULL);
    if (!dev_real) {
        ARLOGe("Unable to resolve device path '%s'.\n", vid->dev);
        ARLOGperror(NULL);
    } else {
        if (strncmp(dev_real, "/dev/", 5) != 0) {
            ARLOGe("Resolved device path '%s' is not in /dev.\n", dev_real);
            free(dev_real);
        } else {
            sysname = &dev_real[5];
        }
    }
    
    if (sysname) {
        // Get udev device for the passed-in node.
        struct udev *udev = udev_new();
        if (!udev) {
            ARLOGe("Unable to query udev.\n");
        } else {
            struct udev_device *dev = udev_device_new_from_subsystem_sysname(udev, "video4linux", sysname);
            if (!dev) {
                ARLOGe("Unable to locate udev video4linux device '%s'.\n", sysname);
            } else {
                ARLOGd("Device Path: %s\n", udev_device_get_devpath(dev)); // e.g. '/devices/pci0000:00/0000:00:1d.7/usb1/1-1/1-1:1.0/video4linux/video0'
                vid->name = strdup(udev_device_get_sysattr_value(dev, "name")); // e.g. 'Logitech Camera'
                ARLOGd("Device Name: %s\n", vid->name);
                struct udev_device *dev_parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
                if (dev_parent) {
                    //char *UID = strdup(udev_device_get_sysattr_value(dev_parent, "serial"));
                    //ARLOGd("UID: %s\n", UID);
                    const char *idVendor = udev_device_get_sysattr_value(dev_parent, "idVendor");
                    const char *idProduct = udev_device_get_sysattr_value(dev_parent, "idProduct");
                    const char *product = udev_device_get_sysattr_value(dev_parent, "product");
                    if (idProduct && idVendor) {
                        if (asprintf(&vid->device_id, "/usb %s:%s%s%s/", idVendor, idProduct, (product ? " " : ""), (product ? product : "")) == -1) {
                            ARLOGperror(NULL);
                            vid->device_id = NULL;
                        }
                    }
                }
                udev_device_unref(dev);            
            }
            udev_unref(udev);
        }
        free(dev_real);
    }
    if (!vid->device_id) ARLOGw("Unable to obtain device_id. cparamSearch will be unavailable.\n");
    else ARLOGi("device_id: '%s'.\n", vid->device_id);
    
    memset(&fmt, 0, sizeof(fmt));
    
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = vid->width;
    fmt.fmt.pix.height      = vid->height;
    fmt.fmt.pix.pixelformat = vid->palette;
    fmt.fmt.pix.field       = vid->field;
    
    if (xioctl(vid->fd, VIDIOC_S_FMT, &fmt) < 0) {
        ARLOGe("ar2VideoOpen: Error setting video format.\n");
        ARLOGperror(NULL); // EINVAL -> Requested buffer type is not supported drivers. EBUSY -> I/O is already in progress or the resource is not available for other reasons.
        goto bail1;
    }
    
    if (vid->frameDurationNumer && vid->frameDurationDenom) {
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = vid->frameDurationNumer;
        parm.parm.capture.timeperframe.denominator = vid->frameDurationDenom;
        if (xioctl(vid->fd, VIDIOC_S_PARM, &parm) < 0) {
            ARLOGe("ar2VideoOpen: Error setting video frame duration to %d/%d.\n",vid->frameDurationNumer, vid->frameDurationDenom);
            ARLOGperror(NULL);
        } else {
            ARLOGi("Set video frame duration to %d/%d.\n",vid->frameDurationNumer, vid->frameDurationDenom);
        }
    }
    
    if (vid->debug) {
        if (vid->width != fmt.fmt.pix.width) {
            ARLOGi("  Width requested: %d\n", vid->width);
            ARLOGi("  Width chosen:    %d\n", fmt.fmt.pix.width);
        } else {
            ARLOGi("  Width: %d\n", fmt.fmt.pix.width);
        }
        if (vid->height != fmt.fmt.pix.height) {
            ARLOGi("  Height requested: %d\n", vid->height);
            ARLOGi("  Height chosen:    %d\n", fmt.fmt.pix.height);
        } else {
            ARLOGi("  Height: %d\n", fmt.fmt.pix.height);
        }
        if (vid->palette != fmt.fmt.pix.pixelformat) {
            printPalette("  Palette requested: ", vid->palette);
            printPalette("  Palette chosen:    ", fmt.fmt.pix.pixelformat);
            vid->palette = fmt.fmt.pix.pixelformat;
        } else {
            printPalette("  Palette: ", fmt.fmt.pix.pixelformat);
        }
    }
    // Get actual camera settings.
    vid->palette = fmt.fmt.pix.pixelformat;
    vid->width = fmt.fmt.pix.width;
    vid->height = fmt.fmt.pix.height;
    
    // Determine format and setup buffer.
    switch (vid->palette) {
#ifdef AR_LITTLE_ENDIAN
        case (V4L2_PIX_FMT_BGR32): // deprecated
        case (V4L2_PIX_FMT_ABGR32): vid->format = AR_PIXEL_FORMAT_ARGB; break;
        case (V4L2_PIX_FMT_RGB32): // deprecated
        case (V4L2_PIX_FMT_ARGB32): vid->format = AR_PIXEL_FORMAT_BGRA; break;
#else
        case (V4L2_PIX_FMT_BGR32): // deprecated
        case (V4L2_PIX_FMT_ABGR32): vid->format = AR_PIXEL_FORMAT_BGRA; break;
        case (V4L2_PIX_FMT_RGB32): // deprecated
        case (V4L2_PIX_FMT_ARGB32): vid->format = AR_PIXEL_FORMAT_ARGB; break;
#endif
        case (V4L2_PIX_FMT_BGR24): vid->format = AR_PIXEL_FORMAT_BGR; break;
        case (V4L2_PIX_FMT_RGB24): vid->format = AR_PIXEL_FORMAT_RGB; break;
        case (V4L2_PIX_FMT_GREY): vid->format = AR_PIXEL_FORMAT_MONO; break;
        case (V4L2_PIX_FMT_YUYV): vid->format = AR_PIXEL_FORMAT_yuvs; break;    // https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-yuyv.html
        case (V4L2_PIX_FMT_UYVY): vid->format = AR_PIXEL_FORMAT_2vuy; break;    // https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-uyvy.html
        case (V4L2_PIX_FMT_NV12): vid->format = AR_PIXEL_FORMAT_420f; break;    // https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-nv12.html
        case (V4L2_PIX_FMT_NV21): vid->format = AR_PIXEL_FORMAT_NV21; break;    // https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-nv12.html
        case (V4L2_PIX_FMT_RGB565X): vid->format = AR_PIXEL_FORMAT_RGB_565; break;
        default: vid->format = AR_PIXEL_FORMAT_INVALID; break;
    }
    if (vid->format == AR_PIXEL_FORMAT_INVALID) {
        ARLOGe("Driver chose palette format unsupported by artoolkitX.\n");
        goto bail1;
    }
    if (vid->format == AR_PIXEL_FORMAT_420f || vid->format == AR_PIXEL_FORMAT_NV21) {
        vid->buffer.bufPlaneCount = 2;
        vid->buffer.bufPlanes = (ARUint8 **)calloc(vid->buffer.bufPlaneCount, sizeof(ARUint8 *));
        if (!vid->buffer.bufPlanes) {
            ARLOGe("Out of memory!\n");
            goto bail1;
        }
    } else {
        vid->buffer.bufPlaneCount = 0;
        vid->buffer.bufPlanes = NULL;
    }

    memset(&ipt, 0, sizeof(ipt));    
    ipt.index = vid->channel;
    ipt.std = vid->mode;
    
    if (xioctl(vid->fd, VIDIOC_ENUMINPUT, &ipt) < 0) {
        ARLOGe("arVideoOpen: Error querying input device type\n");
        goto bail1;
    }
    
    if (vid->debug) {
        if (ipt.type == V4L2_INPUT_TYPE_TUNER) {
            ARLOGi("  Type: Tuner\n");
        } else if (ipt.type == V4L2_INPUT_TYPE_CAMERA) {
            ARLOGi("  Type: Camera\n");
        } else {
            ARLOGi("  Type: Unknown\n");
        }
    }
    
    // Set channel
    if (xioctl(vid->fd, VIDIOC_S_INPUT, &ipt)) {
        ARLOGe("arVideoOpen: Error setting video input\n");
        goto bail1;
    }
    
    // Attempt to set some camera controls
    setControl(vid->fd, V4L2_CID_BRIGHTNESS, vid->brightness);
    setControl(vid->fd, V4L2_CID_CONTRAST, vid->contrast);
    setControl(vid->fd, V4L2_CID_SATURATION, vid->saturation);
    setControl(vid->fd, V4L2_CID_HUE, vid->hue);
    setControl(vid->fd, V4L2_CID_GAMMA, vid->gamma);
    setControl(vid->fd, V4L2_CID_EXPOSURE, vid->exposure);
    setControl(vid->fd, V4L2_CID_GAIN, vid->gain);
    
    // Print out current control values
    if (vid->debug) {
        if (!getControl(vid->fd, V4L2_CID_BRIGHTNESS, &value)) {
            ARLOGi("Brightness: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_CONTRAST, &value)) {
            ARLOGi("Contrast: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_SATURATION, &value)) {
            ARLOGi("Saturation: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_HUE, &value)) {
            ARLOGi("Hue: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAMMA, &value)) {
            ARLOGi("Gamma: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_EXPOSURE, &value)) {
            ARLOGi("Exposure: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAIN, &value)) {
            ARLOGi("Gain: %d\n", value);
        }
    }
    
    // If the user requested a pixel format conversion, allocate a buffer for that.
    if (vid->formatConverted != AR_PIXEL_FORMAT_INVALID) {
        if (vid->formatConverted == AR_PIXEL_FORMAT_RGBA || vid->formatConverted == AR_PIXEL_FORMAT_BGRA) {
            arMalloc(vid->bufferConverted.buff, ARUint8, vid->width*vid->height*4);
        } else {
            ARLOGe("Request for conversion to unsupported pixel format %s.\n", arVideoUtilGetPixelFormatName(vid->formatConverted));
            goto bail1;
        }
    }

    // Setup memory mapping
    memset(&req, 0, sizeof(req));
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(vid->fd, VIDIOC_REQBUFS, &req)) {
        ARLOGe("Error calling VIDIOC_REQBUFS\n");
        goto bail2;
    }
    
    if (req.count < 2) {
        ARLOGe("This device can not be supported by libARvideo.\n");
        ARLOGe("(req.count < 2)\n");
        goto bail2;
    }
    
    vid->internalBufferSet = (AR2VideoInternalBufferSetV4LT *)calloc(req.count , sizeof(AR2VideoInternalBufferSetV4LT));
    if (!vid->internalBufferSet) {
        ARLOGe("ar2VideoOpen: Error allocating buffer memory\n");
        goto bail2;
    }
    for (i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;
        
        if (xioctl(vid->fd, VIDIOC_QUERYBUF, &buf)) {
            ARLOGe("error VIDIOC_QUERYBUF\n");
            goto bail2;
        }
        
        // Keep our own record of the mapped region.
        vid->internalBufferSet[i].length = buf.length;
        vid->internalBufferSet[i].ptr = (uint8_t *)mmap(NULL /* start anywhere */,
                                                        buf.length,
                                                        PROT_READ | PROT_WRITE /* required */,
                                                        MAP_SHARED /* recommended */,
                                                        vid->fd, buf.m.offset);
        if (vid->internalBufferSet[i].ptr == MAP_FAILED) {
            ARLOGperror("mmap error");
            goto bail2;
        }
    }
    vid->internalBufferCount = i;
    
    vid->video_cont_num = -1;
    
    return vid;

bail2:
    free(vid->bufferConverted.buff);
bail1:
    close(vid->fd);
bail:
    free(vid->name);
    free(vid->device_id);
    free(vid->buffer.bufPlanes);
    free(vid);
    return (NULL);
}

int ar2VideoCloseV4L2(AR2VideoParamV4L2T *vid)
{
    int i;

    if (vid->video_cont_num >= 0) {
        ar2VideoCapStopV4L2(vid);
    }
    
    for (i = 0; i < vid->internalBufferCount; i++) {
        munmap(vid->internalBufferSet[i].ptr, vid->internalBufferSet[i].length);
    }
    
    free(vid->bufferConverted.buff);
    close(vid->fd);

#if USE_CPARAM_SEARCH
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
#endif

    free(vid->name);
    free(vid->device_id);
    free(vid->buffer.bufPlanes);
    free(vid);
    
    return 0;
}

int ar2VideoGetIdV4L2(AR2VideoParamV4L2T *vid, ARUint32 *id0, ARUint32 *id1)
{
    if (!vid) return -1;
    
    if (id0) *id0 = 0;
    if (id1) *id1 = 0;
    
    return -1;
}

int ar2VideoGetSizeV4L2(AR2VideoParamV4L2T *vid, int *x,int *y)
{
    if (!vid) return -1;
    
    if (x) *x = vid->width;
    if (y) *y = vid->height;
    
    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;
    
    if (vid->formatConverted != AR_PIXEL_FORMAT_INVALID) return (vid->formatConverted);
    else return (vid->format);
}

int ar2VideoCapStartV4L2(AR2VideoParamV4L2T *vid)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    int i;
    
    if (vid->video_cont_num >= 0) {
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }
    
    vid->video_cont_num = 0;
    
    for (i = 0; i < vid->internalBufferCount; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(vid->fd, VIDIOC_QBUF, &buf)) {
            ARLOGe("ar2VideoCapStart: Error calling VIDIOC_QBUF: %d\n", errno);
            return -1;
        }
    }
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(vid->fd, VIDIOC_STREAMON, &type)) {
        ARLOGe("ar2VideoCapStart: Error calling VIDIOC_STREAMON\n");
        return -1;
    }
    
    return 0;
}

int ar2VideoCapStopV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return -1;

    if (vid->video_cont_num < 0) {
        ARLOGe("arVideoCapStart has never been called.\n");
        return -1;
    }
    
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (xioctl(vid->fd, VIDIOC_STREAMOFF, &type)) {
        ARLOGe("Error calling VIDIOC_STREAMOFF\n");
        return -1;
    }
    
    vid->video_cont_num = -1;
    
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return NULL;
   
    if (vid->video_cont_num < 0) {
        ARLOGe("arVideoCapStart has never been called.\n");
        return NULL;
    }
    
    // Dequeue a buffer. We have a record of the mapped region in vid->internalBufferSet.
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(vid->fd, VIDIOC_DQBUF, &buf) < 0) {
        ARLOGe("Error calling VIDIOC_DQBUF: %d\n", errno);
        return NULL;
    }
    vid->video_cont_num = buf.index;
#ifdef AR2VIDEO_V4L2_DEBUG
    ARLOGd("v4l2_buffer.timestamp=(%ld, %d)\n", buf.timestamp.tv_sec, buf.timestamp.tv_usec);
#endif

    AR2VideoBufferT *ret = NULL;

    if (buf.timestamp.tv_sec >= 0) { // Only process buffers returned with valid timestamps.
        vid->buffer.buff = vid->internalBufferSet[buf.index].ptr;
        if (vid->format == AR_PIXEL_FORMAT_420f || vid->format == AR_PIXEL_FORMAT_NV21) {
            vid->buffer.bufPlanes[0] = vid->buffer.buff;
            vid->buffer.bufPlanes[1] = vid->buffer.buff + vid->width*vid->height;
            vid->buffer.buffLuma = vid->buffer.buff;
        } else if (vid->format == AR_PIXEL_FORMAT_MONO) {
            vid->buffer.buffLuma = vid->buffer.buff;
        } else {
            vid->buffer.buffLuma = NULL;
        }
        vid->buffer.time.sec = (uint64_t)(buf.timestamp.tv_sec);
        vid->buffer.time.usec = (uint32_t)(buf.timestamp.tv_usec);
        vid->buffer.fillFlag = 1;
    
        // Convert if the user asked for it.
        if (vid->formatConverted != AR_PIXEL_FORMAT_INVALID) {
            if (vid->formatConverted == AR_PIXEL_FORMAT_RGBA) {
                videoRGBA((uint32_t *)(vid->bufferConverted.buff), &vid->buffer, vid->width, vid->height, vid->format);
            } else if (vid->formatConverted == AR_PIXEL_FORMAT_BGRA) {
                videoBGRA((uint32_t *)(vid->bufferConverted.buff), &vid->buffer, vid->width, vid->height, vid->format);
            }
            vid->bufferConverted.time.sec = vid->buffer.time.sec;
            vid->bufferConverted.time.usec = vid->buffer.time.usec;
            vid->bufferConverted.fillFlag = 1;
            vid->bufferConverted.buffLuma = NULL;
            ret = &vid->bufferConverted;
        } else {
            ret = &vid->buffer;
        }
    }
    
    // Queue a buffer.
    struct v4l2_buffer buf_next;
    memset(&buf_next, 0, sizeof(buf_next));
    buf_next.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_next.memory = V4L2_MEMORY_MMAP;
    buf_next.index = vid->video_cont_num;
    if (xioctl(vid->fd, VIDIOC_QBUF, &buf_next)) {
        ARLOGe("ar2VideoCapNext: Error calling VIDIOC_QBUF: %d\n", errno);
    }

    return (ret);
}

int ar2VideoGetParamiV4L2(AR2VideoParamV4L2T *vid, int paramName, int *value)
{
    return -1;
}

int ar2VideoSetParamiV4L2(AR2VideoParamV4L2T *vid, int paramName, int  value)
{
    return -1;
}

int ar2VideoGetParamdV4L2(AR2VideoParamV4L2T *vid, int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdV4L2(AR2VideoParamV4L2T *vid, int paramName, double  value)
{
    return -1;
}

int ar2VideoGetParamsV4L2( AR2VideoParamV4L2T *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            *value = (vid->device_id ? strdup(vid->device_id) : NULL);
            break;
        case AR_VIDEO_PARAM_NAME:
            *value = (vid->name ? strdup(vid->name) : NULL);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsV4L2( AR2VideoParamV4L2T *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#if USE_CPARAM_SEARCH
static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamV4L2T *vid = (AR2VideoParamV4L2T *)userdata;
    if (!vid) return;
    
    switch (state) {
        case CPARAM_SEARCH_STATE_INITIAL:
        case CPARAM_SEARCH_STATE_IN_PROGRESS:
            break;
        case CPARAM_SEARCH_STATE_RESULT_NULL:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_OK:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(cparam, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_FAILED_NO_NETWORK:
            ARLOGe("Error during cparamSearch. Internet connection unavailable.\n");
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        default: // Errors.
            ARLOGe("Error %d returned from cparamSearch.\n", (int)state);
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
    }
    if (final) vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
}

int ar2VideoGetCParamAsyncV4L2(AR2VideoParamV4L2T *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1);
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }
    
    if (!vid->device_id) {
        ARLOGe("Error: cparamSearch cannot proceed without device identification.\n");
        return (-1);
    }
    
    int camera_index = 0;
    float focal_length = 0.0f;
    int width = 0, height = 0;
    if (ar2VideoGetSizeV4L2(vid, &width, &height) < 0) {
        ARLOGe("Error: request for camera parameters, but video size is unknown.\n");
        return (-1);
    };
    
    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;
    
    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, camera_index, width, height, focal_length, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
        return (-1);
    }
    
    return (0);
}

#endif // USE_CPARAM_SEARCH

#endif // ARVIDEO_INPUT_V4L2
