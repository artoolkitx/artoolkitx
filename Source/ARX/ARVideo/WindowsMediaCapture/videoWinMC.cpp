/*
 *  videoWinMC.cpp
 *  artoolkitX
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
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <ARX/ARVideo/video.h>

#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE

//#pragma comment(lib,"something.lib")


#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>
#include <ARX/AR/sys/WindowsMediaCapture.h>

// Private API.
static bool ar2VideoWinMCInit2(void);
static bool ar2VideoWinMCFinal2(void);
static void ar2VideoWinMCGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec);
static Platform::String^ getWMCVideoMediaSubTypeForARPixelFormat(AR_PIXEL_FORMAT pf);

static int ar2VideoWinMCRefCount = 0;


struct _AR2VideoParamWinMCT {
    int devNum;
    AR_PIXEL_FORMAT format;
    long bufSize;
    AR2VideoBufferT buffer;
	int grabberInited;
	// WinRT-ivars.
	Windows::Devices::Enumeration::Panel preferredDeviceLocation;
	WindowsMediaCapture *wmc;
};

//
// Public functions
//

int ar2VideoDispOptionWinMC(void)
{
    ARPRINT(" -module=WinMC\n");
	ARPRINT(" -devNum=N\n");
    ARPRINT("    Use device number N (default N=1).\n");
    ARPRINT("    e.g. for a second camera input, N=2.\n");
	ARPRINT(" -format=X\n");
    ARPRINT("    Return images with pixels in format X, where X is one of: \n");
    ARPRINT("    BGRA, BGR, NV12/420f, 2vuy/UYVY, yuvs/YUY2, RGB_565, RGBA_5551.\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    Request video format of width N pixels.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    Request video format of height N pixels.\n");
	ARPRINT(" -position=(rear|back|front|left|right|top|bottom|default)\n");
    ARPRINT("    choose between rear/back and front-mounted camera (where available).\n");
    ARPRINT("    other options include cameras mounted on left, right, top or bottom edges of the device.\n");
	ARPRINT(" -flipV\n");
    ARPRINT("    Flip the image vertically.\n");
	ARPRINT(" -noFlipV\n");
    ARPRINT("    Do not flip the image vertically.\n");
	//ARPRINT(" -flipH\n");
    //ARPRINT("    Flip the image horizontally.\n");
	//ARPRINT(" -noFlipH\n");
    //ARPRINT("    Do not flip the image horizontally.\n");
    ARPRINT("\n");

    return 0;
}

static void stopWMC(AR2VideoParamWinMCT *vid)
{
    ARLOGd("stopWMC(): called.\n");
	if (!vid || !vid->wmc) return;

    ARLOGd("stopWMC(): calling vid->wmc->Capturing().\n");
	if (!vid->wmc->Capturing()) {
		ARLOGe("stopWMC(): Windows.Media.Capture already stopped, exiting.\n");
		return;
	}

	vid->wmc->StopCapture();
    ARLOGd("stopWMC(): exiting.\n");
}

static void errorWMC(void *userdata)
{
	if (!userdata) {
		ARLOGe("Windows.Media.Capture error but no userdata suppplied.\n");
		return;
	}
	AR2VideoParamWinMCT *vid = (AR2VideoParamWinMCT *)userdata;
	ARLOGe("Windows.Media.Capture error.\n");
	stopWMC(vid);
}

static bool startWMC(AR2VideoParamWinMCT *vid, const int width, const int height)
{
	if (!vid || !vid->wmc) return false;

	if (vid->wmc->Capturing()) {
		ARLOGe("Windows.Media.Capture already started.\n");
		return false;
	}

	if (!vid->wmc->StartCapture(width, height, getWMCVideoMediaSubTypeForARPixelFormat(vid->format), vid->devNum - 1, vid->preferredDeviceLocation, errorWMC, (void *)vid)) {
		ARLOGe("Error starting capture.\n");
		return false;
	}

	return true;
}

AR2VideoParamWinMCT *ar2VideoOpenWinMC(const char *config)
{
    AR2VideoParamWinMCT     *vid;
	int					     width = 320;
	int					     height = 240;
    int                      flipH = 0, flipV = 0;
	int						 devNum = 0;
	int						 showDialog = -1;
	const char				*a;
    char                     b[256];
	const char config_default[] = "";
	int                      err_i = 0;

	//ARLOGd("Entering ar2VideoOpenWinMC\n");
	if (ar2VideoWinMCRefCount == 0) {
		if (!ar2VideoWinMCInit2()) return NULL;
	}
    ar2VideoWinMCRefCount++;

    arMallocClear(vid, AR2VideoParamWinMCT, 1);
    vid->format = AR_PIXEL_FORMAT_INVALID;
	vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Unknown;

	// Ensure the provided config is valid, otherwise use default config.
	if (!config) a = config_default;
	else if (!config[0]) a = config_default;
    else a = config;
    if (a != NULL) {
        for (;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;
    
            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-devNum=", 8) == 0) {
                if (sscanf(&b[8], "%d", &devNum) != 1) err_i = 1;
                else if (devNum < 0) {
					ARLOGe("Error: device number must be integer beginning with 1, or 0 to use default device.\n");
                    err_i = 1;
				}
            } else if( strncmp( b, "-format=", 8 ) == 0 ) {
                if (strcmp(b+8, "0") == 0) {
                    vid->format = AR_PIXEL_FORMAT_INVALID;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(b+8, "BGRA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGRA;
                    ARLOGi("Requesting images in BGRA format.\n");
                } else if (strcmp(b+8, "BGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGR;
                    ARLOGi("Requesting images in BGR format.\n");
                } else if (strcmp(b+8, "RGB_565") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB_565;
                    ARLOGi("Requesting images in RGB_565 format.\n");
                } else if (strcmp(b+8, "RGBA_5551") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_5551;
                    ARLOGi("Requesting images in RGB_5551 format.\n");
                } else if (strcmp(b+8, "2vuy") == 0 || strcmp(b+8, "UYVY") == 0) {
                    vid->format = AR_PIXEL_FORMAT_2vuy;
                    ARLOGi("Requesting images in 2vuy/UYVY format.\n");
                } else if (strcmp(b+8, "yuvs") == 0 || strcmp(b+8, "YUY2") == 0) {
                    vid->format = AR_PIXEL_FORMAT_yuvs;
                    ARLOGi("Requesting images in yuvs/YUY2 format.\n");
                } else if (strcmp(b+8, "NV21") == 0) {
                    vid->format = AR_PIXEL_FORMAT_NV21;
                    ARLOGi("Requesting images in NV21 format.\n");
                } else if (strcmp(b+8, "420f") == 0 || strcmp(b+8, "NV12") == 0) {
                    vid->format = AR_PIXEL_FORMAT_420f;
                    ARLOGi("Requesting images in 420f/NV12 format.\n");
                } else {
                    ARLOGe("Ignoring request for unsupported video format '%s'.\n", b+8);
                }
            } else if( strncmp( b, "-position=", 10 ) == 0 ) {
                if (strcmp(b+10, "rear") == 0 || strcmp(b+10, "back") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Back;
                } else if (strcmp(b+10, "front") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Front;
                } else if (strcmp(b+10, "left") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Left;
                } else if (strcmp(b+10, "right") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Right;
                } else if (strcmp(b+10, "top") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Top;
                } else if (strcmp(b+10, "bottom") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Bottom;
                } else if (strcmp(b+10, "default") == 0) {
                    vid->preferredDeviceLocation = Windows::Devices::Enumeration::Panel::Unknown;
                } else {
                    ARLOGe("Error: unsupported video device position requested. Using default.\n");
                }
			} else if( strncmp( b, "-width=", 7 ) == 0 ) {
                if( sscanf( &b[7], "%d", &width ) == 0 ) err_i = 1;
            } else if( strncmp( b, "-height=", 8 ) == 0 ) {
                if( sscanf( &b[8], "%d", &height ) == 0 ) err_i = 1;
            } else if (strcmp(b, "-showDialog") == 0)    {
				showDialog = 1;
            } else if (strcmp(b, "-noShowDialog") == 0)    {
				showDialog = 0;
            } else if (strcmp(b, "-flipH") == 0)    {
				flipH = 1;
            } else if (strcmp(b, "-noFlipH") == 0)    {
				flipH = 0;
            } else if (strcmp(b, "-flipV") == 0)    {
				flipV = 1;
            } else if (strcmp(b, "-noFlipV") == 0)    {
				flipV = 0;
            } else if (strcmp(b, "-module=WinMC") == 0) {
				//ARLOGd("Device set to WinMC\n");
			} else {
				ARLOGe("Unrecognized config token: '%s'\n", b);
                ar2VideoDispOptionWinMC();
                return 0;
            }
            
            if (err_i) goto bail;

            while(*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }


	// Defaults.
	if (vid->format == AR_PIXEL_FORMAT_INVALID) vid->format = AR_PIXEL_FORMAT_BGR;

	// Alloc and init WindowsMediaCapture.
	vid->wmc = new WindowsMediaCapture;
	if (!vid->wmc) {
		ARLOGe("Error creating instance of Windows.Media.Capture.\n");
		goto bail;
	}
    if (flipV) vid->wmc->setFlipV(true);
	if (!startWMC(vid, width, height)) goto bail1;
	    
	return vid;

bail1:
	delete vid->wmc;
	vid->wmc = NULL;
bail:
    free(vid);
    ar2VideoWinMCRefCount--;
    if (ar2VideoWinMCRefCount == 0) ar2VideoWinMCFinal2();
    
    return NULL;
}

int ar2VideoCloseWinMC(AR2VideoParamWinMCT *vid)
{
    ARLOGd("ar2VideoCloseWinMC(): called.\n");
    stopWMC(vid);
	if (vid->wmc) {
		delete vid->wmc;
		vid->wmc = NULL;
	}

	free(vid);

    ar2VideoWinMCRefCount--;
    if (ar2VideoWinMCRefCount == 0) ar2VideoWinMCFinal2();
    
    ARLOGd("ar2VideoCloseWinMC(): exiting, returning 0.\n");
    return 0;
} 

int ar2VideoCapStartWinMC(AR2VideoParamWinMCT *vid)
{
    return 0;
}

int ar2VideoCapStopWinMC(AR2VideoParamWinMCT *vid)
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWinMC(AR2VideoParamWinMCT *vid)
{
	if (!vid) return NULL;
    if (vid->format == AR_PIXEL_FORMAT_INVALID) return NULL;
    
	ARUint8 *buf = (ARUint8 *)vid->wmc->GetFrame();
	if (buf) {
		vid->buffer.buff = buf;
		vid->buffer.fillFlag = 1;
		vid->buffer.buffLuma = NULL;
		ar2VideoWinMCGetTimeStamp(&(vid->buffer.time_sec), &(vid->buffer.time_usec));
		return (&vid->buffer);
	} else {
		return NULL;
	}
}

int ar2VideoGetSizeWinMC(AR2VideoParamWinMCT *vid, int *x,int *y)
{
	if (!vid) return -1;
	if (x) *x = vid->wmc->width();
	if (y) *y = vid->wmc->height();
	return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWinMC(AR2VideoParamWinMCT *vid)
{
	if (!vid) return AR_PIXEL_FORMAT_INVALID;

	return vid->format;
}

int ar2VideoGetIdWinMC(AR2VideoParamWinMCT *vid, ARUint32 *id0, ARUint32 *id1)
{
	if (!vid) return -1;
    return -1;
}

int ar2VideoGetParamiWinMC(AR2VideoParamWinMCT *vid, const int paramName, int *value)
{
    return -1;
}

int ar2VideoSetParamiWinMC(AR2VideoParamWinMCT *vid, const int paramName, const int  value)
{
    return -1;
}

int ar2VideoGetParamdWinMC(AR2VideoParamWinMCT *vid, const int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdWinMC(AR2VideoParamWinMCT *vid, const int paramName, const double  value)
{
    return -1;
}

int ar2VideoGetParamsWinMC(AR2VideoParamWinMCT *vid, const int paramName, char **value)
{
    return -1;
}

int ar2VideoSetParamsWinMC(AR2VideoParamWinMCT *vid, const int paramName, const char *value)
{
    return -1;
}

//
// Private functions.
//

// One-time initialisation.
static bool	ar2VideoWinMCInit2(void)
{
	//ARLOGd("Entering ar2VideoWinMCInit2\n");
    
    return true;
}

// One-time finalisation.
static bool ar2VideoWinMCFinal2(void)
{
	return true;
}


static void ar2VideoWinMCGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec)
{
    struct _timeb sys_time;   

    _ftime(&sys_time);   
    *t_sec  = (ARUint32)sys_time.time;
    *t_usec = (ARUint32)sys_time.millitm * 1000;

    return;
}

static Platform::String^ getWMCVideoMediaSubTypeForARPixelFormat(AR_PIXEL_FORMAT pf)
{
    // Match AR_PIXEL_FORMAT to subType. See http://msdn.microsoft.com/en-us/library/windows/apps/windows.media.mediaproperties.mediaencodingsubtypes.aspx
    switch (pf) {
        case AR_PIXEL_FORMAT_BGRA:
			//return Windows::Media::MediaProperties::MediaEncodingSubtypes::Argb32;
			return Windows::Media::MediaProperties::MediaEncodingSubtypes::Rgb32;
            break;
        case AR_PIXEL_FORMAT_BGR:
			return Windows::Media::MediaProperties::MediaEncodingSubtypes::Rgb24;
            break;
        //case AR_PIXEL_FORMAT_RGB_565:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::Rgb555;
        //    break;
        //case AR_PIXEL_FORMAT_RGBA_5551:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::Rgb5551;
        //    break;
        //case AR_PIXEL_FORMAT_2vuy:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::Uyvy;
        //    break;
        case AR_PIXEL_FORMAT_yuvs:
			return Windows::Media::MediaProperties::MediaEncodingSubtypes::Yuy2;
			break;
        case AR_PIXEL_FORMAT_420f:
			return Windows::Media::MediaProperties::MediaEncodingSubtypes::Nv12;
            break;
        //case AR_PIXEL_FORMAT_NV21:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::Nv21;
        //    break;
        //case AR_PIXEL_FORMAT_MONO: // D3DFMT_L8
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::?;
        //    break;
        //case AR_PIXEL_FORMAT_420v:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::?;
        //    break;
        //case AR_PIXEL_FORMAT_ARGB:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::?;
        //    break;
        //case AR_PIXEL_FORMAT_RGBA:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::;
        //    break;
        //case AR_PIXEL_FORMAT_ABGR:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::;
        //    break;
        //case AR_PIXEL_FORMAT_RGB:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::;
        //    break;
        //case AR_PIXEL_FORMAT_RGB4444:
        //    return Windows::Media::MediaProperties::MediaEncodingSubtypes::?;
        //    break;
        default:
            return nullptr;
    }
}

#endif // ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
