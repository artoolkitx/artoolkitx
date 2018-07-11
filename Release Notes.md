# artoolkitX Release Notes
--------------------------

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
