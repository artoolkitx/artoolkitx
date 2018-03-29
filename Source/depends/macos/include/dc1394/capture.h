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
#include <dc1394/video.h>

#ifndef __DC1394_CAPTURE_H__
#define __DC1394_CAPTURE_H__

/*! \file dc1394/capture.h
    \brief Capture functions
    \author Damien Douxchamps: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

/**
 * The capture policy.
 *
 * Can be blocking (wait for a frame forever) or polling (returns if no frames is in the ring buffer)
 */
typedef enum {
    DC1394_CAPTURE_POLICY_WAIT=672,
    DC1394_CAPTURE_POLICY_POLL
} dc1394capture_policy_t;
#define DC1394_CAPTURE_POLICY_MIN    DC1394_CAPTURE_POLICY_WAIT
#define DC1394_CAPTURE_POLICY_MAX    DC1394_CAPTURE_POLICY_POLL
#define DC1394_CAPTURE_POLICY_NUM   (DC1394_CAPTURE_POLICY_MAX - DC1394_CAPTURE_POLICY_MIN + 1)

/**
* typedef for the callback param for dc1394_capture_set_callback
*/

typedef void (*dc1394capture_callback_t)(dc1394camera_t *, void *);


/**
 * Capture flags. Currently limited to switching automatic functions on/off: channel allocation, bandwidth allocation and automatic
 * starting of ISO transmission
 */
#define DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC   0x00000001U
#define DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC 0x00000002U
#define DC1394_CAPTURE_FLAGS_DEFAULT         0x00000004U /* a reasonable default value: do bandwidth and channel allocation */
#define DC1394_CAPTURE_FLAGS_AUTO_ISO        0x00000008U /* automatically start iso before capture and stop it after */

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Capture Functions
 ***************************************************************************/

/**
 * Setup the capture, using a ring buffer of a certain size (num_dma_buffers) and certain options (flags)
 */
dc1394error_t dc1394_capture_setup(dc1394camera_t *camera, uint32_t num_dma_buffers, uint32_t flags);

/**
 * Stop the capture
 */
dc1394error_t dc1394_capture_stop(dc1394camera_t *camera);

/**
 * Gets a file descriptor to be used for select(). Must be called after dc1394_capture_setup().
 */
int dc1394_capture_get_fileno (dc1394camera_t * camera);

/**
 * Captures a video frame. The returned struct contains the image buffer, among others. This image buffer SHALL NOT be freed, as it represents an area
 * in the memory that belongs to the system. 
 */
dc1394error_t dc1394_capture_dequeue(dc1394camera_t * camera, dc1394capture_policy_t policy, dc1394video_frame_t **frame);

/**
 * Returns a frame to the ring buffer once it has been used.
 */
dc1394error_t dc1394_capture_enqueue(dc1394camera_t * camera, dc1394video_frame_t * frame);

/**
 * Returns DC1394_TRUE if the given frame (previously dequeued) has been
 * detected to be corrupt (missing data, corrupted data, overrun buffer, etc.).
 * Note that certain types of corruption may go undetected in which case
 * DC1394_FALSE will be returned.  The ability to detect corruption also
 * varies between platforms.  Note that corrupt frames still need to be
 * enqueued with dc1394_capture_enqueue() when no longer needed by the user.
 */
dc1394bool_t dc1394_capture_is_frame_corrupt (dc1394camera_t * camera,
        dc1394video_frame_t * frame);

/**
 * Set a callback if supported by the platform (OS X only for now).
 */
void dc1394_capture_set_callback (dc1394camera_t * camera,
        dc1394capture_callback_t callback, void * user_data);

#ifdef __cplusplus
}
#endif

#endif
