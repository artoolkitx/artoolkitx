/*
 * 1394-Based Digital Camera Control Library
 *
 * Low-level register access functions
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

#ifndef __DC1394_REGISTER_H__
#define __DC1394_REGISTER_H__

/*! \file dc1394/register.h
    \brief Functions to directly access camera registers.
    \author Damien Douxchamps: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * No Docs
 */
dc1394error_t dc1394_get_registers (dc1394camera_t *camera,
        uint64_t offset, uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t dc1394_get_register (dc1394camera_t *camera,
        uint64_t offset, uint32_t *value)
{
    return dc1394_get_registers (camera, offset, value, 1);
}

/**
 * No Docs
 */
dc1394error_t dc1394_set_registers (dc1394camera_t *camera,
        uint64_t offset, const uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t dc1394_set_register (dc1394camera_t *camera,
                                                 uint64_t offset, uint32_t value)
{
    return dc1394_set_registers (camera, offset, &value, 1);
}


/********************************************************************************/
/* Get/Set Command Registers                                                    */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t dc1394_get_control_registers (dc1394camera_t *camera,
    uint64_t offset, uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t dc1394_get_control_register (dc1394camera_t *camera,
    uint64_t offset, uint32_t *value)
{
    return dc1394_get_control_registers (camera, offset, value, 1);
}

/**
 * No Docs
 */
dc1394error_t dc1394_set_control_registers (dc1394camera_t *camera,
        uint64_t offset, const uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t dc1394_set_control_register (dc1394camera_t *camera,
    uint64_t offset, uint32_t value)
{
    return dc1394_set_control_registers (camera, offset, &value, 1);
}


/********************************************************************************/
/* Get/Set Advanced Features Registers                                          */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_adv_control_registers(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t
dc1394_get_adv_control_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
    return dc1394_get_adv_control_registers(camera, offset, value, 1);
}

/**
 * No Docs
 */
dc1394error_t
dc1394_set_adv_control_registers(dc1394camera_t *camera, uint64_t offset,
        const uint32_t *value, uint32_t num_regs);

/**
 * No Docs
 */
static inline dc1394error_t
dc1394_set_adv_control_register(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
    return dc1394_set_adv_control_registers(camera, offset, &value, 1);
}


/********************************************************************************/
/* Get/Set Format_7 Registers                                                   */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_format7_register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t *value);

/**
 * No Docs
 */
dc1394error_t
dc1394_set_format7_register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set Absolute Control Registers                                           */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_absolute_register(dc1394camera_t *camera, unsigned int feature, uint64_t offset, uint32_t *value);

/**
 * No Docs
 */
dc1394error_t
dc1394_set_absolute_register(dc1394camera_t *camera, unsigned int feature, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set PIO Feature Registers                                                */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_PIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

/**
 * No Docs
 */
dc1394error_t
dc1394_set_PIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set SIO Feature Registers                                                */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_SIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

/**
 * No Docs
 */
dc1394error_t
dc1394_set_SIO_register(dc1394camera_t *camera, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set Strobe Feature Registers                                             */
/********************************************************************************/

/**
 * No Docs
 */
dc1394error_t
dc1394_get_strobe_register(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

/**
 * No Docs
 */
dc1394error_t
dc1394_set_strobe_register(dc1394camera_t *camera, uint64_t offset, uint32_t value);


#ifdef __cplusplus
}
#endif

#endif /* __DC1394_REGISTER_H__ */
