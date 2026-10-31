/*
 *	videoGStreamer.c
 *  artoolkitX
 *
 *  Video capture module utilising the GStreamer pipeline for AR Toolkit
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018-2026 Philip Lamb
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2008-2015 ARToolworks, Inc.
 *  Copyright 2003-2008 Hartmut Seichter <http://www.technotecture.com>
 *
 *  Author(s): Hartmut Seichter, Philip Lamb
 *
 */

#include "videoGStreamer.h"

#ifdef ARVIDEO_INPUT_GSTREAMER

#include <ARX/ARUtil/log.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <stdlib.h>
#include <string.h>

// The pipeline used when no configuration string is supplied.
#define AR_VIDEO_GSTREAMER_DEFAULT_PIPELINE "videotestsrc ! video/x-raw,format=RGB,width=640,height=480,framerate=30/1 ! identity name=artoolkit sync=true ! fakesink"

// Name of the element in the pipeline whose source pad we tap frames from.
#define AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME "artoolkit"

// How long to wait for state changes and caps negotiation before giving up.
#define AR_VIDEO_GSTREAMER_STATE_CHANGE_TIMEOUT (10 * GST_SECOND)

#define AR_VIDEO_GSTREAMER_MAX_PLANES 4

struct _AR2VideoParamGStreamerT {
    GstElement *pipeline;
    GstElement *tap;       // The element named AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME.
    GstPad     *tapPad;    // The source pad of tap.
    gulong      probeId;
    GstBus     *bus;
    gboolean    isLive;
    gboolean    baseTimeValid;      // Set before the first transition to PLAYING distributes a base time.

    // Everything below here is shared between the GStreamer streaming thread
    // (probe callback and bus handler) and the caller's thread, and is
    // protected by mutex. cond is signalled whenever capsValid or error change.
    GMutex      mutex;
    GCond       cond;
    gboolean    error;

    // Negotiated format at the tap element.
    gboolean        capsValid;
    GstVideoInfo    videoInfo;
    int             width;
    int             height;
    AR_PIXEL_FORMAT pixelFormat;
    unsigned int    planeCount;
    size_t          planeOffset[AR_VIDEO_GSTREAMER_MAX_PLANES];
    size_t          planeRowBytes[AR_VIDEO_GSTREAMER_MAX_PLANES]; // Tightly packed row length.
    unsigned int    planeRows[AR_VIDEO_GSTREAMER_MAX_PLANES];
    size_t          frameBytes;

    // Double-buffered packed frame copies. The streaming thread writes into
    // buffers[fillIndex]; the caller reads from the other one.
    ARUint8    *buffers[2];
    int         fillIndex;
    gboolean    frameReady;
    AR2VideoTimestampT frameTime;
    gboolean    loggedMapFailure;

    // Returned to the caller by ar2VideoGetImageGStreamer.
    AR2VideoBufferT arVideoBuffer;
    ARUint8        *bufPlanes[AR_VIDEO_GSTREAMER_MAX_PLANES];
};

// Map a GStreamer raw video format onto the artoolkitX pixel format with the same memory layout.
static AR_PIXEL_FORMAT pixelFormatForGstVideoFormat(GstVideoFormat format)
{
    switch (format) {
        case GST_VIDEO_FORMAT_RGB:   return AR_PIXEL_FORMAT_RGB;
        case GST_VIDEO_FORMAT_BGR:   return AR_PIXEL_FORMAT_BGR;
        case GST_VIDEO_FORMAT_RGBA:  return AR_PIXEL_FORMAT_RGBA;
        case GST_VIDEO_FORMAT_BGRA:  return AR_PIXEL_FORMAT_BGRA;
        case GST_VIDEO_FORMAT_ARGB:  return AR_PIXEL_FORMAT_ARGB;
        case GST_VIDEO_FORMAT_ABGR:  return AR_PIXEL_FORMAT_ABGR;
        case GST_VIDEO_FORMAT_GRAY8: return AR_PIXEL_FORMAT_MONO;
        case GST_VIDEO_FORMAT_UYVY:  return AR_PIXEL_FORMAT_2vuy;
        case GST_VIDEO_FORMAT_YUY2:  return AR_PIXEL_FORMAT_yuvs;
        case GST_VIDEO_FORMAT_NV12:  return AR_PIXEL_FORMAT_420f;
        case GST_VIDEO_FORMAT_NV21:  return AR_PIXEL_FORMAT_NV21;
        default:                     return AR_PIXEL_FORMAT_INVALID;
    }
}

#define AR_VIDEO_GSTREAMER_SUPPORTED_FORMATS "RGB, BGR, RGBA, BGRA, ARGB, ABGR, GRAY8, UYVY, YUY2, NV12, NV21"

// Called on the streaming thread when the tap element's source pad receives a caps event.
static void handleCaps(AR2VideoParamGStreamerT *vid, GstCaps *caps)
{
    GstVideoInfo videoInfo;
    AR_PIXEL_FORMAT pixelFormat;
    const GstVideoFormatInfo *finfo;
    unsigned int planeCount, p, c;
    size_t planeOffset[AR_VIDEO_GSTREAMER_MAX_PLANES], planeRowBytes[AR_VIDEO_GSTREAMER_MAX_PLANES], frameBytes;
    unsigned int planeRows[AR_VIDEO_GSTREAMER_MAX_PLANES];
    gchar *capsString;

    if (!gst_video_info_from_caps(&videoInfo, caps)) {
        capsString = gst_caps_to_string(caps);
        ARLOGe("ARVideo: GStreamer element '%s' negotiated caps which are not raw video: %s\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME, capsString);
        ARLOGe("ARVideo: Insert 'videoconvert ! video/x-raw,format=RGB' before it in the pipeline.\n");
        g_free(capsString);
        goto invalid;
    }

    pixelFormat = pixelFormatForGstVideoFormat(GST_VIDEO_INFO_FORMAT(&videoInfo));
    if (pixelFormat == AR_PIXEL_FORMAT_INVALID) {
        ARLOGe("ARVideo: GStreamer element '%s' negotiated unsupported video format '%s'. Supported formats are %s.\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME, gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&videoInfo)), AR_VIDEO_GSTREAMER_SUPPORTED_FORMATS);
        ARLOGe("ARVideo: Insert 'videoconvert ! video/x-raw,format=RGB' before it in the pipeline.\n");
        goto invalid;
    }
    if (GST_VIDEO_INFO_IS_INTERLACED(&videoInfo)) {
        ARLOGw("ARVideo: GStreamer element '%s' negotiated interlaced video; frames will be delivered as-is.\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
    }

    // Work out the layout of a tightly packed copy of each plane. GStreamer buffers may have
    // padded strides, but artoolkitX expects rows to be contiguous.
    finfo = videoInfo.finfo;
    planeCount = GST_VIDEO_FORMAT_INFO_N_PLANES(finfo);
    if (planeCount > AR_VIDEO_GSTREAMER_MAX_PLANES) goto invalid;
    frameBytes = 0;
    for (p = 0; p < planeCount; p++) {
        // Find the first component stored in this plane; it defines the plane's subsampling and pixel stride.
        for (c = 0; c < GST_VIDEO_FORMAT_INFO_N_COMPONENTS(finfo); c++) {
            if (GST_VIDEO_FORMAT_INFO_PLANE(finfo, c) == p) break;
        }
        if (c == GST_VIDEO_FORMAT_INFO_N_COMPONENTS(finfo)) goto invalid;
        planeRowBytes[p] = (size_t)GST_VIDEO_FORMAT_INFO_SCALE_WIDTH(finfo, c, GST_VIDEO_INFO_WIDTH(&videoInfo)) * GST_VIDEO_FORMAT_INFO_PSTRIDE(finfo, c);
        planeRows[p] = GST_VIDEO_FORMAT_INFO_SCALE_HEIGHT(finfo, c, GST_VIDEO_INFO_HEIGHT(&videoInfo));
        planeOffset[p] = frameBytes;
        frameBytes += planeRowBytes[p] * planeRows[p];
    }

    ARLOGi("ARVideo: GStreamer negotiated %dx%d %s @ %d/%d fps.\n", GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo), gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&videoInfo)), GST_VIDEO_INFO_FPS_N(&videoInfo), GST_VIDEO_INFO_FPS_D(&videoInfo));

    g_mutex_lock(&vid->mutex);
    if (!vid->buffers[0] || frameBytes != vid->frameBytes) {
        // Normally this runs once, before the first frame. If a source renegotiates to a
        // different size mid-stream, the frame most recently returned to the caller is
        // invalidated here; callers must fetch a fresh frame after any size change.
        free(vid->buffers[0]);
        free(vid->buffers[1]);
        vid->buffers[0] = (ARUint8 *)calloc(frameBytes, 1);
        vid->buffers[1] = (ARUint8 *)calloc(frameBytes, 1);
        if (!vid->buffers[0] || !vid->buffers[1]) {
            ARLOGe("Out of memory!!\n");
            free(vid->buffers[0]);
            free(vid->buffers[1]);
            vid->buffers[0] = vid->buffers[1] = NULL;
            vid->capsValid = FALSE;
            vid->frameReady = FALSE;
            g_cond_broadcast(&vid->cond);
            g_mutex_unlock(&vid->mutex);
            return;
        }
    }
    vid->videoInfo = videoInfo;
    vid->width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    vid->height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
    vid->pixelFormat = pixelFormat;
    vid->planeCount = planeCount;
    for (p = 0; p < planeCount; p++) {
        vid->planeOffset[p] = planeOffset[p];
        vid->planeRowBytes[p] = planeRowBytes[p];
        vid->planeRows[p] = planeRows[p];
    }
    vid->frameBytes = frameBytes;
    vid->frameReady = FALSE; // Any frame already copied has the old layout.
    vid->loggedMapFailure = FALSE;
    vid->capsValid = TRUE;
    g_cond_broadcast(&vid->cond);
    g_mutex_unlock(&vid->mutex);
    return;

invalid:
    g_mutex_lock(&vid->mutex);
    vid->capsValid = FALSE;
    vid->frameReady = FALSE;
    g_cond_broadcast(&vid->cond);
    g_mutex_unlock(&vid->mutex);
}

// Called on the streaming thread for every buffer leaving the tap element.
static void handleBuffer(AR2VideoParamGStreamerT *vid, GstBuffer *buffer)
{
    GstVideoFrame frame;
    ARUint8 *dst;
    unsigned int p, row;
    GstClockTime pts;

    g_mutex_lock(&vid->mutex);
    if (!vid->capsValid) {
        g_mutex_unlock(&vid->mutex);
        return;
    }

    // Mapping the frame rather than the buffer lets GStreamer tell us the real
    // per-plane strides and offsets, which may differ from the packed defaults.
    if (!gst_video_frame_map(&frame, &vid->videoInfo, buffer, GST_MAP_READ)) {
        if (!vid->loggedMapFailure) {
            ARLOGe("ARVideo: Unable to map GStreamer video frame for reading.\n");
            vid->loggedMapFailure = TRUE;
        }
        g_mutex_unlock(&vid->mutex);
        return;
    }

    dst = vid->buffers[vid->fillIndex];
    for (p = 0; p < vid->planeCount; p++) {
        const guint8 *src = (const guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, p);
        gsize srcStride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, p);
        ARUint8 *dstPlane = dst + vid->planeOffset[p];
        if (srcStride == vid->planeRowBytes[p]) {
            memcpy(dstPlane, src, vid->planeRowBytes[p] * vid->planeRows[p]);
        } else {
            for (row = 0; row < vid->planeRows[p]; row++) {
                memcpy(dstPlane + row * vid->planeRowBytes[p], src + row * srcStride, vid->planeRowBytes[p]);
            }
        }
    }
    gst_video_frame_unmap(&frame);

    // Timestamp in the pipeline clock's time base if the buffer carries one, otherwise now.
    // The pipeline clock is the monotonic system clock, matching the V4L2 and Android modules.
    // The base time is only meaningful once the pipeline has been asked to go PLAYING, so a
    // non-live preroll frame copied before then uses the monotonic clock directly. The
    // pipeline's reported state can't be used for this test, as it stays PAUSED until every
    // element has finished the transition, by which time a live source may already have
    // pushed frames that carry valid timestamps.
    pts = GST_BUFFER_PTS(buffer);
    if (GST_CLOCK_TIME_IS_VALID(pts) && vid->baseTimeValid) {
        GstClockTime t = gst_element_get_base_time(vid->pipeline) + pts;
        vid->frameTime.sec = (uint64_t)(t / GST_SECOND);
        vid->frameTime.usec = (uint32_t)((t % GST_SECOND) / 1000);
    } else {
        gint64 now = g_get_monotonic_time();
        vid->frameTime.sec = (uint64_t)(now / G_USEC_PER_SEC);
        vid->frameTime.usec = (uint32_t)(now % G_USEC_PER_SEC);
    }
    vid->frameReady = TRUE;
    g_mutex_unlock(&vid->mutex);
}

static GstPadProbeReturn padProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    AR2VideoParamGStreamerT *vid = (AR2VideoParamGStreamerT *)userData;

    if (!vid) return GST_PAD_PROBE_OK;

    if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        if (event && GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps = NULL;
            gst_event_parse_caps(event, &caps); // caps remain owned by the event.
            if (caps) handleCaps(vid, caps);
        }
    } else if (info->type & GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
        if (buffer) handleBuffer(vid, buffer);
    }

    // Never drop or modify data: the buffer continues downstream to the sink, which releases it.
    return GST_PAD_PROBE_OK;
}

// Receives pipeline bus messages synchronously on the posting thread. Nothing else
// reads the bus, so messages are dropped here to stop them accumulating. A handler
// which returns GST_BUS_DROP takes ownership of the message and must unref it.
static GstBusSyncReply busSyncHandler(GstBus *bus, GstMessage *message, gpointer userData)
{
    AR2VideoParamGStreamerT *vid = (AR2VideoParamGStreamerT *)userData;
    GError *err = NULL;
    gchar *debug = NULL;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &err, &debug);
            ARLOGe("ARVideo: GStreamer error from element '%s': %s\n", GST_OBJECT_NAME(GST_MESSAGE_SRC(message)), err ? err->message : "(unknown)");
            if (debug) ARLOGd("ARVideo: GStreamer debug info: %s\n", debug);
            g_clear_error(&err);
            g_free(debug);
            if (vid) {
                g_mutex_lock(&vid->mutex);
                vid->error = TRUE;
                g_cond_broadcast(&vid->cond);
                g_mutex_unlock(&vid->mutex);
            }
            break;
        case GST_MESSAGE_WARNING:
            gst_message_parse_warning(message, &err, &debug);
            ARLOGw("ARVideo: GStreamer warning from element '%s': %s\n", GST_OBJECT_NAME(GST_MESSAGE_SRC(message)), err ? err->message : "(unknown)");
            if (debug) ARLOGd("ARVideo: GStreamer debug info: %s\n", debug);
            g_clear_error(&err);
            g_free(debug);
            break;
        case GST_MESSAGE_EOS:
            ARLOGi("ARVideo: GStreamer pipeline reached end of stream.\n");
            break;
        default:
            break;
    }
    gst_message_unref(message);
    return GST_BUS_DROP;
}

int ar2VideoDispOptionGStreamer( void )
{
    ARPRINT(" -module=GStreamer\n");
    ARPRINT("\n");
    ARPRINT(" The remainder of the configuration string is a GStreamer pipeline description\n");
    ARPRINT(" in gst-launch-1.0 syntax. The pipeline must contain an element named\n");
    ARPRINT(" '%s' (usually 'identity name=%s') producing raw video in one\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME, AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
    ARPRINT(" of the formats %s,\n", AR_VIDEO_GSTREAMER_SUPPORTED_FORMATS);
    ARPRINT(" and must end in a sink (e.g. 'fakesink'). Frames are copied as they pass\n");
    ARPRINT(" through the named element.\n");
    ARPRINT("\n");
    ARPRINT(" If no pipeline is given, the default is:\n");
    ARPRINT("   %s\n", AR_VIDEO_GSTREAMER_DEFAULT_PIPELINE);
    ARPRINT("\n");
    ARPRINT(" Examples:\n");
    ARPRINT("   v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=RGB,width=640,height=480 ! identity name=%s ! fakesink\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
    ARPRINT("   filesrc location=movie.mp4 ! decodebin ! videoconvert ! video/x-raw,format=RGB ! identity name=%s sync=true ! fakesink\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
    ARPRINT("\n");

    return 0;
}

// Wait up to timeout for a pending state change to complete. Returns FALSE on failure.
static gboolean waitForStateChange(AR2VideoParamGStreamerT *vid, GstState target, GstClockTime timeout)
{
    GstStateChangeReturn ret = gst_element_get_state(vid->pipeline, NULL, NULL, timeout);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ARLOGe("ARVideo: GStreamer pipeline failed to change to state %s.\n", gst_element_state_get_name(target));
        return FALSE;
    } else if (ret == GST_STATE_CHANGE_ASYNC) {
        ARLOGw("ARVideo: Timed out waiting for GStreamer pipeline to change to state %s.\n", gst_element_state_get_name(target));
        return FALSE;
    }
    return TRUE;
}

static void cleanup(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return;

    if (vid->pipeline) {
        gst_element_set_state(vid->pipeline, GST_STATE_NULL);
        gst_element_get_state(vid->pipeline, NULL, NULL, AR_VIDEO_GSTREAMER_STATE_CHANGE_TIMEOUT);
    }
    if (vid->tapPad) {
        if (vid->probeId) gst_pad_remove_probe(vid->tapPad, vid->probeId);
        gst_object_unref(vid->tapPad);
    }
    if (vid->tap) gst_object_unref(vid->tap);
    if (vid->bus) {
        gst_bus_set_sync_handler(vid->bus, NULL, NULL, NULL);
        gst_object_unref(vid->bus);
    }
    if (vid->pipeline) gst_object_unref(vid->pipeline);
    free(vid->buffers[0]);
    free(vid->buffers[1]);
    g_cond_clear(&vid->cond);
    g_mutex_clear(&vid->mutex);
    free(vid);
}

AR2VideoParamGStreamerT *ar2VideoOpenGStreamer(const char *config_in)
{
    const char *config;
    AR2VideoParamGStreamerT *vid = NULL;
    GError *error = NULL;
    GstElement *pipeline;
    GstStateChangeReturn ret;
    gint64 deadline;

    // Fall back to the test signal generator if no pipeline was given.
    if (!config_in || !config_in[0]) config = AR_VIDEO_GSTREAMER_DEFAULT_PIPELINE;
    else config = config_in;

    if (!gst_is_initialized()) {
        if (!gst_init_check(NULL, NULL, &error)) {
            ARLOGe("ARVideo: Unable to initialise GStreamer: %s\n", error ? error->message : "(unknown error)");
            g_clear_error(&error);
            return NULL;
        }
    }
    {
        gchar *versionString = gst_version_string();
        ARLOGi("ARVideo: Using %s.\n", versionString);
        g_free(versionString);
    }

    arMallocClear(vid, AR2VideoParamGStreamerT, 1);
    g_mutex_init(&vid->mutex);
    g_cond_init(&vid->cond);
    vid->pixelFormat = AR_PIXEL_FORMAT_INVALID;

    pipeline = gst_parse_launch(config, &error);
    if (error) {
        // gst_parse_launch can return a usable pipeline together with a recoverable error.
        arLog(NULL, pipeline ? AR_LOG_LEVEL_WARN : AR_LOG_LEVEL_ERROR, "ARVideo: GStreamer pipeline parse %s: %s\n", pipeline ? "warning" : "error", error->message);
        g_clear_error(&error);
    }
    if (!pipeline) {
        ARLOGe("ARVideo: Unable to create GStreamer pipeline from '%s'.\n", config);
        cleanup(vid);
        return NULL;
    }
    if (!GST_IS_PIPELINE(pipeline)) {
        ARLOGe("ARVideo: GStreamer configuration '%s' does not describe a pipeline.\n", config);
        gst_object_unref(pipeline);
        cleanup(vid);
        return NULL;
    }
    vid->pipeline = pipeline;

    vid->tap = gst_bin_get_by_name(GST_BIN(vid->pipeline), AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
    if (!vid->tap) {
        ARLOGe("ARVideo: GStreamer pipeline has no element named '%s'. Add 'identity name=%s' before the sink.\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME, AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
        cleanup(vid);
        return NULL;
    }
    vid->tapPad = gst_element_get_static_pad(vid->tap, "src");
    if (!vid->tapPad) {
        ARLOGe("ARVideo: GStreamer element '%s' has no source pad.\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
        cleanup(vid);
        return NULL;
    }
    vid->probeId = gst_pad_add_probe(vid->tapPad, GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, padProbeCallback, vid, NULL);

    vid->bus = gst_element_get_bus(vid->pipeline);
    gst_bus_set_sync_handler(vid->bus, busSyncHandler, vid, NULL);

    // Preroll so that the format is negotiated and we can report the frame size.
    ret = gst_element_set_state(vid->pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ARLOGe("ARVideo: GStreamer pipeline failed to change to state PAUSED.\n");
        cleanup(vid);
        return NULL;
    }
    if (ret == GST_STATE_CHANGE_ASYNC) ret = gst_element_get_state(vid->pipeline, NULL, NULL, AR_VIDEO_GSTREAMER_STATE_CHANGE_TIMEOUT);
    if (ret == GST_STATE_CHANGE_FAILURE || ret == GST_STATE_CHANGE_ASYNC) {
        ARLOGe("ARVideo: GStreamer pipeline %s while changing to state PAUSED.\n", ret == GST_STATE_CHANGE_FAILURE ? "failed" : "timed out");
        cleanup(vid);
        return NULL;
    }
    vid->isLive = (ret == GST_STATE_CHANGE_NO_PREROLL);

    // Live sources (cameras) don't produce data in PAUSED, so briefly run the pipeline
    // until the format has been negotiated, then pause again until capture is started.
    if (vid->isLive) {
        ARLOGd("ARVideo: GStreamer pipeline is live; starting briefly to negotiate format.\n");
        g_mutex_lock(&vid->mutex);
        vid->baseTimeValid = TRUE;
        g_mutex_unlock(&vid->mutex);
        if (gst_element_set_state(vid->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
            ARLOGe("ARVideo: GStreamer pipeline failed to change to state PLAYING.\n");
            cleanup(vid);
            return NULL;
        }
        deadline = g_get_monotonic_time() + (gint64)(AR_VIDEO_GSTREAMER_STATE_CHANGE_TIMEOUT / 1000);
        g_mutex_lock(&vid->mutex);
        while (!vid->capsValid && !vid->error) {
            if (!g_cond_wait_until(&vid->cond, &vid->mutex, deadline)) break;
        }
        g_mutex_unlock(&vid->mutex);
        gst_element_set_state(vid->pipeline, GST_STATE_PAUSED);
    }

    g_mutex_lock(&vid->mutex);
    if (!vid->capsValid) {
        g_mutex_unlock(&vid->mutex);
        ARLOGe("ARVideo: GStreamer pipeline did not deliver a supported raw video format at element '%s'.\n", AR_VIDEO_GSTREAMER_TAP_ELEMENT_NAME);
        cleanup(vid);
        return NULL;
    }
    g_mutex_unlock(&vid->mutex);

    return vid;
}

int ar2VideoCloseGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (-1);

    cleanup(vid);
    return 0;
}

int ar2VideoGetIdGStreamer(AR2VideoParamGStreamerT *vid, ARUint32 *id0, ARUint32 *id1)
{
    if (!vid) return (-1);

    if (id0) *id0 = 0;
    if (id1) *id1 = 0;

    return (-1);
}

int ar2VideoGetSizeGStreamer(AR2VideoParamGStreamerT *vid, int *x, int *y)
{
    if (!vid) return (-1);

    g_mutex_lock(&vid->mutex);
    if (x) *x = vid->width;
    if (y) *y = vid->height;
    g_mutex_unlock(&vid->mutex);
    return (0);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatGStreamer(AR2VideoParamGStreamerT *vid)
{
    AR_PIXEL_FORMAT pixelFormat;

    if (!vid) return (AR_PIXEL_FORMAT_INVALID);

    g_mutex_lock(&vid->mutex);
    pixelFormat = vid->pixelFormat;
    g_mutex_unlock(&vid->mutex);
    return (pixelFormat);
}

AR2VideoBufferT *ar2VideoGetImageGStreamer(AR2VideoParamGStreamerT *vid)
{
    AR2VideoBufferT *buffer;
    ARUint8 *frame;
    unsigned int p;

    if (!vid) return (NULL);

    g_mutex_lock(&vid->mutex);
    if (!vid->frameReady) {
        g_mutex_unlock(&vid->mutex);
        return (NULL);
    }
    // Hand the just-filled buffer to the caller and let the streaming thread fill the other one.
    // The caller is done with the buffer returned by the previous call, so it's safe to reuse.
    frame = vid->buffers[vid->fillIndex];
    vid->fillIndex = 1 - vid->fillIndex;
    vid->frameReady = FALSE;

    buffer = &vid->arVideoBuffer;
    buffer->buff = frame;
    buffer->time = vid->frameTime;
    buffer->fillFlag = 1;
    if (vid->planeCount > 1) {
        for (p = 0; p < vid->planeCount; p++) vid->bufPlanes[p] = frame + vid->planeOffset[p];
        buffer->bufPlanes = vid->bufPlanes;
        buffer->bufPlaneCount = vid->planeCount;
    } else {
        buffer->bufPlanes = NULL;
        buffer->bufPlaneCount = 0;
    }
    switch (vid->pixelFormat) {
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_NV21:
            buffer->buffLuma = frame; // Plane 0 is luma for the bi-planar formats.
            break;
        default:
            buffer->buffLuma = NULL;
            break;
    }
    g_mutex_unlock(&vid->mutex);

    return (buffer);
}

int ar2VideoCapStartGStreamer(AR2VideoParamGStreamerT *vid)
{
    GstStateChangeReturn ret;

    if (!vid) return (-1);

    g_mutex_lock(&vid->mutex);
    vid->baseTimeValid = TRUE;
    g_mutex_unlock(&vid->mutex);
    ret = gst_element_set_state(vid->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ARLOGe("ARVideo: GStreamer pipeline failed to change to state PLAYING.\n");
        return (-1);
    }
    if (ret == GST_STATE_CHANGE_ASYNC && !waitForStateChange(vid, GST_STATE_PLAYING, AR_VIDEO_GSTREAMER_STATE_CHANGE_TIMEOUT)) return (-1);

    ARLOGd("ARVideo: GStreamer pipeline is PLAYING.\n");
    return 0;
}

int ar2VideoCapStopGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (-1);

    if (gst_element_set_state(vid->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        ARLOGe("ARVideo: GStreamer pipeline failed to change to state PAUSED.\n");
        return (-1);
    }

    ARLOGd("ARVideo: GStreamer pipeline is PAUSED.\n");
    return (0);
}

int ar2VideoGetParamiGStreamer(AR2VideoParamGStreamerT *vid, int paramName, int *value)
{
    return -1;
}

int ar2VideoSetParamiGStreamer(AR2VideoParamGStreamerT *vid, int paramName, int value)
{
    return -1;
}

int ar2VideoGetParamdGStreamer(AR2VideoParamGStreamerT *vid, int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdGStreamer(AR2VideoParamGStreamerT *vid, int paramName, double value)
{
    return -1;
}

int ar2VideoGetParamsGStreamer(AR2VideoParamGStreamerT *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsGStreamer(AR2VideoParamGStreamerT *vid, const int paramName, const char *value)
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif // ARVIDEO_INPUT_GSTREAMER
