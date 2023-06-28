# artoolkitX Release Notes
--------------------------

## Version 1.1.11
### 2023-06-28

 * iOS: prelinked OpenCV and CURL so users of libARX.a don't have to.
 * Documentation update.

## Version 1.1.10
### 2023-06-14

 * Added a new "External" video module with support for a new all-platform ar2VideoPush* API, including in wrapper and ARController.
 * Removed entirely support for Android non-native video. Defaults to native now. Users who need pushed video on Android should update to use the new module and API. JNI versions of new wrapper functions optimised for direct byte buffers, same as old arwVideoPushAndroid* functions.
 * Standardised ARVideo handling of camera focal length hints.
 * Added some functions to support adjusting camera intrinsics to provide alternate means of scaling viewing frustum to work with a viewport of arbitrary size.

Bug fixes:
 * Fix iOS builds picking up stray jpeg lib by ensuring libjpeg is included in the static single-object prelink.

## Version 1.1.9
### 2023-05-18

 * Support for asynchronous tracking (on a secondary thread) has been added to the 2D tracker. When enabled, the tracking rate can run slower than the video capture frame rate. This results in increased smoothness of the display of video frames, at the expense of some memory usage and a possible lag on lower-powered devices between the displayed frame and the tracking results. It has been enabled by default. Use wrapper function arwSetTrackerOptionBool(ARW_TRACKER_OPTION_2D_THREADED, false) to revert to previous behaviour.

Internal changes:
 * Updated OpenCV on Android to v4.6.0.

## Version 1.1.8
### 2023-05-15

Internal changes:
 * Updated OpenCV on iOS to v4.6.0.
 * Added sha1 calculation in ARUtil.

Bug fixes:
 * Fix trackable sizes in 2D tracking example on mobile platforms.

## Version 1.1.7
### 2023-05-08

Internal and API changes:
 * arwGetTrackableStatuses has replaced arwGetTrackables in the wrapper API.
 * Provided options in the square tracker barcode (matrix) mode for auto-creation of trackables for newly recognized barcodes, plus a callback mechanism for auto-created trackables, plus wrapper API.
 * Moved ownership of trackables out of ARController into each ARTracker. Change most methods returning trackable to return shared_ptr.
 * Added getters/setters for square and 2D trackable width/height at runtime.

Bug fixes:
 * Correct handling of GlobalID codes in several places.

## Version 1.1.6
### 2023-04-21

Bug fixes:
 * Correct pattern retrieval buffer origin for barcode, 2D, NFT trackables.
 * Fix pattern colour and barcode handling in pattern retrieval.
 * Minor fix for GL library warning, and locale for apt-cache on Linux.

## Version 1.1.5
### 2023-04-12

 * Corrected 2D planar tracker orientation and scaling issues.
 * Clarified that "2D tracker scale factor" specifies image width, not height. Now in same units (usually millimetres) as other trackers.
 * Overhauled ARTrackable pattern handling, including new support for 2D and NFT surfaces, plus barcodes. Remove ARPattern class.

## Version 1.1.4
### 2023-03-30

 * Corrected a long-standing Windows bug that affected 2D planar tracking with 32-bit video formats.
 * Updated the OpenCV build used by the 2D planar tracker on Windows and switched to static OpenCV libraries. This saves users from having to deploy the OpenCV DLLs alongside built apps.

## Version 1.1.3
### 2023-03-23

 * Added support for setting the number of 2D planar tracker markers that can be simultaneously tracked.

## Version 1.1.2
### 2023-03-16

 * Addition of a new utility check_image_2d_tracking which displays features and tracking templates used by the 2D planar tracker.

## Version 1.1.1
### 2023-03-10

 * Minor additions to the ARX wrapper API to enable fetching of video source input lists.

## Version 1.1.0
### 2023-02-23

This release improves and updates platform support.
 * A new native video input module (Camera2) on Android removes the requirement for pushing of video frames from Java code over JNI. Some video configuration options have changed, in particular you must add "-native" to the video configuration string to use the new module. While ARX retains backwards-compatibility for now, the Java ARXJ framework has been reworked to remove support for pushing video and now uses the new native video capabilities.
 * On Android, the minimum supported OS is now Android 7.0 (API level 24).
 * On iOS, the minimum supported OS is now iOS v11.0.
 * On mac OS, full support for the ARM64 (Apple Silicon) CPU. The minimum supported OS is now mac OS 10.13.
 * On Windows, the default Visual Studio version is now VS 2019.
 * On Linux, the system OpenCV implementation is preferred.
Other minor changes:
 * Added initial Emscripten support.
 * Added videoRGBA and videoBGRA implementations for AR_PIXEL_FORMAT_420v.
 * Bugfix: fix arRefineCorners error when building without OpenCV
 * Update FreakMatcher to Eigen v3.3.7.
 * Remove dependence on opencv imread(), and therefore highgui, imgcodecs, and various image libs.
 * Bugfix: mac OS had conflicting headers for libjpeg static library. Now using libjpeg-turbo 2.1.5.

## Version 1.0.6
### 2020-20-19

This release improves and updates platform support.
 * On iOS, builds now work with newer versions of CMake.
 * On macOS, the command-line utilities now include camera permissions requests.
 * On Windows, improved support for newer OpenGL runtimes.
 * Corrects a bug in calculating marker confidence during stereo tracking.

## Version 1.0.5.1
### 2018-10-15

This release changes only the Android SDK. The Android build now builds against SDK 28 (with build tools v28.0.3).

## Version 1.0.5
### 2018-10-08

This release corrects a build problem with iOS and macOS caused by conflicting libjpeg versions between artoolkitX and OpenCV. As of this release, artoolkitX for macOS and iOS uses libjpeg-turbo and a prerelease version of OpenCV 4.0.0. There are some other minor changes allowing setting of some square tracking and thresholding options.

## Version 1.0.4
### 2018-10-05

This is a minor release which adds the option to set a minimum inlier probability when using robust multi-marker square tracking.

## Version 1.0.3
### 2018-09-25

This release adds new capabilities and addresses a number issues.
 * If OpenCV is available, subpixel refinement of square marker corners is now available (not enabled by default).
 * A significant bug affecting 2D texture tracking has been corrected.
 * A new ARTrackable subclass is available which automatically builds a map of 2D barcode markers. Optionally, if the GTSAM toolkit is available, it will be used to refine the map.
 * On Linux, if OpenCV 3.x is available in the system, it will be used in preference to the supplied OpenCV build.
 * For Android, gradle has been updated to 4.4.
 * For Windows, build script has been improved.
 * Other minor bug fixes.

## Version 1.0.2
### 2018-07-11

This is a minor release which focuses on addressing minor issues. Two cases where multimarker or single marker tracking might have returned an invalid pose as correct when the ICP pose estimation failed have been corrected. The embedded sqlite for Android has been updated.

## Version 1.0.1
### 2018-05-04

This is a minor release which focuses on addressing minor issues. Support for the Linux Raspbian platform has been improved. Also included is preliminary support for a new 12-factor OpenCV 3.x-based lens distortion model.

## Version 1.0
### 2018-03-26

This is the first release of the new artoolkitX SDK.

As well as a brand-new 2D texture tracker developed by the project team and using OpenCV primitives, artoolkitX includes the square tracker and NFT texture tracker from the LGPL-v3 ARToolKit v5, and the external API will be familiar to users transitioning from older versions of ARToolKit.

The component libraries in artoolkitX are combined together into a single dynamic library, or static library, or framework, depending on the platform. Both the lower-level C-API to each component library and the higher-level abstractions in the C++ class structure are available for users to target.

In addition, there is a complete Java library (ARXJ) which interfaces via JNI, and a complete C# library in the separate artoolkitX for Unity project, whichg interfaces via P/Invoke.
