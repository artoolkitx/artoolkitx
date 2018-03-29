/*
 * 1394-Based Digital Camera Control Library
 *
 * Format_7 functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <dc1394/log.h>
#include <dc1394/video.h>

#ifndef __DC1394_FORMAT7_H__
#define __DC1394_FORMAT7_H__

/*! \file dc1394/format7.h
    \brief Functions to control Format_7 (aka scalable format, ROI)

    More details soon
*/

/**
 * A struct containing information about a mode of Format_7, the scalable image format.
 */
typedef struct __dc1394format7mode_t
{
    dc1394bool_t present;

    uint32_t size_x;
    uint32_t size_y;
    uint32_t max_size_x;
    uint32_t max_size_y;

    uint32_t pos_x;
    uint32_t pos_y;

    uint32_t unit_size_x;
    uint32_t unit_size_y;
    uint32_t unit_pos_x;
    uint32_t unit_pos_y;

    dc1394color_codings_t color_codings;
    dc1394color_coding_t color_coding;

    uint32_t pixnum;

    uint32_t packet_size; /* in bytes */
    uint32_t unit_packet_size;
    uint32_t max_packet_size;

    uint64_t total_bytes;

    dc1394color_filter_t color_filter;

} dc1394format7mode_t;

/**
 * A struct containing the list of Format_7 modes.
 * FIXME: this may become very big if format_7 pages are used in IIDC 1.32. It would be better to use a "num" and an allocated list.
 */
typedef struct __dc1394format7modeset_t
{
    dc1394format7mode_t mode[DC1394_VIDEO_MODE_FORMAT7_NUM];
} dc1394format7modeset_t;

/* Parameter flags for dc1394_setup_format7_capture() */
#define DC1394_QUERY_FROM_CAMERA -1
#define DC1394_USE_MAX_AVAIL     -2
#define DC1394_USE_RECOMMENDED   -3

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Format_7 (scalable image format)
 ***************************************************************************/

/* image size */

/**
 * Gets the maximal image size for a given mode.
 */
dc1394error_t dc1394_format7_get_max_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_size,uint32_t *v_size);

/**
 * Gets the unit sizes for a given mode. The image size can only be a multiple of the unit size, and cannot be smaller than it.
 */
dc1394error_t dc1394_format7_get_unit_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_unit, uint32_t *v_unit);

/**
 * Gets the current image size.
 */
dc1394error_t dc1394_format7_get_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *width, uint32_t *height);

/**
 * Sets the current image size
 */
dc1394error_t dc1394_format7_set_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t width, uint32_t height);

/* image position */

/**
 * Gets the current image position
 */
dc1394error_t dc1394_format7_get_image_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *left, uint32_t *top);

/**
 * Sets the current image position
 */
dc1394error_t dc1394_format7_set_image_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t left, uint32_t top);

/**
 * Gets the unit positions for a given mode. The image position can only be a multiple of the unit position (zero is acceptable).
 */
dc1394error_t dc1394_format7_get_unit_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_unit_pos, uint32_t *v_unit_pos);

/* color coding */

/**
 * Gets the current color coding
 */
dc1394error_t dc1394_format7_get_color_coding(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding);

/**
 * Gets the list of color codings available for this mode
 */
dc1394error_t dc1394_format7_get_color_codings(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_codings_t *codings);

/**
 * Sets the current color coding
 */
dc1394error_t dc1394_format7_set_color_coding(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t color_coding);

/**
 * Gets the current color filter
 */
dc1394error_t dc1394_format7_get_color_filter(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_filter_t *color_filter);

/* packet */

/**
 * Get the parameters of the packet size: its maximal size and its unit size. The packet size is always a multiple of the unit bytes and cannot be zero.
 */
dc1394error_t dc1394_format7_get_packet_parameters(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *unit_bytes, uint32_t *max_bytes);

/**
 * Gets the current packet size
 */
dc1394error_t dc1394_format7_get_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *packet_size);

/**
 * Sets the current packet size
 */
dc1394error_t dc1394_format7_set_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t packet_size);

/**
 * Gets the recommended packet size. Ignore if zero.
 */
dc1394error_t dc1394_format7_get_recommended_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *packet_size);

/**
 * Gets the number of packets per frame.
 */
dc1394error_t dc1394_format7_get_packets_per_frame(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *ppf);

/* other */

/**
 * Gets the data depth (e.g. 12, 13, 14 bits/pixel)
 */
dc1394error_t dc1394_format7_get_data_depth(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *data_depth);

/**
 * Gets the frame interval in float format
 */
dc1394error_t dc1394_format7_get_frame_interval(dc1394camera_t *camera, dc1394video_mode_t video_mode, float *interval);

/**
 * Gets the number of pixels per image frame
 */
dc1394error_t dc1394_format7_get_pixel_number(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *pixnum);

/**
 * Get the total number of bytes per frame. This includes padding (to reach an entire number of packets)
 */
dc1394error_t dc1394_format7_get_total_bytes(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint64_t *total_bytes);

/* These functions get the properties of (one or all) format7 mode(s) */

/**
 * Gets the properties of all Format_7 modes supported by the camera.
 */
dc1394error_t dc1394_format7_get_modeset(dc1394camera_t *camera, dc1394format7modeset_t *info);

/**
 * Gets the properties of a Format_7 mode
 */
dc1394error_t dc1394_format7_get_mode_info(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394format7mode_t *f7_mode);

/**
 * Joint function that fully sets a certain ROI taking all parameters into account.
 * Note that this function does not SWITCH to the video mode passed as argument, it mearly sets it
 */
dc1394error_t dc1394_format7_set_roi(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t color_coding,
                                     int32_t packet_size, int32_t left, int32_t top, int32_t width, int32_t height);

/**
 * Joint function that fully gets a certain ROI taking all parameters into account.
 */
dc1394error_t dc1394_format7_get_roi(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding,
                                     uint32_t *packet_size, uint32_t *left, uint32_t *top, uint32_t *width, uint32_t *height);

#ifdef __cplusplus
}
#endif

#endif
