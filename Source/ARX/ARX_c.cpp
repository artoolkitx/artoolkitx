/*
 *  ARX_c.cpp
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb, Julian Looser.
 *
 */

// ----------------------------------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------------------------------

#include <ARX/ARX_c.h>
#include <ARX/ARController.h>
#ifdef DEBUG
#  ifdef _WIN32
#    define MAXPATHLEN MAX_PATH
#    include <direct.h>               // _getcwd()
#    define getcwd _getcwd
#  else
#    include <unistd.h>
#    include <sys/param.h>
#  endif
#endif
#include <stdio.h>
#if !ARX_TARGET_PLATFORM_WINDOWS && !ARX_TARGET_PLATFORM_WINRT
#  include <pthread.h>
#endif

// ----------------------------------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(NAN)
#  define __nan_bytes { 0, 0, 0xc0, 0x7f }
static union { unsigned char __c[4]; float __d; } __nan_union = { __nan_bytes };
#  define NAN (__nan_union.__d)
#endif

// ----------------------------------------------------------------------------------------------------

static ARController *gARTK = NULL;

// ----------------------------------------------------------------------------------------------------

void arwRegisterLogCallback(PFN_LOGCALLBACK callback)
{
    arLogSetLogger(callback, 1); // 1 -> only callback on same thread, as required e.g. by C# interop.
}

void arwSetLogLevel(const int logLevel)
{
    if (logLevel >= 0) {
        arLogLevel = logLevel;
    }
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  artoolkitX lifecycle functions
// ----------------------------------------------------------------------------------------------------

bool arwInitialiseAR()
{
    if (!gARTK) gARTK = new ARController;
	return gARTK->initialiseBase();
}

bool arwGetARToolKitVersion(char *buffer, int length)
{
	if (!buffer) return false;
    if (!gARTK) return false;
    
	if (const char *version = gARTK->getARToolKitVersion()) {
		strncpy(buffer, version, length - 1); buffer[length - 1] = '\0';
		return true;
	}
	return false;
}

int arwGetError()
{
    if (!gARTK) return ARX_ERROR_NONE;
    return gARTK->getError();
}

bool arwChangeToResourcesDir(const char *resourcesDirectoryPath)
{
    bool ok;
#if ARX_TARGET_PLATFORM_ANDROID
    if (resourcesDirectoryPath) ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH, resourcesDirectoryPath, NULL) == 0);
	else ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL, NULL) == 0);
#elif ARX_TARGET_PLATFORM_WINRT
	ok = false; // No current working directory in WinRT.
#else
    if (resourcesDirectoryPath) ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH, resourcesDirectoryPath) == 0);
	else ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL) == 0);
#endif
#ifdef DEBUG
    char buf[MAXPATHLEN];
    ARLOGd("cwd is '%s'.\n", getcwd(buf, sizeof(buf)));
#endif
    return (ok);
}

bool arwStartRunning(const char *vconf, const char *cparaName)
{
    if (!gARTK) return false;
	return gARTK->startRunning(vconf, cparaName, NULL, 0);
}

bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen)
{
    if (!gARTK) return false;
	return gARTK->startRunning(vconf, NULL, cparaBuff, cparaBuffLen);
}

bool arwStartRunningStereo(const char *vconfL, const char *cparaNameL, const char *vconfR, const char *cparaNameR, const char *transL2RName)
{
    if (!gARTK) return false;
	return gARTK->startRunningStereo(vconfL, cparaNameL, NULL, 0L, vconfR, cparaNameR, NULL, 0L, transL2RName, NULL, 0L);
}

bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, const int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen)
{
    if (!gARTK) return false;
	return gARTK->startRunningStereo(vconfL, NULL, cparaBuffL, cparaBuffLenL, vconfR, NULL, cparaBuffR, cparaBuffLenR, NULL, transL2RBuff, transL2RBuffLen);
}

bool arwIsRunning()
{
    if (!gARTK) return false;
	return gARTK->isRunning();
}

bool arwIsInited()
{
    if (!gARTK) return false;
	return gARTK->isInited();
}

bool arwStopRunning()
{
    if (!gARTK) return false;
	return gARTK->stopRunning();
}

bool arwShutdownAR()
{
    if (gARTK) {
        delete gARTK; // Delete the artoolkitX instance to initiate shutdown.
        gARTK = NULL;
    }

    return (true);
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream management
// -------------------------------------------------------------------------------------------------


bool arwGetProjectionMatrix(const float nearPlane, const float farPlane, float p[16])
{
    if (!gARTK) return false;

#ifdef ARDOUBLE_IS_FLOAT
    return gARTK->projectionMatrix(0, nearPlane, farPlane, p);
#else
    ARdouble p0[16];
    if (!gARTK->projectionMatrix(0, nearPlane, farPlane, p0)) {
        return false;
    }
    for (int i = 0; i < 16; i++) p[i] = (float)p0[i];
    return true;
#endif
}

bool arwGetProjectionMatrixStereo(const float nearPlane, const float farPlane, float pL[16], float pR[16])
{
    if (!gARTK) return false;

#ifdef ARDOUBLE_IS_FLOAT
    return (gARTK->projectionMatrix(0, nearPlane, farPlane, pL) && gARTK->projectionMatrix(1, nearPlane, farPlane, pR));
#else
    ARdouble p0L[16];
    ARdouble p0R[16];
    if (!gARTK->projectionMatrix(0, nearPlane, farPlane, p0L) || !gARTK->projectionMatrix(1, nearPlane, farPlane, p0R)) {
        return false;
    }
    for (int i = 0; i < 16; i++) pL[i] = (float)p0L[i];
    for (int i = 0; i < 16; i++) pR[i] = (float)p0R[i];
    return true;
#endif
}


bool arwGetVideoParams(int *width, int *height, int *pixelSize, char *pixelFormatStringBuffer, int pixelFormatStringBufferLen)
{
    AR_PIXEL_FORMAT pf;
    
    if (!gARTK) return false;
	if (!gARTK->videoParameters(0, width, height, &pf)) return false;
    if (pixelSize) *pixelSize = arUtilGetPixelSize(pf);
    if (pixelFormatStringBuffer && pixelFormatStringBufferLen > 0) {
        strncpy(pixelFormatStringBuffer, arUtilGetPixelFormatName(pf), pixelFormatStringBufferLen);
        pixelFormatStringBuffer[pixelFormatStringBufferLen - 1] = '\0'; // guarantee nul termination.
    }
    return true;
}

bool arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, char *pixelFormatStringBufferL, int pixelFormatStringBufferLenL, int *widthR, int *heightR, int *pixelSizeR, char *pixelFormatStringBufferR, int pixelFormatStringBufferLenR)
{
    AR_PIXEL_FORMAT pfL, pfR;
    
    if (!gARTK) return false;
	if (!gARTK->videoParameters(0, widthL, heightL, &pfL)) return false;
	if (!gARTK->videoParameters(1, widthR, heightR, &pfR)) return false;
    if (pixelSizeL) *pixelSizeL = arUtilGetPixelSize(pfL);
    if (pixelSizeR) *pixelSizeR = arUtilGetPixelSize(pfR);
    if (pixelFormatStringBufferL && pixelFormatStringBufferLenL > 0) {
        strncpy(pixelFormatStringBufferL, arUtilGetPixelFormatName(pfL), pixelFormatStringBufferLenL);
        pixelFormatStringBufferL[pixelFormatStringBufferLenL - 1] = '\0'; // guarantee nul termination.
    }
    if (pixelFormatStringBufferR && pixelFormatStringBufferLenR > 0) {
        strncpy(pixelFormatStringBufferR, arUtilGetPixelFormatName(pfR), pixelFormatStringBufferLenR);
        pixelFormatStringBufferR[pixelFormatStringBufferLenR - 1] = '\0'; // guarantee nul termination.
    }
    return true;
}

bool arwCapture()
{
    if (!gARTK) return false;
    return (gARTK->capture());
}

bool arwUpdateAR()
{
    if (!gARTK) return false;
    return gARTK->update();
}

bool arwUpdateTexture32(uint32_t *buffer)
{
    if (!gARTK) return false;
    return gARTK->updateTextureRGBA32(0, buffer);
}

bool arwUpdateTexture32Stereo(uint32_t *bufferL, uint32_t *bufferR)
{
    if (!gARTK) return false;
    return (gARTK->updateTextureRGBA32(0, bufferL) && gARTK->updateTextureRGBA32(1, bufferR));
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream drawing.
// ----------------------------------------------------------------------------------------------------
bool arwDrawVideoInit(const int videoSourceIndex)
{
    if (!gARTK) return false;
    
    return (gARTK->drawVideoInit(videoSourceIndex));
}

bool arwDrawVideoSettings(int videoSourceIndex, int width, int height, bool rotate90, bool flipH, bool flipV, int hAlign, int vAlign, int scalingMode, int32_t viewport[4])
{
    if (!gARTK)return false;
    
    return (gARTK->drawVideoSettings(videoSourceIndex, width, height, rotate90, flipH, flipV, (ARVideoView::HorizontalAlignment)hAlign, (ARVideoView::VerticalAlignment)vAlign, (ARVideoView::ScalingMode)scalingMode, viewport));
}

bool arwDrawVideo(const int videoSourceIndex)
{
    if (!gARTK)return false;
    
    return (gARTK->drawVideo(videoSourceIndex));
}

bool arwDrawVideoFinal(const int videoSourceIndex)
{
    if (!gARTK) return false;
    
    return (gARTK->drawVideoFinal(videoSourceIndex));
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Tracking configuration
// ----------------------------------------------------------------------------------------------------

void arwSetTrackerOptionBool(int option, bool value)
{
    if (!gARTK) return;
    
    if (option == ARW_TRACKER_OPTION_NFT_MULTIMODE) {
#if HAVE_NFT
        gARTK->getNFTTracker()->setNFTMultiMode(value);
#else
        return;
#endif
    } else if (option == ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE) {
        gARTK->getSquareTracker()->setDebugMode(value);
    }
}

void arwSetTrackerOptionInt(int option, int value)
{
    if (!gARTK) return;
    
    if (option == ARW_TRACKER_OPTION_SQUARE_THRESHOLD) {
        if (value < 0 || value > 255) return;
        gARTK->getSquareTracker()->setThreshold(value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_THRESHOLD_MODE) {
        gARTK->getSquareTracker()->setThresholdMode((AR_LABELING_THRESH_MODE)value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_LABELING_MODE) {
        gARTK->getSquareTracker()->setLabelingMode(value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_DETECTION_MODE) {
        gARTK->getSquareTracker()->setPatternDetectionMode(value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_MATRIX_CODE_TYPE) {
        gARTK->getSquareTracker()->setMatrixCodeType((AR_MATRIX_CODE_TYPE)value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_IMAGE_PROC_MODE) {
        gARTK->getSquareTracker()->setImageProcMode(value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE) {
        gARTK->getSquareTracker()->setPatternSize(value);
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX) {
        gARTK->getSquareTracker()->setPatternCountMax(value);
    } else if (option == ARW_TRACKER_OPTION_2D_TRACKER_FEATURE_TYPE) {
#if HAVE_2D
        if (value < 0 || value > 3) return;
        gARTK->get2dTracker()->setDetectorType(value);
#else
        return;
#endif
    }
}

void arwSetTrackerOptionFloat(int option, float value)
{
    if (!gARTK) return;
    
    if (option == ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE) {
        if (value <= 0.0f || value >= 0.5f) return;
        gARTK->getSquareTracker()->setPattRatio(1.0f - 2.0f*value); // Convert from border size to pattern ratio.
    }
}

bool arwGetTrackerOptionBool(int option)
{
    if (!gARTK) return false;
    
    if (option == ARW_TRACKER_OPTION_NFT_MULTIMODE) {
#if HAVE_NFT
        return  gARTK->getNFTTracker()->NFTMultiMode();
#else
        return false;
#endif
    } else if (option == ARW_TRACKER_OPTION_SQUARE_DEBUG_MODE) {
        return gARTK->getSquareTracker()->debugMode();
    }
    return false;
}

int arwGetTrackerOptionInt(int option)
{
    if (!gARTK) return (INT_MAX);
    
    if (option == ARW_TRACKER_OPTION_SQUARE_THRESHOLD) {
        return gARTK->getSquareTracker()->threshold();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_THRESHOLD_MODE) {
        return gARTK->getSquareTracker()->thresholdMode();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_LABELING_MODE) {
        return (int)gARTK->getSquareTracker()->labelingMode();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_DETECTION_MODE) {
        return gARTK->getSquareTracker()->patternDetectionMode();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_MATRIX_CODE_TYPE) {
        return (int)gARTK->getSquareTracker()->matrixCodeType();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_IMAGE_PROC_MODE) {
        return gARTK->getSquareTracker()->imageProcMode();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_SIZE) {
        return gARTK->getSquareTracker()->patternSize();
    } else if (option == ARW_TRACKER_OPTION_SQUARE_PATTERN_COUNT_MAX) {
        return gARTK->getSquareTracker()->patternCountMax();
    }
    return (INT_MAX);
}

float arwGetTrackerOptionFloat(int option)
{
    if (!gARTK) return (NAN);
    
    if (option == ARW_TRACKER_OPTION_SQUARE_BORDER_SIZE) {
        float value = gARTK->getSquareTracker()->pattRatio();
        if (value > 0.0f && value < 1.0f) return (1.0f - value)/2.0f; // Convert from pattern ratio to border size.
    }
    return (NAN);
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Trackable management
// ---------------------------------------------------------------------------------------------

int arwAddTrackable(const char *cfg)
{
    if (!gARTK) return -1;
	return gARTK->addTrackable(cfg);
}

bool arwGetTrackables(int *count_p, ARWTrackableStatus **statuses_p)
{
    if (!gARTK) return false;
    if (!count_p) return false;
    
    unsigned int trackableCount = gARTK->countTrackables();
    *count_p = (int)trackableCount;
    if (statuses_p) {
        if (trackableCount == 0) *statuses_p = NULL;
        else {
            ARWTrackableStatus *st = (ARWTrackableStatus *)calloc(trackableCount, sizeof(ARWTrackableStatus));
            for (unsigned int i = 0; i < trackableCount; i++) {
                ARTrackable *t = gARTK->getTrackableAtIndex(i);
                if (!t) {
                    st[i].uid = -1;
                } else {
                    st[i].uid = t->UID;
                    st[i].visible = t->visible;
#ifdef ARDOUBLE_IS_FLOAT
                    memcpy(st[i].matrix, t->transformationMatrix, 16*sizeof(float));
                    memcpy(st[i].matrixR, t->transformationMatrixR, 16*sizeof(float));
#else
                    for (int j = 0; j < 16; j++) st[i].matrix[j] = (float)t->transformationMatrix[j];
                    for (int j = 0; j < 16; j++) st[i].matrixR[j] = (float)t->transformationMatrixR[j];
#endif
                }
            }
            *statuses_p = st;
        }
    }
    
    return true;
}

bool arwRemoveTrackable(int trackableUID)
{
    if (!gARTK) return false;
	return gARTK->removeTrackable(trackableUID);
}

int arwRemoveAllTrackables()
{
    if (!gARTK) return 0;
	return gARTK->removeAllTrackables();
}

#if HAVE_2D
bool arwLoad2dTrackableDatabase(const char *databaseFileName)
{
    if (!gARTK) return false;
    return gARTK->load2DTrackerImageDatabase(databaseFileName);
}

bool arwSave2dTrackableDatabase(const char *databaseFileName)
{
    if (!gARTK) return false;
    return gARTK->save2DTrackerImageDatabase(databaseFileName);
}
#endif // HAVE_2D

bool arwQueryTrackableVisibilityAndTransformation(int trackableUID, float matrix[16])
{
    ARTrackable *trackable;
    
    if (!gARTK) return false;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwQueryTrackableVisibilityAndTransformation(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return false;
    }
    for (int i = 0; i < 16; i++) matrix[i] = (float)trackable->transformationMatrix[i];
    return trackable->visible;
}

bool arwQueryTrackableVisibilityAndTransformationStereo(int trackableUID, float matrixL[16], float matrixR[16])
{
    ARTrackable *trackable;
    
    if (!gARTK) return false;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwQueryTrackableVisibilityAndTransformationStereo(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return false;
    }
    for (int i = 0; i < 16; i++) matrixL[i] = (float)trackable->transformationMatrix[i];
    for (int i = 0; i < 16; i++) matrixR[i] = (float)trackable->transformationMatrixR[i];
    return trackable->visible;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Trackable patterns
// ---------------------------------------------------------------------------------------------

int arwGetTrackablePatternCount(int trackableUID)
{
    ARTrackable *trackable;
    
    if (!gARTK) return 0;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackablePatternCount(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return 0;
    }
    return trackable->patternCount;
}

bool arwGetTrackablePatternConfig(int trackableUID, int patternID, float matrix[16], float *width, float *height, int *imageSizeX, int *imageSizeY)
{
    ARTrackable *trackable;
    ARPattern *p;
    
    if (!gARTK) return false;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackablePatternConfig(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return false;
    }

    if (!(p = trackable->getPattern(patternID))) {
        ARLOGe("arwGetTrackablePatternConfig(): Trackable with UID %d has no pattern with ID %d.\n", trackableUID, patternID);
        return false;
    }

    if (matrix) {
        for (int i = 0; i < 16; i++) matrix[i] = (float)p->m_matrix[i];
    }
    if (width) *width = (float)p->m_width;
    if (height) *height = (float)p->m_height;
    if (imageSizeX) *imageSizeX = p->m_imageSizeX;
    if (imageSizeY) *imageSizeY = p->m_imageSizeY;
    return true;
}

bool arwGetTrackablePatternImage(int trackableUID, int patternID, uint32_t *buffer)
{
    ARTrackable *trackable;
    ARPattern *p;
    
    if (!gARTK) return false;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackablePatternImage(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return false;
    }
    
    if (!(p = trackable->getPattern(patternID))) {
        ARLOGe("arwGetTrackablePatternImage(): Trackable with UID %d has no pattern with ID %d.\n", trackableUID, patternID);
        return false;
    }

    if (!p->m_image) {
        return false;
    }
    
    memcpy(buffer, p->m_image, sizeof(uint32_t) * p->m_imageSizeX * p->m_imageSizeY);
    return true;

}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Trackable options
// ---------------------------------------------------------------------------------------------

bool arwGetTrackableOptionBool(int trackableUID, int option)
{
    ARTrackable *trackable;
    
    if (!gARTK) return false;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackableOptionBool(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return false;
    }
    
    switch (option) {
        case ARW_TRACKABLE_OPTION_FILTERED:
            return(trackable->isFiltered());
            break;
        case ARW_TRACKABLE_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION:
            if (trackable->type == ARTrackable::SINGLE) return (((ARTrackableSquare *)trackable)->useContPoseEstimation);
            break;
        default:
            ARLOGe("arwGetTrackableOptionBool(): Unrecognised option %d.\n", option);
            break;
    }
    return(false);
}

void arwSetTrackableOptionBool(int trackableUID, int option, bool value)
{
    ARTrackable *trackable;
    
    if (!gARTK) return;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwSetTrackableOptionBool(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return;
    }

    switch (option) {
        case ARW_TRACKABLE_OPTION_FILTERED:
            trackable->setFiltered(value);
            break;
        case ARW_TRACKABLE_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION:
            if (trackable->type == ARTrackable::SINGLE) ((ARTrackableSquare *)trackable)->useContPoseEstimation = value;
            break;
        default:
            ARLOGe("arwSetTrackableOptionBool(): Unrecognised option %d.\n", option);
            break;
    }
}

int arwGetTrackableOptionInt(int trackableUID, int option)
{
    ARTrackable *trackable;
    
    if (!gARTK) return INT_MIN;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackableOptionBool(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return (INT_MIN);
    }
    
    switch (option) {
        case ARW_TRACKABLE_OPTION_MULTI_MIN_SUBMARKERS:
            if (trackable->type == ARTrackable::MULTI) return ((ARTrackableMultiSquare *)trackable)->config->min_submarker;
            break;
        default:
            ARLOGe("arwGetTrackableOptionInt(): Unrecognised option %d.\n", option);
            break;
    }
    return (INT_MIN);
}

void arwSetTrackableOptionInt(int trackableUID, int option, int value)
{
    ARTrackable *trackable;
    
    if (!gARTK) return;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwSetTrackableOptionInt(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return;
    }

    switch (option) {
        case ARW_TRACKABLE_OPTION_MULTI_MIN_SUBMARKERS:
            if (trackable->type == ARTrackable::MULTI) ((ARTrackableMultiSquare *)trackable)->config->min_submarker = value;
            break;
        default:
            ARLOGe("arwSetTrackableOptionInt(): Unrecognised option %d.\n", option);
            break;
    }
}

float arwGetTrackableOptionFloat(int trackableUID, int option)
{
    ARTrackable *trackable;
    
    if (!gARTK) return (NAN);
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwGetTrackableOptionBool(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return (NAN);
    }
    
    switch (option) {
        case ARW_TRACKABLE_OPTION_FILTER_SAMPLE_RATE:
            return ((float)trackable->filterSampleRate());
            break;
        case ARW_TRACKABLE_OPTION_FILTER_CUTOFF_FREQ:
            return ((float)trackable->filterCutoffFrequency());
            break;
        case ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE:
            if (trackable->type == ARTrackable::SINGLE) return ((float)((ARTrackableSquare *)trackable)->getConfidence());
            else return (NAN);
            break;
        case ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE_CUTOFF:
            if (trackable->type == ARTrackable::SINGLE) return ((float)((ARTrackableSquare *)trackable)->getConfidenceCutoff());
            else return (NAN);
            break;
        case ARW_TRACKABLE_OPTION_NFT_SCALE:
#if HAVE_NFT
            if (trackable->type == ARTrackable::NFT) return ((float)((ARTrackableNFT *)trackable)->NFTScale());
            else return (NAN);
#else
            return (NAN);
#endif
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_MATRIX:
            if (trackable->type == ARTrackable::MULTI) return (float)((ARTrackableMultiSquare *)trackable)->config->cfMatrixCutoff;
            else return (NAN);
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_PATTERN:
            if (trackable->type == ARTrackable::MULTI) return (float)((ARTrackableMultiSquare *)trackable)->config->cfPattCutoff;
            else return (NAN);
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_INLIER_PROB:
            if (trackable->type == ARTrackable::MULTI) return (float)((ARTrackableMultiSquare *)trackable)->config->minInlierProb;
            else return (NAN);
            break;
        default:
            ARLOGe("arwGetTrackableOptionFloat(): Unrecognised option %d.\n", option);
            break;
    }
    return (NAN);
}

void arwSetTrackableOptionFloat(int trackableUID, int option, float value)
{
    ARTrackable *trackable;
    
    if (!gARTK) return;
	if (!(trackable = gARTK->findTrackable(trackableUID))) {
        ARLOGe("arwSetTrackableOptionFloat(): Couldn't locate trackable with UID %d.\n", trackableUID);
        return;
    }

    switch (option) {
        case ARW_TRACKABLE_OPTION_FILTER_SAMPLE_RATE:
            trackable->setFilterSampleRate(value);
            break;
        case ARW_TRACKABLE_OPTION_FILTER_CUTOFF_FREQ:
            trackable->setFilterCutoffFrequency(value);
            break;
        case ARW_TRACKABLE_OPTION_SQUARE_CONFIDENCE_CUTOFF:
            if (trackable->type == ARTrackable::SINGLE) ((ARTrackableSquare *)trackable)->setConfidenceCutoff(value);
            break;
        case ARW_TRACKABLE_OPTION_NFT_SCALE:
#if HAVE_NFT
            if (trackable->type == ARTrackable::NFT) ((ARTrackableNFT *)trackable)->setNFTScale(value);
#endif
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_MATRIX:
            if (trackable->type == ARTrackable::MULTI) ((ARTrackableMultiSquare *)trackable)->config->cfMatrixCutoff = value;
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_CONF_PATTERN:
            if (trackable->type == ARTrackable::MULTI) ((ARTrackableMultiSquare *)trackable)->config->cfPattCutoff = value;
            break;
        case ARW_TRACKABLE_OPTION_MULTI_MIN_INLIER_PROB:
            if (trackable->type == ARTrackable::MULTI) ((ARTrackableMultiSquare *)trackable)->config->minInlierProb = value;
            break;
        default:
            ARLOGe("arwSetTrackableOptionFloat(): Unrecognised option %d.\n", option);
            break;
    }
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
// ----------------------------------------------------------------------------------------------------
bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, const float projectionNearPlane, const float projectionFarPlane, float *fovy_p, float *aspect_p, float m[16], float p[16])
{
    if (!gARTK) return false;
    
#ifdef ARDOUBLE_IS_FLOAT
    return gARTK->loadOpticalParams(optical_param_name, optical_param_buff, optical_param_buffLen, projectionNearPlane, projectionFarPlane, fovy_p, aspect_p, m, p);
#else
    ARdouble fovy, aspect, m0[16], p0[16];
	if (!gARTK->loadOpticalParams(optical_param_name, optical_param_buff, optical_param_buffLen, projectionNearPlane, projectionFarPlane, &fovy, &aspect, m0, (p ? p0 : NULL))) {
        return false;
    }
    *fovy_p = (float)fovy;
    *aspect_p = (float)aspect;
    for (int i = 0; i < 16; i++) m[i] = (float)m0[i];
    if (p) for (int i = 0; i < 16; i++) p[i] = (float)p0[i];
    return true;
#endif
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Java API
// ----------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
// Java API
// 
// The following functions provide a JNI compatible wrapper around the first set of 
// exported functions.
// --------------------------------------------------------------------------------------
#if ARX_TARGET_PLATFORM_ANDROID

// Utility function to create a Java float array from a C float array
jfloatArray glArrayToJava(JNIEnv *env, ARdouble *arr, int len) {
	jfloatArray result = NULL;
	if ((result = env->NewFloatArray(len))) env->SetFloatArrayRegion(result, 0, len, arr);
	return result;
}

extern "C" {
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetLogLevel(JNIEnv * env, jobject obj, jint logLevel));
	JNIEXPORT jstring JNICALL JNIFUNCTION(arwGetARToolKitVersion(JNIEnv *env, jobject obj));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetError(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseAR(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwChangeToResourcesDir(JNIEnv *env, jobject obj, jstring resourcesDirectoryPath));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwShutdownAR(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunning(JNIEnv *env, jobject obj, jstring vconf, jstring cparaName));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunningStereo(JNIEnv *env, jobject obj, jstring vconfL, jstring cparaNameL, jstring vconfR, jstring cparaNameR, jstring transL2RName));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsRunning(JNIEnv *env, jobject obj));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsInited(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStopRunning(JNIEnv *env, jobject obj));
	JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwGetProjectionMatrix(JNIEnv *env, jobject obj, jfloat nearPlane, jfloat farPlane));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetProjectionMatrixStereo(JNIEnv *env, jobject obj, jfloat nearPlane, jfloat farPlane, jfloatArray projL, jfloatArray projR));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParams(JNIEnv *env, jobject obj, jintArray width, jintArray height, jintArray pixelSize, jobjectArray pixelFormatStringBuffer));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParamsStereo(JNIEnv *env, jobject obj, jintArray widthL, jintArray heightL, jintArray pixelSizeL, jobjectArray pixelFormatStringL, jintArray widthR, jintArray heightR, jintArray pixelSizeR, jobjectArray  pixelFormatStringR));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwCapture(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateAR(JNIEnv *env, jobject obj));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateTexture32(JNIEnv *env, jobject obj, jbyteArray pinArray));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateTextureStereo32(JNIEnv *env, jobject obj, jbyteArray pinArrayL, jbyteArray pinArrayR));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoInit(JNIEnv *env, jobject obj, jint videoSourceIndex));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoSettings(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, jboolean rotate90, jboolean flipH, jboolean flipV, jint hAlign, jint vAlign, jint scalingMode, jintArray viewport));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideo(JNIEnv *env, jobject obj, jint videoSourceIndex));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoFinal(JNIEnv *env, jobject obj, jint videoSourceIndex));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwAddTrackable(JNIEnv *env, jobject obj, jstring cfg));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwRemoveTrackable(JNIEnv *env, jobject obj, jint trackableUID));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwRemoveAllTrackables(JNIEnv *env, jobject obj));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryTrackableVisibilityAndTransformation(JNIEnv *env, jobject obj, jint trackableUID, jfloatArray matrix));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryTrackableVisibilityAndTransformationStereo(JNIEnv *env, jobject obj, jint trackableUID, jfloatArray matrixL, jfloatArray matrixR));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackablePatternCount(JNIEnv *env, jobject obj, int trackableUID));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionBool(JNIEnv *env, jobject obj, jint option, jboolean value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionInt(JNIEnv *env, jobject obj, jint option, jint value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionFloat(JNIEnv *env, jobject obj, jint option, jfloat value));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetTrackerOptionBool(JNIEnv *env, jobject obj, jint option));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackerOptionInt(JNIEnv *env, jobject obj, jint option));
    JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetTrackerOptionFloat(JNIEnv *env, jobject obj, jint option));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackableOptionBool(JNIEnv *env, jobject obj, jint trackableUID, jint option, jboolean value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackableOptionInt(JNIEnv *env, jobject obj, jint trackableUID, jint option, jint value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackableOptionFloat(JNIEnv *env, jobject obj, jint trackableUID, jint option, jfloat value));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetTrackableOptionBool(JNIEnv *env, jobject obj, jint trackableUID, jint option));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackableOptionInt(JNIEnv *env, jobject obj, jint trackableUID, jint option));
    JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetTrackableOptionFloat(JNIEnv *env, jobject obj, jint trackableUID, jint option));

	// Additional Java-specific function not found in the C-API
    JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPushInit(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, jstring pixelFormat, jint camera_index, jint camera_face));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPush1(JNIEnv *env, jobject obj, jint videoSourceIndex, jbyteArray buf, jint bufSize));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPush2(JNIEnv *env, jobject obj, jint videoSourceIndex,
                                       jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                                       jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                                       jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                                       jobject buf3, jint buf3PixelStride, jint buf3RowStride));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPushFinal(JNIEnv *env, jobject obj, jint videoSourceIndex));

	// ------------------------------------------------------------------------------------
	// JNI Functions Not Yet Implemented
    //bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen, const float nearPlane, const float farPlane);
    //bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, const int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen, const float nearPlane, const float farPlane);
    //bool arwGetTrackables(int *count_p, ARWTrackableStatus **statuses_p);
	//bool arwGetTrackablePatternConfig(int trackableUID, int patternID, float matrix[16], float *width, float *height);
	//bool arwGetTrackablePatternImage(int trackableUID, int patternID, Color *buffer);

    //bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, float *fovy_p, float *aspect_p, float m[16], float p[16]);
	// ------------------------------------------------------------------------------------
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetLogLevel(JNIEnv * env, jobject
                                           obj, jint
                                           logLevel)) {
    arwSetLogLevel(logLevel);
}


JNIEXPORT jstring JNICALL JNIFUNCTION(arwGetARToolKitVersion(JNIEnv *env, jobject obj)) 
{
	char versionString[1024];
    
	if (arwGetARToolKitVersion(versionString, 1024)) return env->NewStringUTF(versionString);		
	return env->NewStringUTF("unknown version");
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetError(JNIEnv *env, jobject obj))
{
    return arwGetError();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseAR(JNIEnv *env, jobject obj)) 
{
	return arwInitialiseAR();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwChangeToResourcesDir(JNIEnv *env, jobject obj, jstring resourcesDirectoryPath)) 
{
    bool ok;
    
    if (resourcesDirectoryPath != NULL) {
        const char *resourcesDirectoryPathC = env->GetStringUTFChars(resourcesDirectoryPath, NULL);
        ok = arwChangeToResourcesDir(resourcesDirectoryPathC);
        env->ReleaseStringUTFChars(resourcesDirectoryPath, resourcesDirectoryPathC);
    } else ok = arwChangeToResourcesDir(NULL);
    
    return ok;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwShutdownAR(JNIEnv *env, jobject obj)) 
{
	return arwShutdownAR();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunning(JNIEnv *env, jobject obj, jstring vconf, jstring cparaName))
{
    const char *vconfC = (env->IsSameObject(vconf, NULL) ? NULL : env->GetStringUTFChars(vconf, NULL));
	const char *cparaNameC = (env->IsSameObject(cparaName, NULL) ? NULL : env->GetStringUTFChars(cparaName, NULL));

	bool running = arwStartRunning(vconfC, cparaNameC);

	if (vconfC) env->ReleaseStringUTFChars(vconf, vconfC);
	if (cparaNameC) env->ReleaseStringUTFChars(cparaName, cparaNameC);

	return running;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunningStereo(JNIEnv *env, jobject obj, jstring vconfL, jstring cparaNameL, jstring vconfR, jstring cparaNameR, jstring transL2RName))
{
	const char *vconfLC = env->GetStringUTFChars(vconfL, NULL);
	const char *cparaNameLC = env->GetStringUTFChars(cparaNameL, NULL);
	const char *vconfRC = env->GetStringUTFChars(vconfR, NULL);
	const char *cparaNameRC = env->GetStringUTFChars(cparaNameR, NULL);
	const char *transL2RNameC = env->GetStringUTFChars(transL2RName, NULL);
    
	bool running = arwStartRunningStereo(vconfLC, cparaNameLC, vconfRC, cparaNameRC, transL2RNameC);
    
	env->ReleaseStringUTFChars(vconfL, vconfLC);
	env->ReleaseStringUTFChars(cparaNameL, cparaNameLC);
	env->ReleaseStringUTFChars(vconfR, vconfRC);
	env->ReleaseStringUTFChars(cparaNameR, cparaNameRC);
	env->ReleaseStringUTFChars(transL2RName, transL2RNameC);
    
	return running;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsRunning(JNIEnv *env, jobject obj)) 
{
	return arwIsRunning();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsInited(JNIEnv *env, jobject obj)) 
{
	return arwIsInited();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStopRunning(JNIEnv *env, jobject obj)) 
{
	return arwStopRunning();
}

#define PIXEL_FORMAT_BUFFER_SIZE 1024

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParams(JNIEnv *env, jobject obj, jintArray width, jintArray height, jintArray pixelSize, jobjectArray pixelFormatString))
{
    int w, h, ps;
    char pf[PIXEL_FORMAT_BUFFER_SIZE];
    
    if (!arwGetVideoParams(&w, &h, &ps, pf, PIXEL_FORMAT_BUFFER_SIZE)) return false;
    if (width) env->SetIntArrayRegion(width, 0, 1, &w);
    if (height) env->SetIntArrayRegion(height, 0, 1, &h);
    if (pixelSize) env->SetIntArrayRegion(pixelSize, 0, 1, &ps);
    if (pixelFormatString) env->SetObjectArrayElement(pixelFormatString, 0, env->NewStringUTF(pf));
    return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParamsStereo(JNIEnv *env, jobject obj, jintArray widthL, jintArray heightL, jintArray pixelSizeL, jobjectArray pixelFormatStringL, jintArray widthR, jintArray heightR, jintArray pixelSizeR, jobjectArray  pixelFormatStringR))
{
    int wL, hL, psL, wR, hR, psR;
    char pfL[PIXEL_FORMAT_BUFFER_SIZE], pfR[PIXEL_FORMAT_BUFFER_SIZE];

    if (!arwGetVideoParamsStereo(&wL, &hL, &psL, pfL, PIXEL_FORMAT_BUFFER_SIZE, &wR, &hR, &psR, pfR, PIXEL_FORMAT_BUFFER_SIZE)) return false;
    if (widthL) env->SetIntArrayRegion(widthL, 0, 1, &wL);
    if (heightL) env->SetIntArrayRegion(heightL, 0, 1, &hL);
    if (pixelSizeL) env->SetIntArrayRegion(pixelSizeL, 0, 1, &psL);
    if (pixelFormatStringL) env->SetObjectArrayElement(pixelFormatStringL, 0, env->NewStringUTF(pfL));
    if (widthR) env->SetIntArrayRegion(widthR, 0, 1, &wR);
    if (heightR) env->SetIntArrayRegion(heightR, 0, 1, &hR);
    if (pixelSizeR) env->SetIntArrayRegion(pixelSizeR, 0, 1, &psR);
    if (pixelFormatStringR) env->SetObjectArrayElement(pixelFormatStringR, 0, env->NewStringUTF(pfR));
    return true;
}

JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwGetProjectionMatrix(JNIEnv *env, jobject obj, jfloat nearPlane, jfloat farPlane))
{
	float proj[16];
    
	if (arwGetProjectionMatrix(nearPlane, farPlane, proj)) return glArrayToJava(env, proj, 16);
	return NULL;
}
	
JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetProjectionMatrixStereo(JNIEnv *env, jobject obj, jfloat nearPlane, jfloat farPlane, jfloatArray projL, jfloatArray projR))
{
	float pL[16];
	float pR[16];
    
	if (!arwGetProjectionMatrixStereo(nearPlane, farPlane, pL, pR)) return false;
    env->SetFloatArrayRegion(projL, 0, 16, pL);
    env->SetFloatArrayRegion(projR, 0, 16, pR);
	return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwCapture(JNIEnv *env, jobject obj))
{
	return arwCapture();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateAR(JNIEnv *env, jobject obj)) 
{
	return arwUpdateAR();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateTexture32(JNIEnv *env, jobject obj, jbyteArray pinArray))
{
    bool updated = false;

    if (jbyte *inArray = env->GetByteArrayElements(pinArray, NULL)) {
        updated = arwUpdateTexture32((uint32_t *)inArray);
        env->ReleaseByteArrayElements(pinArray, inArray, 0); // 0 -> copy back the changes on the native side to the Java side.
    }

    return updated;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateTextureStereo32(JNIEnv *env, jobject obj, jbyteArray pinArrayL, jbyteArray pinArrayR))
{
    bool updated = false;

    if (jbyte *inArrayL = env->GetByteArrayElements(pinArrayL, NULL)) {
        if (jbyte *inArrayR = env->GetByteArrayElements(pinArrayR, NULL)) {
            updated = arwUpdateTexture32Stereo((uint32_t *)inArrayL, (uint32_t *)inArrayR);
            env->ReleaseByteArrayElements(pinArrayR, inArrayR, 0); // 0 -> copy back the changes on the native side to the Java side.
        }
        env->ReleaseByteArrayElements(pinArrayL, inArrayL, 0); // 0 -> copy back the changes on the native side to the Java side.
    }

    return updated;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoInit(JNIEnv *env, jobject obj, jint videoSourceIndex))
{
    return arwDrawVideoInit(videoSourceIndex);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoSettings(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, jboolean rotate90, jboolean flipH, jboolean flipV, jint hAlign, jint vAlign, jint scalingMode, jintArray viewport))
{
    int32_t vp[4];
    if (!arwDrawVideoSettings(videoSourceIndex, width, height, rotate90, flipH, flipV, hAlign, vAlign, scalingMode, (viewport ? vp : NULL))) return false;
    if (viewport) {
        env->SetIntArrayRegion(viewport, 0, 4, vp);
    }
    return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideo(JNIEnv *env, jobject obj, jint videoSourceIndex))
{
    return arwDrawVideo(videoSourceIndex);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwDrawVideoFinal(JNIEnv *env, jobject obj, jint videoSourceIndex))
{
    return arwDrawVideoFinal(videoSourceIndex);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwAddTrackable(JNIEnv *env, jobject obj, jstring cfg))
{
	jboolean isCopy;

	const char *cfgC = env->GetStringUTFChars(cfg, &isCopy);
	int trackableUID = arwAddTrackable(cfgC);
	env->ReleaseStringUTFChars(cfg, cfgC);
	return trackableUID;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwRemoveTrackable(JNIEnv *env, jobject obj, jint trackableUID)) 
{
	return arwRemoveTrackable(trackableUID);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwRemoveAllTrackables(JNIEnv *env, jobject obj)) 
{
	return arwRemoveAllTrackables();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryTrackableVisibilityAndTransformation(JNIEnv *env, jobject obj, jint trackableUID, jfloatArray matrix))
{
    float m[16];
    
    if (!arwQueryTrackableVisibilityAndTransformation(trackableUID, m)) return false;
    env->SetFloatArrayRegion(matrix, 0, 16, m);
    return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryTrackableVisibilityAndTransformationStereo(JNIEnv *env, jobject obj, jint trackableUID, jfloatArray matrixL, jfloatArray matrixR))
{
	float mL[16];
	float mR[16];
    
	if (!arwQueryTrackableVisibilityAndTransformationStereo(trackableUID, mL, mR)) return false;
    env->SetFloatArrayRegion(matrixL, 0, 16, mL);
    env->SetFloatArrayRegion(matrixR, 0, 16, mR);
	return true;
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackablePatternCount(JNIEnv *env, jobject obj, int trackableUID)) 
{
	return arwGetTrackablePatternCount(trackableUID);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionBool(JNIEnv *env, jobject obj, jint option, jboolean value))
{
    return arwSetTrackerOptionBool(option, value);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionInt(JNIEnv *env, jobject obj, jint option, jint value))
{
    return arwSetTrackerOptionInt(option, value);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackerOptionFloat(JNIEnv *env, jobject obj, jint option, jfloat value))
{
    return arwSetTrackerOptionFloat(option, value);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetTrackerOptionBool(JNIEnv *env, jobject obj, jint option))
{
    return arwGetTrackerOptionBool(option);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackerOptionInt(JNIEnv *env, jobject obj, jint option))
{
    return arwGetTrackerOptionInt(option);
}

JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetTrackerOptionFloat(JNIEnv *env, jobject obj, jint option))
{
    return arwGetTrackerOptionFloat(option);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackableOptionInt(JNIEnv *env, jobject obj, jint trackableUID, jint option, jint value))
{
    return arwSetTrackableOptionInt(trackableUID, option, value);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetTrackableOptionFloat(JNIEnv *env, jobject obj, jint trackableUID, jint option, jfloat value)) 
{
    return arwSetTrackableOptionFloat(trackableUID, option, value);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetTrackableOptionBool(JNIEnv *env, jobject obj, jint trackableUID, jint option)) 
{
    return arwGetTrackableOptionBool(trackableUID, option);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetTrackableOptionInt(JNIEnv *env, jobject obj, jint trackableUID, jint option)) 
{
    return arwGetTrackableOptionInt(trackableUID, option);
}

JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetTrackableOptionFloat(JNIEnv *env, jobject obj, jint trackableUID, jint option)) 
{
    return arwGetTrackableOptionFloat(trackableUID, option);
}

// Additional JNI functions not found in the C API.

JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPushInit(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, jstring pixelFormat, jint camera_index, jint camera_face))
{
    if (!gARTK) {
        return -1;
    }

    jboolean isCopy;
    jint ret;

    const char *pixelFormatC = env->GetStringUTFChars(pixelFormat, &isCopy);
    ret = gARTK->androidVideoPushInit(env, obj, videoSourceIndex, width, height, pixelFormatC, camera_index, camera_face);
    env->ReleaseStringUTFChars(pixelFormat, pixelFormatC);
    return ret;
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPush1(JNIEnv *env, jobject obj, jint videoSourceIndex, jbyteArray buf, jint bufSize))
{
    if (!gARTK) {
        return -1;
    }

    return gARTK->androidVideoPush1(env, obj, videoSourceIndex, buf, bufSize);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPush2(JNIEnv *env, jobject obj, jint videoSourceIndex,
                                                        jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                                                        jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                                                        jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                                                        jobject buf3, jint buf3PixelStride, jint buf3RowStride))
{
    if (!gARTK) {
        return -1;
    }

    return gARTK->androidVideoPush2(env, obj, videoSourceIndex, buf0, buf0PixelStride, buf0RowStride, buf1, buf1PixelStride, buf1RowStride, buf2, buf2PixelStride, buf2RowStride, buf3, buf3PixelStride, buf3RowStride);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwAndroidVideoPushFinal(JNIEnv *env, jobject obj, jint videoSourceIndex))
{
    if (!gARTK) {
        return -1;
    }

    return gARTK->androidVideoPushFinal(env, obj, videoSourceIndex);
}

#endif // ARX_TARGET_PLATFORM_ANDROID
