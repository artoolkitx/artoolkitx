/*
 * 1394-Based Digital Camera Control Library
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
#include <stdio.h>

#ifndef __DC1394_CAMERA_H__
#define __DC1394_CAMERA_H__

/*! \file dc1394/camera.h
    \brief Basic system and camera functions
    \author Damien Douxchamps: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

/**
 * List of IIDC versions
 *
 * Currently, the following versions exist: 1.04, 1.20, PTGREY, 1.30 and 1.31 (1.32 coming soon)
 * Observing other versions means that there's a bug crawling somewhere.
 */
typedef enum {
    DC1394_IIDC_VERSION_1_04 = 544,
    DC1394_IIDC_VERSION_1_20,
    DC1394_IIDC_VERSION_PTGREY,
    DC1394_IIDC_VERSION_1_30,
    DC1394_IIDC_VERSION_1_31,
    DC1394_IIDC_VERSION_1_32,
    DC1394_IIDC_VERSION_1_33,
    DC1394_IIDC_VERSION_1_34,
    DC1394_IIDC_VERSION_1_35,
    DC1394_IIDC_VERSION_1_36,
    DC1394_IIDC_VERSION_1_37,
    DC1394_IIDC_VERSION_1_38,
    DC1394_IIDC_VERSION_1_39
} dc1394iidc_version_t;
#define DC1394_IIDC_VERSION_MIN        DC1394_IIDC_VERSION_1_04
#define DC1394_IIDC_VERSION_MAX        DC1394_IIDC_VERSION_1_39
#define DC1394_IIDC_VERSION_NUM       (DC1394_IIDC_VERSION_MAX - DC1394_IIDC_VERSION_MIN + 1)

/**
 * Enumeration of power classes
 *
 * This is currently not used in libdc1394.
 */
typedef enum {
    DC1394_POWER_CLASS_NONE=608,
    DC1394_POWER_CLASS_PROV_MIN_15W,
    DC1394_POWER_CLASS_PROV_MIN_30W,
    DC1394_POWER_CLASS_PROV_MIN_45W,
    DC1394_POWER_CLASS_USES_MAX_1W,
    DC1394_POWER_CLASS_USES_MAX_3W,
    DC1394_POWER_CLASS_USES_MAX_6W,
    DC1394_POWER_CLASS_USES_MAX_10W
} dc1394power_class_t;
#define DC1394_POWER_CLASS_MIN       DC1394_POWER_CLASS_NONE
#define DC1394_POWER_CLASS_MAX       DC1394_POWER_CLASS_USES_MAX_10W
#define DC1394_POWER_CLASS_NUM      (DC1394_POWER_CLASS_MAX - DC1394_POWER_CLASS_MIN + 1)

/**
 * Enumeration of PHY delays
 *
 * This is currently not used in libdc1394.
 */
typedef enum {
    DC1394_PHY_DELAY_MAX_144_NS=640,
    DC1394_PHY_DELAY_UNKNOWN_0,
    DC1394_PHY_DELAY_UNKNOWN_1,
    DC1394_PHY_DELAY_UNKNOWN_2
} dc1394phy_delay_t;
#define DC1394_PHY_DELAY_MIN         DC1394_PHY_DELAY_MAX_144_NS
#define DC1394_PHY_DELAY_MAX         DC1394_PHY_DELAY_UNKNOWN_0
#define DC1394_PHY_DELAY_NUM        (DC1394_PHY_DELAY_MAX - DC1394_PHY_DELAY_MIN + 1)

/**
 * Camera structure
 *
 * This structure represents the camera in libdc1394. It contains a number of useful static information, such as model/vendor names,
 * a few capabilities, some ROM offsets, a unique identifier, etc... 
 */
typedef struct __dc1394_camera
{
    /* system/firmware information */
    uint64_t             guid;
    int                  unit;
    uint32_t             unit_spec_ID;
    uint32_t             unit_sw_version;
    uint32_t             unit_sub_sw_version;
    uint32_t             command_registers_base;
    uint32_t             unit_directory;
    uint32_t             unit_dependent_directory;
    uint64_t             advanced_features_csr;
    uint64_t             PIO_control_csr;
    uint64_t             SIO_control_csr;
    uint64_t             strobe_control_csr;
    uint64_t             format7_csr[DC1394_VIDEO_MODE_FORMAT7_NUM];
    dc1394iidc_version_t iidc_version;
    char               * vendor;
    char               * model;
    uint32_t             vendor_id;
    uint32_t             model_id;
    dc1394bool_t         bmode_capable;
    dc1394bool_t         one_shot_capable;
    dc1394bool_t         multi_shot_capable;
    dc1394bool_t         can_switch_on_off;
    dc1394bool_t         has_vmode_error_status;
    dc1394bool_t         has_feature_error_status;
    int                  max_mem_channel;

    /* not used, for future use: */
    uint32_t             flags;

} dc1394camera_t;

/**
 * A unique identifier for a functional camera unit
 *
 * Since a single camera can contain several functional units (think stereo cameras), the GUID is not enough to identify an IIDC camera.
 * The unit number must also be used, hence this struct. 
 */
typedef struct
{
    uint16_t             unit;
    uint64_t             guid;
} dc1394camera_id_t;

/**
 * A list of cameras
 *
 * Usually returned by dc1394_camera_eumerate(). 
 */
typedef struct __dc1394camera_list_t
{
    uint32_t             num;
    dc1394camera_id_t    *ids;
} dc1394camera_list_t;

typedef struct __dc1394_t dc1394_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     General system functions
 ***************************************************************************/

/**
 * Creates a new context in which cameras can be searched and used. This should be called before using any other libdc1394 function. 
 */
dc1394_t* dc1394_new (void);

/**
 * Liberates a context. Last function to use in your program. After this, no libdc1394 function can be used.
 */
void dc1394_free (dc1394_t *dc1394);

/**
 * Sets and gets the broadcast flag of a camera. If the broadcast flag is set,
 * all devices on the bus will execute the command. Useful to sync ISO start
 * commands or setting a bunch of cameras at the same time. Broadcast only works
 * with identical devices (brand/model). If the devices are not identical your
 * mileage may vary. Some cameras may not answer broadcast commands at all. Also,
 * this only works with cameras on the SAME bus (IOW, the same port).
 */
dc1394error_t dc1394_camera_set_broadcast(dc1394camera_t *camera, dc1394bool_t pwr);
dc1394error_t dc1394_camera_get_broadcast(dc1394camera_t *camera, dc1394bool_t *pwr);

/**
 * Resets the IEEE1394 bus which camera is attached to.  Calling this function is
 * "rude" to other devices because it causes them to re-enumerate on the bus and
 * may cause a temporary disruption in their current activities.  Thus, use it
 * sparingly.  Its primary use is if a program shuts down uncleanly and needs to
 * free leftover ISO channels or bandwidth.  A bus reset will free those things
 * as a side effect.
 */
dc1394error_t dc1394_reset_bus(dc1394camera_t *camera);
dc1394error_t dc1394_read_cycle_timer (dc1394camera_t * camera,
        uint32_t * cycle_timer, uint64_t * local_time);

/**
 * Gets the IEEE 1394 node ID of the camera.
 */
dc1394error_t dc1394_camera_get_node(dc1394camera_t *camera, uint32_t *node,
        uint32_t * generation);


/***************************************************************************
     Camera functions
 ***************************************************************************/

/**
 * Returns the list of cameras available on the computer. If present, multiple cards will be probed
 */
dc1394error_t dc1394_camera_enumerate(dc1394_t *dc1394, dc1394camera_list_t **list);

/**
 * Frees the memory allocated in dc1394_enumerate_cameras for the camera list
 */
void dc1394_camera_free_list(dc1394camera_list_t *list);

/**
 * Create a new camera based on a GUID (Global Unique IDentifier)
 */
dc1394camera_t * dc1394_camera_new(dc1394_t *dc1394, uint64_t guid);

/**
 * Create a new camera based on a GUID and a unit number (for multi-unit cameras)
 */
dc1394camera_t * dc1394_camera_new_unit(dc1394_t *dc1394, uint64_t guid, int unit);

/**
 * Frees a camera structure
 */
void dc1394_camera_free(dc1394camera_t *camera);

/**
 * Print various camera information, such as GUID, vendor, model, supported IIDC specs, etc...
 */
dc1394error_t dc1394_camera_print_info(dc1394camera_t *camera, FILE *fd);

/**
 * Returns a pointer to a string identifying the platform for the cameras. Platforms strings are:
 * juju, linux, macosx, windows, usb
 */
dc1394error_t dc1394_camera_get_platform_string(dc1394camera_t *camera, const char **platform);

#ifdef __cplusplus
}
#endif

#endif
