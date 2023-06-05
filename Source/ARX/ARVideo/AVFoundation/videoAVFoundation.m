/*
 *  videoAVFoundation.m
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
 *  Copyright 2008-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2008-05-04	PRL		Written.
 *
 */

#include <ARX/ARVideo/video.h>    // arParamLoadFromBuffer().
#include "videoAVFoundation.h"

#ifdef ARVIDEO_INPUT_AVFOUNDATION

#import <Foundation/Foundation.h>
#import "CameraVideo.h"
#import "MovieVideo.h"
#import "videoAVFoundationCameraVideoTookPictureDelegate.h"
#import "cparams.h"
#import "../cparamSearch.h"
#if TARGET_OS_IOS
#  import <UIKit/UIKit.h> // UIDevice
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>

struct _AR2VideoParamAVFoundationT  {
    CameraVideo       *cameraVideo;
    MovieVideo        *movieVideo;
    AR2VideoBufferT    buffer;
    videoAVFoundationCameraVideoTookPictureDelegate *cameraVideoTookPictureDelegate;
    AR_VIDEO_FRAME_READY_CALLBACK frameReadyCallback;
    void *frameReadyUserdata;
    AR_VIDEO_AVFOUNDATION_FOCUS_PRESET focalLengthPreset;
    float              focalLength;
    float              focusPointOfInterestX;
    float              focusPointOfInterestY;
    BOOL               itsAMovie;
    UInt64             currentFrameTimestamp;
    UInt64             hostClockFrequency;
    void             (*cparamSearchCallback)(const ARParam *, void *);
    void              *cparamSearchUserdata;
    char              *device_id;
    char              *name;
};

int getFrameParameters(AR2VideoParamAVFoundationT *vid);

int ar2VideoDispOptionAVFoundation( void )
{
    ARPRINT(" -module=AVFoundation\n");
    ARPRINT("\n");
    ARPRINT(" -source=n Choose input n (numbered from 0).\n");
    ARPRINT(" -uid=X Choose input with UID X.\n");
    ARPRINT(" -preset=(qvga|cif|480p|540p|vga|720p|1080p|2160p|low|medium|high)\n");
    ARPRINT("     specify camera settings preset to use. cif=352x288, vga/480p=640x480,\n");
    ARPRINT("     720p=1280x720, 1080p=1920x1080, 2160p=3140x2160, qvga=320x240, 540p=960x540.\n");
    ARPRINT("     default value is 'medium'.\n");
    ARPRINT("     qvga|540p not available on iOS.\n");
    ARPRINT(" -position=(rear|back|front)\n");
    ARPRINT("    choose between rear/back and front-mounted camera (where available).\n");
    ARPRINT("    default value is 'rear'.\n");
    ARPRINT(" -format=(BGRA|420v|420f|2vuy|yuvs|RGBA)\n");
    ARPRINT("    choose format of pixels returned by arVideoGetImage().\n");
    ARPRINT("    default value is '420f'.\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    request width of returned images.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    request height of returned images.\n");
    ARPRINT(" -bufferpow2\n");
    ARPRINT("    requests that images are returned in a buffer which has power-of-two dimensions. N.B. IGNORED IN THIS RELEASE.\n");
    ARPRINT(" -[no]flipv\n");
    ARPRINT("    Flip camera image vertically.\n");
    ARPRINT(" -[no]mt\n");
    ARPRINT("    \"Multithreaded\", i.e. allow new frame callbacks on non-main thread.\n");
    //ARPRINT(" -[no]fliph\n");
    //ARPRINT("    Flip camera image horizontally.\n");
    ARPRINT(" -cachedir=/path/to/cparam_cache.db\n");
    ARPRINT("    Specifies the path in which to look for/store camera parameter cache files.\n");
    ARPRINT("    Default is app's cache directory\n");
    ARPRINT(" -cacheinitdir=/path/to/cparam_cache_init.db\n");
    ARPRINT("    Specifies the path in which to look for/store initial camera parameter cache file.\n");
    ARPRINT("    Default is app's bundle directory.\n");
    ARPRINT("\n");

    return 0;
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListAVFoundation(const char *config_in)
{
    NSArray *captureDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    int count = (int)captureDevices.count;
    if (count <= 0) {
        return NULL;
    }

    ARVideoSourceInfoListT *sil;
    arMallocClear(sil, ARVideoSourceInfoListT, 1);
    sil->count = count;
    arMallocClear(sil->info, ARVideoSourceInfoT, count);
    int i = 0;
    for (AVCaptureDevice *captureDevice in captureDevices) {
        
        // Position.
        switch (captureDevice.position) {
            case AVCaptureDevicePositionBack:
                sil->info[i].flags |= AR_VIDEO_POSITION_BACK;
                break;
            case AVCaptureDevicePositionFront:
                sil->info[i].flags |= AR_VIDEO_POSITION_FRONT;
                break;
            case AVCaptureDevicePositionUnspecified:
            default:
                sil->info[i].flags |= AR_VIDEO_POSITION_UNKNOWN;
                break;
        }
        
        sil->info[i].flags |= AR_VIDEO_SOURCE_INFO_FLAG_OPEN_ASYNC; // All AVFoundation require async opening.
        sil->info[i].name = strdup([captureDevice.localizedName UTF8String]);
        sil->info[i].model = strdup([captureDevice.modelID UTF8String]);
        sil->info[i].UID = strdup([captureDevice.uniqueID UTF8String]);
        if (asprintf(&sil->info[i].open_token, "-uid=%s", sil->info[i].UID) < 0) {
            ARLOGperror(NULL);
            sil->info[i].open_token = NULL;
        }
        i++;
    }
    
    return (sil);
}

AR2VideoParamAVFoundationT *ar2VideoOpenAVFoundation( const char *config )
{
    return (ar2VideoOpenAsyncAVFoundation(config, NULL, NULL));
}

AR2VideoParamAVFoundationT *ar2VideoOpenAsyncAVFoundation(const char *config, void (*callback)(void *), void *userdata)
{
	int                 err_i = 0;
    char               *cacheDir = NULL;
    char               *cacheInitDir = NULL;
    char               *csdu = NULL;
    char               *csat = NULL;
    AR2VideoParamAVFoundationT *vid;
    const char         *a;
    char                b[1024];
    int                 itsAMovie = 0;
    char				movieConf[256] = "-pause -loop";
	int					i;
    int                 width = 0;
    int                 height = 0;
    uint32_t            format = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
	int                 flipH = -1, flipV = -1;
    AVCaptureDevicePosition position = AVCaptureDevicePositionUnspecified;
    int                 camera_index = 0;
    NSString            *preset = nil, *uid = nil;
    BOOL                multithreaded = FALSE;

    if (config) {
        a = config;
        err_i = 0;
        for (;;) {
            
            if (itsAMovie) {
                NSURL *url;
                NSString *pathnameOrURL = [NSString stringWithCString:b encoding:NSUTF8StringEncoding];
                //url = [NSURL URLWithString:pathnameOrURL];
                url = [[NSBundle mainBundle] URLForResource:pathnameOrURL withExtension:nil];
                if (!url) {
                    ARLOGe("Unable to locate requested movie resource '%s'.\n", b);
                    return (NULL);
                }

                strncat(movieConf, a, sizeof(movieConf) - strlen(movieConf) - 1);
                
                MovieVideo *movieVideo = [[MovieVideo alloc] initWithURL:url config:movieConf];
                [movieVideo start];
                
                // Allocate the parameters structure and fill it in.
				arMallocClear(vid, AR2VideoParamAVFoundationT, 1);
				vid->itsAMovie = TRUE;
				vid->movieVideo = movieVideo;
				return (vid);

            } else {
                while( *a == ' ' || *a == '\t' ) a++; // Skip whitespace.
                if( *a == '\0' ) break;
                
                if( sscanf(a, "%s", b) == 0 ) break;
                
                if (strncmp(a, "-movie=", 7) == 0) {
                    // Attempt to read in movie pathname or URL, allowing for quoting of whitespace.
                    a += 7; // Skip "-movie=" characters.
                    if (*a == '"') {
                        a++;
                        // Read all characters up to next '"'.
                        i = 0;
                        while (i < (sizeof(b) - 1) && *a != '\0') {
                            b[i] = *a;
                            a++;
                            if (b[i] == '"') break;
                            i++;
                        }
                        b[i] = '\0';
                    } else {
                        sscanf(a, "%s", b);
                    }
                    if (!strlen(b)) err_i = 1;
                    else itsAMovie = 1;
                } else if( strncmp( b, "-width=", 7 ) == 0 ) {
                    if( sscanf( &b[7], "%d", &width ) == 0 ) {
                        ar2VideoDispOptionAVFoundation();
                        return (NULL);
                    }   
                } else if( strncmp( b, "-height=", 8 ) == 0 ) {
                    if( sscanf( &b[8], "%d", &height ) == 0 ) {
                        ar2VideoDispOptionAVFoundation();
                        return (NULL);
                    }   
                } else if( strncmp( b, "-format=", 8 ) == 0 ) {
                    if (strcmp(b+8, "0") == 0) {
                        format = 0;
                        ARLOGi("Requesting images in system default format.\n");
                    } else if (strcmp(b+8, "BGRA") == 0) {
                        format = kCVPixelFormatType_32BGRA;
                        ARLOGi("Requesting images in BGRA format.\n");
                    } else if (strcmp(b+8, "420v") == 0) {
                        format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
                        ARLOGi("Requesting images in 420v format.\n");
                    } else if (strcmp(b+8, "420f") == 0) {
                        format = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
                        ARLOGi("Requesting images in 420f format.\n");
                    } else if (strcmp(b+8, "2vuy") == 0) {
                        format = kCVPixelFormatType_422YpCbCr8;
                        ARLOGi("Requesting images in 2vuy format.\n");
                    } else if (strcmp(b+8, "yuvs") == 0) {
                        format = kCVPixelFormatType_422YpCbCr8_yuvs;
                        ARLOGi("Requesting images in yuvs format.\n");
                    } else if (strcmp(b+8, "RGBA") == 0) {
                        format = kCVPixelFormatType_32RGBA;
                        ARLOGi("Requesting images in RGBA format.\n");
                    } else {
                        ARLOGw("Ignoring request for unsupported video format '%s'.\n", b+8);
                    }
                } else if (strncmp(b, "-source=", 8) == 0) {
                    if (sscanf(b+8, "%d", &camera_index) == 0) err_i = 1;
                    else ARLOGi("Requesting capture device with index %d.\n", camera_index);
                } else if (strncmp(b, "-uid=", 5) == 0) {
                    uid = [NSString stringWithUTF8String:b+5];
                    if (!uid) err_i = 1;
                    else ARLOGi("Requesting capture device with UID '%s'.\n", b+5);
                } else if (strncmp(b, "-preset=", 8) == 0) {
                    if (strcmp(b+8, "medium") == 0) {
                        preset = AVCaptureSessionPresetMedium;
                        ARLOGi("Requesting capture session preset 'medium'.\n");
                    } else if (strcmp(b+8, "high") == 0) {
                        preset = AVCaptureSessionPresetHigh;
                        ARLOGi("Requesting capture session preset 'high'.\n");
                    } else if (strcmp(b+8, "low") == 0) {
                        preset = AVCaptureSessionPresetLow;
                        ARLOGi("Requesting capture session preset 'low'.\n");
                    } else if (strcmp(b+8, "1080p") == 0) {
                        if (@available(macos 10.15, ios 5.0, macCatalyst 14.0, *)) {
                            preset = AVCaptureSessionPreset1920x1080;
                            ARLOGi("Requesting capture session preset '1080p'.\n");
                        } else {
                            ARLOGw("Ignoring request for unsupported 1920x1080 format.\n");
                        }
                    } else if (strcmp(b+8, "2160p") == 0) {
                        if (@available(macos 10.15, ios 9.0, macCatalyst 14.0, *)) {
                            preset = AVCaptureSessionPreset3840x2160;
                            ARLOGi("Requesting capture session preset '2160p'.\n");
                        } else {
                            ARLOGw("Ignoring request for unsupported 3840x2160 format.\n");
                        }
                    } else if (strcmp(b+8, "720p") == 0) {
                        preset = AVCaptureSessionPreset1280x720;
                        ARLOGi("Requesting capture session preset '720p'.\n");
                    } else if (strcmp(b+8, "480p") == 0 || strcmp(b+8, "vga") == 0) {
                        preset = AVCaptureSessionPreset640x480;
                        ARLOGi("Requesting capture session preset 'vga'.\n");
                    } else if (strcmp(b+8, "cif") == 0) {
                        preset = AVCaptureSessionPreset352x288;
                        ARLOGi("Requesting capture session preset 'cif'.\n");
                    } else if (strcmp(b+8, "photo") == 0) {
                        ARLOGi("Requesting capture session preset 'photo'.\n");
                        preset = AVCaptureSessionPresetPhoto;
                    } else if (strcmp(b+8, "540p") == 0) {
#if !TARGET_OS_IOS
                        preset = AVCaptureSessionPreset960x540;
                        ARLOGi("Requesting capture session preset '540p'.\n");
#else
                        ARLOGw("Ignoring request for unsupported 960x540 format.\n");
#endif
                    } else if (strcmp(b+8, "qvga") == 0) {
#if !TARGET_OS_IOS
                        preset = AVCaptureSessionPreset320x240;
                        ARLOGi("Requesting capture session preset 'qvga'.\n");
#else
                        ARLOGw("Ignoring request for unsupported 320x240 format.\n");
#endif
                    } else {
                        ARLOGw("Unsupported video preset requested. Using default.\n");
                    }
                } else if( strncmp( b, "-position=", 10 ) == 0 ) {
                    if (strcmp(b+10, "rear") == 0 || strcmp(b+10, "back") == 0) {
                        position = AVCaptureDevicePositionBack;
                    } else if (strcmp(b+10, "front") == 0) {
                        position = AVCaptureDevicePositionFront;
                    } else {
                        ARLOGw("Unsupported video device position requested. Using default.\n");
                    }
                } else if (strcmp(b, "-flipv") == 0) {
                    flipV = 1;
                } else if (strcmp(b, "-noflipv") == 0) {
                    flipV = 0;
                } else if (strcmp(b, "-fliph") == 0) {
                    flipH = 1;
                } else if (strcmp(b, "-nofliph") == 0) {
                    flipH = 0;
                } else if (strcmp(b, "-mt") == 0) {
                    ARLOGi("Setting video capture callbacks to non-main thread.\n");
                    multithreaded = TRUE;
                } else if (strcmp(b, "-nomt") == 0) {
                    ARLOGi("Setting video capture callbacks to main thread.\n");
                    multithreaded = FALSE;
                } else if (strcmp(b, "-bufferpow2") == 0) {
                    ARLOGw("\"-bufferpow2\" argument, \"Images are returned in power-of-2 sized buffer\" is currently ignored.\n");
                } else if (strncmp(a, "-cachedir=", 10) == 0) {
                    // Attempt to read in pathname, allowing for quoting of whitespace.
                    a += 10; // Skip "-cachedir=" characters.
                    if (*a == '"') {
                        a++;
                        // Read all characters up to next '"'.
                        i = 0;
                        while (i < (sizeof(b) - 1) && *a != '\0') {
                            b[i] = *a;
                            a++;
                            if (b[i] == '"') break;
                            i++;
                        }
                        b[i] = '\0';
                    } else {
                        sscanf(a, "%s", b);
                    }
                    if (!strlen(b)) {
                        ARLOGe("Error: Configuration option '-cachedir=' must be followed by path (optionally in double quotes).\n");
                        err_i = 1;
                    } else {
                        free(cacheDir);
                        cacheDir = strdup(b);
                    }
                } else if (strncmp(a, "-cacheinitdir=", 14) == 0) {
                    // Attempt to read in pathname, allowing for quoting of whitespace.
                    a += 14; // Skip "-cacheinitdir=" characters.
                    if (*a == '"') {
                        a++;
                        // Read all characters up to next '"'.
                        i = 0;
                        while (i < (sizeof(b) - 1) && *a != '\0') {
                            b[i] = *a;
                            a++;
                            if (b[i] == '"') break;
                            i++;
                        }
                        b[i] = '\0';
                    } else {
                        sscanf(a, "%s", b);
                    }
                    if (!strlen(b)) {
                        ARLOGe("Error: Configuration option '-cacheinitdir=' must be followed by path (optionally in double quotes).\n");
                        err_i = 1;
                    } else {
                        free(cacheInitDir);
                        cacheInitDir = strdup(b);
                    }
                } else if (strncmp(a, "-csdu=", 6) == 0) {
                    // Attempt to read in download URL.
                    a += 6; // Skip "-csdu=" characters.
                    sscanf(a, "%s", b);
                    free(csdu);
                    if (!strlen(b)) {
                        csdu = NULL;
                    } else {
                        csdu = strdup(b);
                    }
                } else if (strncmp(a, "-csat=", 6) == 0) {
                    // Attempt to read in authentication token, allowing for quoting of whitespace.
                    a += 6; // Skip "-csat=" characters.
                    if (*a == '"') {
                        a++;
                        // Read all characters up to next '"'.
                        i = 0;
                        while (i < (sizeof(b) - 1) && *a != '\0') {
                            b[i] = *a;
                            a++;
                            if (b[i] == '"') break;
                            i++;
                        }
                        b[i] = '\0';
                    } else {
                        sscanf(a, "%s", b);
                    }
                    free(csat);
                    if (!strlen(b)) {
                        csat = NULL;
                    } else {
                        csat = strdup(b);
                    }
                } else if (strcmp(b, "-module=AVFoundation") == 0) { // Ignored.
                } else {
                    err_i = 1;
                }
                
                if (err_i) {
					ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                    ar2VideoDispOptionAVFoundation();
                    return (NULL);
                }
                
                while( *a != ' ' && *a != '\t' && *a != '\0') a++;
            }
        }// end: for (;;)
    }// end: if (config)
    
    // Post-process options.
    // If the user requested width and height, and not any particular preset, try to match them
    // to a suitable preset.
    if (!preset) {
        if (width && height) {
            if (width == 640 && height == 480) {
                preset = AVCaptureSessionPreset640x480;
                ARLOGi("Requesting capture session preset '480p'.\n");
            } else if (width == 352 && height == 288) {
                preset = AVCaptureSessionPreset352x288;
                ARLOGi("Requesting capture session preset 'cif'.\n");
            } else if (width == 1280 && height == 720) {
                preset = AVCaptureSessionPreset1280x720;
                ARLOGi("Requesting capture session preset '720p'.\n");
#if !TARGET_OS_IOS
            } else if (width == 320 && height == 240) {
                preset = AVCaptureSessionPreset320x240;
                ARLOGi("Requesting capture session preset 'qvga'.\n");
            } else if (width == 960 && height == 540) {
                preset = AVCaptureSessionPreset960x540;
                ARLOGi("Requesting capture session preset '540p'.\n");
#endif
            } else if (width == 1920 && height == 1080) {
                if (@available(macos 10.15, ios 5.0, macCatalyst 14.0, *)) preset = AVCaptureSessionPreset1920x1080;
                ARLOGi("Requesting capture session preset '1080p'.\n");
            } else if (width == 3840 && height == 2160 ) {
                if (@available(macos 10.15, ios 9.0, macCatalyst 14.0, *)) preset = AVCaptureSessionPreset3840x2160;
                ARLOGi("Requesting capture session preset '2160p'.\n");
            }
        } 
        if (!preset) preset = AVCaptureSessionPresetMedium;
    }

#if USE_CPARAM_SEARCH
    // Initialisation required before cparamSearch can be used.
    if (!cacheDir) {
        cacheDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR);
    }
    if (!cacheInitDir) {
        cacheInitDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR);
    }
    if (cparamSearchInit(cacheDir, cacheInitDir, false, csdu, csat) < 0) {
        free(cacheDir);
        ARLOGe("videoAVFoundation::ar2VideoOpenAsyncAVFoundation(): Unable to initialise cparamSearch.\n");
        return (NULL);
    }
#endif
    free(cacheDir);
    cacheDir = NULL;
    free(cacheInitDir);
    cacheInitDir = NULL;
    free(csdu);
    csdu = NULL;
    free(csat);
    csat = NULL;

    arMallocClear(vid, AR2VideoParamAVFoundationT, 1);
    vid->focusPointOfInterestX = vid->focusPointOfInterestY = -1.0f;
    vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_NONE;
    vid->focalLength = 0.0f;
    
    // Init the CameraVideo object.
    vid->cameraVideo = [[CameraVideo alloc] init];
    if (!vid->cameraVideo) {
        ARLOGe("Error: Unable to open connection to camera.\n");
        free (vid);
        return (NULL);
    }
    vid->cameraVideoTookPictureDelegate = nil; // Init.
    vid->cameraVideo.pause = TRUE; // Start paused. ar2VideoCapStart will unpause.
    [vid->cameraVideo setCaptureDevicePosition:position];
    if (flipV == 1) vid->cameraVideo.flipV = TRUE;
    if (flipH == 1) vid->cameraVideo.flipH = TRUE;
    [vid->cameraVideo setCaptureDeviceIndex:camera_index];
    if (uid) vid->cameraVideo.captureDeviceIDUID = uid;
    [vid->cameraVideo setCaptureSessionPreset:preset];
    if (format) [vid->cameraVideo setPixelFormat:format];
    vid->cameraVideo.multithreaded = multithreaded;
    
    if (!callback) {
        
        [vid->cameraVideo start];
        
    } else {
        
        [vid->cameraVideo startAsync:^() {
            
            // This block only gets called if vid->cameraVideo.running == TRUE.
            if (!vid->cameraVideo.width || !vid->cameraVideo.height || !vid->cameraVideo.bytesPerRow || getFrameParameters(vid) < 0) {
                ARLOGe("Error: Unable to open connection to camera.\n");  // Callback must check for error state and call arVideoClose() in this case.
            }
            
            (*callback)(userdata);
        }];
        
    }
    
    if (!vid->cameraVideo.running) {
        ARLOGe("Error: Unable to open connection to camera.\n");
        [vid->cameraVideo release];
        vid->cameraVideo = nil;
        free (vid);
        return (NULL);
    }
    
    // Set the device_id.
    vid->name = strdup([vid->cameraVideo.captureDeviceIDName UTF8String]);
#if TARGET_OS_IOS
    NSString *deviceType = [UIDevice currentDevice].model;
    char *machine = NULL;
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    arMalloc(machine, char, size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    asprintf(&vid->device_id, "apple/%s/%s", [deviceType UTF8String], machine);
    free(machine);
#else
    asprintf(&vid->device_id, "%s/%s/", [vid->cameraVideo.captureDeviceIDManufacturer UTF8String], [vid->cameraVideo.captureDeviceIDModel UTF8String]);
#endif

    // If doing synchronous opening, check parameters right now.
    if (!callback) {
        if (!vid->cameraVideo.width || !vid->cameraVideo.height || !vid->cameraVideo.bytesPerRow || getFrameParameters(vid) < 0) {
            ARLOGe("Error: Unable to open connection to camera.\n");
            [vid->cameraVideo release];
            vid->cameraVideo = nil;
            free (vid);
            return (NULL);
        }
        
    }
    
    return (vid);
}

int getFrameParameters(AR2VideoParamAVFoundationT *vid)
{
#ifdef DEBUG
    // Report video size and compression type.
    OSType formatType = vid->cameraVideo.pixelFormat;
    if (formatType > 0x28) ARLOGd("Video formatType is %c%c%c%c, size is %ldx%ld.\n", (char)((formatType >> 24) & 0xFF),
                                  (char)((formatType >> 16) & 0xFF),
                                  (char)((formatType >>  8) & 0xFF),
                                  (char)((formatType >>  0) & 0xFF), vid->cameraVideo.width, vid->cameraVideo.height);
    else ARLOGd("Video formatType is %u, size is %ldx%ld.\n", (int)formatType, vid->cameraVideo.width, vid->cameraVideo.height);
#endif
    
    // Allocate structures for multi-planar frames.
    vid->buffer.bufPlaneCount = (unsigned int)vid->cameraVideo.planeCount;
    if (vid->buffer.bufPlaneCount) {
        vid->buffer.bufPlanes = (ARUint8 **)calloc(sizeof(ARUint8 *), vid->buffer.bufPlaneCount);
        if (!vid->buffer.bufPlanes) {
            ARLOGe("Out of memory!\n");
            return (-1);
        }
    } else vid->buffer.bufPlanes = NULL;
    
    vid->hostClockFrequency = vid->cameraVideo.timestampsPerSecond;
    
    return (0);
}

int ar2VideoCloseAVFoundation( AR2VideoParamAVFoundationT *vid )
{
#if USE_CPARAM_SEARCH
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
#endif

    if (vid) {
        if (vid->buffer.bufPlanes) free(vid->buffer.bufPlanes);
        if (vid->itsAMovie && vid->movieVideo) {
            [vid->movieVideo stop];
            [vid->movieVideo release];
        } else if (vid->cameraVideo) {
            [vid->cameraVideo stop];
            [vid->cameraVideo release];
        }
        if (vid->cameraVideoTookPictureDelegate) [vid->cameraVideoTookPictureDelegate release];
        free(vid->name);
        free(vid->device_id);
        free(vid);
        return 0;
    }
    return (-1);    
} 

int ar2VideoCapStartAsyncAVFoundation(AR2VideoParamAVFoundationT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata)
{
    if (!vid) return (-1);
    if (!vid->cameraVideo) return (-1);
    if (!callback) return (-1);

    vid->frameReadyCallback = callback;
    vid->frameReadyUserdata = userdata;
    // The videoAVFoundationCameraVideoTookPictureDelegate class implements the appropriate delegate method.
    if (!vid->cameraVideoTookPictureDelegate) vid->cameraVideoTookPictureDelegate = [[videoAVFoundationCameraVideoTookPictureDelegate alloc] init]; // Create an instance of delegate.
    [vid->cameraVideo setTookPictureDelegate:vid->cameraVideoTookPictureDelegate];
    [vid->cameraVideo setTookPictureDelegateUserData:vid];
    
    return ar2VideoCapStartAVFoundation(vid);
}

int ar2VideoCapStartAVFoundation( AR2VideoParamAVFoundationT *vid )
{
    if (!vid) return (-1);
    
    if (vid->itsAMovie) {
        if (!vid->movieVideo) return (-1);
        [vid->movieVideo setPaused:FALSE];
    } else {
        if (!vid->cameraVideo) return (-1);
        vid->cameraVideo.pause = FALSE;
    }
    
    return (0);
}

int ar2VideoCapStopAVFoundation( AR2VideoParamAVFoundationT *vid )
{
    if (!vid) return (-1);
    
    if (vid->itsAMovie) {
        if (!vid->movieVideo) return (-1);
        [vid->movieVideo setPaused:TRUE];
    } else {
        if (!vid->cameraVideo) return (-1);
        vid->cameraVideo.pause = TRUE;
    }
    
    // If receiving capture messages, turn them off.
    if (vid->cameraVideo && vid->cameraVideoTookPictureDelegate) {
        vid->frameReadyCallback = NULL;
        vid->frameReadyUserdata = NULL;
        [vid->cameraVideo setTookPictureDelegate:nil];
        [vid->cameraVideo setTookPictureDelegateUserData:NULL];
        [vid->cameraVideoTookPictureDelegate release]; // Destroy instance of delegate.
        vid->cameraVideoTookPictureDelegate = nil;
    }

    return (0);
}

AR2VideoBufferT *ar2VideoGetImageAVFoundation( AR2VideoParamAVFoundationT *vid )
{
    if (!vid) return (NULL);
    
    if (vid->cameraVideo) {
        
        UInt64 timestamp;
        if (!vid->buffer.bufPlaneCount) {
            unsigned char *bufDataPtr = [vid->cameraVideo frameTimestamp:&timestamp ifNewerThanTimestamp:vid->currentFrameTimestamp];
            if (!bufDataPtr) return (NULL);
            vid->buffer.buff = bufDataPtr;
        } else {
            BOOL ret = [vid->cameraVideo framePlanes:vid->buffer.bufPlanes count:vid->buffer.bufPlaneCount timestamp:&timestamp ifNewerThanTimestamp:vid->currentFrameTimestamp];
            if (!ret) return (NULL);
            vid->buffer.buff = vid->buffer.bufPlanes[0];
        }
        vid->currentFrameTimestamp = timestamp;
        vid->buffer.time.sec  = (uint64_t)(timestamp / vid->hostClockFrequency);
        vid->buffer.time.usec = (uint32_t)((timestamp % vid->hostClockFrequency) / (vid->hostClockFrequency / 1000000ull));
        
    } else if (vid->itsAMovie && vid->movieVideo) {
        
        unsigned char *bufDataPtr = (vid->movieVideo).bufDataPtr;
        if (!bufDataPtr) return (NULL);
        vid->buffer.buff = bufDataPtr;
        vid->buffer.time.sec  = 0;
        vid->buffer.time.usec = 0;
       
    } else return (NULL);

    vid->buffer.fillFlag  = 1;
    vid->buffer.buffLuma = NULL;
    
    return &(vid->buffer);
}

int ar2VideoGetSizeAVFoundation(AR2VideoParamAVFoundationT *vid, int *x,int *y)
{
    if (!vid || !x || !y) return (-1);
    
    if (vid->itsAMovie) {
        *x = (int)((vid->movieVideo).contentWidth);
        *y = (int)((vid->movieVideo).contentHeight);
    } else {
        *x = (int)((vid->cameraVideo).width);
        *y = (int)((vid->cameraVideo).height);
    }
    if (!*x) {
#ifdef DEBUG
        ARLOGe("Unable to determine video image width.\n");
#endif
        return (-1);
    }
    if (!*y) {
#ifdef DEBUG
        ARLOGe("Unable to determine video image height.\n");
#endif
        return (-1);
    }
    
    return 0;
}

int ar2VideoSetBufferSizeAVFoundation(AR2VideoParamAVFoundationT *vid, const int width, const int height)
{
    if (!vid) return (-1);
    //
    return (0);
}

int ar2VideoGetBufferSizeAVFoundation(AR2VideoParamAVFoundationT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (vid->itsAMovie) {
        if (width) *width = (int)((vid->movieVideo).bufWidth);
        if (height) *height = (int)((vid->movieVideo).bufHeight);
    } else {
        if (width) *width = (int)((vid->cameraVideo).width);
        if (height) *height = (int)((vid->cameraVideo).height);
    }
    return (0);
}

int ar2VideoGetPixelFormatAVFoundation( AR2VideoParamAVFoundationT *vid )
{
    if (!vid) return (-1);
    if (vid->itsAMovie && vid->movieVideo) {
        return (vid->movieVideo.ARPixelFormat);
    } else if (vid->cameraVideo) {
        switch ((vid->cameraVideo.pixelFormat)) {
            case kCVPixelFormatType_32BGRA:
                return (AR_PIXEL_FORMAT_BGRA);
                break; 
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: // All devices except iPhone 3G recommended.
                return (AR_PIXEL_FORMAT_420v);
                break;
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                return (AR_PIXEL_FORMAT_420f);
                break;
            case kCVPixelFormatType_422YpCbCr8: // iPhone 3G recommended.
                return (AR_PIXEL_FORMAT_2vuy);
                break;
            case kCVPixelFormatType_422YpCbCr8_yuvs:
                return (AR_PIXEL_FORMAT_yuvs);
                break; 
            case kCVPixelFormatType_24RGB:
                return (AR_PIXEL_FORMAT_RGB);
                break; 
            case kCVPixelFormatType_24BGR:
                return (AR_PIXEL_FORMAT_BGR);
                break; 
            case kCVPixelFormatType_32ARGB:
                return (AR_PIXEL_FORMAT_ARGB);
                break; 
            case kCVPixelFormatType_32ABGR:
                return (AR_PIXEL_FORMAT_ABGR);
                break; 
            case kCVPixelFormatType_32RGBA:
                return (AR_PIXEL_FORMAT_RGBA);
                break; 
            case kCVPixelFormatType_8IndexedGray_WhiteIsZero:
                return (AR_PIXEL_FORMAT_MONO);
                break;
            default:
                return (AR_PIXEL_FORMAT)-1;
                break; 
        }
    }
    return (AR_PIXEL_FORMAT)-1;
}

int ar2VideoGetIdAVFoundation( AR2VideoParamAVFoundationT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiAVFoundation( AR2VideoParamAVFoundationT *vid, int paramName, int *value )
{
    NSString *iOSDevice;
    AVCaptureDevicePosition position;
    
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_GET_IMAGE_ASYNC:
            if (vid->cameraVideo) *value = TRUE;
            else *value = FALSE; 
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_WILL_CAPTURE_NEXT_FRAME:
            if (vid->cameraVideo) *value = vid->cameraVideo.willSaveNextFrame;
            else return (-1);
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_IOS_DEVICE:
            if (vid->cameraVideo) {
                iOSDevice = vid->cameraVideo.iOSDevice;
                if      (iOSDevice == CameraVideoiOSDeviceiPhone3G)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE3G;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone3GS)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE3GS;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone4)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE4;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone4S)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE4S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5c)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5C;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5s)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6Plus) *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6PLUS;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6S)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6SPlus) *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6SPLUS;
                else if (iOSDevice == CameraVideoiOSDeviceiPhoneSE)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONESE;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone7)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE7;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone7Plus) *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE7PLUS;
                else if (iOSDevice == CameraVideoiOSDeviceiPodTouch4) *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPODTOUCH4;
                else if (iOSDevice == CameraVideoiOSDeviceiPodTouch5) *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPODTOUCH5;
                else if (iOSDevice == CameraVideoiOSDeviceiPad2)      *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD2;
                else if (iOSDevice == CameraVideoiOSDeviceiPad3)      *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD3;
                else if (iOSDevice == CameraVideoiOSDeviceiPad4)      *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD4;
                else if (iOSDevice == CameraVideoiOSDeviceiPadAir)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADAIR;
                else if (iOSDevice == CameraVideoiOSDeviceiPadAir2)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADAIR2;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini2)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI2;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini3)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI3;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini4)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI4;
                else if (iOSDevice == CameraVideoiOSDeviceiPadPro129)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADPRO129;
                else if (iOSDevice == CameraVideoiOSDeviceiPadPro97)  *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADPRO97;
                else if (iOSDevice == CameraVideoiOSDeviceiPhoneX)    *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceiPodX)      *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPOD_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceiPadX)      *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceAppleTVX)   *value = AR_VIDEO_AVFOUNDATION_IOS_DEVICE_APPLETV_GENERIC;
                else *value = -1;
            } else return (-1);
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET:
            *value = vid->focalLengthPreset;
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_CAMERA_POSITION:
            if (vid->cameraVideo) {
                position = vid->cameraVideo.captureDevicePosition;
                if      (position == AVCaptureDevicePositionUnspecified) *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_UNSPECIFIED;
                else if (position == AVCaptureDevicePositionBack)        *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_REAR;
                else if (position == AVCaptureDevicePositionFront)       *value = AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT;
                else *value = -1;
            } else return (-1);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiAVFoundation( AR2VideoParamAVFoundationT *vid, int paramName, int  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_AVFOUNDATION_WILL_CAPTURE_NEXT_FRAME:
            vid->cameraVideo.willSaveNextFrame = (BOOL)value;
#ifdef DEBUG
            ARLOGe("willSaveNextFrame.\n");
#endif
            break;
        case AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET:
            if (value < 0) return (-1);
            vid->focalLengthPreset = value;
            vid->focalLength = 0.0f;
            break;
        case AR_VIDEO_FOCUS_MODE:
            if (value == AR_VIDEO_FOCUS_MODE_FIXED) {
                if (![vid->cameraVideo setFocus:AVCaptureFocusModeLocked atPixelCoords:CGPointMake(0.0f, 0.0f)]) {
                    return (-1);
                };
            } else if (value == AR_VIDEO_FOCUS_MODE_AUTO) {
                if (![vid->cameraVideo setFocus:AVCaptureFocusModeContinuousAutoFocus atPixelCoords:CGPointMake(0.0f, 0.0f)]) {
                    return (-1);
                };
            } else if (value == AR_VIDEO_FOCUS_MODE_POINT_OF_INTEREST) {
                if (vid->focusPointOfInterestX < 0.0f || vid->focusPointOfInterestY < 0.0f) {
                    ARLOGw("Warning: request for focus on point-of-interest, but point of interest not yet set.\n");
                } else {
                    if (![vid->cameraVideo setFocus:AVCaptureFocusModeAutoFocus atPixelCoords:CGPointMake(vid->focusPointOfInterestX, vid->focusPointOfInterestY)]) {
                        return (-1);
                    };
                }
            } else if (value == AR_VIDEO_FOCUS_MODE_MANUAL) {
                ARLOGe("Error: request for manual focus but this mode not currently supported on iOS.\n");
                return (-1);
            } else {
                ARLOGe("Error: request for focus mode %d but this mode not currently supported on iOS.\n", value);
                return (-1);
            }
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdAVFoundation( AR2VideoParamAVFoundationT *vid, int paramName, double *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH:
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:
            if (vid->focalLength > 0.0f) {
                *value = vid->focalLength;
            } else {
                switch (vid->focalLengthPreset) {
                    case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                        *value = INFINITY;
                        break;
                    case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                        *value = 1.0;
                        break;
                    case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                        *value = 0.01;
                        break;
                    case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        *value = 0.3;
                        break;
                    case AR_VIDEO_AVFOUNDATION_FOCUS_NONE:
                    default:
                        *value = 0.0;
                        break;
                }
            }
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdAVFoundation( AR2VideoParamAVFoundationT *vid, int paramName, double  value )
{
    if (!vid) return (-1);
    
    float valuef = (float)value;
    switch (paramName) {
        case AR_VIDEO_FOCUS_MANUAL_DISTANCE:
            ARLOGe("Error: request for manual focus but this mode not currently supported on iOS.\n");
            return (-1);
            break;
        case AR_VIDEO_FOCUS_POINT_OF_INTEREST_X:
            vid->focusPointOfInterestX = valuef;
            break;
        case AR_VIDEO_FOCUS_POINT_OF_INTEREST_Y:
            vid->focusPointOfInterestY = valuef;
            break;
        case AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH:
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:
            vid->focalLength = valuef;
            // Also find the closest preset.
            if (valuef <= 0.0f) {
                vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_NONE;
            } else if (valuef > 6.0f) {
                vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_INF;
            } else if (valuef < 0.05f) {
                vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_MACRO;
            } else if (valuef > 0.5f) {
                vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_1_0M;
            } else {
                vid->focalLengthPreset = AR_VIDEO_AVFOUNDATION_FOCUS_0_3M;
            }
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsAVFoundation( AR2VideoParamAVFoundationT *vid, const int paramName, char **value )
{
   if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            *value = (vid->device_id ? strdup(vid->device_id) : NULL);
            break;
        case AR_VIDEO_PARAM_NAME:
            *value = (vid->name ? strdup(vid->name) : NULL);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsAVFoundation( AR2VideoParamAVFoundationT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

id ar2VideoGetNativeVideoInstanceAVFoundation(AR2VideoParamAVFoundationT *vid)
{
    if (!vid) return (nil);
    if (vid->itsAMovie) return (vid->movieVideo);
    else return (vid->cameraVideo);
}

// A class that implements the CameraVideoTookPictureDelegate protocol.
@implementation videoAVFoundationCameraVideoTookPictureDelegate

- (void) cameraVideoTookPicture:(id)sender userData:(void *)data
{
    AR2VideoParamAVFoundationT *vid = (AR2VideoParamAVFoundationT *)data; // Cast to correct type;
    if (vid) {
        if (vid->frameReadyCallback) (vid->frameReadyCallback)(vid->frameReadyUserdata);
    }
}

@end

int ar2VideoGetCParamAVFoundation(AR2VideoParamAVFoundationT *vid, ARParam *cparam)
{
#if !TARGET_OS_IOS
    return (-1);
#else
    NSString *iOSDevice;
    const unsigned char *cparambytes;
    const char *cparamname;
    
    if (!vid || !cparam) return (-1);

    if (vid->cameraVideo) {
        iOSDevice = vid->cameraVideo.iOSDevice;
        if (iOSDevice == CameraVideoiOSDeviceiPhone3G ||
            iOSDevice == CameraVideoiOSDeviceiPhone3GS) {
            cparambytes = camera_para_iPhone;
            cparamname = "camera_para_iPhone.dat";
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone4) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPhone_4_front;
                cparamname = "camera_para_iPhone_4_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_0_3m.dat";
                            break;
                    }
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_4_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_4_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_4_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_4_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone4S ||
                   iOSDevice == CameraVideoiOSDeviceiPad3 ||
                   iOSDevice == CameraVideoiOSDeviceiPadMini) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPhone_4S_front;
                cparamname = "camera_para_iPhone_4S_front.dat";
            }  else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_0_3m.dat";
                            break;
                    }
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPad4 ||
                   iOSDevice == CameraVideoiOSDeviceiPadAir ||
                   iOSDevice == CameraVideoiOSDeviceiPhone5 ||
                   iOSDevice == CameraVideoiOSDeviceiPodTouch5 ||
                   iOSDevice == CameraVideoiOSDeviceiPodTouch6 ||
                   iOSDevice == CameraVideoiOSDeviceiPhone5c) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5_front_1280x720;
                    cparamname = "camera_para_iPhone_5_front_1280x720.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5_front_640x480;
                    cparamname = "camera_para_iPhone_5_front_640x480.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_5_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_5_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_5_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_5_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone5s) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5s_front_1280x720;
                    cparamname = "camera_para_iPhone_5s_front_1280x720.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5s_front_640x480;
                    cparamname = "camera_para_iPhone_5s_front_640x480.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPodTouch4) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPod_touch_4G_front;
                cparamname = "camera_para_iPod_touch_4G_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    cparambytes = camera_para_iPod_touch_4G_rear_1280x720;
                    cparamname = "camera_para_iPod_touch_4G_rear_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPod_touch_4G_rear_640x480;
                    cparamname = "camera_para_iPod_touch_4G_rear_640x480.dat";
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPad2) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPad_2_front;
                cparamname = "camera_para_iPad_2_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    cparambytes = camera_para_iPad_2_rear_1280x720;
                    cparamname = "camera_para_iPad_2_rear_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPad_2_rear_640x480;
                    cparamname = "camera_para_iPad_2_rear_640x480.dat";
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadMini2 || iOSDevice == CameraVideoiOSDeviceiPadMini3) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_3_front_1280x720;
                    cparamname = "camera_para_iPad_mini_3_front_1280x720.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_inf;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_macro;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_3_front_640x480;
                    cparamname = "camera_para_iPad_mini_3_front_640x480.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_inf;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_1_0m;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_macro;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_0_3m;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6Plus) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_Plus_front_1280x720;
                    cparamname = "camera_para_iPhone_6_Plus_front_1280x720.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_Plus_front_640x480;
                    cparamname = "camera_para_iPhone_6_Plus_front_640x480.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_front_1280x720;
                    cparamname = "camera_para_iPhone_6_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6_rear_1280x720;
                    cparamname = "camera_para_iPhone_6_rear_1280x720_0_3m.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        }
        else if (iOSDevice == CameraVideoiOSDeviceiPhone6SPlus) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6s_Plus_front_1280x720;
                    cparamname = "camera_para_iPhone_6s_Plus_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6s_Plus_rear_1280x720;
                    cparamname = "camera_para_iPhone_6s_Plus_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6S) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6s_front_1280x720;
                    cparamname = "camera_para_iPhone_6s_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6s_rear_1280x720;
                    cparamname = "camera_para_iPhone_6s_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadMini4) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_4_front_1280x720;
                    cparamname = "camera_para_iPad_mini_4_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPad_mini_4_rear_1280x720;
                    cparamname = "camera_para_iPad_mini_4_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadAir2) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_Air_2_front_1280x720;
                    cparamname = "camera_para_iPad_Air_2_front_1280x720.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_inf;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_macro;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_Air_2_front_640x480;
                    cparamname = "camera_para_iPad_Air_2_front_640x480.dat";
                } else {
                    switch (vid->focalLengthPreset) {
                        case AR_VIDEO_AVFOUNDATION_FOCUS_INF:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_inf;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_1_0m;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_macro;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_0_3m;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else {
            cparambytes = NULL;
            cparamname = NULL;
        }
    } else {
        // !vid->cameraVideo
        cparambytes = NULL;
        cparamname = NULL;
    }

    if (cparambytes) {
        ARLOGi("Using %s for device %s.\n", cparamname, [iOSDevice cStringUsingEncoding:NSUTF8StringEncoding]);
        return (arParamLoadFromBuffer(cparambytes, cparam_size, cparam));
    } else {
        ARLOGi("No predefined camera parameters for device %s.\n", [iOSDevice cStringUsingEncoding:NSUTF8StringEncoding]);
        return (-1);
    }
#endif // !TARGET_OS_IOS
}

#if USE_CPARAM_SEARCH
static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamAVFoundationT *vid = (AR2VideoParamAVFoundationT *)userdata;
    if (!vid) return;

    switch (state) {
        case CPARAM_SEARCH_STATE_INITIAL:
        case CPARAM_SEARCH_STATE_IN_PROGRESS:
            break;
        case CPARAM_SEARCH_STATE_RESULT_NULL:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_OK:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(cparam, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_FAILED_NO_NETWORK:
            ARLOGe("Error during cparamSearch. Internet connection unavailable.\n");
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        default: // Errors.
            ARLOGe("Error %d returned from cparamSearch.\n", (int)state);
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
    }
    if (final) vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
}

int ar2VideoGetCParamAsyncAVFoundation(AR2VideoParamAVFoundationT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1);
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }

    if (!vid->device_id) {
        ARLOGe("Error: device identification not available.\n");
        return (-1);
    }

    int camera_index = (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront ? 1 : 0);
    float focalLength = 0.0f;
    if (vid->focalLength > 0.0f) {
        focalLength = vid->focalLength;
    } else {
        switch (vid->focalLengthPreset) {
            case AR_VIDEO_AVFOUNDATION_FOCUS_0_3M: focalLength = 0.3f; break;
            case AR_VIDEO_AVFOUNDATION_FOCUS_1_0M: focalLength = 1.0f; break;
            case AR_VIDEO_AVFOUNDATION_FOCUS_INF: focalLength = INFINITY; break;
            case AR_VIDEO_AVFOUNDATION_FOCUS_MACRO: focalLength = 0.01f; break;
            default: break;
        }
    }
    int width = 0, height = 0;
    if (ar2VideoGetSizeAVFoundation(vid, &width, &height) < 0) {
        ARLOGe("Error: request for camera parameters, but video size is unknown.\n");
        return (-1);
    };
    
    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;
    
    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, camera_index, width, height, focalLength, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
        return (-1);
    }
    
    return (0);
}

#endif // USE_CPARAM_SEARCH

#endif //  ARVIDEO_INPUT_AVFOUNDATION
