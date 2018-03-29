/*
 *	videoGStreamer.h
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

/* include GLib for GStreamer */
#include <glib.h>

/* include GStreamer itself */
#include <gst/gst.h>

/* using memcpy */
#include <string.h>

#define GSTREAMER_TEST_LAUNCH_CFG "videotestsrc ! video/x-raw, format=RGB,width=640,height=480,framerate=30/1 ! identity name=artoolkit sync=true ! fakesink"

struct _AR2VideoParamGStreamerT {
	
	/* size and pixel format of the image */
	int	width, height;
    AR_PIXEL_FORMAT pixelFormat;

	/* the actual video buffer */
    ARUint8             *videoBuffer;
    AR2VideoBufferT arVideoBuffer;

	/* GStreamer pipeline */
	GstElement *pipeline;
	
	/* GStreamer identity needed for probing */
	GstElement *probe;

};


static gboolean cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);
	const GstCaps *caps;
	GstStructure *str;
	
	gint width,height;
	gdouble rate;
	
	AR2VideoParamGStreamerT *vid = (AR2VideoParamGStreamerT *)u_data;

	if (vid == NULL) return FALSE;

	/* only do initialy for the buffer */
	if (vid->videoBuffer == NULL && buffer) {
		g_print("ARVideo error! Buffer not allocated\n");
	}

	if (gst_buffer_map(buffer, info, GST_MAP_READ)) {
		memcpy(vid->videoBuffer, (void *)info->data, info->size);
		gst_buffer_unmap(buffer, info);
	} else {
		g_print("ARVideo error! Buffer not readable\n");
	}
	return TRUE;
}

int ar2VideoDispOptionGStreamer( void )
{
    return 0;
}

static void video_caps_notify(GObject* obj, GParamSpec* pspec, gpointer data)
{
	const GstCaps *caps;
	GstStructure *str;
	
	gint width = 0, height = 0, framerate_n = 0, framerate_d = 0;
	gdouble framerate = 0.0;
	
	AR2VideoParamGStreamerT *vid = (AR2VideoParamGStreamerT*)data;

	caps = gst_pad_get_current_caps((GstPad*)obj);

	if (caps) {

		str=gst_caps_get_structure(caps,0);

		/* Get some data about the frame */
		gst_structure_get_int(str, "width", &width);
		gst_structure_get_int(str, "height", &height);
        if (gst_structure_get_fraction(str, "framerate", &framerate_n, &framerate_d)) {
            if (framerate_d) framerate = (gdouble)framerate_n / (gdouble)framerate_d;
        }
		
		g_print("ARVideo: GStreamer negotiated %dx%d@%.3ffps\n", width, height, framerate);

		vid->width = width;
		vid->height = height;
		vid->pixelFormat = AR_PIXEL_FORMAT_RGB;

		g_print("ARVideo: allocating %d bytes\n",(vid->width * vid->height * arVideoUtilGetPixelSize(vid->pixelFormat)));

		/* allocate the buffer */	
		arMalloc(vid->videoBuffer, ARUint8, (vid->width * vid->height * arVideoUtilGetPixelSize(vid->pixelFormat)) );
	}
}

AR2VideoParamGStreamerT *ar2VideoOpenGStreamer(const char *config_in)
{
    const char *config = NULL;
    AR2VideoParamGStreamerT *vid = NULL;
    GError *error = NULL;
    int i;
    GstPad *pad, *peerpad;
    GstStateChangeReturn _ret;
    int is_live;

    /* setting up defaults - we fall back to the TV test signal simulator */
    if (!config_in) config = GSTREAMER_TEST_LAUNCH_CFG;
    else if (!config_in[0]) config = GSTREAMER_TEST_LAUNCH_CFG;
    else config = config_in;

    /* initialise GStreamer */
    gst_init(0,0);

    /* init ART structure */
    arMalloc( vid, AR2VideoParamGStreamerT, 1 );

    /* initialise buffer */
    vid->videoBuffer = NULL;

    /* report the current version and features */
    g_print ("ARVideo: %s\n", gst_version_string());

    vid->pipeline = gst_parse_launch (config, &error);

    if (!vid->pipeline) {
        g_print ("Parse error: %s\n", error->message);
        free(vid);
        return (NULL);
    };

    /* get the video sink */
    vid->probe = gst_bin_get_by_name(GST_BIN(vid->pipeline), "artoolkit");

    if (!vid->probe) {
        g_print("Pipeline has no element named 'artoolkit'!\n");
        free(vid);
        return (NULL);
    };

    /* get the pad from the probe (the source pad seems to be more flexible) */
    pad = gst_element_get_static_pad(vid->probe, "src");

    /* get the peerpad aka sink */
    peerpad = gst_pad_get_peer(pad);

    /* install the probe callback for capturing */
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, cb_have_data, vid, NULL);

    g_signal_connect(pad, "notify::caps", G_CALLBACK(video_caps_notify), vid);

    /* Needed to fill the information for ARVidInfo */
    gst_element_set_state (vid->pipeline, GST_STATE_READY);

    /* wait until it's up and running or failed. */
    if (GST_STATE_CHANGE_FAILURE == (_ret = gst_element_get_state(vid->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE))) {
        g_error ("ARVideo: failed to put GStreamer into READY state!\n");
    } else {
        is_live = (_ret == GST_STATE_CHANGE_NO_PREROLL) ? 1 : 0;
        g_print ("ARVideo: GStreamer pipeline is READY!\n");
    }

    /* Needed to fill the information for ARVidInfo */
    _ret = gst_element_set_state(vid->pipeline, GST_STATE_PAUSED);

    is_live = (_ret == GST_STATE_CHANGE_NO_PREROLL) ? 1 : 0;

    /* wait until it's up and running or failed */
    if (gst_element_get_state (vid->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE) {
        g_error ("ARVideo: failed to put GStreamer into PAUSED state!\n");
    } else {
        g_print ("ARVideo: GStreamer pipeline is PAUSED.\n");
    }
    /* dismiss the pad */
    gst_object_unref (pad);

    /* dismiss the peer-pad */
    gst_object_unref (peerpad);

    /* now preroll for live sources */
    if (is_live) {

        g_print ("ARVideo: Performing special prerolling for GStreamer live source.\n");

        /* set playing state of the pipeline */
        gst_element_set_state (vid->pipeline, GST_STATE_PLAYING);

        /* wait until it's up and running or failed */
        if (gst_element_get_state (vid->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE) {
            g_error ("ARVideo: failed to put GStreamer into PLAYING state!\n");
        } else {
            g_print ("ARVideo: GStreamer pipeline is PLAYING.\n");
        }

        /* set playing state of the pipeline */
        gst_element_set_state (vid->pipeline, GST_STATE_PAUSED);

        /* wait until it's up and running or failed */
        if (gst_element_get_state (vid->pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
            g_error ("ARVideo: failed to put GStreamer into PAUSED state!\n");
        } else {
            g_print ("ARVideo: GStreamer pipeline is PAUSED.\n");
        }
    }

    /* return the video handle */
    return (vid);
}

int ar2VideoCloseGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (-1);
    
	/* stop the pipeline */
	gst_element_set_state (vid->pipeline, GST_STATE_NULL);
	
	/* free the pipeline handle */
	gst_object_unref (GST_OBJECT (vid->pipeline));

	return 0;
}


int ar2VideoGetIdGStreamer(AR2VideoParamGStreamerT *vid, ARUint32 *id0, ARUint32 *id1)
{
    if (!vid) return (-1);
	
    *id0 = 0;
    *id1 = 0;
	
    return (-1);
}

int ar2VideoGetSizeGStreamer(AR2VideoParamGStreamerT *vid, int *x, int *y) 
{
    if (!vid) return (-1);
    
    *x = vid->width; // width of your static image
    *y = vid->height; // height of your static image
    return (0);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    
    return (vid->pixelFormat);
}

AR2VideoBufferT *ar2VideoGetImageGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (NULL);
    
	/* just return the bare video buffer */
    (vid->arVideoBuffer).buff = vid->videoBuffer;
    (vid->arVideoBuffer).buffLuma = NULL;
    (vid->arVideoBuffer).fillFlag = 1;
    return (&(vid->arVideoBuffer));
}

int ar2VideoCapStartGStreamer(AR2VideoParamGStreamerT *vid) 
{
    if (!vid) return (-1);

	GstStateChangeReturn _ret;

	/* set playing state of the pipeline */
	_ret = gst_element_set_state (vid->pipeline, GST_STATE_PLAYING);

	if (_ret == GST_STATE_CHANGE_ASYNC) {

		/* wait until it's up and running or failed */
		if (gst_element_get_state (vid->pipeline, 
				NULL, NULL, GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE) 
		{
    		g_error ("ARVideo: failed to put GStreamer into PLAYING state!\n");    	
    		return -1;
  
        } else {
			g_print ("ARVideo: GStreamer pipeline is PLAYING.\n");
		} 
	}
	return 0; 
}

int ar2VideoCapStopGStreamer(AR2VideoParamGStreamerT *vid)
{
    if (!vid) return (-1);
    
	/* stop pipeline */
    if (gst_element_set_state (vid->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        g_error ("ARVideo: failed to put GStreamer into PAUSED state!\n");    	
        return -1;
        
    } else {
        g_print ("ARVideo: GStreamer pipeline is PAUSED.\n");
    } 
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
