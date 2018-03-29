/*
 * 1394-Based Digital Camera Control Library
 *
 * Functions for the manual allocations of ISO ressources.
 *
 * Written by David Moore <dcm@acm.org>
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

#ifndef __DC1394_ISO_H__
#define __DC1394_ISO_H__

/*! \file dc1394/iso.h
    \brief Functions to manually manage the ISO resources (channels and bandwidth)
    \author Damien Douxchamps: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

#include <dc1394/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * dc1394_iso_set_persist
 * @param camera A camera handle.
 *
 * Calling this function will cause isochronous channel and bandwidth
 * allocations to persist beyond the lifetime of this dc1394camera_t
 * instance.  Normally (when this function is not called), any allocations
 * would be automatically released upon freeing this camera or a
 * premature shutdown of the application (if possible).  For this function
 * to be used, it must be called prior to any allocations or an error will
 * be returned.
 *
 * @return \a DC1394_SUCCESS if the operation succeeded.
 * \a DC1394_FUNCTION_NOT_SUPPORTED if the current platform/driver does not
 * allow persistent allocations.
 */
dc1394error_t dc1394_iso_set_persist (dc1394camera_t * camera);

/**
 * dc1394_iso_allocate_channel:
 * @param camera A camera handle.
 * @param channels_allowed A bitmask of acceptable channels for the allocation.
 *   The LSB corresponds to channel 0 and the MSB corresponds to channel
 *   63.  Only channels whose bit is set will be considered for the allocation.
 *   If \a channels_allowed = 0, the complete set of channels supported by
 *   this camera will be considered for the allocation.
 * @param channel The allocated channel number is returned here.
 *
 * Allocates an isochronous channel.  This
 * function may be called multiple times, each time allocating an additional
 * channel.  The channel is automatically re-allocated if there is a bus
 * reset.  The channel is automatically released when this dc1394camera_t
 * is freed or if the application shuts down prematurely.  If the channel
 * needs to persist beyond the lifetime of this application, call
 * \a dc1394_iso_set_persist() first.  Note that this function does _not_
 * automatically program @a camera to use the allocated channel for isochronous
 * streaming.  You must do that manually using \a dc1394_video_set_iso_channel().
 *
 * @return \a DC1394_SUCCESS if the operation succeeded.  The allocated
 * channel is stored in \a channel. \a DC1394_FUNCTION_NOT_SUPPORTED if the
 * current driver/platform does not allow channel allocation.
 * \a DC1394_NO_ISO_CHANNEL if none of the requested channels are available.
 */
dc1394error_t dc1394_iso_allocate_channel (dc1394camera_t * camera,
    uint64_t channels_allowed, int * channel);

/**
 * dc1394_iso_release_channel:
 * @param camera A camera handle.
 * @param channel The channel number to release.
 *
 * Releases a previously allocated channel.  It is acceptable to release
 * channels that were allocated by a different process or host.  If
 * attempting to release a channel that is already released, the function
 * will succeed.
 *
 * @return \a DC1394_SUCCESS if the operation succeeded.
 * \a DC1394_FUNCTION_NOT_SUPPORTED if the current driver/platform does not
 * allow channel release.
 */
dc1394error_t dc1394_iso_release_channel (dc1394camera_t * camera,
    int channel);

/**
 * dc1394_iso_allocate_bandwidth:
 * @param camera A camera handle.
 * @param bandwidth_units The number of isochronous bandwidth units to allocate.
 *
 * Allocates isochronous bandwidth.  This functions allocates bandwidth
 * _in addition_ to any previous allocations.  It may be called multiple
 * times.  The bandwidth is automatically re-allocated if there is a bus
 * reset.  The bandwidth is automatically released if this camera is freed
 * or the application shuts down prematurely.  If the bandwidth needs to
 * persist beyond the lifetime of this application, call
 * \a dc1394_iso_set_persist() first.
 *
 * @return \a DC1394_SUCCESS if the operation succeeded.
 * \a DC1394_FUNCTION_NOT_SUPPORTED if the current driver/platform does not
 * allow bandwidth allocation. \a DC1394_NO_BANDWIDTH if there is not enough
 * available bandwidth to support the allocation.  In this case,
 * no bandwidth is allocated.
 */
dc1394error_t dc1394_iso_allocate_bandwidth (dc1394camera_t * camera,
    int bandwidth_units);

/**
 * dc1394_iso_release_bandwidth:
 * @param camera A camera handle.
 * @param bandwidth_units The number of isochronous bandwidth units to free.
 *
 * Releases previously allocated isochronous bandwidth.  Each \a dc1394camera_t
 * keeps track of a running total of bandwidth that has been allocated.
 * Released bandwidth is subtracted from this total for the sake of
 * automatic re-allocation and automatic release on shutdown.  It is also
 * acceptable for a camera to release more bandwidth than it has allocated
 * (to clean up for another process for example).  In this case, the
 * running total of bandwidth is not affected.  It is acceptable to
 * release more bandwidth than is allocated in total for the bus.  In this
 * case, all bandwidth is released and the function succeeds.
 *
 * @return \a DC1394_SUCCESS if the operation succeeded.
 * \a DC1394_FUNCTION_NOT_SUPPORTED if the current driver/platform does not
 * allow bandwidth release.
 */
dc1394error_t dc1394_iso_release_bandwidth (dc1394camera_t * camera,
    int bandwidth_units);

/**
 * dc1394_iso_release_all:
 * @param camera A camera handle.
 *
 * Releases all channels and bandwidth that have been previously allocated
 * for this dc1394camera_t.  Note that this information can only be tracked
 * per process, and there is no knowledge of allocations for this camera
 * by previous processes.  To release resources in such a case, the manual
 * release functions \a dc1394_iso_release_channel() and
 * \a dc1394_iso_release_bandwidth() must be used.
 *
 * @return \a DC1394_SUCCESS if the operation succeeded. \a DC1394_FAILURE
 * if some resources were not able to be released.
 */
dc1394error_t dc1394_iso_release_all (dc1394camera_t * camera);

#ifdef __cplusplus
}
#endif

#endif
