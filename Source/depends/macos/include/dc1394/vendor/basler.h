/*
 * 1394-Based Digital Camera Control Library
 *
 * Basler Smart Feature Framework specific extensions
 * 
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 *
 * Copyright (C) 2006 Tobii Technology AB, Stockholm Sweden
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

#ifndef __DC1394_VENDOR_BASLER_H__
#define __DC1394_VENDOR_BASLER_H__

#include "basler_sff.h"

/*! \file dc1394/vendor/basler.h
    \brief No docs yet

    More details soon
*/


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tests whether the camera supports Basler SFF
 */
dc1394error_t dc1394_basler_sff_is_available (dc1394camera_t* camera, dc1394bool_t *available);

/**
 * Tests whether the camera supports the specified SFF feature
 */
dc1394error_t dc1394_basler_sff_feature_is_available (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394bool_t *available);

/**
 * Enables or disables a specific feature
 */
dc1394error_t dc1394_basler_sff_feature_enable (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394switch_t on_off);

/**
 * checks if a feature has been enabled
 */
dc1394error_t dc1394_basler_sff_feature_is_enabled (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394bool_t *is_enabled);

/**
 * Checks the crc checksum of the supplied frame
 */
dc1394bool_t dc1394_basler_sff_check_crc (const uint8_t* frame_buffer, uint32_t frame_size);

/**
 * Initializes an iteration
 */
dc1394error_t dc1394_basler_sff_chunk_iterate_init (dc1394basler_sff_t* chunk, void *frame_buffer, uint32_t frame_size, dc1394bool_t has_crc_checksum);

/**
 * Iterates over the available SFF chunks in the frame buffer
 */
dc1394error_t dc1394_basler_sff_chunk_iterate (dc1394basler_sff_t* chunk);

/**
 * Finds a specific SFF chunk in the frame buffer
 */
dc1394error_t dc1394_basler_sff_chunk_find (dc1394basler_sff_feature_t feature_id, void** chunk_data, void* frame_buffer, uint32_t frame_size, dc1394bool_t has_crc_checksum);

/**
 * prints info about one feature
 */
dc1394error_t dc1394_basler_sff_feature_print (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, FILE *fd);

/**
 * prints info about all features
 */
dc1394error_t dc1394_basler_sff_feature_print_all (dc1394camera_t* camera, FILE *fd);

#ifdef __cplusplus
}
#endif

#endif
