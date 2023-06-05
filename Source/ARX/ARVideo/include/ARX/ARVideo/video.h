/*
 *	video.h
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Atsishi Nakazawa, Philip Lamb
 *
 */
/*
 *
 * Author: Hirokazu Kato, Atsishi Nakazawa
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *         nakazawa@inolab.sys.es.osaka-u.ac.jp
 *
 * Revision: 4.3
 * Date: 2002/01/01
 *
 */

#ifndef AR_VIDEO_H
#define AR_VIDEO_H

#include <ARX/AR/ar.h>
#include <ARX/ARVideo/videoConfig.h>
#include <limits.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    AR_VIDEO_MODULE_DUMMY              = 0,
    AR_VIDEO_MODULE_EXTERNAL           = 1,
    AR_VIDEO_MODULE_RESERVED2          = 2,
    AR_VIDEO_MODULE_1394               = 3,
    AR_VIDEO_MODULE_RESERVED4          = 4,
    AR_VIDEO_MODULE_RESERVED5          = 5,
    AR_VIDEO_MODULE_RESERVED6          = 6,
    AR_VIDEO_MODULE_RESERVED7          = 7,
    AR_VIDEO_MODULE_RESERVED8          = 8,
    AR_VIDEO_MODULE_RESERVED9          = 9,
    AR_VIDEO_MODULE_RESERVED10         = 10,
    AR_VIDEO_MODULE_GSTREAMER          = 11,
    AR_VIDEO_MODULE_AVFOUNDATION       = 12,
    AR_VIDEO_MODULE_RESERVED13         = 13,
    AR_VIDEO_MODULE_IMAGE              = 14,
    AR_VIDEO_MODULE_ANDROID            = 15,
    AR_VIDEO_MODULE_WINDOWS_MEDIA_FOUNDATION = 16,
    AR_VIDEO_MODULE_WINDOWS_MEDIA_CAPTURE = 17,
    AR_VIDEO_MODULE_V4L2               = 18,
    AR_VIDEO_MODULE_EMSCRIPTEN         = 19,
    AR_VIDEO_MODULE_MAX                = 19,
} AR_VIDEO_MODULE;

//
// arVideoParamGet/arVideoParamSet names.
//

#define  AR_VIDEO_1394_BRIGHTNESS                      65
#define  AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON           66
#define  AR_VIDEO_1394_BRIGHTNESS_AUTO_ON              67
#define  AR_VIDEO_1394_BRIGHTNESS_MAX_VAL              68
#define  AR_VIDEO_1394_BRIGHTNESS_MIN_VAL              69
#define  AR_VIDEO_1394_EXPOSURE                        70
#define  AR_VIDEO_1394_EXPOSURE_FEATURE_ON             71
#define  AR_VIDEO_1394_EXPOSURE_AUTO_ON                72
#define  AR_VIDEO_1394_EXPOSURE_MAX_VAL                73
#define  AR_VIDEO_1394_EXPOSURE_MIN_VAL                74
#define  AR_VIDEO_1394_WHITE_BALANCE                   75
#define  AR_VIDEO_1394_WHITE_BALANCE_UB                76
#define  AR_VIDEO_1394_WHITE_BALANCE_VR                77
#define  AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON        78
#define  AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON           79
#define  AR_VIDEO_1394_WHITE_BALANCE_MAX_VAL           80
#define  AR_VIDEO_1394_WHITE_BALANCE_MIN_VAL           81
#define  AR_VIDEO_1394_SHUTTER_SPEED                   82
#define  AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON        83
#define  AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON           84
#define  AR_VIDEO_1394_SHUTTER_SPEED_MAX_VAL           85
#define  AR_VIDEO_1394_SHUTTER_SPEED_MIN_VAL           86
#define  AR_VIDEO_1394_GAIN                            87
#define  AR_VIDEO_1394_GAIN_FEATURE_ON                 88
#define  AR_VIDEO_1394_GAIN_AUTO_ON                    89
#define  AR_VIDEO_1394_GAIN_MAX_VAL                    90
#define  AR_VIDEO_1394_GAIN_MIN_VAL                    91
#define  AR_VIDEO_1394_FOCUS                           92
#define  AR_VIDEO_1394_FOCUS_FEATURE_ON                93
#define  AR_VIDEO_1394_FOCUS_AUTO_ON                   94
#define  AR_VIDEO_1394_FOCUS_MAX_VAL                   95
#define  AR_VIDEO_1394_FOCUS_MIN_VAL                   96
#define  AR_VIDEO_1394_GAMMA                           97
#define  AR_VIDEO_1394_GAMMA_FEATURE_ON                98
#define  AR_VIDEO_1394_GAMMA_AUTO_ON                   99
#define  AR_VIDEO_1394_GAMMA_MAX_VAL                  100
#define  AR_VIDEO_1394_GAMMA_MIN_VAL                  101

#define  AR_VIDEO_PARAM_GET_IMAGE_ASYNC               200 ///< int, readonly. If non-zero, this module can deliver new frames async (via a callback passed to ar2VideoCapStartAsync).
#define  AR_VIDEO_PARAM_DEVICEID                      201 ///< string, readonly. Optional. Unique name for this exact model of video device, consisting of vendor, model and board identifiers separated by '/' characters.
#define  AR_VIDEO_PARAM_NAME                          202 ///< string, readonly. Optional. Human-readable name for this model of video device.

/// double. Camera lens focal length, i.e. optimal distance from camera aperture to the focal plane at which objects are optimally in-focus.
/// When setting, this acts as a hint, i.e. it does not change the actual camera lens.
/// When getting, on systems where the focal length is measured, the measured value, otherwise the value of the hint previously set.
/// If unknown or unset, returns 0.
#define  AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH           300
#define  AR_VIDEO_FOCUS_MODE                          301 ///< int, values from constants AR_VIDEO_FOCUS_MODE_*
#define  AR_VIDEO_FOCUS_MANUAL_DISTANCE               302 ///< double. When focus mode is AR_VIDEO_FOCUS_MODE_MANUAL, manually set camera lens to focus at specified distance measured in metres.
#define  AR_VIDEO_FOCUS_POINT_OF_INTEREST_X           303 ///< double. When focus mode is AR_VIDEO_FOCUS_MODE_FIXED or AR_VIDEO_FOCUS_MODE_AUTOFOCUS, x coordinate in video frame (from top left) of point to use as metering centre for camera lens auto-focus requests.
#define  AR_VIDEO_FOCUS_POINT_OF_INTEREST_Y           304 ///< double. When focus mode is AR_VIDEO_FOCUS_MODE_FIXED or AR_VIDEO_FOCUS_MODE_AUTOFOCUS, y coordinate in video frame (from top left) of point to use as metering centre for a camera lens auto-focus request.

#define  AR_VIDEO_PARAM_AVFOUNDATION_IOS_DEVICE                400 ///< int, values from enumeration AR_VIDEO_AVFOUNDATION_IOS_DEVICE.
#define  AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET              401 ///< int, values from enumeration AR_VIDEO_AVFOUNDATION_FOCUS_PRESET.
#define  AR_VIDEO_PARAM_AVFOUNDATION_CAMERA_POSITION           402 ///< int, values from enumeration AR_VIDEO_AVFOUNDATION_CAMERA_POSITION.
#define  AR_VIDEO_PARAM_AVFOUNDATION_WILL_CAPTURE_NEXT_FRAME   403 ///< int (0=false, 1=true). If true, next incoming frame will also be captured to system's camera roll.

#define  AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX          500 ///< int
#define  AR_VIDEO_PARAM_ANDROID_CAMERA_FACE           501 ///< int
#define  AR_VIDEO_PARAM_ANDROID_INTERNET_STATE        502 ///< int
#define  AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH          503 ///< double. Synonym for AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH.

#define  AR_VIDEO_GET_VERSION                     INT_MAX

// For arVideoParamGet(AR_VIDEO_FOCUS_MODE, ...)
#define  AR_VIDEO_FOCUS_MODE_FIXED                    0
#define  AR_VIDEO_FOCUS_MODE_AUTO                     1
#define  AR_VIDEO_FOCUS_MODE_POINT_OF_INTEREST        2
#define  AR_VIDEO_FOCUS_MODE_MANUAL                   3

typedef enum {
    AR_VIDEO_SIZE_PREFERENCE_ANY = 0,                   ///< Accept any size video frame.
    AR_VIDEO_SIZE_PREFERENCE_EXACT,                     ///< Accept only the exact size requested.
    AR_VIDEO_SIZE_PREFERENCE_CLOSEST_SAME_ASPECT,       ///< Accept the closest size to the requested with the same aspect ratio.
    AR_VIDEO_SIZE_PREFERENCE_CLOSEST_PIXEL_COUNT,       ///< Accept the closest size to the requested pixel count (w x h).
    AR_VIDEO_SIZE_PREFERENCE_SAME_ASPECT,               ///< Accept any size with the same aspect ratio.
    AR_VIDEO_SIZE_PREFERENCE_LARGEST_WITH_MAXIMUM,      ///< Accept the largest size, but no larger than requested.
    AR_VIDEO_SIZE_PREFERENCE_SMALLEST_WITH_MINIMUM,     ///< Accept the smallest size, but no smaller than requested.
    AR_VIDEO_SIZE_PREFERENCE_LARGEST,                   ///< Accept the largest size.
    AR_VIDEO_SIZE_PREFERENCE_SMALLEST                   ///< Accept the smallest size.
} ARVideoSizePreference;


///
/// @brief Values returned by arVideoParamGeti(AR_VIDEO_PARAM_AVFOUNDATION_IOS_DEVICE, ...)
///
typedef enum {
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE3G = 1,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE3GS,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE4,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPODTOUCH4,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD2,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD3,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE_GENERIC,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPOD_GENERIC,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD_GENERIC,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE4S,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_APPLETV_GENERIC,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPODTOUCH5,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPAD4,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5C,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE5S,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADAIR,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI2,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI3,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADMINI4,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADAIR2,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6PLUS,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6S,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE6SPLUS,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONESE,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADPRO129,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE7,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPHONE7PLUS,
    AR_VIDEO_AVFOUNDATION_IOS_DEVICE_IPADPRO97,
} AR_VIDEO_AVFOUNDATION_IOS_DEVICE;

///
/// @brief Values returned by arVideoParamGeti/arVideoParamSeti(AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET, ...)
///
typedef enum {
    AR_VIDEO_AVFOUNDATION_FOCUS_NONE = 0,   ///< No focus preset set.
    AR_VIDEO_AVFOUNDATION_FOCUS_MACRO,      ///< Focus preset to camera's shortest macro setting.
    AR_VIDEO_AVFOUNDATION_FOCUS_0_3M,       ///< Focus preset to 0.3 metres.
    AR_VIDEO_AVFOUNDATION_FOCUS_1_0M,       ///< Focus preset to 1.0 metres.
    AR_VIDEO_AVFOUNDATION_FOCUS_INF         ///< Focus preset to optical infinity.
} AR_VIDEO_AVFOUNDATION_FOCUS_PRESET;

///
/// @brief Values returned by arVideoParamGeti(AR_VIDEO_PARAM_AVFOUNDATION_CAMERA_POSITION, ...)
///
typedef enum {
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_UNKNOWN = -1,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_UNSPECIFIED = 0,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_REAR,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_REAR_STEREO_LEFT,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_REAR_STEREO_RIGHT,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT_STEREO_LEFT,
    AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT_STEREO_RIGHT
} AR_VIDEO_AVFOUNDATION_CAMERA_POSITION;

///
/// @brief Values returned by  arVideoParamGeti(AR_VIDEO_PARAM_ANDROID_CAMERA_FACE, ...)
///
typedef enum {
    AR_VIDEO_ANDROID_CAMERA_FACE_REAR = 0,
    AR_VIDEO_ANDROID_CAMERA_FACE_FRONT,
} AR_VIDEO_ANDROID_CAMERA_FACE;

///
/// @brief Values for device position, as encoded in ARVideoSourceInfoT.flags & AR_VIDEO_SOURCE_INFO_POSITION_MASK.
///
#define AR_VIDEO_POSITION_UNKNOWN     0x0000 ///< Camera physical position on device unknown.
#define AR_VIDEO_POSITION_FRONT       0x0008 ///< Camera is on front of device pointing towards user.
#define AR_VIDEO_POSITION_BACK        0x0010 ///< Camera is on back of device pointing away from user.
#define AR_VIDEO_POSITION_LEFT        0x0018 ///< Camera is on left of device pointing to user's left.
#define AR_VIDEO_POSITION_RIGHT       0x0020 ///< Camera is on right of device pointing to user's right.
#define AR_VIDEO_POSITION_TOP         0x0028 ///< Camera is on top of device pointing toward ceiling when device is held upright.
#define AR_VIDEO_POSITION_BOTTOM      0x0030 ///< Camera is on bottom of device pointing towards floor when device is held upright.
#define AR_VIDEO_POSITION_OTHER       0x0038 ///< Camera physical position on device is known but none of the above.

///
/// @brief Values for device stereo mode, as encoded in ARVideoSourceInfoT.flags & AR_VIDEO_SOURCE_INFO_STEREO_MODE_MASK.
///
#define AR_VIDEO_STEREO_MODE_MONO                        0x0000 ///< Device is monoscopic.
#define AR_VIDEO_STEREO_MODE_LEFT                        0x0040 ///< Device is left eye of a stereoscopic pair.
#define AR_VIDEO_STEREO_MODE_RIGHT                       0x0080 ///< Device is right eye of a stereoscopic pair.
#define AR_VIDEO_STEREO_MODE_FRAME_SEQUENTIAL            0x00C0 ///< Device is left and right stereo images in sequential frames.
#define AR_VIDEO_STEREO_MODE_SIDE_BY_SIDE                0x0100 ///< Device is left and right stereo images in a single frame, arranged horizontally with left eye on left.
#define AR_VIDEO_STEREO_MODE_OVER_UNDER                  0x0140 ///< Device is left and right stereo images in a single frame, arranged vertically with left eye on top.
#define AR_VIDEO_STEREO_MODE_HALF_SIDE_BY_SIDE           0x0180 ///< Device is left and right stereo images in a single frame with the frames scaled to half-width, arranged horizontally with left eye on left.
#define AR_VIDEO_STEREO_MODE_OVER_UNDER_HALF_HEIGHT      0x01C0 ///< Device is left and right stereo images in a single frame with the frames scaled to half-height, arranged vertically with left eye on top.
#define AR_VIDEO_STEREO_MODE_ROW_INTERLACED              0x0200 ///< Device is left and right stereo images in a single frame with row interleaving, where pixels in even-numbered rows are sampled from the left eye, and pixels in odd-number rows from the right eye.
#define AR_VIDEO_STEREO_MODE_COLUMN_INTERLACED           0x0240 ///< Device is left and right stereo images in a single frame with column interleaving, where pixels in even-numbered columns are sampled from the left eye, and pixels in odd-number columns from the right eye.
#define AR_VIDEO_STEREO_MODE_ROW_AND_COLUMN_INTERLACED   0x0280 ///< Device is left and right stereo images in a single frame with row and column interleaving, where pixels where the evenness/oddness of the row is the same as the column are sampled from the left eye, and the remaining pixels from the right eye.
#define AR_VIDEO_STEREO_MODE_ANAGLYPH_RG                 0x02C0 ///< Device is left and right stereo images in a single frame, where both eyes are converted to mono and the left eye is carried in the red channel and the right eye in the green channel.
#define AR_VIDEO_STEREO_MODE_ANAGLYPH_RB                 0x0300 ///< Device is left and right stereo images in a single frame, where both eyes are converted to mono and the left eye is carried in the red channel and the right eye in the blue channel.
#define AR_VIDEO_STEREO_MODE_RESERVED0                   0x0340 ///< Reserved for future use.
#define AR_VIDEO_STEREO_MODE_RESERVED1                   0x0380 ///< Reserved for future use.
#define AR_VIDEO_STEREO_MODE_RESERVED2                   0x03C0 ///< Reserved for future use.

///
/// @brief Values for ARVideoSourceInfoT.flags.
///
#define AR_VIDEO_SOURCE_INFO_FLAG_OFFLINE       0x0001      ///< 0 = unknown or not offline, 1 = offline.
#define AR_VIDEO_SOURCE_INFO_FLAG_IN_USE        0x0002      ///< 0 = unknown or not in use, 1 = in use.
#define AR_VIDEO_SOURCE_INFO_FLAG_OPEN_ASYNC    0x0004      ///< 0 = open normally, 1 = open async.
#define AR_VIDEO_SOURCE_INFO_POSITION_MASK      0x0038      ///< compare (value & AR_VIDEO_SOURCE_INFO_POSITION_MASK) against enums.
#define AR_VIDEO_SOURCE_INFO_STEREO_MODE_MASK   0x03C0      ///< compare (value & AR_VIDEO_SOURCE_INFO_STEREO_MODE_MASK) against enums.

///
/// @brief Values describing a video source.
///
typedef struct {
    char *name;             ///< UTF-8 encoded string representing the name of the source, in a form suitable for presentation to an end-user, e.g. in a list of inputs.
    char *model;            ///< UTF-8 encoded string representing the model of the source, where this information is available. May be NULL if model information is not attainable.
    char *UID;              ///< UTF-8 encoded string representing a unique ID for this source, and suitable for passing to arVideoOpen/ar2VideoOpen as a UID in the configuration. May be NULL if sources cannot be uniquely identified.
    uint32_t flags;
    char *open_token;       ///< UTF-8 encoded string containing the token that should be passed (in the space-separated list of tokens to arVideoOpen/ar2VideoOpen, in order to select this source to be opened. Note that this token is only valid so long as the underlying video hardware configuration does not change, so should not be stored between sessions.
} ARVideoSourceInfoT;

///
/// @brief Values describing a list of video sources.
///
typedef struct {
    int count;
    ARVideoSourceInfoT *info;
} ARVideoSourceInfoListT;

typedef void (*AR_VIDEO_FRAME_READY_CALLBACK)(void *);

#ifdef _WIN32
#  ifdef ARVIDEO_STATIC
#    define ARVIDEO_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define ARVIDEO_EXTERN __declspec(dllexport)
#    else
#      define ARVIDEO_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define ARVIDEO_EXTERN
#endif

#include <ARX/ARVideo/videoLuma.h>

typedef struct {
    int module;
    void *moduleParam;
    ARVideoLumaInfo *lumaInfo;
} AR2VideoParamT;

// AR2VideoBufferT is defined in <ARX/AR/ar.h>

ARVIDEO_EXTERN AR_VIDEO_MODULE   arVideoGetDefaultModule(void);

/*!
    @brief Open a video input module.
    @details Opening a video input module selects, connects to, and configures a video source
        for other video operations. Once this call has returned, other APIs can be invoked.
    @oaram config A configuration string, consisting of a series of space-separated configuration
        tokens. While the configuration string options are largely platform- and system-dependent,
        the token "-module=X" where X is a video input module name is always accepted.
        For information on configuration options, see https://github.com/artoolkitx/artoolkitx/wiki/artoolkitX-video-module-configuration-reference
    @see arVideoOpenAsync ar2VideoOpen ar2VideoOpenAsync arVideoClose ar2VideoClose
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoOpen            (const char *config);

/*!
    @brief Open a video input module, invoking a callback once opening is complete.
    @details Opening a video input module selects, connects to, and configures a video source
        for other video operations.
        This variant returns immediately while continuing the opening operation asynchronously,
        invoking a user-supplied callback once opening has completed. The only API permissible to
        call between this function and the invocation of the callback is arVideoGetModule.
    @oaram config A configuration string, consisting of a series of space-separated configuration
        tokens. While the configuration string options are largely platform- and system-dependent,
        the token "-module=X" where X is a video input module name is always accepted.
        For information on configuration options, see https://github.com/artoolkitx/artoolkitx/wiki/artoolkitX-video-module-configuration-reference
    @param callback The callback to invoke once opening is complete. In most cases, the callback
        will be invoked on a different thread, so care must be taken if the caller is sensitive to this.
    @param userdata An arbitrary caller-supplied pointer which will be passed to the callback.
    @see arVideoOpenAsync ar2VideoOpen ar2VideoOpenAsync arVideoClose ar2VideoClose arVideoGetModule
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoOpenAsync       (const char *config, void (*callback)(void *), void *userdata);

ARVIDEO_EXTERN int               arVideoClose           (void);
ARVIDEO_EXTERN int               arVideoDispOption      (void);
ARVIDEO_EXTERN AR_VIDEO_MODULE   arVideoGetModule       (void);
ARVIDEO_EXTERN int               arVideoGetId           (ARUint32 *id0, ARUint32 *id1);
ARVIDEO_EXTERN int               arVideoGetSize         (int *x, int *y);
ARVIDEO_EXTERN int               arVideoGetPixelSize    (void);
ARVIDEO_EXTERN AR_PIXEL_FORMAT   arVideoGetPixelFormat  (void);

/*!
    @brief Get a frame image from the video module.
    @return NULL if no image was available, or a pointer to an AR2VideoBufferT holding the image.
        The returned pointer remains valid until either the next call to arVideoGetImage, or a
        call to arVideoCapStop.
 */
ARVIDEO_EXTERN AR2VideoBufferT  *arVideoGetImage        (void);

/*!
    @brief Start video capture.
    @details Each call to arVideoCapStart must be balanced with a call to arVideoCapStop.
    @see arVideoCapStop
 */
ARVIDEO_EXTERN int               arVideoCapStart        (void);

/*!
    @brief Start video capture with asynchronous notification of new frame arrival.
    @param callback A function to call when a new frame arrives. This function may be
        called anytime until the function arVideoCapStop has been called successfully.
        The callback may occur on a different thread to the calling thread and it is
        up to the user to synchronise the callback with any procedures that must run
        on the main thread, a rendering thread, or other arbitrary thread.
    @param userdata Optional user data pointer which will be passed to the callback as a parameter. May be NULL.
 */
ARVIDEO_EXTERN int               arVideoCapStartAsync   (AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata);


/*!
    @brief Stop video capture.
    @details Each call to arVideoCapStop must match a call to arVideoCapStart.
    @see arVideoCapStart
 */
ARVIDEO_EXTERN int               arVideoCapStop         (void);

/*!
    @brief Get value of an integer parameter from active video module.
    @param paramName Name of parameter to get, as defined in <ARX/ARVideo/video.h>
    @param value Pointer to integer, which will be filled with the value of the parameter.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoGetParami       (int paramName, int *value);

/*!
    @brief Set value of an integer parameter in active video module.
    @param paramName Name of parameter to set, as defined in <ARX/ARVideo/video.h>
    @param value Integer value to set the parameter to.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoSetParami       (int paramName, int  value);

/*!
    @brief Get value of a double-precision floating-point parameter from active video module.
    @param paramName Name of parameter to get, as defined in <ARX/ARVideo/video.h>
    @param value Pointer to double, which will be filled with the value of the parameter.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoGetParamd       (int paramName, double *value);

/*!
    @brief Set value of a double-precision floating-point parameter in active video module.
    @param paramName Name of parameter to set, as defined in <ARX/ARVideo/video.h>
    @param value Double value to set the parameter to.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoSetParamd       (int paramName, double  value);

/*!
    @brief Get value of a string parameter from active video module.
    @param paramName Name of parameter to get, as defined in <ARX/ARVideo/video.h>
    @param value Pointer to pointer, which will be filled with a pointer to a C-string
        (nul-terminated, UTF-8) containing the value of the parameter. The string returned is
        allocated internally, and it is the responsibility of the caller to call free() on the
        returned value.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoGetParams       (const int paramName, char **value);

/*!
    @brief Get value of a string parameter in active video module.
    @param paramName Name of parameter to set, as defined in <ARX/ARVideo/video.h>
    @param value Pointer to C-string (nul-terminated, UTF-8) containing the value to set the parameter to.
    @return -1 in case of error, 0 in case of no error.
 */
ARVIDEO_EXTERN int               arVideoSetParams       (const int paramName, const char  *value);

ARVIDEO_EXTERN int               arVideoSaveParam       (char *filename);
ARVIDEO_EXTERN int               arVideoLoadParam       (char *filename);
ARVIDEO_EXTERN int               arVideoSetBufferSize   (const int width, const int height);
ARVIDEO_EXTERN int               arVideoGetBufferSize   (int *width, int *height);

ARVIDEO_EXTERN int               arVideoGetCParam       (ARParam *cparam);
ARVIDEO_EXTERN int               arVideoGetCParamAsync  (void (*callback)(const ARParam *, void *), void *userdata);

ARVIDEO_EXTERN int               arVideoUtilGetPixelSize(const AR_PIXEL_FORMAT arPixelFormat);
ARVIDEO_EXTERN const char       *arVideoUtilGetPixelFormatName(const AR_PIXEL_FORMAT arPixelFormat);
#if !AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
ARVIDEO_EXTERN int               arVideoSaveImageJPEG(int w, int h, AR_PIXEL_FORMAT pixFormat, ARUint8 *pixels, const char *filename, const int quality /* 0 to 100 */, const int flipV);
#endif // !AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT

typedef enum {
    AR_VIDEO_ASPECT_RATIO_1_1,       ///< 1.0:   "Square".
    AR_VIDEO_ASPECT_RATIO_11_9,      ///< 1.222: Equivalent to well-known sizes 176x144 (QCIF), 352x288 (CIF).
    AR_VIDEO_ASPECT_RATIO_5_4,       ///< 1.25:  Equivalent to well-known sizes 1280x1024 (SXGA), 2560x2048.
    AR_VIDEO_ASPECT_RATIO_4_3,       ///< 1.333: Equivalent to well-known sizes 320x240 (QVGA), 480x360, 640x480 (VGA), 768x576 (576p), 800x600 (SVGA), 960x720, 1024x768 (XGA), 1152x864, 1280x960, 1400x1050, 1600x1200, 2048x1536.
    AR_VIDEO_ASPECT_RATIO_3_2,       ///< 1.5:   Equivalent to well-known sizes 240x160, 480x320, 960x640, 720x480 (480p), 1152x768, 1280x854, 1440x960.
    AR_VIDEO_ASPECT_RATIO_14_9,      ///< 1.556:
    AR_VIDEO_ASPECT_RATIO_8_5,       ///< 1.6:   Equivalent to well-known sizes 320x200, 1280x800, 1440x900, 1680x1050, 1920x1200, 2560x1600.

    AR_VIDEO_ASPECT_RATIO_5_3,       ///< 1.667: Equivalent to well-known sizes 800x480, 1280x768, 1600x960.
    AR_VIDEO_ASPECT_RATIO_16_9,      ///< 1.778: "Widescreen". Equivalent to well-known sizes 1280x720 (720p), 1920x1080 (1080p).
    AR_VIDEO_ASPECT_RATIO_9_5,       ///< 1.8:   Equivalent to well-known sizes 864x480.
    AR_VIDEO_ASPECT_RATIO_17_9,      ///< 1.889: Equivalent to well-known sizes 2040x1080.
    AR_VIDEO_ASPECT_RATIO_21_9,      ///< 2.333: "Ultrawide". Equivalent to well-known sizes 2560x1080, 1280x512.
    AR_VIDEO_ASPECT_RATIO_UNIQUE,    ///< Value not easily representable as a ratio of integers.
    AR_VIDEO_ASPECT_RATIO_INVALID    ///< Either width or height is zero.
} AR_VIDEO_ASPECT_RATIO;

/*!
    @brief Determine the approximate aspect ratio for a given image resolution.
    @details
        A convenience method which makes it easy to determine the approximate aspect ratio
        of an image with the given resolution (expressed in pixel width and height).
        Returns a symbolic constant for the aspect ratio, which makes it easy to determine
        whether two different resolutions have the same aspect ratio. Assumes square pixels.
    @param w Width in pixels
    @param h Height in pixels
    @result If a matching commonly-used aspect ratio can be found, returns symbolic constant for that aspect ratio.
*/
AR_VIDEO_ASPECT_RATIO arVideoUtilFindAspectRatio(int w, int h);

/*!
    @brief Determine the approximate aspect ratio for a given image resolution.
    @details
        A convenience method which makes it easy to determine the approximate aspect ratio
        of an image with the given resolution (expressed in pixel width and height).
        Returns a string for the aspect ratio. Assumes square pixels.
    @param w Width in pixels
    @param h Height in pixels
    @result If a matching commonly-used aspect ratio can be found, returns string name for that aspect ratio. This string must be free'd when finished with.
*/
char *arVideoUtilFindAspectRatioName(int w, int h);

/*!
    @brief   Get the version of artoolkitX with which the arVideo library was built.
    @details
        It is highly recommended that
        any calling program that depends on features in a certain
        artoolkitX version, check at runtime that it is linked to a version
        of artoolkitX that can supply those features. It is NOT sufficient
        to check the artoolkitX SDK header versions, since with artoolkitX implemented
        in dynamically-loaded libraries, there is no guarantee that the
        version of artoolkitX installed on the machine at run-time will be as
        recent as the version of the artoolkitX SDK which the host
        program was compiled against.
    @result
        Returns the full version number of the artoolkitX version corresponding
        to this video library, in binary coded decimal (BCD) format.

        BCD format allows simple tests of version number in the caller
        e.g. if ((arGetVersion(NULL) >> 16) > 0x0272) printf("This release is later than 2.72\n");

        The major version number is encoded in the most-significant byte
        (bits 31-24), the minor version number in the second-most-significant
        byte (bits 23-16), the tiny version number in the third-most-significant
        byte (bits 15-8), and the build version number in the least-significant
        byte (bits 7-0).

        If the returned value is equal to -1, it can be assumed that the actual
        version is in the range 0x04000000 to 0x04040100.
    @since Available in ARToolKit v4.4.2 and later. The underlying
        functionality can also be used when compiling against earlier versions of
        ARToolKit (v4.0 to 4.4.1) with a slightly different syntax: replace
        "arVideoGetVersion()" in your code with "arVideoGetParami(AR_VIDEO_GET_VERSION, NULL)".
 */
#define  arVideoGetVersion() arVideoGetParami(AR_VIDEO_GET_VERSION, NULL)

ARVIDEO_EXTERN ARVideoSourceInfoListT *ar2VideoCreateSourceInfoList(const char *config);

ARVIDEO_EXTERN void              ar2VideoDeleteSourceInfoList(ARVideoSourceInfoListT **p);

/*!
    @brief Open a video input module and return control object.
    @details Opening a video input module selects, connects to, and configures a video source
        for other video operations. Once this call has returned, other APIs can be invoked.
    @oaram config A configuration string, consisting of a series of space-separated configuration
        tokens. While the configuration string options are largely platform- and system-dependent,
        the token "-module=X" where X is a video input module name is always accepted.
        For information on configuration options, see https://github.com/artoolkitx/artoolkitx/wiki/artoolkitX-video-module-configuration-reference
    @see arVideoOpenAsync ar2VideoOpen ar2VideoOpenAsync arVideoClose ar2VideoClose
    @return NULL in case of error, or allocates and returns a pointer to an AR2VideoParamT structure
        if successful. The allocation is dispoed of by ar2VideoClose.
 */
ARVIDEO_EXTERN AR2VideoParamT   *ar2VideoOpen            (const char *config);

/*!
    @brief Open a video input module, and return control object, invoking a callback once opening is complete.
    @details Opening a video input module selects, connects to, and configures a video source
        for other video operations.
        This variant returns immediately while continuing the opening operation asynchronously,
        invoking a user-supplied callback once opening has completed. The only API permissible to
        call between this function and the invocation of the callback is ar2VideoGetModule.
    @oaram config A configuration string, consisting of a series of space-separated configuration
        tokens. While the configuration string options are largely platform- and system-dependent,
        the token "-module=X" where X is a video input module name is always accepted.
        For information on configuration options, see https://github.com/artoolkitx/artoolkitx/wiki/artoolkitX-video-module-configuration-reference
    @param callback The callback to invoke once opening is complete. In most cases, the callback
        will be invoked on a different thread, so care must be taken if the caller is sensitive to this.
    @param userdata An arbitrary pointer which will be passed to the callback.
    @see arVideoOpenAsync ar2VideoOpen ar2VideoOpenAsync arVideoClose ar2VideoClose ar2VideoGetModule
    @return NULL in case of error, or allocates and returns a pointer to an AR2VideoParamT structure
        if successful. The allocation is dispoed of by ar2VideoClose.
 */
ARVIDEO_EXTERN AR2VideoParamT   *ar2VideoOpenAsync       (const char *config, void (*callback)(void *), void *userdata);

ARVIDEO_EXTERN int               ar2VideoClose           (AR2VideoParamT *vid);
ARVIDEO_EXTERN int               ar2VideoDispOption      (AR2VideoParamT *vid);
ARVIDEO_EXTERN AR_VIDEO_MODULE   ar2VideoGetModule       (AR2VideoParamT *vid);
ARVIDEO_EXTERN int               ar2VideoGetId           (AR2VideoParamT *vid, ARUint32 *id0, ARUint32 *id1);
ARVIDEO_EXTERN int               ar2VideoGetSize         (AR2VideoParamT *vid, int *x,int *y);
ARVIDEO_EXTERN int               ar2VideoGetPixelSize    (AR2VideoParamT *vid);
ARVIDEO_EXTERN AR_PIXEL_FORMAT   ar2VideoGetPixelFormat  (AR2VideoParamT *vid);
ARVIDEO_EXTERN AR2VideoBufferT  *ar2VideoGetImage        (AR2VideoParamT *vid);
ARVIDEO_EXTERN int               ar2VideoCapStart        (AR2VideoParamT *vid);
ARVIDEO_EXTERN int               ar2VideoCapStartAsync   (AR2VideoParamT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata);
ARVIDEO_EXTERN int               ar2VideoCapStop         (AR2VideoParamT *vid);
ARVIDEO_EXTERN int               ar2VideoGetParami       (AR2VideoParamT *vid, int paramName, int *value);
ARVIDEO_EXTERN int               ar2VideoSetParami       (AR2VideoParamT *vid, int paramName, int  value);
ARVIDEO_EXTERN int               ar2VideoGetParamd       (AR2VideoParamT *vid, int paramName, double *value);
ARVIDEO_EXTERN int               ar2VideoSetParamd       (AR2VideoParamT *vid, int paramName, double  value);
ARVIDEO_EXTERN int               ar2VideoGetParams       (AR2VideoParamT *vid, const int paramName, char **value);
ARVIDEO_EXTERN int               ar2VideoSetParams       (AR2VideoParamT *vid, const int paramName, const char  *value);
ARVIDEO_EXTERN int               ar2VideoSaveParam       (AR2VideoParamT *vid, char *filename);
ARVIDEO_EXTERN int               ar2VideoLoadParam       (AR2VideoParamT *vid, char *filename);
ARVIDEO_EXTERN int               ar2VideoSetBufferSize   (AR2VideoParamT *vid, const int width, const int height);
ARVIDEO_EXTERN int               ar2VideoGetBufferSize   (AR2VideoParamT *vid, int *width, int *height);
ARVIDEO_EXTERN int               ar2VideoGetCParam       (AR2VideoParamT *vid, ARParam *cparam);
ARVIDEO_EXTERN int               ar2VideoGetCParamAsync  (AR2VideoParamT *vid, void (*callback)(const ARParam *, void *), void *userdata);

ARVIDEO_EXTERN int ar2VideoPushInit(AR2VideoParamT *vid, int width, int height, const char *pixelFormat, int cameraIndex, int cameraPosition);
ARVIDEO_EXTERN int ar2VideoPush(AR2VideoParamT *vid,
                                ARUint8 *buf0p, int buf0Size, int buf0PixelStride, int buf0RowStride,
                                ARUint8 *buf1p, int buf1Size, int buf1PixelStride, int buf1RowStride,
                                ARUint8 *buf2p, int buf2Size, int buf2PixelStride, int buf2RowStride,
                                ARUint8 *buf3p, int buf3Size, int buf3PixelStride, int buf3RowStride);
ARVIDEO_EXTERN int ar2VideoPushFinal(AR2VideoParamT *vid);

#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_H
