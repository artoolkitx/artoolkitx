/*
 * 1394-Based Digital Camera Control Library
 *
 * Utilities
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

#ifndef __DC1394_UTILS_H__
#define __DC1394_UTILS_H__

/*! \file dc1394/utils.h
    \brief Utility functions
    \author Damien Douxchamps: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the image width and height (in pixels) corresponding to a video mode. Works for scalable and non-scalable video modes.
 */
dc1394error_t dc1394_get_image_size_from_video_mode(dc1394camera_t *camera, uint32_t video_mode, uint32_t *width, uint32_t *height);

/**
 * Returns the given framerate as a float.
 */
dc1394error_t dc1394_framerate_as_float(dc1394framerate_t framerate_enum, float *framerate);

/**
 * Returns the number of bits per pixel for a certain color coding. This is the size of the data sent on the bus, the effective
 * data depth may vary. Example: RGB16 is 16, YUV411 is 8, YUV422 is 8.
 */
dc1394error_t dc1394_get_color_coding_data_depth(dc1394color_coding_t color_coding, uint32_t * bits);

/**
 * Returns the bit-space used by a pixel. This is different from the data depth! For instance, RGB16 has a bit space of 48 bits,
 * YUV422 is 16bits and YU411 is 12bits. 
 */
dc1394error_t dc1394_get_color_coding_bit_size(dc1394color_coding_t color_coding, uint32_t* bits);

/**
 * Returns the color coding from the video mode. Works with scalable image formats too.
 */
dc1394error_t dc1394_get_color_coding_from_video_mode(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding);

/**
 * Tells whether the color mode is color or monochrome
 */
dc1394error_t dc1394_is_color(dc1394color_coding_t color_mode, dc1394bool_t *is_color);

/**
 * Tells whether the video mode is scalable or not.
 */
dc1394bool_t dc1394_is_video_mode_scalable(dc1394video_mode_t video_mode);

/**
 * Tells whether the video mode is "still image" or not ("still image" is currently not supported by any cameras on the market)
 */
dc1394bool_t dc1394_is_video_mode_still_image(dc1394video_mode_t video_mode);

/**
 * Tells whether two IDs refer to the same physical camera unit.
 */
dc1394bool_t dc1394_is_same_camera(dc1394camera_id_t id1, dc1394camera_id_t id2);

/**
 * Returns a descriptive name for a feature 
 */
const char * dc1394_feature_get_string(dc1394feature_t feature);

/**
 * Returns a descriptive string for an error code
 */
const char * dc1394_error_get_string(dc1394error_t error);

/**
 * Calculates the CRC16 checksum of a memory region. Useful to verify the CRC of an image buffer, for instance.
 */
uint16_t dc1394_checksum_crc16 (const uint8_t* buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* _DC1394_UTILS_H */

