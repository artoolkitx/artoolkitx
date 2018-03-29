/*
 * 1394-Based Digital Camera Control Library
 *
 * A few type definitions
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

#ifndef __DC1394_TYPES_H__
#define __DC1394_TYPES_H__

/*! \file dc1394/types.h
    \brief Various types that must be defined here

    More details soon
*/

#include <stdint.h>

/**
 * Enumeration of video modes. Note that the notion of IIDC "format" is not present here, except in the format_7 name.
 */
typedef enum {
    DC1394_VIDEO_MODE_160x120_YUV444= 64,
    DC1394_VIDEO_MODE_320x240_YUV422,
    DC1394_VIDEO_MODE_640x480_YUV411,
    DC1394_VIDEO_MODE_640x480_YUV422,
    DC1394_VIDEO_MODE_640x480_RGB8,
    DC1394_VIDEO_MODE_640x480_MONO8,
    DC1394_VIDEO_MODE_640x480_MONO16,
    DC1394_VIDEO_MODE_800x600_YUV422,
    DC1394_VIDEO_MODE_800x600_RGB8,
    DC1394_VIDEO_MODE_800x600_MONO8,
    DC1394_VIDEO_MODE_1024x768_YUV422,
    DC1394_VIDEO_MODE_1024x768_RGB8,
    DC1394_VIDEO_MODE_1024x768_MONO8,
    DC1394_VIDEO_MODE_800x600_MONO16,
    DC1394_VIDEO_MODE_1024x768_MONO16,
    DC1394_VIDEO_MODE_1280x960_YUV422,
    DC1394_VIDEO_MODE_1280x960_RGB8,
    DC1394_VIDEO_MODE_1280x960_MONO8,
    DC1394_VIDEO_MODE_1600x1200_YUV422,
    DC1394_VIDEO_MODE_1600x1200_RGB8,
    DC1394_VIDEO_MODE_1600x1200_MONO8,
    DC1394_VIDEO_MODE_1280x960_MONO16,
    DC1394_VIDEO_MODE_1600x1200_MONO16,
    DC1394_VIDEO_MODE_EXIF,
    DC1394_VIDEO_MODE_FORMAT7_0,
    DC1394_VIDEO_MODE_FORMAT7_1,
    DC1394_VIDEO_MODE_FORMAT7_2,
    DC1394_VIDEO_MODE_FORMAT7_3,
    DC1394_VIDEO_MODE_FORMAT7_4,
    DC1394_VIDEO_MODE_FORMAT7_5,
    DC1394_VIDEO_MODE_FORMAT7_6,
    DC1394_VIDEO_MODE_FORMAT7_7
} dc1394video_mode_t;
#define DC1394_VIDEO_MODE_MIN            DC1394_VIDEO_MODE_160x120_YUV444
#define DC1394_VIDEO_MODE_MAX       DC1394_VIDEO_MODE_FORMAT7_7
#define DC1394_VIDEO_MODE_NUM      (DC1394_VIDEO_MODE_MAX - DC1394_VIDEO_MODE_MIN + 1)

/* Special min/max are defined for Format_7 */
#define DC1394_VIDEO_MODE_FORMAT7_MIN       DC1394_VIDEO_MODE_FORMAT7_0
#define DC1394_VIDEO_MODE_FORMAT7_MAX       DC1394_VIDEO_MODE_FORMAT7_7
#define DC1394_VIDEO_MODE_FORMAT7_NUM      (DC1394_VIDEO_MODE_FORMAT7_MAX - DC1394_VIDEO_MODE_FORMAT7_MIN + 1)

/**
 * Enumeration of colour codings. For details on the data format please read the IIDC specifications.
 */
typedef enum {
    DC1394_COLOR_CODING_MONO8= 352,
    DC1394_COLOR_CODING_YUV411,
    DC1394_COLOR_CODING_YUV422,
    DC1394_COLOR_CODING_YUV444,
    DC1394_COLOR_CODING_RGB8,
    DC1394_COLOR_CODING_MONO16,
    DC1394_COLOR_CODING_RGB16,
    DC1394_COLOR_CODING_MONO16S,
    DC1394_COLOR_CODING_RGB16S,
    DC1394_COLOR_CODING_RAW8,
    DC1394_COLOR_CODING_RAW16
} dc1394color_coding_t;
#define DC1394_COLOR_CODING_MIN     DC1394_COLOR_CODING_MONO8
#define DC1394_COLOR_CODING_MAX     DC1394_COLOR_CODING_RAW16
#define DC1394_COLOR_CODING_NUM    (DC1394_COLOR_CODING_MAX - DC1394_COLOR_CODING_MIN + 1)

/**
 * RAW sensor filters. These elementary tiles tesselate the image plane in RAW modes. RGGB should be interpreted in 2D as
 *
 *    RG
 *    GB
 *
 * and similarly for other filters.
 */
typedef enum {
    DC1394_COLOR_FILTER_RGGB = 512,
    DC1394_COLOR_FILTER_GBRG,
    DC1394_COLOR_FILTER_GRBG,
    DC1394_COLOR_FILTER_BGGR
} dc1394color_filter_t;
#define DC1394_COLOR_FILTER_MIN        DC1394_COLOR_FILTER_RGGB
#define DC1394_COLOR_FILTER_MAX        DC1394_COLOR_FILTER_BGGR
#define DC1394_COLOR_FILTER_NUM       (DC1394_COLOR_FILTER_MAX - DC1394_COLOR_FILTER_MIN + 1)

/**
 * Byte order for YUV formats (may be expanded to RGB in the future)
 *
 * IIDC cameras always return data in UYVY order, but conversion functions can change this if requested.
 */
typedef enum {
    DC1394_BYTE_ORDER_UYVY=800,
    DC1394_BYTE_ORDER_YUYV
} dc1394byte_order_t;
#define DC1394_BYTE_ORDER_MIN        DC1394_BYTE_ORDER_UYVY
#define DC1394_BYTE_ORDER_MAX        DC1394_BYTE_ORDER_YUYV
#define DC1394_BYTE_ORDER_NUM       (DC1394_BYTE_ORDER_MAX - DC1394_BYTE_ORDER_MIN + 1)

/**
 * A struct containing a list of color codings
 */
typedef struct
{
    uint32_t                num;
    dc1394color_coding_t    codings[DC1394_COLOR_CODING_NUM];
} dc1394color_codings_t;

/**
 * A struct containing a list of video modes
 */
typedef struct
{
    uint32_t                num;
    dc1394video_mode_t      modes[DC1394_VIDEO_MODE_NUM];
} dc1394video_modes_t;

/**
 * Yet another boolean data type
 */
typedef enum {
    DC1394_FALSE= 0,
    DC1394_TRUE
} dc1394bool_t;

/**
 * Yet another boolean data type, a bit more oriented towards electrical-engineers
 */
typedef enum {
    DC1394_OFF= 0,
    DC1394_ON
} dc1394switch_t;


#endif
