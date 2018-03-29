/*
 * 1394-Based Digital Camera Control Library
 *
 * Video format headers
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dc1394/log.h>

/*! \file dc1394/video.h
    \brief Functions related to video modes, formats, framerate and video flow.

    More details soon
*/

#ifndef __DC1394_VIDEO_H__
#define __DC1394_VIDEO_H__

/**
 * Enumeration of iso data speeds
 *
 * Most (if not all) cameras are compatible with 400Mbps speed. Only older cameras (pre-1999) may still only work at sub-400
 * speeds. However, speeds lower than 400Mbps are still useful: they can be used for longer distances (e.g. 10m cables).
 * Speeds over 400Mbps are only available in "B" mode (DC1394_OPERATION_MODE_1394B).
 */
typedef enum {
    DC1394_ISO_SPEED_100= 0,
    DC1394_ISO_SPEED_200,
    DC1394_ISO_SPEED_400,
    DC1394_ISO_SPEED_800,
    DC1394_ISO_SPEED_1600,
    DC1394_ISO_SPEED_3200
} dc1394speed_t;
#define DC1394_ISO_SPEED_MIN                   DC1394_ISO_SPEED_100
#define DC1394_ISO_SPEED_MAX                   DC1394_ISO_SPEED_3200
#define DC1394_ISO_SPEED_NUM                  (DC1394_ISO_SPEED_MAX - DC1394_ISO_SPEED_MIN + 1)

/**
 * Enumeration of video framerates
 *
 * This enumeration is used for non-Format_7 modes. The framerate can be lower than expected if the exposure time is longer
 * than the requested frame period. Framerate can be controlled in a number of other ways: framerate feature, external trigger,
 * software trigger, shutter throttling and packet size (Format_7)
 */
typedef enum {
    DC1394_FRAMERATE_1_875= 32,
    DC1394_FRAMERATE_3_75,
    DC1394_FRAMERATE_7_5,
    DC1394_FRAMERATE_15,
    DC1394_FRAMERATE_30,
    DC1394_FRAMERATE_60,
    DC1394_FRAMERATE_120,
    DC1394_FRAMERATE_240
} dc1394framerate_t;
#define DC1394_FRAMERATE_MIN               DC1394_FRAMERATE_1_875
#define DC1394_FRAMERATE_MAX               DC1394_FRAMERATE_240
#define DC1394_FRAMERATE_NUM              (DC1394_FRAMERATE_MAX - DC1394_FRAMERATE_MIN + 1)

/**
 * Operation modes
 *
 * Two operation modes exist: the legacy and most common 1394a, and the newer 1394B. The latter allows speeds over 400Mbps, but
 * can also be used at other speeds.
 */
typedef enum {
    DC1394_OPERATION_MODE_LEGACY = 480,
    DC1394_OPERATION_MODE_1394B
} dc1394operation_mode_t;
#define DC1394_OPERATION_MODE_MIN    DC1394_OPERATION_MODE_LEGACY
#define DC1394_OPERATION_MODE_MAX    DC1394_OPERATION_MODE_1394B
#define DC1394_OPERATION_MODE_NUM   (DC1394_OPERATION_MODE_MAX - DC1394_OPERATION_MODE_MIN + 1)

/**
 * List of framerates
 *
 * dc1394framerates_t contains a list of available framerates for a particular video mode.
 */
typedef struct {
    uint32_t                num;
    dc1394framerate_t       framerates[DC1394_FRAMERATE_NUM];
} dc1394framerates_t;

/**
 * Video frame structure.
 *
 * dc1394video_frame_t is the structure returned by the capture functions. It contains the captured image as well as a number of
 * information. 
 *
 * In general this structure should be calloc'ed so that members such as "allocated size"
 * are properly set to zero. Don't forget to free the "image" member before freeing the struct itself.
 */
typedef struct __dc1394_video_frame
{
    unsigned char          * image;                 /* the image. May contain padding data too (vendor specific). Read/write allowed. Free NOT allowed if
						       returned by dc1394_capture_dequeue() */
    uint32_t                 size[2];               /* the image size [width, height] */
    uint32_t                 position[2];           /* the WOI/ROI position [horizontal, vertical] == [0,0] for full frame */
    dc1394color_coding_t     color_coding;          /* the color coding used. This field is valid for all video modes. */
    dc1394color_filter_t     color_filter;          /* the color filter used. This field is valid only for RAW modes and IIDC 1.31 */
    uint32_t                 yuv_byte_order;        /* the order of the fields for 422 formats: YUYV or UYVY */
    uint32_t                 data_depth;            /* the number of bits per pixel. The number of grayscale levels is 2^(this_number).
                                                       This is independent from the colour coding */
    uint32_t                 stride;                /* the number of bytes per image line */
    dc1394video_mode_t       video_mode;            /* the video mode used for capturing this frame */
    uint64_t                 total_bytes;           /* the total size of the frame buffer in bytes. May include packet-
                                                       multiple padding and intentional padding (vendor specific) */
    uint32_t                 image_bytes;           /* the number of bytes used for the image (image data only, no padding) */
    uint32_t                 padding_bytes;         /* the number of extra bytes, i.e. total_bytes-image_bytes.  */
    uint32_t                 packet_size;           /* the size of a packet in bytes. (IIDC data) */
    uint32_t                 packets_per_frame;     /* the number of packets per frame. (IIDC data) */
    uint64_t                 timestamp;             /* the unix time [microseconds] at which the frame was captured in
                                                       the video1394 ringbuffer */
    uint32_t                 frames_behind;         /* the number of frames in the ring buffer that are yet to be accessed by the user */
    dc1394camera_t           *camera;               /* the parent camera of this frame */
    uint32_t                 id;                    /* the frame position in the ring buffer */
    uint64_t                 allocated_image_bytes; /* amount of memory allocated in for the *image field. */
    dc1394bool_t             little_endian;         /* DC1394_TRUE if little endian (16bpp modes only),
                                                       DC1394_FALSE otherwise */
    dc1394bool_t             data_in_padding;       /* DC1394_TRUE if data is present in the padding bytes in IIDC 1.32 format,
                                                       DC1394_FALSE otherwise */
} dc1394video_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Video functions: formats, framerates,...
 ***************************************************************************/

/**
 * Gets a list of video modes supported by the camera.
 */
dc1394error_t dc1394_video_get_supported_modes(dc1394camera_t *camera, dc1394video_modes_t *video_modes);

/**
 * Gets a list of supported video framerates for a given video mode. This function only works with non-scalable formats.
 */
dc1394error_t dc1394_video_get_supported_framerates(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394framerates_t *framerates);

/**
 * Gets the current framerate. This is meaningful only if the video mode is not scalable.
 */
dc1394error_t dc1394_video_get_framerate(dc1394camera_t *camera, dc1394framerate_t *framerate);

/**
 * Sets the current framerate. This is meaningful only if the video mode is not scalable.
 */
dc1394error_t dc1394_video_set_framerate(dc1394camera_t *camera, dc1394framerate_t framerate);

/**
 * Gets the current vide mode.
 */
dc1394error_t dc1394_video_get_mode(dc1394camera_t *camera, dc1394video_mode_t *video_mode);

/**
 * Sets the current vide mode.
 */
dc1394error_t dc1394_video_set_mode(dc1394camera_t *camera, dc1394video_mode_t video_mode);

/**
 * Gets the current operation mode.
 */
dc1394error_t dc1394_video_get_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t *mode);

/**
 * Sets the current operation mode.
 */
dc1394error_t dc1394_video_set_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t mode);

/**
 * Gets the current ISO speed.
 */
dc1394error_t dc1394_video_get_iso_speed(dc1394camera_t *camera, dc1394speed_t *speed);

/**
 * Sets the current ISO speed. Speeds over 400Mbps require 1394B.
 */
dc1394error_t dc1394_video_set_iso_speed(dc1394camera_t *camera, dc1394speed_t speed);

/**
 * Gets the current ISO channel
 */
dc1394error_t dc1394_video_get_iso_channel(dc1394camera_t *camera, uint32_t * channel);

/**
 * Sets the current ISO channel
 */
dc1394error_t dc1394_video_set_iso_channel(dc1394camera_t *camera, uint32_t channel);

/**
 * Gets the current data depth, in bits. Only meaningful for 16bpp video modes (RAW16, RGB48, MONO16,...)  
 */
dc1394error_t dc1394_video_get_data_depth(dc1394camera_t *camera, uint32_t *depth);

/**
 * Starts/stops the isochronous data transmission. In other words, use this to control the image flow.
 */
dc1394error_t dc1394_video_set_transmission(dc1394camera_t *camera, dc1394switch_t pwr);

/**
 * Gets the status of the video transmission
 */
dc1394error_t dc1394_video_get_transmission(dc1394camera_t *camera, dc1394switch_t *pwr);

/**
 * Turns one-shot mode on or off
 */
dc1394error_t dc1394_video_set_one_shot(dc1394camera_t *camera, dc1394switch_t pwr);

/**
 * Gets the status of the one-shot mode.
 */
dc1394error_t dc1394_video_get_one_shot(dc1394camera_t *camera, dc1394bool_t *is_on);

/**
 * Turns multishot mode on or off
 */
dc1394error_t dc1394_video_set_multi_shot(dc1394camera_t *camera, uint32_t numFrames, dc1394switch_t pwr);

/**
 * Gets the status of the multi-shot mode.
 */
dc1394error_t dc1394_video_get_multi_shot(dc1394camera_t *camera, dc1394bool_t *is_on, uint32_t *numFrames);

/**
 * Gets the bandwidth usage of a camera.
 *
 * This function returns the bandwidth that is used by the camera *IF* ISO was ON.
 * The returned value is in bandwidth units. The 1394 bus has 4915 bandwidth units
 * available per cycle. Each unit corresponds to the time it takes to send one
 * quadlet at ISO speed S1600. The bandwidth usage at S400 is thus four times the
 * number of quadlets per packet. Thanks to Krisitian Hogsberg for clarifying this.
 */
dc1394error_t dc1394_video_get_bandwidth_usage(dc1394camera_t *camera, uint32_t *bandwidth);

#ifdef __cplusplus
}
#endif

#endif
