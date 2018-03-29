/*
 * 1394-Based Digital Camera Control Library
 *
 * Pixelink (PxL) specific extensions for Multi-camera control.
 * 
 * Written by
 *     Aravind Sundaresan <a.sundaresan@gmail.com>
 *     James Sherman <shermanj@umd.edu>
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

#ifndef __DC1394_VENDOR_PIXELINK_H__
#define __DC1394_VENDOR_PIXELINK_H__

#include <dc1394/log.h>
#include <dc1394/types.h>

/*! \file dc1394/vendor/pixelink.h
    \brief No docs yet

    More details soon
*/

//#define PIXELINK_DEBUG_LOWEST_LEVEL
//#define PIXELINK_DEBUG_DISPLAY

/*
 * The following deal with the Pixelink specific extensions for cameras
 * PL-A74x. They have been tested on PL-A742 cameras.
 */
#define PxL_MAX_STRING_LENGTH        256

/*
 * Any definition with PxL_ACR prefix refers to the Advanced Feature Control
 * and Status Registers. They are actually offsets from the
 * Advanced_Feature_Inq Value.
 */

/* Camera Info Registers */
#define PxL_ACR_SERIAL_NUM_OFFSET        0x0008U
#define PxL_ACR_SERIAL_NUM_LENGTH        0x000cU
#define PxL_ACR_FPGA_VERSION                0x0010U
#define PxL_ACR_FW_VERSION                0x0014U
#define PxL_ACR_CAMERA_DESC_OFFSET        0x0018U
#define PxL_ACR_CAMERA_DESC_LENGTH        0x001cU

/* Advanced Feature Inquiry Registers */
#define PxL_ACR_NAME_INQUIRY                0x0100U
#define PxL_ACR_NAME_OFFSET                0x0104U
#define PxL_ACR_NAME_LENGTH                0x0108U

/* Advanced Feature Inquiry Registers (GPIO) */
#define PxL_ACR_GPIO_INQ                 0x0128U
#define PxL_ACR_GPIO_PARM1_ABS                0x012CU
#define PxL_ACR_GPIO_PARM2_ABS                0x0130U
#define PxL_ACR_GPIO_PARM3_ABS                0x0134U

#define PxL_ACR_GPIO_0_CFG                0x0300U
#define PxL_ACR_GPIO_1_CFG                0x0304U
#define PxL_ACR_GPIO_2_CFG                0x0308U
#define PxL_ACR_GPIO_3_CFG                0x030CU

/*
 * The following are some of the constants that are register specific.
 */
#define PxL_GPO_CFG_ENABLE                        0x80000000U
#define PxL_GPO_CFG_DISABLE                        0x00000000U
#define PxL_GPO_CFG_POLARITY_HIGH                0x40000000U
#define PxL_GPO_CFG_POLARITY_LOW                0x00000000U
#define PxL_GPO_CFG_MODE_STROBE                        0x00000000U
#define PxL_GPO_CFG_MODE_NORMAL                        0x00000001U
#define PxL_GPO_CFG_MODE_PULSE                        0x00000002U
#define PxL_GPO_CFG_MODE_BUSY                        0x00000003U
#define PxL_GPO_CFG_MODE_FLASH                        0x00000004U

/**
 * No Docs
 */
typedef enum {
    DC1394_PxL_GPIO_POLARITY_NONE=0,
    DC1394_PxL_GPIO_POLARITY_HIGH,
    DC1394_PxL_GPIO_POLARITY_LOW
} dc1394pxl_gpio_polarity_t;

/**
 * No Docs
 */
typedef enum {
    DC1394_PxL_GPIO_MODE_STROBE=0,
    DC1394_PxL_GPIO_MODE_NORMAL,
    DC1394_PxL_GPIO_MODE_PULSE,
    DC1394_PxL_GPIO_MODE_BUSY,
    DC1394_PxL_GPIO_MODE_FLASH
} dc1394pxl_gpio_mode_t;

/* IEEE 32 bit floating point type */
typedef float float32_t;

/**
 * GPIO Information structure
 */
typedef struct __dc1394_pxl_gpio_info_struct {
    uint32_t       number;
    dc1394bool_t presence;
    dc1394bool_t polarity;
    dc1394bool_t mode_strobe;
    dc1394bool_t mode_normal;
    dc1394bool_t mode_pulse;
    dc1394bool_t mode_busy;
    dc1394bool_t mode_flash;
} dc1394_pxl_gpio_info_t;

/**
 * Camera information
 */
typedef struct __dc1394_pxl_camera_info_struct {
    uint32_t fpga_version;
    uint32_t fw_version;
    char serial_number[PxL_MAX_STRING_LENGTH];
    char description[PxL_MAX_STRING_LENGTH];
} dc1394_pxl_camera_info_t;

/**
 * Advanced feature inquiry
 */
typedef struct __dc1394_pxl_adv_feature_info_struct {
    dc1394bool_t name_presence;
    uint32_t name_offset;
    char name[PxL_MAX_STRING_LENGTH];
} dc1394_pxl_adv_feature_info_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_convert_float32_to_quadlet(double , uint32_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_convert_uint32_to_float32(uint32_t , double *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_camera_name(dc1394camera_t *, char *, uint32_t);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_camera_info(dc1394camera_t *, dc1394_pxl_camera_info_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_camera_serial_number(dc1394camera_t *, uint32_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_gpo_param(dc1394camera_t *, uint32_t, uint32_t *, uint32_t *, uint32_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_gpo_param_min_max(dc1394camera_t *, uint32_t, uint32_t *,
                uint32_t *, uint32_t *, uint32_t *, uint32_t *, uint32_t *,
                uint32_t *, uint32_t *, uint32_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_get_gpo_config(dc1394camera_t *, uint32_t, uint32_t *);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_set_gpo_config(dc1394camera_t *, uint32_t, uint32_t);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_set_gpio_mode_param(dc1394camera_t *, uint32_t ,
        dc1394pxl_gpio_polarity_t, dc1394pxl_gpio_mode_t, double, double, double);

/**
 * No Docs
 */
dc1394error_t
dc1394_pxl_print_camera_info(dc1394camera_t *, FILE *fd);

#ifdef __cplusplus
}
#endif

#endif

