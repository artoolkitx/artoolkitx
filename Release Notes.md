# artoolkitX Release Notes
--------------------------

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
