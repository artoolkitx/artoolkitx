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

#ifndef __DC1394_VENDOR_BASLER_SFF_H__
#define __DC1394_VENDOR_BASLER_SFF_H__

#include <stdint.h>
/*! \file dc1394/vendor/basler_sff.h
    \brief No docs yet

    More details soon
*/

/**
 * \struct dc1394basler_sff_guid_t
 * Basler SFF Guid struct, this structure is used to identify chunks and to
 * request features from the camera, is basically just a normal GUID value
 */
typedef struct __dc1394basler_sff_guid_t {
  uint32_t d1;
  uint16_t d2, d3;
  uint8_t  d4[8];
} dc1394basler_sff_guid_t;

/**
 * \struct dc1394basler_sff_chunk_tail_t
 * SFF chunks are read from the end to the beginning (that is backwards),
 * each chunk ends in a chunk-tail, which contains information about the
 * size of the chunk as well as the GUID used to identify which chunk this
 * really is.
 */
typedef struct __dc1394basler_sff_chunk_tail_t {
  dc1394basler_sff_guid_t chunk_guid; /**< the chunk GUID, this is different from
             the CSR GUID used to request the feature */
  uint32_t chunk_size; /**< the size of this chunk, including this struct */
  uint32_t inverted_chunk_size; /**< the bitwise complement of the chunk size */
} dc1394basler_sff_chunk_tail_t;

/**
 * SFF feature IDs
 */
typedef enum dc1394basler_sff_feature_t {
  DC1394_BASLER_SFF_FEATURE_MIN = 0,

  /**
   * SFF Extended data stream, this feature must be enabled
   * in order to use any other smart feature.
   * Please refer to struct dc1394basler_sff_extended_data_stream
   * for more information.
    */
  DC1394_BASLER_SFF_EXTENDED_DATA_STREAM = 0,

  /**
   * The frame counter feature numbers images sequentially as they
   * are captured, the counter starts at 0 and wraps at 2^32-1.
   * The counter increments by one for each captured frame.
   * Whenever the camera is powered off, the counter resets to 0.
   * Please refer to struct dc1394basler_sff_frame_counter_t
   * for more information.
   */
  DC1394_BASLER_SFF_FRAME_COUNTER,

  /**
   * The cycle time stamp feature adds a chunk to each image frame
   * containing the value of the IEEE1394 bus cycle timer.
   * These counters are sampled at the start of exposure of each image.
   * Please refer to struct dc1394basler_sff_cycle_time_stamp
   * for more information.
   */
  DC1394_BASLER_SFF_CYCLE_TIME_STAMP,

  /**
   * The DCAM values smart features adds a chunk to each image
   * containing the current settings for some standard DCAM features.
   * The settings are sampled at the start of exposure of each image.
   * Please refer to struct dc1394basler_sff_dcam_values_t
   * for more information.
   */
  DC1394_BASLER_SFF_DCAM_VALUES,

  /**
   * The CRC checksum feature adds a chunk to each image frame
   * containing a 16bit CRC checksum computed using the Z-modem
   * algorithm. The checksum is computed for all the image data
   * and all other SFF chunks except the CRC checksum chunk.
   * Please refer to the function dc1394_basler_validate_checksum()
   * for more information
   */
  DC1394_BASLER_SFF_CRC_CHECKSUM,

  /**
   * The test images feature is used to check the camera's basic
   * functionality and its ability to transmit an image via
   * the video data cable. The test image can be used for
   * service purposes and for failure diagnostics. In test mode
   * the image is generated with a software program and the camera's
   * digital devices and does not use the optics the pixel array
   * or the ADCs.
   *
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_TEST_IMAGES,

  /**
   * Basler cameras include a register that contains version numbers
   * for the camera's internal software. For troubleshooting
   * purposes, Basler technical support may ask you to read
   * this register and to supply the results.
   *
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_EXTENDED_VERSION_INFO,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_LOOKUP_TABLE,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_TRIGGER_FLAG_AND_COUNTER,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_OUTPUT_PORT_0_CONFIGURATION,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_OUTPUT_PORT_1_CONFIGURATION,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_OUTPUT_PORT_2_CONFIGURATION,

  /**
   * <b>This feature is not implemented</b>
   */
  DC1394_BASLER_SFF_OUTPUT_PORT_3_CONFIGURATION,

  DC1394_BASLER_SFF_FEATURE_MAX
} dc1394basler_sff_feature_t;

/**
 * \struct dc1394basler_sff_t
 * Data type used by this API to define SFFs and also when iterating
 */
typedef struct __dc1394basler_sff_t {
  dc1394basler_sff_feature_t feature_id;

  /* the following members are used for iterating */
  void* frame_buffer;
  uint32_t frame_size;
  void* current_iter;

  /* when iterating this member points to the beginning
   * to the data for this chunk, in order to use it should
   * be casted to a pointer of the type for this feature */
  void* chunk_data;
} dc1394basler_sff_t;

/**
 * This structure is used to capture the SFF extended data stream chunk.
 * According to the Basler manuals the extended data stream chunk
 * also contains to members pixel_data and gap of variable size;
 * these members are ignored in this API because they can be obtained
 * from other sources. The pixel_data member which is the actual image frame
 * is all data from the beginning of the frame buffer until width*height*bytes_per_pixel
 * bytes. The gap is required on some cameras for technical reason but not used
 * otherwise. The size of the gap can be computed by computing
 * frame_size - sizeof all chunks - image_size.
 */
typedef struct dc1394basler_sff_extended_data_stream_t {
  /* pixel_data and gap members ignored, because they are not needed in this context */
  uint32_t stride;
  uint8_t reserved3[3];
  uint8_t data_depth;
  uint16_t top;
  uint16_t left;
  uint16_t height;
  uint16_t width;
  uint8_t reserved2[3];
  uint8_t color_coding_id;
  uint8_t reserved1[3];
  uint8_t color_filter_id;
  dc1394basler_sff_chunk_tail_t tail;
} dc1394basler_sff_extended_data_stream_t;

/**
 * No Docs
 */
typedef struct dc1394basler_sff_frame_counter_t {
  uint32_t counter;
  dc1394basler_sff_chunk_tail_t tail;
} dc1394basler_sff_frame_counter_t;

/**
 * No Docs
 */
typedef struct dc1394basler_sff_cycle_time_stamp_t {
  union {
    struct {
      uint32_t cycle_offset: 12;
      uint32_t cycle_count:  13;
      uint32_t second_count:  7;
    } structured;
    struct {
      uint32_t value;
    } unstructured;
  } cycle_time_stamp;
} dc1394basler_sff_cycle_time_stamp_t;

/**
 * No Docs
 */
typedef struct dc1394basler_dcam_csr_value_t {
  uint32_t value: 12;
  uint32_t reserved2: 12;
  uint32_t a_m_mode: 1;
  uint32_t on_off: 1;
  uint32_t one_push: 1;
  uint32_t reserved1: 2;
  uint32_t abs_control: 1;
  uint32_t presence_inq: 1;
} dc1394basler_dcam_csr_value_t;

/**
 * No Docs
 */
typedef struct dc1394basler_dcam_whitebalance_csr_value_t {
  uint32_t v_r_value: 12;
  uint32_t u_b_value: 12;
  uint32_t a_m_mode: 1;
  uint32_t on_off: 1;
  uint32_t one_push: 1;
  uint32_t reserved1: 2;
  uint32_t abs_control: 1;
  uint32_t presence_inq: 1;
} dc1394basler_dcam_whitebalance_csr_value_t;

/**
 * No Docs
 */
typedef struct dc1394basler_sff_dcam_values_t {
  /* gain */
  dc1394basler_dcam_csr_value_t gain_csr;
  uint32_t gain_absolute_value;

  /* shutter */
  dc1394basler_dcam_csr_value_t shutter_csr;
  uint32_t shutter_absolute_value;

  /* gamma */
  dc1394basler_dcam_csr_value_t gamma_csr;
  uint32_t gamma_absolute_value;

  /* white balance */
  dc1394basler_dcam_whitebalance_csr_value_t whitebalance_csr;
  uint32_t whitebalance_absolute_value;

  /* brightness */
  dc1394basler_dcam_csr_value_t brightness_csr;
  uint32_t brightness_absolute_value;

  /* tail */
  dc1394basler_sff_chunk_tail_t tail;
} dc1394basler_sff_dcam_values_t;

/**
 * No Docs
 */
typedef struct dc1394basler_sff_crc_checksum_t {
  uint8_t crc_checksum_low;
  uint8_t crc_checksum_high;
  uint8_t reserved1;
  uint8_t reserved2;
} dc1394basler_sff_crc_checksum_t;

#endif
