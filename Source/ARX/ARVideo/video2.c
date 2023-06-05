/*
 *  video2.c
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*
 *   author: Hirokazu Kato (kato@sys.im.hiroshima-cu.ac.jp)
 *
 *   Revision: 6.0   Date: 2003/09/29
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ARX/ARVideo/video.h>
#include <ARX/AR/config.h>
#include <ARX/ARUtil/time.h>
#ifdef ARVIDEO_INPUT_DUMMY
#include "Dummy/videoDummy.h"
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
#include "External/videoExternal.h"
#endif
#ifdef ARVIDEO_INPUT_V4L2
#include "Video4Linux2/videoV4L2.h"
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
#include "libdc1394/video1394.h"
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
#include "GStreamer/videoGStreamer.h"
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
#include "AVFoundation/videoAVFoundation.h"
#endif
#ifdef ARVIDEO_INPUT_IMAGE
#include "Image/videoImage.h"
#endif
#ifdef ARVIDEO_INPUT_ANDROID
#include "Android/videoAndroid.h"
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
#include "WindowsMediaFoundation/videoWindowsMediaFoundation.h"
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
#include "WindowsMediaCapture/videoWindowsMediaCapture.h"
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
#include "Emscripten/videoEmscripten.h"
#endif
    

static const char *ar2VideoGetConfig(const char *config_in)
{
    const char *config = NULL;

    /* If no config string is supplied, we should use the environment variable, otherwise set a sane default */
    if (!config_in || !(config_in[0])) {
        /* None supplied, lets see if the user supplied one from the shell */
#ifndef _WINRT
        char *envconf = getenv("ARTOOLKITX_VCONF");
        if (envconf && envconf[0]) {
            config = envconf;
            ARLOGi("Using video config from environment \"%s\".\n", envconf);
        } else {
#endif // !_WINRT
            config = NULL;
            ARLOGi("Using default video config.\n");
#ifndef _WINRT
        }
#endif // !_WINRT
    } else {
        config = config_in;
        ARLOGi("Using supplied video config \"%s\".\n", config_in);
    }

    return config;
}

static int ar2VideoGetModuleWithConfig(const char *config, const char **configStringFollowingDevice_p)
{
    int                        module;
    const char                *a;
#   define B_SIZE ((unsigned int)256)
    char                       b[B_SIZE];

    module = arVideoGetDefaultModule();

    if (configStringFollowingDevice_p) *configStringFollowingDevice_p = NULL;

    a = config;
    if (a) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", b) == 0) break;

            if (strcmp(b, "-module=Dummy") == 0)             {
                module = AR_VIDEO_MODULE_DUMMY;
            } else if (strcmp(b, "-module=External") == 0) {
                module = AR_VIDEO_MODULE_EXTERNAL;
            } else if (strcmp(b, "-module=V4L") == 0 || strcmp(b, "-module=V4L2") == 0) {
                module = AR_VIDEO_MODULE_V4L2;
            } else if (strcmp(b, "-module=1394") == 0) {
                module = AR_VIDEO_MODULE_1394;
            } else if (strcmp(b, "-module=GStreamer") == 0)    {
                module = AR_VIDEO_MODULE_GSTREAMER;
                if (configStringFollowingDevice_p) *configStringFollowingDevice_p = a;
            } else if (strcmp(b, "-module=AVFoundation") == 0)    {
                module = AR_VIDEO_MODULE_AVFOUNDATION;
            } else if (strcmp(b, "-module=Image") == 0)    {
                module = AR_VIDEO_MODULE_IMAGE;
            } else if (strcmp(b, "-module=Android") == 0)    {
                module = AR_VIDEO_MODULE_ANDROID;
            } else if (strcmp(b, "-module=WinMF") == 0)    {
                module = AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION;
            } else if (strcmp(b, "-module=WinMC") == 0)    {
                module = AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE;
            } else if (strcmp(b, "-module=Emscripten") == 0)    {
                module = AR_VIDEO_MODULE_EMSCRIPTEN;
            }

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    if (configStringFollowingDevice_p) {
        if (*configStringFollowingDevice_p) {
            while(**configStringFollowingDevice_p != ' ' && **configStringFollowingDevice_p != '\t' && **configStringFollowingDevice_p != '\0') (*configStringFollowingDevice_p)++;
            while(**configStringFollowingDevice_p == ' ' || **configStringFollowingDevice_p == '\t') (*configStringFollowingDevice_p)++;
        } else {
            *configStringFollowingDevice_p = config;
        }
    }

    return (module);
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoList(const char *config_in)
{
    int module = ar2VideoGetModuleWithConfig(ar2VideoGetConfig(config_in), NULL);
#ifdef ARVIDEO_INPUT_DUMMY
    if (module == AR_VIDEO_MODULE_DUMMY) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (module == AR_VIDEO_MODULE_EXTERNAL) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoCreateSourceInfoListV4L2(config_in);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (module == AR_VIDEO_MODULE_1394) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (module == AR_VIDEO_MODULE_GSTREAMER) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoCreateSourceInfoListAVFoundation(config_in);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (module == AR_VIDEO_MODULE_IMAGE) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoCreateSourceInfoListAndroid(config_in);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoCreateSourceInfoListWinMF(config_in);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return (NULL);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return (NULL);
    }
#endif
    return (NULL);
}

void ar2VideoDeleteSourceInfoList(ARVideoSourceInfoListT **p)
{
    int i;

    if (!p || !*p) return;

    for (i = 0; i < (*p)->count; i++) {
        free((*p)->info[i].name);
        free((*p)->info[i].model);
        free((*p)->info[i].UID);
        free((*p)->info[i].open_token);
    }
    free((*p)->info);
    free(*p);

    *p = NULL;
}

AR2VideoParamT *ar2VideoOpen(const char *config_in)
{
    AR2VideoParamT            *vid;
    const char                *config;
    // Some devices won't understand the "-module=" option, so we need to pass
    // only the portion following that option to them.
    const char                *configStringFollowingDevice = NULL;

    arMallocClear(vid, AR2VideoParamT, 1);
    config = ar2VideoGetConfig(config_in);
    vid->module = ar2VideoGetModuleWithConfig(config, &configStringFollowingDevice);

    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
#ifdef ARVIDEO_INPUT_DUMMY
        if ((vid->moduleParam = (void *)ar2VideoOpenDummy(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"Dummy\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
#ifdef ARVIDEO_INPUT_EXTERNAL
        ARLOGe("ar2VideoOpen: Error: module \"External\" requires ar2VideoOpenAsync.\n");
#else
        ARLOGe("ar2VideoOpen: Error: module \"External\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
#ifdef ARVIDEO_INPUT_V4L2
        if ((vid->moduleParam = (void *)ar2VideoOpenV4L2(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"V4L2\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_1394) {
#ifdef ARVIDEO_INPUT_LIBDC1394
        if ((vid->moduleParam = (void *)ar2VideoOpen1394(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"1394\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
#ifdef ARVIDEO_INPUT_GSTREAMER
        if ((vid->moduleParam = (void *)ar2VideoOpenGStreamer(configStringFollowingDevice)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"GStreamer\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
#ifdef ARVIDEO_INPUT_AVFOUNDATION
        if ((vid->moduleParam = (void *)ar2VideoOpenAVFoundation(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"AVFoundation\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
#ifdef ARVIDEO_INPUT_IMAGE
        if ((vid->moduleParam = (void *)ar2VideoOpenImage(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"Image\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
#ifdef ARVIDEO_INPUT_ANDROID
        ARLOGe("ar2VideoOpen: Error: module \"Android\" requires ar2VideoOpenAsync.\n");
#else
        ARLOGe("ar2VideoOpen: Error: module \"Android\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
        if ((vid->moduleParam = (void *)ar2VideoOpenWinMF(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"WinMF\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
        if ((vid->moduleParam = (void *)ar2VideoOpenWinMC(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"WinMC\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
        if ((vid->moduleParam = (void *)ar2VideoOpenEmscripten(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: module \"Emscripten\" not supported on this build/architecture/system.\n");
#endif
    }

    free(vid);
    return NULL;
}

AR2VideoParamT *ar2VideoOpenAsync(const char *config_in, void (*callback)(void *), void *userdata)
{
    AR2VideoParamT            *vid;
    const char                *config;
    // Some devices won't understand the "-module=" option, so we need to pass
    // only the portion following that option to them.
    const char                *configStringFollowingDevice = NULL;

    if (!callback) return NULL;

    arMallocClear(vid, AR2VideoParamT, 1);
    config = ar2VideoGetConfig(config_in);
    vid->module = ar2VideoGetModuleWithConfig(config, &configStringFollowingDevice);

    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
#ifdef ARVIDEO_INPUT_EXTERNAL
        if ((vid->moduleParam = (void *)ar2VideoOpenAsyncExternal(config, callback, userdata)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpenAsync: Error: module \"External\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
#ifdef ARVIDEO_INPUT_AVFOUNDATION
        if ((vid->moduleParam = (void *)ar2VideoOpenAsyncAVFoundation(config, callback, userdata)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpenAsync: Error: module \"AVFoundation\" not supported on this build/architecture/system.\n");
#endif
    }
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
#ifdef ARVIDEO_INPUT_ANDROID
        if ((vid->moduleParam = (void *)ar2VideoOpenAsyncAndroid(config, callback, userdata)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpenAsync: Error: module \"Android\" not supported on this build/architecture/system.\n");
#endif
    }

    free(vid);
    return NULL;
}

int ar2VideoClose(AR2VideoParamT *vid)
{
    int ret;

    if (!vid) return -1;
    if (vid->lumaInfo) {
        if (arVideoLumaFinal(&(vid->lumaInfo)) < 0) {
            ARLOGe("ar2VideoClose: Error disposing of luma info.\n");
        }
    }
    ret = -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        ret = ar2VideoCloseDummy((AR2VideoParamDummyT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        ret = ar2VideoCloseExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        ret = ar2VideoCloseV4L2((AR2VideoParamV4L2T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        ret = ar2VideoClose1394((AR2VideoParam1394T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        ret = ar2VideoCloseGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        ret = ar2VideoCloseAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        ret = ar2VideoCloseImage((AR2VideoParamImageT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        ret = ar2VideoCloseAndroid((AR2VideoParamAndroidT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        ret = ar2VideoCloseWinMF((AR2VideoParamWinMFT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        ret = ar2VideoCloseWinMC((AR2VideoParamWinMCT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        ret = ar2VideoCloseEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam);
    }
#endif
    free (vid);
    return (ret);
}

int ar2VideoDispOption(AR2VideoParamT *vid)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoDispOptionDummy();
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoDispOptionExternal();
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoDispOptionV4L2();
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoDispOption1394();
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoDispOptionGStreamer();
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoDispOptionAVFoundation();
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoDispOptionImage();
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoDispOptionAndroid();
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoDispOptionWinMF();
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoDispOptionWinMC();
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoDispOptionEmscripten();
    }
#endif
    return (-1);
}

AR_VIDEO_MODULE ar2VideoGetModule(AR2VideoParamT *vid)
{
    if (!vid) return -1;
    return vid->module;
}

int ar2VideoGetId(AR2VideoParamT *vid, ARUint32 *id0, ARUint32 *id1)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetIdDummy((AR2VideoParamDummyT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetIdExternal((AR2VideoParamExternalT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetIdV4L2((AR2VideoParamV4L2T *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetId1394((AR2VideoParam1394T *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetIdGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetIdAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetIdImage((AR2VideoParamImageT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetIdAndroid((AR2VideoParamAndroidT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetIdWinMF((AR2VideoParamWinMFT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetIdWinMC((AR2VideoParamWinMCT *)vid->moduleParam, id0, id1);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetIdEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, id0, id1);
    }
#endif
    return (-1);
}

int ar2VideoGetSize(AR2VideoParamT *vid, int *x,int *y)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetSizeDummy((AR2VideoParamDummyT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetSizeExternal((AR2VideoParamExternalT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetSizeV4L2((AR2VideoParamV4L2T *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetSize1394((AR2VideoParam1394T *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetSizeGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetSizeAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetSizeImage((AR2VideoParamImageT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetSizeAndroid((AR2VideoParamAndroidT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetSizeWinMF((AR2VideoParamWinMFT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetSizeWinMC((AR2VideoParamWinMCT *)vid->moduleParam, x, y);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetSizeEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, x, y);
    }
#endif
    return (-1);
}

int ar2VideoGetPixelSize(AR2VideoParamT *vid)
{
    return (arVideoUtilGetPixelSize(ar2VideoGetPixelFormat(vid)));
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormat(AR2VideoParamT *vid)
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetPixelFormatDummy((AR2VideoParamDummyT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetPixelFormatExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetPixelFormatV4L2((AR2VideoParamV4L2T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetPixelFormat1394((AR2VideoParam1394T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetPixelFormatGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetPixelFormatAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetPixelFormatImage((AR2VideoParamImageT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetPixelFormatAndroid((AR2VideoParamAndroidT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetPixelFormatWinMF((AR2VideoParamWinMFT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetPixelFormatWinMC((AR2VideoParamWinMCT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetPixelFormatEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam);
    }
#endif
    return (AR_PIXEL_FORMAT_INVALID);
}

AR2VideoBufferT *ar2VideoGetImage(AR2VideoParamT *vid)
{
    AR2VideoBufferT *ret = NULL;

    if (!vid) return (NULL);
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        ret = ar2VideoGetImageDummy((AR2VideoParamDummyT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        ret = ar2VideoGetImageExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        ret = ar2VideoGetImageV4L2((AR2VideoParamV4L2T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        ret = ar2VideoGetImage1394((AR2VideoParam1394T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        ret = ar2VideoGetImageGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        ret = ar2VideoGetImageAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        ret = ar2VideoGetImageImage((AR2VideoParamImageT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        ret = ar2VideoGetImageAndroid((AR2VideoParamAndroidT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        ret = ar2VideoGetImageWinMF((AR2VideoParamWinMFT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        ret = ar2VideoGetImageWinMC((AR2VideoParamWinMCT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        ret = ar2VideoGetImageEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam);
    }
#endif
    if (ret) {
        // Supply a timestamp if the video module didn't provide one.
        if (!ret->time.sec && !ret->time.usec) {
            arUtilTimeSinceEpoch(&ret->time.sec, &ret->time.usec);
        }
        // Do a conversion to luma-only if the video module didn't provide one.
        if (!ret->buffLuma) {
            AR_PIXEL_FORMAT pixFormat;
            pixFormat = ar2VideoGetPixelFormat(vid);
            if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
                ARLOGe("ar2VideoGetImage unable to get pixel format.\n");
                return (NULL);
            }
            if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21) {
                ret->buffLuma = ret->buff;
            } else {
                if (!vid->lumaInfo) {
                    int xsize, ysize;
                    if (ar2VideoGetSize(vid, &xsize, &ysize) < 0) {
                        ARLOGe("ar2VideoGetImage unable to get size.\n");
                        return (NULL);
                    }
                    vid->lumaInfo = arVideoLumaInit(xsize, ysize, pixFormat);
                    if (!vid->lumaInfo) {
                        ARLOGe("ar2VideoGetImage unable to initialise luma conversion.\n");
                        return (NULL);
                    }
                }
                ret->buffLuma = arVideoLuma(vid->lumaInfo, ret->buff);
            }
        }
    }
    return (ret);
}

int ar2VideoCapStart(AR2VideoParamT *vid)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoCapStartDummy((AR2VideoParamDummyT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoCapStartExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoCapStartV4L2((AR2VideoParamV4L2T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoCapStart1394((AR2VideoParam1394T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoCapStartGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoCapStartAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoCapStartImage((AR2VideoParamImageT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoCapStartAndroid((AR2VideoParamAndroidT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoCapStartWinMF((AR2VideoParamWinMFT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoCapStartWinMC((AR2VideoParamWinMCT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoCapStartEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam);
    }
#endif
    return (-1);
}

int ar2VideoCapStartAsync (AR2VideoParamT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoCapStartAsyncAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, callback, userdata);
    }
#endif
    return (-1);
}

int ar2VideoCapStop(AR2VideoParamT *vid)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoCapStopDummy((AR2VideoParamDummyT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoCapStopExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoCapStopV4L2((AR2VideoParamV4L2T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoCapStop1394((AR2VideoParam1394T *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoCapStopGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoCapStopAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoCapStopImage((AR2VideoParamImageT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
		return ar2VideoCapStopAndroid((AR2VideoParamAndroidT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoCapStopWinMF((AR2VideoParamWinMFT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoCapStopWinMC((AR2VideoParamWinMCT *)vid->moduleParam);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoCapStopEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam);
    }
#endif
    return (-1);
}

int ar2VideoGetParami(AR2VideoParamT *vid, int paramName, int *value)
{
    if (paramName == AR_VIDEO_GET_VERSION) {
#if (AR_HEADER_VERSION_MAJOR >= 10)
        return (-1);
#else
        return (0x01000000 * ((unsigned int)AR_HEADER_VERSION_MAJOR) +
                0x00100000 * ((unsigned int)AR_HEADER_VERSION_MINOR / 10) +
                0x00010000 * ((unsigned int)AR_HEADER_VERSION_MINOR % 10) +
                0x00001000 * ((unsigned int)AR_HEADER_VERSION_TINY / 10) +
                0x00000100 * ((unsigned int)AR_HEADER_VERSION_TINY % 10) +
                0x00000010 * ((unsigned int)AR_HEADER_VERSION_DEV / 10) +
                0x00000001 * ((unsigned int)AR_HEADER_VERSION_DEV % 10)
               );
#endif
    }

    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetParamiDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetParamiExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetParamiV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetParami1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetParamiGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetParamiAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetParamiImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetParamiAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetParamiWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetParamiWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetParamiEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}

int ar2VideoSetParami(AR2VideoParamT *vid, int paramName, int value)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoSetParamiDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoSetParamiExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoSetParamiV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoSetParami1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoSetParamiGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoSetParamiAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoSetParamiImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoSetParamiAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoSetParamiWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoSetParamiWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoSetParamiEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}

int ar2VideoGetParamd(AR2VideoParamT *vid, int paramName, double *value)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetParamdDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetParamdExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetParamdV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetParamd1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetParamdGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetParamdAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetParamdImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetParamdAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetParamdWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetParamdWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetParamdEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}

int ar2VideoSetParamd(AR2VideoParamT *vid, int paramName, double value)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoSetParamdDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoSetParamdExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoSetParamdV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoSetParamd1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoSetParamdGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoSetParamdAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoSetParamdImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoSetParamdAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoSetParamdWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoSetParamdWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoSetParamdEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}


int ar2VideoGetParams(AR2VideoParamT *vid, const int paramName, char **value)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetParamsDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoGetParamsExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetParamsV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoGetParams1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoGetParamsGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetParamsAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetParamsImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetParamsAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoGetParamsWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoGetParamsWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetParamsEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}

int ar2VideoSetParams(AR2VideoParamT *vid, const int paramName, const char *value)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoSetParamsDummy((AR2VideoParamDummyT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoSetParamsExternal((AR2VideoParamExternalT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoSetParamsV4L2((AR2VideoParamV4L2T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoSetParams1394((AR2VideoParam1394T *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_GSTREAMER
    if (vid->module == AR_VIDEO_MODULE_GSTREAMER) {
        return ar2VideoSetParamsGStreamer((AR2VideoParamGStreamerT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoSetParamsAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoSetParamsImage((AR2VideoParamImageT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoSetParamsAndroid((AR2VideoParamAndroidT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoSetParamsWinMF((AR2VideoParamWinMFT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE
    if (vid->module == AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE) {
        return ar2VideoSetParamsWinMC((AR2VideoParamWinMCT *)vid->moduleParam, paramName, value);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoSetParamsEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, paramName, value);
    }
#endif
    return (-1);
}

int ar2VideoSaveParam(AR2VideoParamT *vid, char *filename)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoSaveParam1394((AR2VideoParam1394T *)vid->moduleParam, filename);
    }
#endif
    return (-1);
}

int ar2VideoLoadParam(AR2VideoParamT *vid, char *filename)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_LIBDC1394
    if (vid->module == AR_VIDEO_MODULE_1394) {
        return ar2VideoLoadParam1394((AR2VideoParam1394T *)vid->moduleParam, filename);
    }
#endif
    return (-1);
}

int ar2VideoSetBufferSize(AR2VideoParamT *vid, const int width, const int height)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoSetBufferSizeDummy((AR2VideoParamDummyT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoSetBufferSizeAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoSetBufferSizeImage((AR2VideoParamImageT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoSetBufferSizeEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, width, height);
    }
#endif
    return (-1);
}

int ar2VideoGetBufferSize(AR2VideoParamT *vid, int *width, int *height)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_DUMMY
    if (vid->module == AR_VIDEO_MODULE_DUMMY) {
        return ar2VideoGetBufferSizeDummy((AR2VideoParamDummyT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetBufferSizeAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_IMAGE
    if (vid->module == AR_VIDEO_MODULE_IMAGE) {
        return ar2VideoGetBufferSizeImage((AR2VideoParamImageT *)vid->moduleParam, width, height);
    }
#endif
#ifdef ARVIDEO_INPUT_EMSCRIPTEN
    if (vid->module == AR_VIDEO_MODULE_EMSCRIPTEN) {
        return ar2VideoGetBufferSizeEmscripten((AR2VideoParamEmscriptenT *)vid->moduleParam, width, height);
    }
#endif
    return (-1);
}

int ar2VideoGetCParam(AR2VideoParamT *vid, ARParam *cparam)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetCParamAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, cparam);
    }
#endif
    return (-1);
}

int ar2VideoGetCParamAsync(AR2VideoParamT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return -1;
#if USE_CPARAM_SEARCH
#ifdef ARVIDEO_INPUT_V4L2
    if (vid->module == AR_VIDEO_MODULE_V4L2) {
        return ar2VideoGetCParamAsyncV4L2((AR2VideoParamV4L2T *)vid->moduleParam, callback, userdata);
    }
#endif
#ifdef ARVIDEO_INPUT_AVFOUNDATION
    if (vid->module == AR_VIDEO_MODULE_AVFOUNDATION) {
        return ar2VideoGetCParamAsyncAVFoundation((AR2VideoParamAVFoundationT *)vid->moduleParam, callback, userdata);
    }
#endif
#ifdef ARVIDEO_INPUT_ANDROID
    if (vid->module == AR_VIDEO_MODULE_ANDROID) {
        return ar2VideoGetCParamAsyncAndroid((AR2VideoParamAndroidT *)vid->moduleParam, callback, userdata);
    }
#endif
#endif // USE_CPARAM_SEARCH
    return (-1);
}

int ar2VideoPushInit(AR2VideoParamT *vid, int width, int height, const char *pixelFormat, int cameraIndex, int cameraPosition)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoPushInitExternal((AR2VideoParamExternalT *)vid->moduleParam, width, height, pixelFormat, cameraIndex, cameraPosition);
    }
#endif
    return (-1);
}

int ar2VideoPush(AR2VideoParamT *vid,
                 ARUint8 *buf0p, int buf0Size, int buf0PixelStride, int buf0RowStride,
                 ARUint8 *buf1p, int buf1Size, int buf1PixelStride, int buf1RowStride,
                 ARUint8 *buf2p, int buf2Size, int buf2PixelStride, int buf2RowStride,
                 ARUint8 *buf3p, int buf3Size, int buf3PixelStride, int buf3RowStride)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoPushExternal((AR2VideoParamExternalT *)vid->moduleParam, buf0p, buf0Size, buf0PixelStride, buf0RowStride, buf1p, buf1Size, buf1PixelStride, buf1RowStride, buf2p, buf2Size, buf2PixelStride, buf2RowStride, buf3p, buf3Size, buf3PixelStride, buf3RowStride);
    }
#endif
    return (-1);
}

int ar2VideoPushFinal(AR2VideoParamT *vid)
{
    if (!vid) return -1;
#ifdef ARVIDEO_INPUT_EXTERNAL
    if (vid->module == AR_VIDEO_MODULE_EXTERNAL) {
        return ar2VideoPushFinalExternal((AR2VideoParamExternalT *)vid->moduleParam);
    }
#endif
    return (-1);
}
