/*
 *  videoAndroid.c
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
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoAndroid.h"

#ifdef ARVIDEO_INPUT_ANDROID

#include "videoAndroidPrivate.h"
#include <sys/time.h>
#include <ARX/ARVideo/videoRGBA.h>
#include <ARX/ARUtil/android.h> // PROP_VALUE_MAX
#include "../cparamSearch.h"

#include "camera_utils.h"
#include <unistd.h> // usleep


#undef ARVIDEO_ANDROID_NATIVE

/**
 * Range of Camera Exposure Time:
 *     Camera's capability range have a very long range which may be disturbing
 *     on camera. For this sample purpose, clamp to a range showing visible
 *     video on preview: 100000ns ~ 250000000ns
 */
static const uint64_t kMinExposureTime = static_cast<uint64_t>(1000000);
static const uint64_t kMaxExposureTime = static_cast<uint64_t>(250000000);

static void *openAsyncThread(void *arg);

int ar2VideoDispOptionAndroid(void)
{
    ARPRINT(" -module=Android\n");
    ARPRINT(" -native Select native video access. Now default.\n");
    ARPRINT(" -nonative No longer supported. Prevously, requested legacy Java/JNI interface via arwVideoPush* functions.\n");
    ARPRINT(" -uid=X Choose input with UID X.\n");
    ARPRINT(" -source=N\n");
    ARPRINT("    Acquire video from connected source device with index N (default = 0).\n");
    ARPRINT("    When -position is not default, index is into list of devices at that position.\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    specifies desired width of image.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    specifies desired height of image.\n");
    ARPRINT(" -position=(rear|back|front|external)\n");
    ARPRINT("    choose between rear/back, front-mounted, or external camera (where available).\n");
    ARPRINT("    default value is 'rear', unless -source is specified, .\n");
    ARPRINT(" -format=[0|RGBA].\n");
    ARPRINT("    Specifies the pixel format for output images.\n");
    ARPRINT("    0=use system default. RGBA=output RGBA, including conversion if necessary.\n");
    ARPRINT(" -prefer=(any|exact|closestsameaspect|closestpixelcount|sameaspect|\n");
    ARPRINT("    largestwithmaximum|smallestwithminimum|largest|smallest)\n");
    ARPRINT("    indiciate video stream size preference. Used alongside -width and -height.\n");
    ARPRINT("    default value is 'any'.\n");
    ARPRINT(" -cachedir=/path/to/cparam_cache.db\n");
    ARPRINT("    Specifies the path in which to look for/store camera parameter cache files.\n");
    ARPRINT("    Default is a folder 'cparam_cache' in the current working directory.\n");
    ARPRINT(" -cacheinitdir=/path/to/cparam_cache_init.db\n");
    ARPRINT("    Specifies the path in which to look for/store initial camera parameter cache file.\n");
    ARPRINT("    Default is a folder 'cparam_cache' in the current working directory.\n");
    ARPRINT("\n");

    return 0;
}

static ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListAndroid2(const char *config_in, ACameraManager* cameraMgr)
{
    if (!cameraMgr) {
        return NULL;
    }
    ACameraIdList* cameraIds = nullptr;
    CALL_MGR(getCameraIdList(cameraMgr, &cameraIds));
    if (cameraIds->numCameras <= 0) {
        return NULL;
    }

    ARVideoSourceInfoListT *sil;
    arMallocClear(sil, ARVideoSourceInfoListT, 1);
    sil->count = cameraIds->numCameras;
    arMallocClear(sil->info, ARVideoSourceInfoT, cameraIds->numCameras);
    for (int i = 0; i < cameraIds->numCameras; i++) {

        const char* id = cameraIds->cameraIds[i];
        ACameraMetadata* metadataObj;
        CALL_MGR(getCameraCharacteristics(cameraMgr, id, &metadataObj));

        int32_t count = 0;
        const uint32_t* tags = nullptr;
        ACameraMetadata_getAllTags(metadataObj, &count, &tags);
        for (int tagIdx = 0; tagIdx < count; ++tagIdx) {
            if (ACAMERA_LENS_FACING == tags[tagIdx]) {
                ACameraMetadata_const_entry lensInfo = {
                    0,
                };
                CALL_METADATA(getConstEntry(metadataObj, tags[tagIdx], &lensInfo));

                // Position.
                switch (static_cast<acamera_metadata_enum_android_lens_facing_t>(lensInfo.data.u8[0])) {
                    case ACAMERA_LENS_FACING_BACK:
                        sil->info[i].flags |= AR_VIDEO_POSITION_BACK;
                        break;
                    case ACAMERA_LENS_FACING_FRONT:
                        sil->info[i].flags |= AR_VIDEO_POSITION_FRONT;
                        break;
                    case ACAMERA_LENS_FACING_EXTERNAL:
                    default:
                        sil->info[i].flags |= AR_VIDEO_POSITION_UNKNOWN;
                        break;
                }
                break;
            } // TODO: else if (ACAMERA_? == tags[tagIdx])
        }
        ACameraMetadata_free(metadataObj);

        sil->info[i].flags |= AR_VIDEO_SOURCE_INFO_FLAG_OPEN_ASYNC; // All Android require async opening.
        //sil->info[i].name = ""; // TODO: Name suitable for user presentation.
        //sil->info[i].model = ""; // TODO: Machine-readable model string.
        sil->info[i].UID = strdup(id);
        if (asprintf(&sil->info[i].open_token, "-uid=%s", sil->info[i].UID) < 0) {
            ARLOGperror(NULL);
            sil->info[i].open_token = NULL;
        }

    }

    ACameraManager_deleteCameraIdList(cameraIds);

    return (sil);
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListAndroid(const char *config_in)
{
    ACameraManager* cameraMgr = ACameraManager_create();
    if (!cameraMgr) {
        return NULL;
    }
    ARVideoSourceInfoListT *sil = ar2VideoCreateSourceInfoListAndroid2(config_in, cameraMgr);
    ACameraManager_delete(cameraMgr);
    return sil;
}

static void onDeviceDisconnected(void* ctx, ACameraDevice* dev)
{
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    const char* id = ACameraDevice_getId(dev);
    ARLOGw("device %s is disconnected", id);

    //vid->camAvailable_ = false;
    ACameraDevice_close(vid->cameraDevice_);
    ar2VideoCloseAndroid(vid);
}

static void onDeviceError(void* ctx, ACameraDevice* dev, int err)
{
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    const char* id = ACameraDevice_getId(dev);
    ARLOGi("CameraDevice %s is in error %#x", id, err);
    PrintCameraDeviceError(err);

    switch (err) {
        case ERROR_CAMERA_IN_USE:
            //vid->camAvailable_ = false;
            break;
        case ERROR_CAMERA_SERVICE:
        case ERROR_CAMERA_DEVICE:
        case ERROR_CAMERA_DISABLED:
        case ERROR_MAX_CAMERAS_IN_USE:
            //vid->camAvailable_ = false;
            break;
        default:
            ARLOGi("Unknown Camera Device Error: %#x", err);
    }
}

static void onCameraAvailable(void* ctx, const char* id)
{
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    vid->cameraAvailable_ = true;
}

static void onCameraUnavailable(void* ctx, const char* id)
{
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    vid->cameraAvailable_ = false;
}

static bool getSensorOrientation(AR2VideoParamAndroidT *vid, const char *cameraId, int32_t* facing, int32_t* angle)
{
    if (!vid || !cameraId || !vid->cameraMgr_) {
        return false;
    }

    ACameraMetadata* metadata;
    ACameraMetadata_const_entry face, orientation;
    CALL_MGR(getCameraCharacteristics(vid->cameraMgr_, cameraId, &metadata));
    CALL_METADATA(getConstEntry(metadata, ACAMERA_LENS_FACING, &face));
    CALL_METADATA(getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation));
    ARLOGi("====Current SENSOR_ORIENTATION: %8d", orientation.data.i32[0]);
    ACameraMetadata_free(metadata);

    if (facing) *facing = (int32_t)(face.data.u8[0]);
    if (angle) *angle = orientation.data.i32[0];
    return true;
}

AR2VideoParamAndroidT *ar2VideoOpenAsyncAndroid(const char *config, void (*callback)(void *), void *userdata)
{
    char                     *cacheDir = NULL;
    char                     *cacheInitDir = NULL;
    char                     *csdu = NULL;
    char                     *csat = NULL;
    AR2VideoParamAndroidT    *vid;
    const char               *a;
    char                      line[1024];
    int err_i = 0;
    int i;
    int width = 0, height = 0;
    int convertToRGBA = 0;
    int position = -1;
    int source = -1;
    char *uid = NULL;
    ARVideoSizePreference sizePreference = AR_VIDEO_SIZE_PREFERENCE_ANY;

    bool sizePreferenceExact = false;

    arMallocClear(vid, AR2VideoParamAndroidT, 1);

    a = config;
    if(a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;
            if (strcmp(line, "-module=Android") == 0) {
            } else if (strcmp(line, "-native") == 0) {
            } else if (strcmp(line, "-nonative") == 0) {
                ARLOGw("Ignoring obsolete configuration option '-nonative'. Will use native capture.\n");
            } else if (strncmp(line, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &width) == 0) {
                    ARLOGe("Error: Configuration option '-width=' must be followed by width in integer pixels.\n");
                    err_i = 1;
                }
            } else if (strncmp(line, "-height=", 8) == 0) {
                if (sscanf(&line[8], "%d", &height) == 0) {
                    ARLOGe("Error: Configuration option '-height=' must be followed by height in integer pixels.\n");
                    err_i = 1;
                }
            } else if (strncmp(line, "-prefer=", 8) == 0) {
                if (strcmp(line+8, "any") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_ANY;
                } else if (strcmp(line+8, "exact") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_EXACT;
                } else if (strcmp(line+8, "closestsameaspect") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_CLOSEST_SAME_ASPECT;
                } else if (strcmp(line+8, "closestpixelcount") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_CLOSEST_PIXEL_COUNT;
                } else if (strcmp(line+8, "sameaspect") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_SAME_ASPECT;
                } else if (strcmp(line+8, "largestwithmaximum") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_LARGEST_WITH_MAXIMUM;
                } else if (strcmp(line+8, "smallestwithminimum") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_SMALLEST_WITH_MINIMUM;
                } else if (strcmp(line+8, "largest") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_LARGEST;
                } else if (strcmp(line+8, "smallest") == 0) {
                    sizePreference = AR_VIDEO_SIZE_PREFERENCE_SMALLEST;
                } else {
                    ARLOGw("Unsupported video size preference. Using default.\n");
                }
            } else if (strncmp(line, "-format=", 8) == 0) {
                if (strcmp(line+8, "0") == 0) {
                    convertToRGBA = 0;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(line+8, "RGBA") == 0) {
                    convertToRGBA = 1;
                    ARLOGi("Requesting images in RGBA format.\n");
                } else {
                    ARLOGe("Ignoring unsupported request for conversion to video format '%s'.\n", line+8);
                }
            } else if (strncmp(line, "-position=", 10) == 0) {
                if (strcmp(line+10, "rear") == 0 || strcmp(line+10, "back") == 0) {
                    position = AR_VIDEO_POSITION_BACK;
                } else if (strcmp(line+10, "front") == 0) {
                    position = AR_VIDEO_POSITION_FRONT;
                } else if (strcmp(line+10, "external") == 0) {
                    position = AR_VIDEO_POSITION_UNKNOWN;
                } else {
                    ARLOGw("Unsupported video device position requested. Using default.\n");
                }
            } else if (strncmp(line, "-uid=", 5) == 0) {
                uid = strdup(line+5);
                if (!uid) err_i = 1;
                else ARLOGi("Requesting capture device with UID '%s'.\n", line+5);
            } else if (strncmp(a, "-cachedir=", 10) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 10; // Skip "-cachedir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cachedir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheDir);
                    cacheDir = strdup(line);
                }
            } else if (strncmp(a, "-cacheinitdir=", 14) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 14; // Skip "-cacheinitdir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cacheinitdir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheInitDir);
                    cacheInitDir = strdup(line);
                }
            } else if (strncmp(a, "-csdu=", 6) == 0) {
                // Attempt to read in download URL.
                a += 6; // Skip "-csdu=" characters.
                sscanf(a, "%s", line);
                free(csdu);
                if (!strlen(line)) {
                    csdu = NULL;
                } else {
                    csdu = strdup(line);
                }
            } else if (strncmp(a, "-csat=", 6) == 0) {
                // Attempt to read in authentication token, allowing for quoting of whitespace.
                a += 6; // Skip "-csat=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                free(csat);
                if (!strlen(line)) {
                    csat = NULL;
                } else {
                    csat = strdup(line);
                }
            } else if (strncmp(line, "-source=", 8) == 0) {
                if (sscanf(&line[8], "%d", &source) == 0) err_i = 1;
            } else {
                err_i = 1;
            }

            if (err_i) {
				ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionAndroid();
                goto bail;
			}

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

	// Initialisation required before cparamSearch can be used.
    if (cparamSearchInit(cacheDir ? cacheDir : "cparam_cache", cacheInitDir ? cacheInitDir : "cparam_cache", false, csdu, csat) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };

    // Initial state.
    vid->pixelFormat = AR_PIXEL_FORMAT_INVALID;
    vid->convertToRGBA = convertToRGBA;
    if (!vid->focalLength) vid->focalLength = AR_VIDEO_ANDROID_FOCAL_LENGTH_DEFAULT;
    vid->capturing = false;
    vid->position = AR_VIDEO_POSITION_UNKNOWN;
	vid->callback = callback;
	vid->userdata = userdata;
	vid->cameraCaptureSessionState = ARVideoAndroidCameraCaptureSessionState_CLOSED;

    // In lieu of identifying the actual camera, we use manufacturer/model/board to identify a device,
    // and assume that identical devices have identical cameras.
    // Handset ID, via <sys/system_properties.h>.
    vid->device_id = (char *)calloc(1, PROP_VALUE_MAX*3+2); // From <sys/system_properties.h>. 3 properties plus separators.
    int len;
    len = android_system_property_get(ANDROID_OS_BUILD_MANUFACTURER, vid->device_id); // len = (int)strlen(device_id).
    vid->device_id[len] = '/';
    len++;
    len += android_system_property_get(ANDROID_OS_BUILD_MODEL, vid->device_id + len);
    vid->device_id[len] = '/';
    len++;
    len += android_system_property_get(ANDROID_OS_BUILD_BOARD, vid->device_id + len);

	pthread_mutex_init(&(vid->frameLock), NULL);

    // Native camera init.
    {
        vid->cameraMgr_ = ACameraManager_create();
        if (!vid->cameraMgr_) {
            ARLOGe("ar2VideoOpenAsyncAndroid(): Can't connect to camera manager.\n");
            goto bail1;
        }

        // Select camera.
        // 1. First get list of cams.
        ARVideoSourceInfoListT *sil = ar2VideoCreateSourceInfoListAndroid2(config, vid->cameraMgr_);
        if (!sil || sil->count == 0) {
            ARLOGe("ar2VideoOpenAsyncAndroid(): No camera available.\n");
            ar2VideoDeleteSourceInfoList(&sil);
            goto bail2;
        }
        // 2. If user requested by UID, try to locate it.
        if (uid) {
            for (i = 0; i < sil->count; i++) {
                if (strcmp(uid, sil->info[i].UID) == 0) {
                    break;
                }
            }
            if (i == sil->count) {
                ARLOGw("Unable to find video capture device with UID %s. Ignoring.\n", uid);
                free(uid);
                uid = NULL;
            } else {
                vid->position = sil->info[i].flags & AR_VIDEO_SOURCE_INFO_POSITION_MASK;
                vid->camera_index = i;
            }
        }
        // 3. If no UID, go by position, unless position was default and source not default.
        if (!uid && !(position == -1 && source != -1)) {
            if (position == -1) position = AR_VIDEO_POSITION_BACK;
            int matchPositionIndex = source == -1 ? 0 : source;
            int matchedPositionCount = 0;
            for (i = 0; i < sil->count; i++) {
                if ((sil->info[i].flags & AR_VIDEO_SOURCE_INFO_POSITION_MASK) == position) {
                    if (matchedPositionCount == matchPositionIndex) {
                        uid = strdup(sil->info[i].UID);
                        vid->position = position;
                        vid->camera_index = i;
                        break;
                    } else {
                        matchedPositionCount++;
                    }
                }
            }
        }
        // 4. If no UID or preferred position, just choose device zero-indexed by "-source",
        // or if source > 0 and not enough devices, the first device.
        if (!uid) {
            i = source != -1 && source < sil->count ? source : 0;
            uid = strdup(sil->info[i].UID);
            vid->position = sil->info[i].flags & AR_VIDEO_SOURCE_INFO_POSITION_MASK;
            vid->camera_index = i;
        }
        ar2VideoDeleteSourceInfoList(&sil);

        ACameraDevice_stateCallbacks cameraDeviceListener = {
            .context = vid,
            .onDisconnected = onDeviceDisconnected,
            .onError = onDeviceError,
        };

        // Now open camera.
        CALL_MGR(openCamera(vid->cameraMgr_, uid, &cameraDeviceListener, &vid->cameraDevice_));
        if (!vid->cameraDevice_) {
            ARLOGe("Failed to open camera.");
            goto bail2;
        }

        vid->cameraAvailabilityCallbacks = {
            .context = vid,
            .onCameraAvailable = onCameraAvailable,
            .onCameraUnavailable = onCameraUnavailable,
        };
        CALL_MGR(registerAvailabilityCallback(vid->cameraMgr_, &vid->cameraAvailabilityCallbacks));

        ACameraMetadata* metadata;
        CALL_MGR(getCameraCharacteristics(vid->cameraMgr_, uid, &metadata));
        ACameraMetadata_const_entry entry = {0};

        // Initialize camera controls(exposure time and sensitivity), pick
        // up value of 2% * range + min as starting value (just a number, no magic)
        camera_status_t status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);
        if (status == ACAMERA_OK) {
            vid->exposureRangeMin_ = entry.data.i64[0];
            if (vid->exposureRangeMin_ < kMinExposureTime) {
                vid->exposureRangeMin_ = kMinExposureTime;
            }
            vid->exposureRangeMax_ = entry.data.i64[1];
            if (vid->exposureRangeMax_ > kMaxExposureTime) {
                vid->exposureRangeMax_ = kMaxExposureTime;
            }
            vid->exposureTime_ = vid->exposureRangeMin_ + (int64_t)((vid->exposureRangeMax_ - vid->exposureRangeMin_) * 0.02f);
        } else {
            ARLOGw("Unsupported ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE");
            vid->exposureRangeMin_ = vid->exposureRangeMax_ = 0l;
            vid->exposureTime_ = 0l;
        }
        status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);
        if (status == ACAMERA_OK) {
            vid->sensitivityRangeMin_ = entry.data.i32[0];
            vid->sensitivityRangeMax_ = entry.data.i32[1];
            vid->sensitivity_ = vid->sensitivityRangeMin_ + (int64_t)((vid->sensitivityRangeMax_ - vid->sensitivityRangeMin_) * 0.02f);
        } else {
            ARLOGw("Unsupported ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE");
            vid->sensitivityRangeMin_ = vid->sensitivityRangeMax_ = 0;
            vid->sensitivity_ = 0;
        }

        // At the moment, just using NV21 for YUV_420_888.
        vid->pixelFormat = AR_PIXEL_FORMAT_NV21; // NV21 and also used for YUV_420_888.

        // Query for available capture resolutions and find one preferred by user.
        entry = {0};
        CALL_METADATA(getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry)); // format of the data: format, width, height, input?, type int32

        int area = width * height;
        float closestAreaRatio = FLT_MAX;
        int largestArea = 0;
        int smallestArea = INT_MAX;
        AR_VIDEO_ASPECT_RATIO aspect = arVideoUtilFindAspectRatio(width, height);
        if ((!width || !height) && sizePreference != AR_VIDEO_SIZE_PREFERENCE_ANY && sizePreference != AR_VIDEO_SIZE_PREFERENCE_LARGEST && sizePreference != AR_VIDEO_SIZE_PREFERENCE_SMALLEST) {
            ARLOGw("Warning: Video frame size preference needs width and height. Reverting to any size.\n");
            sizePreference = AR_VIDEO_SIZE_PREFERENCE_ANY;
        }
        bool done = false;
        for (int i = 0; !done && i < entry.count; i += 4) {
            int32_t input = entry.data.i32[i + 3];
            int32_t format = entry.data.i32[i + 0];
            if (input) continue;
            if (format == AIMAGE_FORMAT_YUV_420_888 /*|| format == AIMAGE_FORMAT_JPEG*/) {
                int32_t formatWidth = entry.data.i32[i + 1];
                int32_t formatHeight = entry.data.i32[i + 2];
                int formatArea = formatWidth * formatHeight;
                switch (sizePreference) {
                    case AR_VIDEO_SIZE_PREFERENCE_ANY:
                        vid->width = formatWidth;
                        vid->height = formatHeight;
                        done = true;
                        break;
                    case AR_VIDEO_SIZE_PREFERENCE_EXACT:
                        if (formatWidth == width && formatHeight == height) {
                            vid->width = formatWidth;
                            vid->height = formatHeight;
                            done = true;
                        }
                        break;
                    case AR_VIDEO_SIZE_PREFERENCE_LARGEST_WITH_MAXIMUM:
                        if (formatWidth > width || formatHeight > height) continue;
                        // N.B.: fall through.
                    case AR_VIDEO_SIZE_PREFERENCE_LARGEST:
                        if (formatArea > largestArea) {
                            vid->width = formatWidth;
                            vid->height = formatHeight;
                            largestArea = formatArea;
                        }
                        break;
                    case AR_VIDEO_SIZE_PREFERENCE_SMALLEST_WITH_MINIMUM:
                        if (formatWidth < width || formatHeight < height) continue;
                        // N.B.: fall through.
                    case AR_VIDEO_SIZE_PREFERENCE_SMALLEST:
                        if (formatArea < smallestArea) {
                            vid->width = formatWidth;
                            vid->height = formatHeight;
                            smallestArea = formatArea;
                        }
                        break;
                    case AR_VIDEO_SIZE_PREFERENCE_SAME_ASPECT:
                    case AR_VIDEO_SIZE_PREFERENCE_CLOSEST_SAME_ASPECT:
                        {
                            AR_VIDEO_ASPECT_RATIO formatAspect = arVideoUtilFindAspectRatio(formatWidth, formatHeight);
                            if ((aspect != AR_VIDEO_ASPECT_RATIO_UNIQUE && aspect != formatAspect) || (aspect == AR_VIDEO_ASPECT_RATIO_UNIQUE && (width != formatWidth || height != formatHeight))) continue;
                            if (sizePreference == AR_VIDEO_SIZE_PREFERENCE_SAME_ASPECT) {
                                vid->width = formatWidth;
                                vid->height = formatHeight;
                                done = true;
                                break;
                            }
                        }
                        // N.B.: fall through as passed aspect ratio test.
                    case AR_VIDEO_SIZE_PREFERENCE_CLOSEST_PIXEL_COUNT:
                        {
                            float areaRatio = formatArea > area ? formatArea / area : area / formatArea;
                            if (areaRatio < closestAreaRatio) {
                                vid->width = formatWidth;
                                vid->height = formatHeight;
                                closestAreaRatio = areaRatio;
                                if (closestAreaRatio == 1.0f) done = true;
                            }
                        }
                        break;
                } // switch (sizePreference)
            } // format == AIMAGE_FORMAT_YUV_420_888
        }
        if (!vid->width || !vid->height) {
            // Size selection failed. Try VGA.
            vid->width = 640;
            vid->height = 480;
            ARLOGw("Unable to find video size matching request. Trying 640x480.");
        } else {
            ARLOGi("Found video size matching criteria of %dx%d.", vid->width, vid->height);
        }

        // Query for sensor rotation in degrees in range [0, 360).
        entry = {0};
        CALL_METADATA(getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &entry));
        vid->sensorRotation_ = (entry.data.i32[0] + 360) % 360;
        // If we want to take into account device orientation:
        //int32_t imageRotation;
        //if (vid->position == ARVIDEO_POSITION_FRONT) {
        //    imageRotation = (vid->sensorRotation_ + vid->deviceRotation_) % 360;
        //    imageRotation = (360 - imageRotation) % 360;
        //} else {
        //    imageRotation = (vid->sensorRotation_ - vid->deviceRotation_ + 360) % 360;
        //}
        //ARLOGi("Device rotation: %d, present rotation angle: %d", vid->deviceRotation_, imageRotation);

        // Done with metadata.
        ACameraMetadata_free(metadata);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1); // Preclude the need to do pthread_join on the thread after it exits.
    pthread_t t;
    err_i = pthread_create(&t, &attr, openAsyncThread, vid);
    pthread_attr_destroy(&attr);
    if (err_i != 0) {
        ARLOGe("ar2VideoOpenAsyncAndroid(): pthread_create error %d.\n");
        goto bail2;
    }

    goto done;

bail2:
    ACameraManager_delete(vid->cameraMgr_);
bail1:
    pthread_mutex_destroy(&vid->frameLock);
    free(vid->device_id);
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
bail:
    free(vid);
    vid = NULL;

done:
    free(cacheDir);
    free(cacheInitDir);
    free(csdu);
    free(csat);
    free(uid);
    return (vid);
}

// Just need to ensure callback happens on separate thread.
static void *openAsyncThread(void *arg)
{
    int err;

    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)arg; // Cast the thread start arg to the correct type.

    usleep(1000); // Wait 1 millisecond.
    (vid->callback)(vid->userdata);

    return (NULL);
}

int ar2VideoCloseAndroid(AR2VideoParamAndroidT *vid)
{
    if (!vid) return (-1); // Sanity check.

    if (vid->capturing) ar2VideoCapStopAndroid(vid);

    if (vid->cameraDevice_) {
        CALL_DEV(close(vid->cameraDevice_));
        vid->cameraDevice_ = nullptr;
    }
    if (vid->cameraMgr_) {
        CALL_MGR(unregisterAvailabilityCallback(vid->cameraMgr_, &vid->cameraAvailabilityCallbacks));
        vid->cameraAvailabilityCallbacks = {0};
        ACameraManager_delete(vid->cameraMgr_);
        vid->cameraMgr_ = nullptr;
    }

	pthread_mutex_destroy(&(vid->frameLock));
    free(vid->device_id);
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }

    free(vid);

    return 0;
}

int ar2VideoGetIdAndroid(AR2VideoParamAndroidT *vid, ARUint32 *id0, ARUint32 *id1)
{
    return -1;
}

int ar2VideoGetSizeAndroid(AR2VideoParamAndroidT *vid, int *x, int *y)
{
    if (!vid) return -1; // Sanity check.

    if (x) *x = vid->width;
    if (y) *y = vid->height;

    return 0;
}

static void onImageCallback(void* ctx, AImageReader* reader)
{
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    AImage *image = nullptr;
    media_status_t status = AImageReader_acquireLatestImage(reader, &image);
    if (status != AMEDIA_OK) {
        ARLOGe("Unable to acquire latest image.");
        goto done;
    }

    pthread_mutex_lock(&(vid->frameLock));
    if (vid->imageReady) {
        AImage_delete(vid->imageReady);
    }
    vid->imageReady = image;
done:
    pthread_mutex_unlock(&(vid->frameLock));
}

AR2VideoBufferT *ar2VideoGetImageAndroid(AR2VideoParamAndroidT *vid)
{
    AR2VideoBufferT *ret = NULL;

    if (!vid || !vid->capturing) return NULL; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));

    if (vid->imageReady) {
        if (vid->imageCheckedOutDownstream) {
            AImage_delete(vid->imageCheckedOutDownstream);
        }

        // Get the image planes.

        // YUV_420_888 as NV21 format.
        uint8_t *data0 = nullptr;
        int len0 = 0;
        // Y plane first. Note that YUV_420_888 guarantees pixel stride == 1.
        AImage_getPlaneData(vid->imageReady, 0, &data0, &len0);
        vid->buffer.bufPlanes[0] = vid->buffer.buffLuma = (ARUint8 *)data0;

        // Next, U (Cb) and V (Cr) planes.
        uint8_t *data1 = nullptr;
        int len1 = 0;
        uint8_t *data2 = nullptr;
        int len2 = 0;
        AImage_getPlaneData(vid->imageReady, 1, &data1, &len1);
        AImage_getPlaneData(vid->imageReady, 2, &data2, &len2);

        // U and V planes both have pixelstride of 2, rowstride of pixelstride * vid->width/2, and are interleaved by 1 byte, so it's already NV21 and we can do a direct copy.
        vid->buffer.bufPlanes[1] = (ARUint8 *)data2;

        int64_t timestampNs;
        media_status_t status = AImage_getTimestamp(vid->imageReady, &timestampNs);
        uint64_t timestampUs = (uint64_t)timestampNs / 1000;
        vid->buffer.time.sec  = timestampUs / 1000000u;
        vid->buffer.time.usec = (uint32_t)(timestampUs % 1000000u);

        // Convert if the user requested RGBA.
        if (vid->convertToRGBA) {
            videoRGBA((uint32_t *)&vid->buffer.buff, &(vid->buffer), vid->width, vid->height, vid->pixelFormat);
        } else {
            vid->buffer.buff = vid->buffer.bufPlanes[0];
        }

        vid->buffer.fillFlag = 1;
        ret = &vid->buffer;

        vid->imageCheckedOutDownstream = vid->imageReady;
        vid->imageReady = NULL;
    }

    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

// CaptureSession state callbacks
static void onSessionClosed(void* ctx, ACameraCaptureSession* ses)
{
    ARLOGi("session %p closed", ses);
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    vid->cameraCaptureSessionState = ARVideoAndroidCameraCaptureSessionState_CLOSED;
}

static void onSessionReady(void* ctx, ACameraCaptureSession* ses)
{
    ARLOGi("session %p ready", ses);
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    vid->cameraCaptureSessionState = ARVideoAndroidCameraCaptureSessionState_READY;
}

static void onSessionActive(void* ctx, ACameraCaptureSession* ses)
{
    ARLOGi("session %p active", ses);
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)ctx;
    vid->cameraCaptureSessionState = ARVideoAndroidCameraCaptureSessionState_ACTIVE;
}

int ar2VideoCapStartAndroid(AR2VideoParamAndroidT *vid)
{
    int ret = -1;

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    // Check if already capturing.
    if (!vid->capturing) {
        vid->capturing = true;

        media_status_t status;
        status = AImageReader_new(vid->width, vid->height, AIMAGE_FORMAT_YUV_420_888, 4, &vid->imageReader_); // Max 4 buffers: one checked out downstream, one filled, two available to fill.
        if (!vid->imageReader_ || status != AMEDIA_OK) {
            ARLOGe("ar2VideoCapStartAndroid: Failed to create AImageReader.");
            goto done;
        }
        // Set up buffers for YUV formats.
        if (vid->pixelFormat != AR_PIXEL_FORMAT_NV21 && vid->pixelFormat != AR_PIXEL_FORMAT_420f) { // 420f == NV12.
            ARLOGe("ar2VideoCapStartAndroid: Can't use pixel format %s.", arUtilGetPixelFormatName(vid->pixelFormat));
            goto done;
        }
        vid->buffer.bufPlaneCount = 2;
        vid->buffer.bufPlanes = (ARUint8 **)calloc(vid->buffer.bufPlaneCount, sizeof(ARUint8 *));
        if (vid->convertToRGBA) {
            vid->buffer.buff = (ARUint8 *)malloc(vid->width * vid->height * 4);
        }

        AImageReader_ImageListener listener {
            .context = vid,
            .onImageAvailable = onImageCallback,
        };
        AImageReader_setImageListener(vid->imageReader_, &listener);

        //
        // Start capture session.
        //
        CALL_CONTAINER(create(&vid->captureSessionOutputContainer_));

        ANativeWindow *nativeWindow;
        status = AImageReader_getWindow(vid->imageReader_, &vid->imageReaderNativeWindow_);
        ASSERT(status == AMEDIA_OK, "Could not get ANativeWindow");
        ANativeWindow_acquire(vid->imageReaderNativeWindow_); // Increment reference count.

        CALL_OUTPUT(create(vid->imageReaderNativeWindow_, &vid->captureSessionOutput_));
        CALL_CONTAINER(add(vid->captureSessionOutputContainer_, vid->captureSessionOutput_));
        CALL_TARGET(create(vid->imageReaderNativeWindow_, &vid->captureRequestTarget_));
        CALL_DEV(createCaptureRequest(vid->cameraDevice_, TEMPLATE_PREVIEW, &vid->captureRequest_));
        CALL_REQUEST(addTarget(vid->captureRequest_, vid->captureRequestTarget_));

        vid->cameraCaptureSessionState = ARVideoAndroidCameraCaptureSessionState_READY;
        ACameraCaptureSession_stateCallbacks sessionListener = {
            .context = vid,
            .onClosed = onSessionClosed,
            .onReady = onSessionReady,
            .onActive = onSessionActive,
        };
        CALL_DEV(createCaptureSession(vid->cameraDevice_, vid->captureSessionOutputContainer_, &sessionListener, &vid->captureSession_));
        CALL_SESSION(setRepeatingRequest(vid->captureSession_, nullptr, 1, &vid->captureRequest_, nullptr)); // ACameraCaptureSession_captureCallbacks *callbacks = nullptr, int requests = 1, int *captureSequenceId

        ret = 0;
    }
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

int ar2VideoCapStopAndroid(AR2VideoParamAndroidT *vid)
{
    int ret = -1;

    if (!vid) return -1; // Sanity check.

    pthread_mutex_lock(&(vid->frameLock));
    if (!vid->capturing) goto done; // Not capturing.
    vid->capturing = false;

    if (vid->cameraCaptureSessionState == ARVideoAndroidCameraCaptureSessionState_ACTIVE) {
        ACameraCaptureSession_stopRepeating(vid->captureSession_);
    }
    ACameraCaptureSession_close(vid->captureSession_);

    CALL_REQUEST(removeTarget(vid->captureRequest_, vid->captureRequestTarget_));
    ACaptureRequest_free(vid->captureRequest_);
    ACameraOutputTarget_free(vid->captureRequestTarget_);
    CALL_CONTAINER(remove(vid->captureSessionOutputContainer_, vid->captureSessionOutput_));
    ACaptureSessionOutput_free(vid->captureSessionOutput_);

    ANativeWindow_release(vid->imageReaderNativeWindow_);
    ACaptureSessionOutputContainer_free(vid->captureSessionOutputContainer_);

    if (vid->imageReady) {
        AImage_delete(vid->imageReady);
        vid->imageReady = NULL;
    }
    if (vid->imageCheckedOutDownstream) {
        AImage_delete(vid->imageCheckedOutDownstream);
        vid->imageCheckedOutDownstream = NULL;
    }
    AImageReader_delete(vid->imageReader_);
    vid->imageReader_ = NULL;

    free(vid->buffer.bufPlanes);
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    if (vid->convertToRGBA) {
        free(vid->buffer.buff);
    }
    vid->buffer.buff = vid->buffer.buffLuma = NULL;

    ret = 0;
done:
    pthread_mutex_unlock(&(vid->frameLock));
    return (ret);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatAndroid(AR2VideoParamAndroidT *vid)
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID; // Sanity check.

    if (vid->convertToRGBA) {
        return (AR_PIXEL_FORMAT_RGBA);
    } else {
        return vid->pixelFormat;
    }
}

int ar2VideoGetParamiAndroid(AR2VideoParamAndroidT *vid, int paramName, int *value)
{
    if (!vid || !value) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX:
            *value = vid->camera_index;
            break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_FACE:
            *value = (vid->position == AR_VIDEO_POSITION_FRONT ? 1 : 0);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiAndroid(AR2VideoParamAndroidT *vid, int paramName, int  value)
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_INTERNET_STATE: return (cparamSearchSetInternetState(value)); break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdAndroid(AR2VideoParamAndroidT *vid, int paramName, double *value)
{
    if (!vid || !value) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH:
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:
            *value = (double)vid->focalLength;
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdAndroid(AR2VideoParamAndroidT *vid, int paramName, double  value)
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_CAMERA_FOCAL_LENGTH:
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:
            vid->focalLength = (float)value;
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsAndroid(AR2VideoParamAndroidT *vid, const int paramName, char **value)
{
    if (!vid || !value) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            *value = strdup(vid->device_id);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsAndroid(AR2VideoParamAndroidT *vid, const int paramName, const char  *value)
{
    if (!vid) return (-1); // Sanity check.

    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            free (vid->device_id);
            if (value) vid->device_id = strdup(value);
            else vid->device_id = NULL;
            break;
        default:
            return (-1);
    }
    return (0);
}

static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)userdata;
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
    if (final) {
        vid->cparamSearchCallback = (void (*)(const ARParam *, void *))nullptr;
        vid->cparamSearchUserdata = nullptr;
    }
}

int ar2VideoGetCParamAsyncAndroid(AR2VideoParamAndroidT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1); // Sanity check.
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }

    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;

    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, vid->camera_index, vid->width, vid->height, vid->focalLength, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = (void (*)(const ARParam *, void *))nullptr;
        vid->cparamSearchUserdata = nullptr;
        return (-1);
    }

    return (0);
}

#endif // ARVIDEO_INPUT_ANDROID
