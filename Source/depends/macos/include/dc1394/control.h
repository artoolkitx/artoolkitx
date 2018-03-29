/*
 * 1394-Based Digital Camera Control Library
 *
 * Generic camera control functions
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

#ifndef __DC1394_CONTROL_H__
#define __DC1394_CONTROL_H__

/*! \file dc1394/control.h
    \brief Diverse controls of camera features
    \author Gord Peters: main writer
    \author Chris Urmson: some additions
    \author Damien Douxchamps: some additions
    \author Peter Antoniac: documentation maintainer
    \author Rudolf Leitgeb: documentation writer

    This is the main include file of the library. It lists most of the library
    functions, enumerations and data structures.
*/

/**
 * Enumeration of trigger modes
 */
typedef enum {
    DC1394_TRIGGER_MODE_0= 384,
    DC1394_TRIGGER_MODE_1,
    DC1394_TRIGGER_MODE_2,
    DC1394_TRIGGER_MODE_3,
    DC1394_TRIGGER_MODE_4,
    DC1394_TRIGGER_MODE_5,
    DC1394_TRIGGER_MODE_14,
    DC1394_TRIGGER_MODE_15
} dc1394trigger_mode_t;
#define DC1394_TRIGGER_MODE_MIN     DC1394_TRIGGER_MODE_0
#define DC1394_TRIGGER_MODE_MAX     DC1394_TRIGGER_MODE_15
#define DC1394_TRIGGER_MODE_NUM    (DC1394_TRIGGER_MODE_MAX - DC1394_TRIGGER_MODE_MIN + 1)

/**
 * Enumeration of camera features
 */
typedef enum {
    DC1394_FEATURE_BRIGHTNESS= 416,
    DC1394_FEATURE_EXPOSURE,
    DC1394_FEATURE_SHARPNESS,
    DC1394_FEATURE_WHITE_BALANCE,
    DC1394_FEATURE_HUE,
    DC1394_FEATURE_SATURATION,
    DC1394_FEATURE_GAMMA,
    DC1394_FEATURE_SHUTTER,
    DC1394_FEATURE_GAIN,
    DC1394_FEATURE_IRIS,
    DC1394_FEATURE_FOCUS,
    DC1394_FEATURE_TEMPERATURE,
    DC1394_FEATURE_TRIGGER,
    DC1394_FEATURE_TRIGGER_DELAY,
    DC1394_FEATURE_WHITE_SHADING,
    DC1394_FEATURE_FRAME_RATE,
    DC1394_FEATURE_ZOOM,
    DC1394_FEATURE_PAN,
    DC1394_FEATURE_TILT,
    DC1394_FEATURE_OPTICAL_FILTER,
    DC1394_FEATURE_CAPTURE_SIZE,
    DC1394_FEATURE_CAPTURE_QUALITY
} dc1394feature_t;
#define DC1394_FEATURE_MIN           DC1394_FEATURE_BRIGHTNESS
#define DC1394_FEATURE_MAX           DC1394_FEATURE_CAPTURE_QUALITY
#define DC1394_FEATURE_NUM          (DC1394_FEATURE_MAX - DC1394_FEATURE_MIN + 1)

/**
 * Enumeration of trigger sources
 */
typedef enum {
    DC1394_TRIGGER_SOURCE_0= 576,
    DC1394_TRIGGER_SOURCE_1,
    DC1394_TRIGGER_SOURCE_2,
    DC1394_TRIGGER_SOURCE_3,
    DC1394_TRIGGER_SOURCE_SOFTWARE
} dc1394trigger_source_t;
#define DC1394_TRIGGER_SOURCE_MIN      DC1394_TRIGGER_SOURCE_0
#define DC1394_TRIGGER_SOURCE_MAX      DC1394_TRIGGER_SOURCE_SOFTWARE
#define DC1394_TRIGGER_SOURCE_NUM     (DC1394_TRIGGER_SOURCE_MAX - DC1394_TRIGGER_SOURCE_MIN + 1)

/**
 * External trigger polarity
 */
typedef enum {
    DC1394_TRIGGER_ACTIVE_LOW= 704,
    DC1394_TRIGGER_ACTIVE_HIGH
} dc1394trigger_polarity_t;
#define DC1394_TRIGGER_ACTIVE_MIN    DC1394_TRIGGER_ACTIVE_LOW
#define DC1394_TRIGGER_ACTIVE_MAX    DC1394_TRIGGER_ACTIVE_HIGH
#define DC1394_TRIGGER_ACTIVE_NUM   (DC1394_TRIGGER_ACTIVE_MAX - DC1394_TRIGGER_ACTIVE_MIN + 1)

/**
 * Control modes for a feature (excl. absolute control)
 */
typedef enum {
    DC1394_FEATURE_MODE_MANUAL= 736,
    DC1394_FEATURE_MODE_AUTO,
    DC1394_FEATURE_MODE_ONE_PUSH_AUTO
} dc1394feature_mode_t;
#define DC1394_FEATURE_MODE_MIN      DC1394_FEATURE_MODE_MANUAL
#define DC1394_FEATURE_MODE_MAX      DC1394_FEATURE_MODE_ONE_PUSH_AUTO
#define DC1394_FEATURE_MODE_NUM     (DC1394_FEATURE_MODE_MAX - DC1394_FEATURE_MODE_MIN + 1)

/**
 * List of feature modes
 */
typedef struct
{
    uint32_t                num;
    dc1394feature_mode_t    modes[DC1394_FEATURE_MODE_NUM];
} dc1394feature_modes_t;

/**
 * List of trigger modes
 */
typedef struct
{
    uint32_t                num;
    dc1394trigger_mode_t    modes[DC1394_TRIGGER_MODE_NUM];
} dc1394trigger_modes_t;

/**
 * List of trigger sources
 */
typedef struct
{
    uint32_t                num;
    dc1394trigger_source_t  sources[DC1394_TRIGGER_SOURCE_NUM];
} dc1394trigger_sources_t;

/**
 * A structure containing all information about a feature.
 *
 * Some fields are only valid for some features (e.g. trigger, white balance,...)
 */
typedef struct __dc1394feature_info_t_struct
{
    dc1394feature_t    id;
    dc1394bool_t       available;
    dc1394bool_t       absolute_capable;
    dc1394bool_t       readout_capable;
    dc1394bool_t       on_off_capable;
    dc1394bool_t       polarity_capable;
    dc1394switch_t     is_on;
    dc1394feature_mode_t     current_mode;
    dc1394feature_modes_t    modes;
    dc1394trigger_modes_t    trigger_modes;
    dc1394trigger_mode_t     trigger_mode;
    dc1394trigger_polarity_t trigger_polarity;
    dc1394trigger_sources_t  trigger_sources;
    dc1394trigger_source_t   trigger_source;
    uint32_t           min;
    uint32_t           max;
    uint32_t           value;
    uint32_t           BU_value;
    uint32_t           RV_value;
    uint32_t           B_value;
    uint32_t           R_value;
    uint32_t           G_value;
    uint32_t           target_value;

    dc1394switch_t     abs_control;
    float              abs_value;
    float              abs_max;
    float              abs_min;

} dc1394feature_info_t;

/**
 * The list of features
 */
typedef struct __dc1394featureset_t
{
    dc1394feature_info_t    feature[DC1394_FEATURE_NUM];
} dc1394featureset_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Features
 ***************************************************************************/

/**
 * Collects the available features for the camera described by node and stores them in features.
 */
dc1394error_t dc1394_feature_get_all(dc1394camera_t *camera, dc1394featureset_t *features);

/**
 * Stores the bounds and options associated with the feature described by feature->feature_id
 */
dc1394error_t dc1394_feature_get(dc1394camera_t *camera, dc1394feature_info_t *feature);

/**
 * Displays the bounds and options of the given feature
 */
dc1394error_t dc1394_feature_print(dc1394feature_info_t *feature, FILE *fd);

/**
 * Displays the bounds and options of every feature supported by the camera
 */
dc1394error_t dc1394_feature_print_all(dc1394featureset_t *features, FILE *fd);

/**
 * Gets the whitebalance values
 */
dc1394error_t dc1394_feature_whitebalance_get_value(dc1394camera_t *camera, uint32_t *u_b_value, uint32_t *v_r_value);

/**
 * Sets the whitebalance values
 */
dc1394error_t dc1394_feature_whitebalance_set_value(dc1394camera_t *camera, uint32_t u_b_value, uint32_t v_r_value);

/**
 * Gets the temperature values (target and current)
 */
dc1394error_t dc1394_feature_temperature_get_value(dc1394camera_t *camera, uint32_t *target_temperature, uint32_t *temperature);

/**
 * Sets the temperature values (target only) FIXME: COULD BE DROPPED? already in the standard feature_set_value()?
 */
dc1394error_t dc1394_feature_temperature_set_value(dc1394camera_t *camera, uint32_t target_temperature);

/**
 * Gets the white shading values
 */
dc1394error_t dc1394_feature_whiteshading_get_value(dc1394camera_t *camera, uint32_t *r_value, uint32_t *g_value, uint32_t *b_value);

/**
 * Sets the white shading values
 */
dc1394error_t dc1394_feature_whiteshading_set_value(dc1394camera_t *camera, uint32_t r_value, uint32_t g_value, uint32_t b_value);

/**
 * Gets the value of a feature
 */
dc1394error_t dc1394_feature_get_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *value);

/**
 * Sets the value of a feature
 */
dc1394error_t dc1394_feature_set_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t value);

/**
 * Tells whether a feature is present or not
 */
dc1394error_t dc1394_feature_is_present(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value);

/**
 * Tells whether a feature is readable or not
 */
dc1394error_t dc1394_feature_is_readable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value);

/**
 * Gets the boundaries of a feature
 */
dc1394error_t dc1394_feature_get_boundaries(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *min, uint32_t *max);

/**
 * Tells whether a feature is switcheable or not (ON/OFF)
 */
dc1394error_t dc1394_feature_is_switchable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value);

/**
 * Gets the power status of a feature (ON or OFF)
 */
dc1394error_t dc1394_feature_get_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *pwr);

/**
 * Sets the power status of a feature (ON or OFF)
 */
dc1394error_t dc1394_feature_set_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t pwr);

/**
 * Gets the list of control modes for a feature (manual, auto, etc...)
 */
dc1394error_t dc1394_feature_get_modes(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_modes_t *modes);

/**
 * Gets the current control modes for a feature
 */
dc1394error_t dc1394_feature_get_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t *mode);

/**
 * Sets the current control modes for a feature
 */
dc1394error_t dc1394_feature_set_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t mode);

/**
 * Tells whether a feature can be controlled in absolute mode
 */
dc1394error_t dc1394_feature_has_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value);

/**
 * Gets the absolute boundaries of a feature
 */
dc1394error_t dc1394_feature_get_absolute_boundaries(dc1394camera_t *camera, dc1394feature_t feature, float *min, float *max);

/**
 * Gets the absolute value of a feature
 */
dc1394error_t dc1394_feature_get_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float *value);

/**
 * Sets the absolute value of a feature
 */
dc1394error_t dc1394_feature_set_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float value);

/**
 * Gets the status of absolute control of a feature
 */
dc1394error_t dc1394_feature_get_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *pwr);

/**
 * Sets the feature in absolute control mode (ON/OFF)
 */
dc1394error_t dc1394_feature_set_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t pwr);

/***************************************************************************
     Trigger
 ***************************************************************************/

/**
 * Sets the polarity of the external trigger
 */
dc1394error_t dc1394_external_trigger_set_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t polarity);

/**
 * Gets the polarity of the external trigger
 */
dc1394error_t dc1394_external_trigger_get_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t *polarity);

/**
 * Tells whether the external trigger can change its polarity or not.
 */
dc1394error_t dc1394_external_trigger_has_polarity(dc1394camera_t *camera, dc1394bool_t *polarity_capable);

/**
 * Switch between internal and external trigger
 */
dc1394error_t dc1394_external_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr);

/**
 * Gets the status of the external trigger
 */
dc1394error_t dc1394_external_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr);

/**
 * Sets the external trigger mode
 */
dc1394error_t dc1394_external_trigger_set_mode(dc1394camera_t *camera, dc1394trigger_mode_t mode);

/**
 * Gets the external trigger mode
 */
dc1394error_t dc1394_external_trigger_get_mode(dc1394camera_t *camera, dc1394trigger_mode_t *mode);

/**
 * Sets the external trigger source
 */
dc1394error_t dc1394_external_trigger_set_source(dc1394camera_t *camera, dc1394trigger_source_t source);

/**
 * Gets the external trigger source
 */
dc1394error_t dc1394_external_trigger_get_source(dc1394camera_t *camera, dc1394trigger_source_t *source);

/**
 * Gets the list of available external trigger source
 */
dc1394error_t dc1394_external_trigger_get_supported_sources(dc1394camera_t *camera, dc1394trigger_sources_t *sources);

/**
 * Turn software trigger on or off
 */
dc1394error_t dc1394_software_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr);

/**
 * Gets the state of software trigger
 */
dc1394error_t dc1394_software_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr);

/***************************************************************************
     PIO, SIO and Strobe Functions
 ***************************************************************************/

/**
 * Sends a quadlet on the PIO (output)
 */
dc1394error_t dc1394_pio_set(dc1394camera_t *camera, uint32_t value);

/**
 * Gets the current quadlet at the PIO (input)
 */
dc1394error_t dc1394_pio_get(dc1394camera_t *camera, uint32_t *value);

/***************************************************************************
     Other functionalities
 ***************************************************************************/

/**
 * reset a camera to factory default settings
 */
dc1394error_t dc1394_camera_reset(dc1394camera_t *camera);

/**
 * turn a camera on or off
 */
dc1394error_t dc1394_camera_set_power(dc1394camera_t *camera, dc1394switch_t pwr);

/**
 * Download a camera setup from the memory.
 */
dc1394error_t dc1394_memory_busy(dc1394camera_t *camera, dc1394bool_t *value);

/**
 * Uploads a camera setup in the memory.
 *
 * Note that this operation can only be performed a certain number of
 * times for a given camera, as it requires reprogramming of an EEPROM.
 */
dc1394error_t dc1394_memory_save(dc1394camera_t *camera, uint32_t channel);

/**
 * Tells whether the writing of the camera setup in memory is finished or not.
 */
dc1394error_t dc1394_memory_load(dc1394camera_t *camera, uint32_t channel);


#ifdef __cplusplus
}
#endif

#endif /* __DC1394_CONTROL_H__ */
