/*
 *	CameraVideo.h
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

#import <AVFoundation/AVFoundation.h>

@protocol CameraVideoTookPictureDelegate<NSObject>
@required
/*
    This delegate method is called each time a frame is captured
    from the video stream.
 
    The frame can be retrieved with the call:
    
    UInt64 timestamp;
    unsigned char *frameData = frameTimestamp:&timestamp;
 */
- (void) cameraVideoTookPicture:(id)sender userData:(void *)data;
@optional
/*
    This delegate method is called if the user requested a high-resolution JPEG photo.
    You can write the JPEG to the user's photo roll with this code:
 
    #import <AssetsLibrary/AssetsLibrary.h>
    ALAssetsLibrary *library = [[ALAssetsLibrary alloc] init];
    [library writeImageDataToSavedPhotosAlbum:jpegData metadata:nil completionBlock:^(NSURL *assetURL, NSError *error) {
        if (error) {
            NSLog(@"Error writing captured photo to photo album.\n");
        }
    }];
    [library release];
 
    or you can write it directly to a file:
 
    if (![jpegData writeToFile:jpegPath atomically:NO]) {
        NSLog(@"Error writing captured photo to '%@'\n", jpegPath);
    }
 
    There is no guarantee that this call will be made on any particular thread.
*/
- (void) cameravideoTookPictureHires:(id)sender userData:(void *)data jpegData:(NSData *)jpegData;
@end

typedef void (^CameraVideoStartAsyncCompletionBlock)(void);

extern NSString *const CameraVideoiOSDeviceiPhone; // "iPhone"
extern NSString *const CameraVideoiOSDeviceiPhone3G; // "iPhone 3G"
extern NSString *const CameraVideoiOSDeviceiPhone3GS; // "iPhone 3GS"
extern NSString *const CameraVideoiOSDeviceiPhone4; // "iPhone 4"
extern NSString *const CameraVideoiOSDeviceiPhone4S; // "iPhone 4S"
extern NSString *const CameraVideoiOSDeviceiPhone5; // "iPhone 5"
extern NSString *const CameraVideoiOSDeviceiPhone5s; // "iPhone 5s"
extern NSString *const CameraVideoiOSDeviceiPhone5c; // "iPhone 5c"
extern NSString *const CameraVideoiOSDeviceiPhone6; // "iPhone 6"
extern NSString *const CameraVideoiOSDeviceiPhone6Plus; // "iPhone 6 Plus"
extern NSString *const CameraVideoiOSDeviceiPhone6S; // "iPhone 6S"
extern NSString *const CameraVideoiOSDeviceiPhone6SPlus; // "iPhone 6S Plus"
extern NSString *const CameraVideoiOSDeviceiPhoneSE; // "iPhone SE"
extern NSString *const CameraVideoiOSDeviceiPhone7; // "iPhone 7"
extern NSString *const CameraVideoiOSDeviceiPhone7Plus; // "iPhone 7 Plus"
extern NSString *const CameraVideoiOSDeviceiPodTouch4; // "iPod Touch (4th Generation)"
extern NSString *const CameraVideoiOSDeviceiPodTouch5; // "iPod Touch (5th Generation)"
extern NSString *const CameraVideoiOSDeviceiPodTouch6; // "iPod Touch (6th Generation)"
extern NSString *const CameraVideoiOSDeviceiPad2; // "iPad 2"
extern NSString *const CameraVideoiOSDeviceiPad3; // "iPad (3rd generation)"
extern NSString *const CameraVideoiOSDeviceiPad4; // "iPad (4th generation)"
extern NSString *const CameraVideoiOSDeviceiPadAir; // "iPad Air"
extern NSString *const CameraVideoiOSDeviceiPadAir2; // "iPad Air 2"
extern NSString *const CameraVideoiOSDeviceiPadMini; // "iPad mini"
extern NSString *const CameraVideoiOSDeviceiPadMini2; // "iPad mini (2nd generation)"
extern NSString *const CameraVideoiOSDeviceiPadMini3; // "iPad mini (3rd generation)"
extern NSString *const CameraVideoiOSDeviceiPadMini4; // "iPad mini (4th generation)"
extern NSString *const CameraVideoiOSDeviceiPadPro129; // "iPad Pro 12.9\""
extern NSString *const CameraVideoiOSDeviceiPadPro97; // "iPad Pro 9.7\""
extern NSString *const CameraVideoiOSDeviceiPhoneX; // "iPhone (Unknown model)"
extern NSString *const CameraVideoiOSDeviceiPodX; // "iPod (Unknown model)"
extern NSString *const CameraVideoiOSDeviceiPadX; // "iPad (Unknown model)"
extern NSString *const CameraVideoiOSDeviceAppleTVX; // "Apple TV (Unknown model)"

@class CameraVideo;

@interface CameraVideo : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (id) init;

@property(readonly) NSString *iOSDevice;

/*
 Set or get the video image quality/size.
 
 Attempts to change this property while (running == TRUE) will be ignored.
 
 Acceptable values:
     AVCaptureSessionPresetHigh         // iPhone 3G: 400x304.
                                        // iPhone 3GS: 640x480.
                                        // iPhone 4/iPod touch 4G/iPad 2: rear 1280x720, front 640x480.
                                        // iPhone 4S and later, iPad (3rd generation): rear 1920x1080, front 640x480.
                                        // iPhone (4th generation), iPad Air, iPad Mini, iPad Mini (2nd generation): rear 1920x1080, front 1280x720
                                        // iPhone 5, 5c, 5s: rear 1920x1080, front 1280x720.
     AVCaptureSessionPresetMedium       // iPhone 3G: 400x304
                                        // iPhone 3GS/iPhone 4/iPod touch 4G/iPad 2: 480x360
     AVCaptureSessionPresetLow          // iPhone 3G/iPhone 3GS/iPhone 4/iPod touch 4G/iPad 2: 192x144.
     AVCaptureSessionPreset640x480      // iPhone 3G: not supported. iPhone 3GS/iPhone 4/iPod touch 4G/iPad 2:640x480.
     AVCaptureSessionPreset1280x720     // iPhone 3G/3GS: not supported. iPhone 4/iPod touch 4G/iPad 2:1280x720.
     AVCaptureSessionPreset352x288      // iOS 5.0-only, iPhone 3GS and later.
     AVCaptureSessionPreset1920x1080    // iOS 5.0-only, iPhone 4S and later, iPad (3rd generation) and later.
 Default value is AVCaptureSessionPresetMedium.
 
 N.B. 1920x1080 and 1280x720 are 16:9 aspect ratio.
      640x480, 480x360, 192x144 are 4:3 aspect ratio.
 
 */
@property(nonatomic, assign) NSString *captureSessionPreset;

/*
 Set or get the video device position.
 
 Attempts to change this property while (running == TRUE) will be ignored.

 Acceptable values:
     AVCaptureDevicePositionUnspecified
     AVCaptureDevicePositionBack
     AVCaptureDevicePositionFront
 Default value is AVCaptureDevicePositionUnspecified.
 */
@property(nonatomic) AVCaptureDevicePosition captureDevicePosition;

/*
 Set or get the video device index.
 
 Attempts to change this property while (running == TRUE) will be ignored.
 
 Set or get the index of the capture device in use.
 When captureDevicePosition==AVCaptureDevicePositionUnspecified, this is the index into the list of all available devices.
 Otherwise, it is the index into the list of devices at that position.
 Default value is 0.
 */
@property(nonatomic) int captureDeviceIndex;

@property(nonatomic, readonly) NSString *captureDeviceIDManufacturer;
@property(nonatomic, readonly) NSString *captureDeviceIDName;
@property(nonatomic, readonly) NSString *captureDeviceIDModel;

/*
 Set or get the video device unique ID.
 
 Attempts to change this property while (running == TRUE) will be ignored.
 
 Set or get the unique ID of the capture device in use/to use.
 If set, it will override captureDeviceIndex.
 Default value set is nil.
 */
@property(nonatomic, copy) NSString *captureDeviceIDUID;

/*
 Set or get the video image pixel format.
 
 Attempts to change this property while (running == TRUE) will be ignored.
 
 Acceptable values:
    kCVPixelFormatType_32BGRA                       (Default.)
    kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    kCVPixelFormatType_420YpCbCr8BiPlanarFullRange  (Recommended for iPhone 3GS, 4 and later.)
    kCVPixelFormatType_422YpCbCr8                   (Recommended for iPhone 3G)
  */
@property(nonatomic) OSType pixelFormat;

// Starts capture from camera and waits until the first frame has
// been received. Then sets 'running' and returns to caller.
// Note that since this runs the runloop, it may deadlock if
// a runloop task is invoked which waits on this this thread.
// It is recommended to use -startAsync: instead.
// In cases of error, 'running' will not be set on return.
- (void) start;

// Starts capture from camera, sets 'running' and returns to caller
// immediately. Once the first frame has been received, invokes
// 'completion' on main queue.
// In cases of error, 'running' will not be set on return.
- (void) startAsync:(CameraVideoStartAsyncCompletionBlock)completion;

// Set once -start or -startAsync: has been called successfully.
@property(nonatomic, readonly) BOOL running;
@property(nonatomic) BOOL pause;

// The delegate which gets call each time a new frame is available, and its userdata.
// See discussion of CameraVideoTookPictureDelegate above.
@property(nonatomic, assign) id <CameraVideoTookPictureDelegate> tookPictureDelegate;
@property(nonatomic, assign) void *tookPictureDelegateUserData;

// These values are valid only once the first frame has been received.
// When invalid, they return 0.
// When a multi-planar format is in use, these are the same as calling
// -widthOfPlane:0, -heightOfPlane:0 or -bytesPerRowOfPlane:0.
@property(nonatomic, readonly) size_t width;
@property(nonatomic, readonly) size_t height;
@property(nonatomic, readonly) size_t bytesPerRow;

@property(nonatomic, readonly) size_t planeCount; // 0 for non-planar formats, or number of planes (will be 2 for kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange and kCVPixelFormatType_420YpCbCr8BiPlanarFullRange.
- (size_t)widthOfPlane:(unsigned int)plane;
- (size_t)heightOfPlane:(unsigned int)plane;
- (size_t)bytesPerRowOfPlane:(unsigned int)plane;

@property(nonatomic) BOOL flipV;
@property(nonatomic) BOOL flipH;

// Invoke this method to request the capture of a high resolution photo from the video
// stream. If successful, the CameraVideoTookPictureDelegate delegate protocol
// method -cameravideoTookPictureHires:userData:jpegData: will be invoked, so make
// sure you've set the delegate before invocation.
- (void) takePhoto;
// You can also do the same thing, just from a notification with this method.
- (void) takePhotoViaNotification:(NSNotification *)notification;

// If set, when the next frame arrives it will be saved to the user's camera roll.
// Only supported when using 32-bit RGB pixel formats, e.g. pixelFormat = kCVPixelFormatType_32BGRA.
@property BOOL willSaveNextFrame;

// Sets focus mode. When mode == AVCaptureFocusModeAutoFocus, then coords must be a
// valid 2D coordinate in pixels, with 0,0 at the top-left of the frame where the frame
// is considered upright when the device is held in landscape mode with the home button on the right.
// Note that this DOES NOT give a visual indication of the focus point; that is up to the
// caller to display, should he or she wish to.
- (BOOL) setFocus:(AVCaptureFocusMode)mode atPixelCoords:(CGPoint)coords;

// Get a pointer to the most recent frame.
// If timestampOut is non-NULL, it will be filled with a timestamp using the same
// timebase as CVGetCurrentHostTime().
- (unsigned char *) frameTimestamp:(UInt64 *)timestampOut;

// Get a pointer to the most recent only if it is newer than 'timestamp'.
// Otherwise returns NULL.
// If timestampOut is non-NULL, it will be filled with a timestamp using the same
// timebase as CVGetCurrentHostTime().
- (unsigned char *) frameTimestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp;

- (BOOL) framePlanes:(unsigned char **)bufDataPtrs count:(size_t)count timestamp:(UInt64 *)timestampOut;
- (BOOL) framePlanes:(unsigned char **)bufDataPtrs count:(size_t)count timestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp;
@property(nonatomic, readonly) UInt64 timestampsPerSecond;

// If set, callbacks on the delegate method -cameraVideoTookPicture:userData: will
// be made on a serial queue not necessarily attached to the main thread.
// Defaults to FALSE, i.e. callbacks are on the main thread. This allows OpenGL
// operations, which should normally be on the main thread, to be called during the callback.
// Changing this variable after calling -start will not take effect until -start is next called.
@property(nonatomic) BOOL multithreaded;

- (void) stop;
- (void) dealloc;


@end
