/*
 *  CameraVideo.m
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
 */

#import <ARX/ARVideo/video.h>
#ifdef ARVIDEO_INPUT_AVFOUNDATION

#import "CameraVideo.h"
#import <CoreVideo/CVHostTime.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pthread.h>
#import <Availability.h>
#import <Accelerate/Accelerate.h>
#if TARGET_OS_IOS
#  import <UIKit/UIKit.h>
#endif

NSString *const CameraVideoiOSDeviceiPhone3G = @"iPhone 3G";
NSString *const CameraVideoiOSDeviceiPhone3GS = @"iPhone 3GS";
NSString *const CameraVideoiOSDeviceiPhone4 = @"iPhone 4";
NSString *const CameraVideoiOSDeviceiPhone4S = @"iPhone 4S";
NSString *const CameraVideoiOSDeviceiPhone5 = @"iPhone 5";
NSString *const CameraVideoiOSDeviceiPhone5s = @"iPhone 5s";
NSString *const CameraVideoiOSDeviceiPhone5c = @"iPhone 5c";
NSString *const CameraVideoiOSDeviceiPhone6 = @"iPhone 6";
NSString *const CameraVideoiOSDeviceiPhone6Plus = @"iPhone 6 Plus";
NSString *const CameraVideoiOSDeviceiPhone6S = @"iPhone 6s";
NSString *const CameraVideoiOSDeviceiPhone6SPlus = @"iPhone 6s Plus";
NSString *const CameraVideoiOSDeviceiPhoneSE = @"iPhone SE";
NSString *const CameraVideoiOSDeviceiPhone7 = @"iPhone 7";
NSString *const CameraVideoiOSDeviceiPhone7Plus = @"iPhone 7 Plus";
NSString *const CameraVideoiOSDeviceiPodTouch4 = @"iPod Touch (4th Generation)";
NSString *const CameraVideoiOSDeviceiPodTouch5 = @"iPod Touch (5th Generation)";
NSString *const CameraVideoiOSDeviceiPodTouch6 = @"iPod Touch (6th Generation)";
NSString *const CameraVideoiOSDeviceiPad2 = @"iPad 2";
NSString *const CameraVideoiOSDeviceiPad3 = @"iPad (3rd generation)";
NSString *const CameraVideoiOSDeviceiPad4 = @"iPad (4th generation)";
NSString *const CameraVideoiOSDeviceiPadAir = @"iPad Air";
NSString *const CameraVideoiOSDeviceiPadAir2 = @"iPad Air 2";
NSString *const CameraVideoiOSDeviceiPadMini = @"iPad mini";
NSString *const CameraVideoiOSDeviceiPadMini2 = @"iPad mini (2nd generation)";
NSString *const CameraVideoiOSDeviceiPadMini3 = @"iPad mini (3rd generation)";
NSString *const CameraVideoiOSDeviceiPadMini4 = @"iPad mini (4th generation)";
NSString *const CameraVideoiOSDeviceiPadPro129 = @"iPad Pro 12.9\"";
NSString *const CameraVideoiOSDeviceiPadPro97 = @"iPad Pro 9.7\"";
NSString *const CameraVideoiOSDeviceiPhoneX = @"iPhone (Unknown model)";
NSString *const CameraVideoiOSDeviceiPodX = @"iPod (Unknown model)";
NSString *const CameraVideoiOSDeviceiPadX = @"iPad (Unknown model)";
NSString *const CameraVideoiOSDeviceAppleTVX= @"Apple TV (Unknown model)";

typedef struct {
    size_t width;
    size_t height;
    OSType pixelFormat;
    unsigned char *bufDataPtr0;
    unsigned char *bufDataPtr1;
    size_t bytesPerRow;
    size_t bytesPerRowSrc;
} cameraVideoBuf_t;

#ifdef DEBUG
//#define CAMERA_VIDEO_DEBUG
#endif

@interface CameraVideo (CameraVideoPrivate)
// AVCaptureVideoDataOutputSampleBufferDelegate methods.
- (void) captureOutput:(AVCaptureOutput *)captureOutput  didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer  fromConnection:(AVCaptureConnection *)connection;
@end

@implementation CameraVideo
{
    NSString *iOSDevice;
    
    AVCaptureSession *captureSession;
    NSString *captureSessionPreset;
    AVCaptureDevice *captureDevice;
    AVCaptureDevicePosition captureDevicePosition;
    NSString *captureDeviceDesiredUID;
    int captureDeviceIndex;
    AVCaptureVideoDataOutput *captureVideoDataOutput;
    AVCaptureStillImageOutput *captureStillImageOutput;
    UInt64 latestFrameHostTime;
    
    BOOL running;
    CameraVideoStartAsyncCompletionBlock captureStartAsyncCompletion;
    OSType pixelFormat;
    size_t planeCount;
    cameraVideoBuf_t *planes;
    int nextBuf;
    id <CameraVideoTookPictureDelegate> tookPictureDelegate;
    void *tookPictureDelegateUserData;
    BOOL willSaveNextFrame;
    BOOL flipV;
    BOOL flipH;
    BOOL pause;
    
    BOOL multithreaded;
    pthread_mutex_t frameLock_pthread_mutex;
    dispatch_queue_t captureQueue;
    
    vImage_Buffer      imageSrcBuf;
    vImage_Buffer      imageDestBuf;
}

@synthesize tookPictureDelegate = tookPictureDelegate, tookPictureDelegateUserData = tookPictureDelegateUserData;
@synthesize willSaveNextFrame = willSaveNextFrame;
@synthesize iOSDevice = iOSDevice;
@synthesize flipV = flipV, flipH = flipH;
@synthesize multithreaded = multithreaded;
@synthesize running = running;
@synthesize pause = pause;
@synthesize planeCount = planeCount;

- (id) init
{
	if ((self = [super init])) {
        
        // Determine the machine name, e.g. "iPhone1,1".
        size_t size;
        sysctlbyname("hw.machine", NULL, &size, NULL, 0); // Get size of data to be returned.
        char *name = malloc(size);
        sysctlbyname("hw.machine", name, &size, NULL, 0);
        NSString *machine = [NSString stringWithCString:name encoding:NSASCIIStringEncoding];
        free(name);

        // See http://support.apple.com/kb/HT3939 for model numbers,
        // Either: devices with these major numbers are no longer produced.
        // or: devices with same major number have different cameras.
        if ([machine isEqualToString:@"iPhone1,2"]) iOSDevice = CameraVideoiOSDeviceiPhone3G;
        else if ([machine isEqualToString:@"iPhone2,1"]) iOSDevice = CameraVideoiOSDeviceiPhone3GS;
        else if ([machine isEqualToString:@"iPad2,5"] || [machine isEqualToString:@"iPad2,6"] || [machine isEqualToString:@"iPad2,7"]) iOSDevice = CameraVideoiOSDeviceiPadMini; // 5=Wifi, 6=Wifi+GSM, 7=Wifi+CDMA.
        else if ([machine isEqualToString:@"iPad3,4"] || [machine isEqualToString:@"iPad3,5"] || [machine isEqualToString:@"iPad3,6"]) iOSDevice = CameraVideoiOSDeviceiPad4; // 4=Wifi, 5=Wifi+GSM, 6=Wifi+CDMA.
        else if ([machine isEqualToString:@"iPad4,4"] || [machine isEqualToString:@"iPad4,5"] || [machine isEqualToString:@"iPad4,6"]) iOSDevice = CameraVideoiOSDeviceiPadMini2; // 4=Wifi, 5=Wifi+Cellular, 6=Wifi+Cellular(China).
        else if ([machine isEqualToString:@"iPad4,7"] || [machine isEqualToString:@"iPad4,8"] || [machine isEqualToString:@"iPad4,9"]) iOSDevice = CameraVideoiOSDeviceiPadMini3; // 7=Wifi, 8=Wifi+Cellular, 9=Wifi+Cellular(China).
        else if ([machine isEqualToString:@"iPad5,1"] || [machine isEqualToString:@"iPad5,2"]) iOSDevice = CameraVideoiOSDeviceiPadMini4;
        else if ([machine isEqualToString:@"iPad5,3"] || [machine isEqualToString:@"iPad5,4"]) iOSDevice = CameraVideoiOSDeviceiPadAir2; // 3=Wifi, 4=Wifi+Cellular.
        else if ([machine isEqualToString:@"iPad6,3"] || [machine isEqualToString:@"iPad6,4"]) iOSDevice = CameraVideoiOSDeviceiPadPro97; // 3=Wifi, 4=Wifi+Cellular.
        else if ([machine isEqualToString:@"iPad6,7"] || [machine isEqualToString:@"iPad6,8"]) iOSDevice = CameraVideoiOSDeviceiPadPro129; // 7=Wifi, 8=Wifi+Cellular.
        else if ([machine isEqualToString:@"iPhone2,1"]) iOSDevice = CameraVideoiOSDeviceiPhone3GS;
        else if ([machine isEqualToString:@"iPhone2,1"]) iOSDevice = CameraVideoiOSDeviceiPhone3GS;
        else if ([machine isEqualToString:@"iPhone5,1"] || [machine isEqualToString:@"iPhone5,2"]) iOSDevice = CameraVideoiOSDeviceiPhone5; // 1=GSM(A1428), 2=GSM+CDMA(A1429).
        else if ([machine isEqualToString:@"iPhone5,3"] || [machine isEqualToString:@"iPhone5,4"]) iOSDevice = CameraVideoiOSDeviceiPhone5c; //
        else if ([machine isEqualToString:@"iPhone7,1"]) iOSDevice = CameraVideoiOSDeviceiPhone6Plus;
        else if ([machine isEqualToString:@"iPhone7,2"]) iOSDevice = CameraVideoiOSDeviceiPhone6;
        else if ([machine isEqualToString:@"iPhone8,1"]) iOSDevice = CameraVideoiOSDeviceiPhone6S;
        else if ([machine isEqualToString:@"iPhone8,2"]) iOSDevice = CameraVideoiOSDeviceiPhone6SPlus;
        else if ([machine isEqualToString:@"iPhone8,4"]) iOSDevice = CameraVideoiOSDeviceiPhoneSE;
        else if ([machine isEqualToString:@"iPhone9,1"] || [machine isEqualToString:@"iPhone9,3"]) iOSDevice = CameraVideoiOSDeviceiPhone7;
        else if ([machine isEqualToString:@"iPhone9,2"] || [machine isEqualToString:@"iPhone9,4"]) iOSDevice = CameraVideoiOSDeviceiPhone7Plus;
        // Either: all known devices with same major number have identical cameras,
        // or: no known match found, provide best attempt at matching.
        else if ([machine hasPrefix:@"iPad4,"]) iOSDevice = CameraVideoiOSDeviceiPadAir; // 1=Wifi, 2=Wifi+Cellular, 3=Wifi+GSM Cellular (China).
        else if ([machine hasPrefix:@"iPad3,"]) iOSDevice = CameraVideoiOSDeviceiPad3; // 1=Wifi, 2=Wifi+CDMA/GSM, 3=Wifi+GSM.
        else if ([machine hasPrefix:@"iPad2,"]) iOSDevice = CameraVideoiOSDeviceiPad2; // 1=Wifi, 2=Wifi+GSM, 3=Wifi+CDMA, 4=Wifi Rev A.
        else if ([machine hasPrefix:@"iPad"]) iOSDevice = CameraVideoiOSDeviceiPadX;
        else if ([machine hasPrefix:@"iPhone3,"]) iOSDevice = CameraVideoiOSDeviceiPhone4; // 1=GSM, 3=CDMA.
        else if ([machine hasPrefix:@"iPhone4,"]) iOSDevice = CameraVideoiOSDeviceiPhone4S; // 1=.
        else if ([machine hasPrefix:@"iPhone5,"]) iOSDevice = CameraVideoiOSDeviceiPhone5;
        else if ([machine hasPrefix:@"iPhone6,"]) iOSDevice = CameraVideoiOSDeviceiPhone5s; // 1=GSM/CDMA(A1533,A1453), 2=GSM(A1457,A1528,A1530).
        else if ([machine hasPrefix:@"iPhone"]) iOSDevice = CameraVideoiOSDeviceiPhoneX;
        else if ([machine hasPrefix:@"iPod4,"]) iOSDevice = CameraVideoiOSDeviceiPodTouch4; // 1=.
        else if ([machine hasPrefix:@"iPod5,"]) iOSDevice = CameraVideoiOSDeviceiPodTouch5; // 1=.
        else if ([machine hasPrefix:@"iPod7,"]) iOSDevice = CameraVideoiOSDeviceiPodTouch6; // 1=.
        else if ([machine hasPrefix:@"iPod"]) iOSDevice = CameraVideoiOSDeviceiPodX;
        else if ([machine hasPrefix:@"AppleTV"]) iOSDevice = CameraVideoiOSDeviceAppleTVX;
        else iOSDevice = nil;
        
        captureSession = nil;
        captureSessionPreset = AVCaptureSessionPresetMedium;
        captureDevice = nil;
        captureDevicePosition = AVCaptureDevicePositionUnspecified;
        captureDeviceIndex = 0;
        captureDeviceDesiredUID = nil;
        captureVideoDataOutput = nil;
        captureStillImageOutput = nil;
        
        running = FALSE;
        captureStartAsyncCompletion = NULL;
        pixelFormat = kCVPixelFormatType_32BGRA;
        planes = NULL;
        planeCount = 0;
        nextBuf = 0;
        tookPictureDelegate = nil;
        tookPictureDelegateUserData = NULL;
        flipV = flipH = FALSE;
        
        // Create a mutex to protect access to the frame.
        int err;
        if ((err = pthread_mutex_init(&frameLock_pthread_mutex, NULL)) != 0) {
            NSLog(@"Error creating mutex.\n");
            [self release];
            return (nil);
        }
        captureQueue = NULL;
    }
    return (self);
}

- (NSString *)captureSessionPreset {
    return captureSessionPreset;
}

- (void)setCaptureSessionPreset:(NSString *)newValue {
    if (running) return;
    captureSessionPreset = newValue;
}

- (AVCaptureDevicePosition)captureDevicePosition {
    return captureDevicePosition;
}

- (void)setCaptureDevicePosition:(AVCaptureDevicePosition)newValue {
    if (running) return;
    captureDevicePosition = newValue;
}

- (int)captureDeviceIndex {
    return captureDeviceIndex;
}

- (void)setCaptureDeviceIndex:(int)newValue {
    if (running) return;
    captureDeviceIndex = newValue;
}

- (OSType)pixelFormat {
    return pixelFormat;
}

- (void)setPixelFormat:(OSType)newPixelFormat {
    if (running) return;
    pixelFormat = newPixelFormat;
}

- (NSString *)captureDeviceIDManufacturer {
    if (!captureDevice) return NULL;
    if ([captureDevice respondsToSelector:@selector(manufacturer)]) { // macOS 10.9 and later only.
        return [captureDevice performSelector:@selector(manufacturer)];
    }
    return NULL;
}

- (NSString *)captureDeviceIDName {
    if (!captureDevice) return NULL;
    return captureDevice.localizedName;
}

- (NSString *)captureDeviceIDModel {
    if (!captureDevice) return NULL;
    return captureDevice.modelID;
}

- (NSString *)captureDeviceIDUID {
    if (!captureDevice) return NULL;
    return captureDevice.uniqueID;
}

- (void)setCaptureDeviceIDUID:(NSString *)captureDeviceIDUID_in
{
    if (running) return;
    captureDeviceDesiredUID = [captureDeviceIDUID_in copy];
}

- (void) start
{
    [self startAsync:NULL];
}

- (void) startAsync:(CameraVideoStartAsyncCompletionBlock)completion
{
    
    if (running) [self stop]; // Restart functionality.

    captureSession = [[AVCaptureSession alloc] init];
    if (!captureSession) {
        NSLog(@"Unable to initialise video capture session.\n");
        return;
    }
    
    //
    // Set up the input.
    //

	NSArray *captureDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    if (captureDeviceDesiredUID) {
        for (captureDevice in captureDevices) {
            if ([captureDevice.uniqueID isEqualToString:captureDeviceDesiredUID]) break;
        }
        if (!captureDevice) {
            NSLog(@"Unable to find video capture device with UID %@. Ignoring.\n", captureDeviceDesiredUID);
        }
    }
    if (!captureDevice) {
        if (captureDevicePosition != AVCaptureDevicePositionUnspecified) {
            int deviceAtPositionIndex = 0;
            for (captureDevice in captureDevices) {
                if (captureDevice.position != captureDevicePosition) continue;
                if (deviceAtPositionIndex == captureDeviceIndex) break;
                deviceAtPositionIndex++;
            }
            if (!captureDevice) {
                NSLog(@"Unable to acquire video capture device (with position %s, index %d).\n", (captureDevicePosition == AVCaptureDevicePositionBack ? "rear" : "front"), captureDeviceIndex);
                goto bail0;
            }
        } else {
            if (captureDeviceIndex >= captureDevices.count) {
                NSLog(@"Unable to acquire video capture device (with index %d).\n", captureDeviceIndex);
                goto bail0;
            }
            captureDevice = captureDevices[captureDeviceIndex];
        }
    }
    
    NSError *error = nil;
    
    AVCaptureDeviceInput *captureDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];
    if (!captureDeviceInput) {
        NSLog(@"Unable to acquire video capture device input.\n");
        goto bail1;
    }
    [captureSession addInput:captureDeviceInput];
    
    //
    // Set up the video data output.
    //
    captureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    if (!captureVideoDataOutput) {
        NSLog(@"Unable to init video data output.\n");
        goto bail1;
    }
    
    // Check the provided pixel format.
    int pfOK = FALSE;
    NSArray *supportedPFs = captureVideoDataOutput.availableVideoCVPixelFormatTypes;
#ifdef DEBUG
    NSLog(@"Supported pixel formats:\n");
#endif
    for (NSNumber *pf in supportedPFs) {
        OSType pfOSType = [pf unsignedIntValue];
        if (pfOSType == pixelFormat) {
            pfOK = TRUE;
#ifndef DEBUG
            break;
#endif
        }
#ifdef DEBUG
        if (pfOSType <= 0x28) NSLog(@"  0x%08x\n", (unsigned int)pfOSType);
#  ifdef AR_BIG_ENDIAN
        else NSLog(@"%4c, ", (char *)&pfOSType);
#  else
        else NSLog(@"  %c%c%c%c\n", (((char *)&pfOSType)[3]), (((char *)&pfOSType)[2]), (((char *)&pfOSType)[1]), (((char *)&pfOSType)[0]));
#  endif
#endif
    }
    if (!pfOK) {
        NSLog(@"Error: requested pixel format not supported.\n");
        goto bail2;
    }
    
    captureVideoDataOutput.videoSettings = [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSNumber numberWithInteger:pixelFormat], kCVPixelBufferPixelFormatTypeKey,
                                            nil];
    
    if (multithreaded) {
        captureQueue = dispatch_queue_create("org.artoolkitx.arx.CameraVideo", NULL);
        [captureVideoDataOutput setSampleBufferDelegate:self queue:captureQueue];
    } else {
        [captureVideoDataOutput setSampleBufferDelegate:self queue:dispatch_get_main_queue()];
    }
	[captureVideoDataOutput setAlwaysDiscardsLateVideoFrames:YES]; // Discard if the data output queue is blocked (including during processing of any still image).

    if ([captureSession canAddOutput:captureVideoDataOutput]) [captureSession addOutput:captureVideoDataOutput];
    
    if ([captureSession canSetSessionPreset:captureSessionPreset]) {
        captureSession.sessionPreset = captureSessionPreset;
    } else {
        NSLog(@"Unsupported capture preset %@.\n", captureSessionPreset);
        goto bail3;
     }
    
    //
    // A second output, for still images (if needed).
    //
	captureStillImageOutput = [[AVCaptureStillImageOutput alloc] init];
    [captureStillImageOutput setOutputSettings:[NSDictionary dictionaryWithObject:AVVideoCodecJPEG
                                                                           forKey:AVVideoCodecKey]];
	if ([captureSession canAddOutput:captureStillImageOutput]) [captureSession addOutput:captureStillImageOutput];
    
    
    //
    // Get things running.
    //
    
    /* Notifications
     AVCaptureSessionRuntimeErrorNotification
     AVCaptureSessionErrorKey
     AVCaptureSessionDidStartRunningNotification
     AVCaptureSessionDidStopRunningNotification
     AVCaptureSessionWasInterruptedNotification
     AVCaptureSessionInterruptionEndedNotification
     */
    
    // Deallocate any old bufDataPtr.
    if (planes) {
        if (!planeCount) {
            free(planes[0].bufDataPtr0);
            free(planes[0].bufDataPtr1);
        } else {
            for (int i = 0; i < planeCount; i++) {
                free(planes[i].bufDataPtr0);
                free(planes[i].bufDataPtr1);
            }
        }
        free(planes);
        planes = NULL;
        planeCount = 0;
        nextBuf = 0;
    }

    if (!completion) {
        captureStartAsyncCompletion = NULL;
    } else {
        captureStartAsyncCompletion = Block_copy(completion);
    }

    [captureSession startRunning]; // Session start is asychronous, so -startRunning returns immediately.

    if (!completion) {
        // Wait for a frame to be returned so we can get frame size and pixel format.
        // Effectively a spinlock.
        int timeout = 2000; // 2000 x 1 millisecond (see below).
        while (!planes && (0 < timeout--)) {
            SInt32 result;
            do {
                result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, true);
            } while (result == kCFRunLoopRunHandledSource);
            if (planes) break;
            usleep(1000); // Wait 1 millisecond.
        };
        if (!planes) {
            NSLog(@"Camera not responding after 2 seconds waiting. Giving up.");
            goto bail4;
        }
    }
    
    running = TRUE;
    return;
    
bail4:
    [captureSession stopRunning];
    [captureStillImageOutput release];
    captureStillImageOutput = nil;
bail3:
    if (captureQueue) {
        dispatch_release(captureQueue);
        captureQueue = NULL;
    }
bail2:
    [captureVideoDataOutput release];
    captureVideoDataOutput = nil;
bail1:
    captureDevice = nil;
bail0:
    [captureSession release];
    captureSession = nil;
    return;
}

- (void) captureOutput:(AVCaptureOutput *)captureOutput  didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer  fromConnection:(AVCaptureConnection *)connection
{
    //NSLog(@"captureOutput");
    if (captureSession.sessionPreset != captureSessionPreset) return; // Don't try to return video frames during still image capture.
    
    UInt64 hostTime = CVGetCurrentHostTime(); // [[sampleBuffer attributeForKey:QTSampleBufferHostTimeAttribute] unsignedLongLongValue];
    //CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);

    CVPixelBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    
    pthread_mutex_lock(&frameLock_pthread_mutex);

    if (!planes) {
        pixelFormat = CVPixelBufferGetPixelFormatType(imageBuffer);
        //CFDictionaryRef pfDesc = CVPixelFormatDescriptionCreateWithPixelFormatType(CFAllocatorGetDefault(), pixelFormat);
        /* pfDesc for 'BGRA'
        {
            BitsPerBlock = 32;
            BlackBlock = <000000ff>;
            CGBitmapContextCompatibility = 1;
            CGBitmapInfo = 8196;
            CGImageCompatibility = 1;
            ContainsAlpha = 1;
            FillExtendedPixelsCallback = <00000000 5539de31 00000000>;
            IOSurfaceCoreAnimationCompatibility = 1;
            IOSurfaceOpenGLESFBOCompatibility = 1;
            IOSurfaceOpenGLESTextureCompatibility = 1;
            OpenGLESCompatibility = 1;
            PixelFormat = 1111970369;
        }
        */
        /* pfDesc for '420f'
        {
            ContainsAlpha = 0;
            IOSurfaceCoreAnimationCompatibility = 1;
            IOSurfaceOpenGLESFBOCompatibility = 1;
            IOSurfaceOpenGLESTextureCompatibility = 1;
            OpenGLESCompatibility = 1;
            PixelFormat = 875704422;
            Planes =     (
                        {
                    BitsPerBlock = 8;
                    BlackBlock = <00>;
                    FillExtendedPixelsCallback = <00000000 a93cde31 00000000>;
                },
                        {
                    BitsPerBlock = 16;
                    BlackBlock = <8080>;
                    FillExtendedPixelsCallback = <00000000 993bde31 00000000>;
                    HorizontalSubsampling = 2;
                    VerticalSubsampling = 2;
                }
            );
        }
        */
        /* pfDesc for '420v'
        {
            ContainsAlpha = 0;
            IOSurfaceCoreAnimationCompatibility = 1;
            IOSurfaceOpenGLESFBOCompatibility = 1;
            IOSurfaceOpenGLESTextureCompatibility = 1;
            OpenGLESCompatibility = 1;
            PixelFormat = 875704438;
            Planes =     (
                        {
                    BitsPerBlock = 8;
                    BlackBlock = <10>;
                    FillExtendedPixelsCallback = <00000000 a93cde31 00000000>;
                },
                        {
                    BitsPerBlock = 16;
                    BlackBlock = <8080>;
                    FillExtendedPixelsCallback = <00000000 993bde31 00000000>;
                    HorizontalSubsampling = 2;
                    VerticalSubsampling = 2;
                }
            );
        }
        */
        size_t pixelSize;
        switch (pixelFormat) {
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: // All devices except iPhone 3G recommended.
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                pixelSize = 1;
                break;
            case kCVPixelFormatType_32BGRA:
            case kCVPixelFormatType_32ARGB:
            case kCVPixelFormatType_32ABGR:
            case kCVPixelFormatType_32RGBA:
                pixelSize = 4;
                break;
            case kCVPixelFormatType_422YpCbCr8: // iPhone 3G recommended.
            case kCVPixelFormatType_422YpCbCr8_yuvs:
                pixelSize = 2;
                break;
            case kCVPixelFormatType_24RGB:
            case kCVPixelFormatType_24BGR:
                pixelSize = 3;
                break;
            case kCVPixelFormatType_8IndexedGray_WhiteIsZero:
                pixelSize = 1;
                break;
            default:
                pixelSize = 0;
                break;
        }
        if (pixelSize) {
            planeCount = CVPixelBufferGetPlaneCount(imageBuffer);
            if (!planeCount) {
                planes = (cameraVideoBuf_t *)calloc(sizeof(cameraVideoBuf_t), 1);
                planes[0].bytesPerRowSrc = CVPixelBufferGetBytesPerRow(imageBuffer);
                planes[0].width = CVPixelBufferGetWidth(imageBuffer);
                planes[0].height = CVPixelBufferGetHeight(imageBuffer);
                planes[0].bytesPerRow = planes[0].width * pixelSize;
                planes[0].bufDataPtr0 = (unsigned char *)valloc(planes[0].height*planes[0].bytesPerRow);
                planes[0].bufDataPtr1 = (unsigned char *)valloc(planes[0].height*planes[0].bytesPerRow);
                if (!planes[0].bufDataPtr0 || !planes[0].bufDataPtr1) {
                    NSLog(@"Error: Out of memory!\n");
                    goto bail;
                }
            } else {
                size_t vallocSize;
                planes = (cameraVideoBuf_t *)calloc(sizeof(cameraVideoBuf_t), planeCount);

                for (unsigned int i = 0; i < planeCount; i++) {
                    planes[i].bytesPerRowSrc = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, i);
                    planes[i].width = CVPixelBufferGetWidthOfPlane(imageBuffer, i);
                    planes[i].height = CVPixelBufferGetHeightOfPlane(imageBuffer, i);
                    if (i == 0) planes[i].bytesPerRow = planes[i].width * pixelSize;
                    else if (i == 1) planes[i].bytesPerRow = planes[i].width * 2;
                    vallocSize = planes[i].height * planes[i].bytesPerRow;
                    if (0 != vallocSize) {
                        planes[i].bufDataPtr0 = (unsigned char *)valloc(vallocSize);
                        planes[i].bufDataPtr1 = (unsigned char *)valloc(vallocSize);
                    }
                    if (0 == vallocSize || !planes[i].bufDataPtr0 || !planes[i].bufDataPtr1) {
                        NSLog(@"Error: Out of memory!\n");
                        goto bail;
                    }
                }
            }
            
            if (captureStartAsyncCompletion) {
                // Since we're almost certainly not on the main thread, dispatch block on main
                // queue rather than invoking directly, unless the user requested otherwise.
                dispatch_async(captureQueue ? captureQueue : dispatch_get_main_queue(), captureStartAsyncCompletion);
                Block_release(captureStartAsyncCompletion);
                captureStartAsyncCompletion = NULL;
            }
        } //end: if (pixelSize)
    } //if (!planes)
    
    if (pause || !planes) {
        bail:
            pthread_mutex_unlock(&frameLock_pthread_mutex);
            return;
    }

    uint8_t *srcAddress, *destAddress;
    
    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    
    if (!planeCount) {
        srcAddress = CVPixelBufferGetBaseAddress(imageBuffer);
        destAddress = (nextBuf ? planes[0].bufDataPtr1 : planes[0].bufDataPtr0);
        if (!self.flipV && !self.flipH) {
            if (planes[0].bytesPerRow == planes[0].bytesPerRowSrc) memcpy(destAddress, srcAddress, planes[0].height*planes[0].bytesPerRow);
            else {
                for (int r = 0; r < planes[0].height; r++) {
                    memcpy(destAddress + r*planes[0].bytesPerRow, srcAddress + r*planes[0].bytesPerRowSrc, planes[0].bytesPerRow);
                }
            }
        } else {
            // Do flipV/flipH using vImage (available iOS 5.0 and later).
            imageDestBuf.data = destAddress;
            imageSrcBuf.width = imageDestBuf.width = planes[0].width;
            imageSrcBuf.height = imageDestBuf.height = planes[0].height;
            imageSrcBuf.rowBytes = planes[0].bytesPerRowSrc;
            imageDestBuf.rowBytes = planes[0].bytesPerRow;
            imageSrcBuf.data = srcAddress;
            vImage_Error err;
            if (self.flipV) {
                uint8_t black[4] = {0, 0, 0, 0};
                if (self.flipH) err = vImageRotate90_ARGB8888(&imageSrcBuf, &imageDestBuf, 2, black, 0);
                else err = vImageVerticalReflect_ARGB8888(&imageSrcBuf, &imageDestBuf, 0);
            } else /* (self.flipH) */ {
                err = vImageHorizontalReflect_ARGB8888(&imageSrcBuf, &imageDestBuf, 0);
            }
            if (err != kvImageNoError) {
                ARLOGe("Error manipulating image.\n");
            }
        }

#if TARGET_OS_IOS
        if (willSaveNextFrame) {
            CGBitmapInfo bi;
            if      (pixelFormat == kCVPixelFormatType_32BGRA) bi = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little;
            else if (pixelFormat == kCVPixelFormatType_32ARGB) bi = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Big;
            else if (pixelFormat == kCVPixelFormatType_32RGBA) bi = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big;
            else if (pixelFormat == kCVPixelFormatType_32ABGR) bi = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Little;
            else {
                ARLOGe("Error: Request to save frame to camera roll, but not using 32-bit RGB pixel format.\n");
                bi = 0;
            }
            if (bi != 0) {
                CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, destAddress, planes[0].bytesPerRow * planes[0].height, NULL);
                CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
                CGImageRef newImgRef = CGImageCreate(planes[0].width, planes[0].height, 8, 32, planes[0].bytesPerRow, cs, bi, dataProvider, NULL, false, kCGRenderingIntentDefault);
                CGDataProviderRelease(dataProvider);
                CGColorSpaceRelease(cs);
                UIImage *frameImage = [UIImage imageWithCGImage:newImgRef]; // Appears to do a CGImageRetain().
                CGImageRelease(newImgRef);
                if (frameImage) {
                    UIImageWriteToSavedPhotosAlbum(frameImage, nil, nil, NULL);
                    willSaveNextFrame = NO;
                    NSURL *shutterSoundURL = [[NSBundle mainBundle] URLForResource: @"slr_camera_shutter" withExtension: @"wav"];
                    if (shutterSoundURL) {
                        SystemSoundID shutterSound;
                        if (AudioServicesCreateSystemSoundID((CFURLRef)shutterSoundURL, &shutterSound) == noErr) {
                            AudioServicesPlaySystemSound(shutterSound);
                        }
                    }
                }
            }
        }
#endif // TARGET_OS_IOS
        
    } else {
        for (unsigned int i = 0; i < planeCount; i++) {
            srcAddress = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, i);
            destAddress = (nextBuf ? planes[i].bufDataPtr1 : planes[i].bufDataPtr0);
            if (planes[i].bytesPerRow == planes[i].bytesPerRowSrc) {
                memcpy(destAddress, srcAddress, planes[i].height*planes[i].bytesPerRow);
            } else {
                for (int r = 0; r < planes[i].height; r++) {
                    memcpy(destAddress + r*planes[i].bytesPerRow, srcAddress + r*planes[i].bytesPerRowSrc, planes[i].bytesPerRow);
                }
            }
        }
    }
    
    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

    latestFrameHostTime = hostTime;

    pthread_mutex_unlock(&frameLock_pthread_mutex);
    
    if (tookPictureDelegate)
        [tookPictureDelegate cameraVideoTookPicture:self userData:tookPictureDelegateUserData];
}

- (size_t)width
{
    if (planes) {
        return (planes[0].width);
    } else return 0;
}

- (size_t)height
{
    if (planes) {
        return (planes[0].height);
    } else return 0;
}

- (size_t)bytesPerRow
{
    if (planes) {
        return (planes[0].bytesPerRow);
    } else return 0;
}

- (size_t)widthOfPlane:(unsigned int)plane
{
    if (planes && (plane == 0 || plane < planeCount)) {
        return (planes[plane].width);
    } else return 0;
}

- (size_t)heightOfPlane:(unsigned int)plane
{
    if (planes && (plane == 0 || plane < planeCount)) {
        return (planes[plane].height);
    } else return 0;
}

- (size_t)bytesPerRowOfPlane:(unsigned int)plane
{
    if (planes && (plane == 0 || plane < planeCount)) {
        return (planes[plane].bytesPerRow);
    } else return 0;
}

- (unsigned char *) frameTimestamp:(UInt64 *)timestampOut
{
    if (planes) {
        unsigned char *ret;
        pthread_mutex_lock(&frameLock_pthread_mutex);
        if (timestampOut) *timestampOut = latestFrameHostTime;
        ret = (nextBuf ? planes[0].bufDataPtr1 : planes[0].bufDataPtr0);
        nextBuf = !nextBuf;
        pthread_mutex_unlock(&frameLock_pthread_mutex);
        return ret;
    } else return (NULL);
}

- (unsigned char *) frameTimestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp
{
    // This check should still be safe to perform without
    // a lock since the producer thread will only ever increase latestFrameHostTime.
    if (latestFrameHostTime <= timestamp) return (NULL);
    return ([self frameTimestamp:timestampOut]);
}

- (BOOL) framePlanes:(unsigned char **)bufDataPtrs count:(size_t)count timestamp:(UInt64 *)timestampOut
{
    if (planes && count > 0 && count <= planeCount) {
        pthread_mutex_lock(&frameLock_pthread_mutex);
        if (timestampOut) *timestampOut = latestFrameHostTime;
        for (int i = 0; i < count; i++) bufDataPtrs[i] = (nextBuf ? planes[i].bufDataPtr1 : planes[i].bufDataPtr0);
        nextBuf = !nextBuf;
        pthread_mutex_unlock(&frameLock_pthread_mutex);
        return (TRUE);
    } else return (FALSE);
}

- (BOOL) framePlanes:(unsigned char **)bufDataPtrs count:(size_t)count timestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp
{
    // This check should still be safe to perform without
    // a lock since the producer thread will only ever increase latestFrameHostTime.
    if (latestFrameHostTime <= timestamp) return (FALSE);
    return ([self framePlanes:bufDataPtrs count:count timestamp:timestampOut]);
}

- (UInt64) timestampsPerSecond
{
    return ((UInt64)CVGetHostClockFrequency());
}

- (void) takePhoto
{
    if (!running) return;
    
    if (captureSessionPreset != AVCaptureSessionPresetPhoto && [captureSession canSetSessionPreset:AVCaptureSessionPresetPhoto]) [captureSession setSessionPreset:AVCaptureSessionPresetPhoto];
    
    // Find out the current orientation and tell the still image output.
	AVCaptureConnection *captureStillImageConnection = [captureStillImageOutput connectionWithMediaType:AVMediaTypeVideo];
	//UIDeviceOrientation curDeviceOrientation = [[UIDevice currentDevice] orientation];
	//AVCaptureVideoOrientation avcaptureOrientation = [self avOrientationForDeviceOrientation:curDeviceOrientation];
	//[captureStillImageConnection setVideoOrientation:avcaptureOrientation];

	[captureStillImageOutput captureStillImageAsynchronouslyFromConnection:captureStillImageConnection
                                                         completionHandler:^(CMSampleBufferRef imageDataSampleBuffer, NSError *error) {
                                                             if (error) {
                                                                 NSLog(@"Image capture failed.\n");
                                                             } else {
                                                                 NSData *jpegData = [AVCaptureStillImageOutput jpegStillImageNSDataRepresentation:imageDataSampleBuffer];
                                                                 if (tookPictureDelegate) {
                                                                     if ([tookPictureDelegate respondsToSelector:@selector(cameravideoTookPictureHires:userData:jpegData:)]) {
                                                                         [tookPictureDelegate cameravideoTookPictureHires:self userData:tookPictureDelegateUserData jpegData:jpegData];
                                                                     }
                                                                 }
                                                                 if (captureSessionPreset != AVCaptureSessionPresetPhoto && [captureSession canSetSessionPreset:captureSessionPreset]) [captureSession setSessionPreset:captureSessionPreset];
                                                             }
                                                         }];

}

- (void) takePhotoViaNotification:(NSNotification *)notification
{
    [self takePhoto];
}

- (BOOL) setFocus:(AVCaptureFocusMode)mode atPixelCoords:(CGPoint)coords
{
    if (!captureDevice) return FALSE;
    if (![captureDevice isFocusModeSupported:mode]) return FALSE;
    
    NSError *error = nil;
    [captureDevice lockForConfiguration:&error];
    if (error) {
        NSLog(@"Error: unable to set focus mode. %@.", error.localizedDescription);
        return FALSE;
    } else {
        [captureDevice setFocusMode:mode];
        if (mode == AVCaptureFocusModeAutoFocus && [captureDevice isFocusPointOfInterestSupported]) {
            CGPoint coordsNorm = CGPointMake(coords.x/(float)self.width, coords.y/(float)self.height);
            [captureDevice setFocusPointOfInterest:coordsNorm];
        }
        [captureDevice unlockForConfiguration];
        return TRUE;
    }
}

- (void) stop
{
    if (!running) return;
    running = FALSE;
    
    if (captureStartAsyncCompletion) {
        Block_release(captureStartAsyncCompletion);
        captureStartAsyncCompletion = NULL;
    }
    
    [captureSession stopRunning];
    if (captureQueue) {
        dispatch_release(captureQueue);
        captureQueue = NULL;
    }
    [captureVideoDataOutput release];
    captureVideoDataOutput = nil;
    [captureStillImageOutput release];
    captureStillImageOutput = nil;
    captureDevice = nil;
    [captureSession release];
    captureSession = nil;
}

- (void) dealloc
{
    if (running) [self stop];
    
    if (captureQueue) dispatch_release(captureQueue);
    pthread_mutex_destroy(&frameLock_pthread_mutex);
    
    if (planes) {
        if (!planeCount) {
            free(planes[0].bufDataPtr0);
            free(planes[0].bufDataPtr1);
        } else {
            for (int i = 0; i < planeCount; i++) {
                free(planes[i].bufDataPtr0);
                free(planes[i].bufDataPtr1);
            }
        }
        free(planes);
        planes = NULL;
        planeCount = 0;
        nextBuf = 0;
    }
    
    [super dealloc];
}

#ifdef CAMERA_VIDEO_DEBUG
- (BOOL) respondsToSelector:(SEL)sel
{
    BOOL r = [super respondsToSelector:sel];
    if (!r)
        NSLog(@"CameraVideo was just asked for a response to selector \"%@\" (we do%@)\n", NSStringFromSelector(sel), (r ? @"" : @" not"));
    return r;
}
#endif

@end




#endif //  ARVIDEO_INPUT_AVFOUNDATION
