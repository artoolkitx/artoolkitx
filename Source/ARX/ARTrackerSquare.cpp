/*
 *  ARTracker.cpp
 *  artoolkitX
 *
 *  A C++ class implementing the artoolkitX square fiducial marker tracker.
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

#include <ARX/ARTrackerSquare.h>
#include <ARX/ARTrackableSquare.h>
#include <ARX/ARTrackableMultiSquare.h>
#include <ARX/ARTrackableMultiSquareAuto.h>
#include <ARX/AR/ar.h>
#include <algorithm>

ARTrackerSquare::ARTrackerSquare() :
    m_trackables(),
    m_threshold(AR_DEFAULT_LABELING_THRESH),
    m_thresholdMode(AR_LABELING_THRESH_MODE_DEFAULT),
    m_imageProcMode(AR_DEFAULT_IMAGE_PROC_MODE),
    m_labelingMode(AR_DEFAULT_LABELING_MODE),
    m_pattRatio(AR_PATT_RATIO),
    m_patternDetectionMode(AR_DEFAULT_PATTERN_DETECTION_MODE),
    m_matrixCodeType(AR_MATRIX_CODE_TYPE_DEFAULT),
    m_debugMode(FALSE),
    m_patternSize(AR_PATT_SIZE1),
    m_patternCountMax(AR_PATT_NUM_MAX),
    m_matrixModeAutoCreateNewTrackables(false),
    m_matrixModeAutoCreateNewTrackablesDefaultWidth(k_matrixModeAutoCreateNewTrackablesDefaultWidth_default),
    m_matrixModeAutoCreateNewTrackablesCallback(nullptr),
    m_arHandle0(NULL),
    m_arHandle1(NULL),
    m_arPattHandle(NULL),
    m_ar3DHandle(NULL),
    m_ar3DStereoHandle(NULL)
{
    
}

ARTrackerSquare::~ARTrackerSquare()
{
    terminate();
}

bool ARTrackerSquare::initialize()
{
    // Create pattern handle so square template trackables can begin to be added.
    if ((m_arPattHandle = arPattCreateHandle2(m_patternSize, m_patternCountMax)) == NULL) {
        ARLOGe("Error: arPattCreateHandle2.\n");
        return false;
    }
    
    return true;
}

bool ARTrackerSquare::isRunning()
{
    return ((bool)m_arHandle0 && (!(bool)m_ar3DStereoHandle || (bool)m_arHandle1));
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Configuration functions
// If an arHandle is current, it will be updated. If no arHandle is current, the value will be
// set on initialisation (in initAR()). The value can be queried at any time.
// ----------------------------------------------------------------------------------------------------
void ARTrackerSquare::setDebugMode(bool debug)
{
    m_debugMode = debug;
    if (m_arHandle0) {
        arSetDebugMode(m_arHandle0, m_debugMode ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE);
        ARLOGi("Debug mode set to %s.\n", debug ? "on." : "off.");
    }
    if (m_arHandle1) {
        arSetDebugMode(m_arHandle1, m_debugMode ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE);
        ARLOGi("Debug mode set to %s.\n", debug ? "on." : "off.");
    }
}

bool ARTrackerSquare::debugMode() const
{
    return m_debugMode;
}

void ARTrackerSquare::setImageProcMode(int mode)
{
    m_imageProcMode = mode;
    
    if (m_arHandle0) {
        arSetImageProcMode(m_arHandle0, mode);
        ARLOGi("Image proc. mode set to %d.\n", m_imageProcMode);
    }
    if (m_arHandle1) {
        arSetImageProcMode(m_arHandle1, mode);
        ARLOGi("Image proc. mode set to %d.\n", m_imageProcMode);
     }
}

int ARTrackerSquare::imageProcMode() const
{
    return m_imageProcMode;
}

void ARTrackerSquare::setThreshold(int thresh)
{
    if (thresh < 0 || thresh > 255) return;
    m_threshold = thresh;
    if (m_arHandle0) {
        arSetLabelingThresh(m_arHandle0, m_threshold);
        ARLOGi("Threshold set to %d\n", m_threshold);
    }
    if (m_arHandle1) {
        arSetLabelingThresh(m_arHandle1, m_threshold);
        ARLOGi("Threshold set to %d\n", m_threshold);
    }
}

int ARTrackerSquare::threshold() const
{
    return m_threshold;
}

void ARTrackerSquare::setThresholdMode(AR_LABELING_THRESH_MODE mode)
{
    m_thresholdMode = mode;
    if (m_arHandle0) {
        arSetLabelingThreshMode(m_arHandle0, m_thresholdMode);
        ARLOGi("Threshold mode set to %d\n", (int)m_thresholdMode);
    }
    if (m_arHandle1) {
        arSetLabelingThreshMode(m_arHandle1, m_thresholdMode);
        ARLOGi("Threshold mode set to %d\n", (int)m_thresholdMode);
    }
}

AR_LABELING_THRESH_MODE ARTrackerSquare::thresholdMode() const
{
    return m_thresholdMode;
}

void ARTrackerSquare::setLabelingMode(int mode)
{
    m_labelingMode = mode;
    if (m_arHandle0) {
        arSetLabelingMode(m_arHandle0, m_labelingMode);
        ARLOGi("Labeling mode set to %d\n", m_labelingMode);
    }
    if (m_arHandle1) {
        arSetLabelingMode(m_arHandle1, m_labelingMode);
        ARLOGi("Labeling mode set to %d\n", m_labelingMode);
    }
}

int ARTrackerSquare::labelingMode() const
{
    return m_labelingMode;
}

void ARTrackerSquare::setPatternDetectionMode(int mode)
{
    m_patternDetectionMode = mode;
    if (m_arHandle0) {
        arSetPatternDetectionMode(m_arHandle0, m_patternDetectionMode);
        ARLOGi("Pattern detection mode set to %d.\n", m_patternDetectionMode);
    }
    if (m_arHandle1) {
        arSetPatternDetectionMode(m_arHandle1, m_patternDetectionMode);
        ARLOGi("Pattern detection mode set to %d.\n", m_patternDetectionMode);
    }
}

int ARTrackerSquare::patternDetectionMode() const
{
    return m_patternDetectionMode;
}

void ARTrackerSquare::setPattRatio(float ratio)
{
    if (ratio <= 0.0f || ratio >= 1.0f) return;
    m_pattRatio = (ARdouble)ratio;
    if (m_arHandle0) {
        arSetPattRatio(m_arHandle0, m_pattRatio);
        ARLOGi("Pattern ratio size set to %d.\n", m_pattRatio);
    }
    if (m_arHandle1) {
        arSetPattRatio(m_arHandle1, m_pattRatio);
        ARLOGi("Pattern ratio size set to %d.\n", m_pattRatio);
    }
}

float ARTrackerSquare::pattRatio() const
{
    return (float)m_pattRatio;
}

void ARTrackerSquare::setMatrixCodeType(AR_MATRIX_CODE_TYPE type)
{
    m_matrixCodeType = type;
    if (m_arHandle0) {
        arSetMatrixCodeType(m_arHandle0, m_matrixCodeType);
        ARLOGi("Matrix code type set to %d.\n", (int)m_matrixCodeType);
    }
    if (m_arHandle1) {
        arSetMatrixCodeType(m_arHandle1, m_matrixCodeType);
        ARLOGi("Matrix code type set to %d.\n", (int)m_matrixCodeType);
    }
}

AR_MATRIX_CODE_TYPE ARTrackerSquare::matrixCodeType() const
{
    return m_matrixCodeType;
}

void ARTrackerSquare::setPatternSize(int patternSize)
{
    if (!m_arPattHandle) return;
    if (m_arPattHandle->patt_num > 0) {
        ARLOGe("Attempt to set pattern size but patterns already loaded. Unload first and then retry.\n");
    }
    if (patternSize < 16 || patternSize > AR_PATT_SIZE1_MAX) {
        return;
        ARLOGe("Attempt to set pattern size to invalid value %d.\n", patternSize);
    }
    
    arPattDeleteHandle(m_arPattHandle);
    m_patternSize = patternSize;
    m_arPattHandle = arPattCreateHandle2(m_patternSize, m_patternCountMax);
}

int ARTrackerSquare::patternSize() const
{
    return m_patternSize;
}

void ARTrackerSquare::setPatternCountMax(int patternCountMax)
{
    if (!m_arPattHandle) return;
    if (m_arPattHandle->patt_num > 0) {
        ARLOGe("Attempt to set pattern count max but patterns already loaded. Unload first and then retry.\n");
    }
    if (patternCountMax > AR_PATT_NUM_MAX || patternCountMax <= 0) {
        ARLOGe("Attempt to set pattern count max to invalid value %d.\n", patternCountMax);
        return;
    }
    
    arPattDeleteHandle(m_arPattHandle);
    m_patternCountMax = patternCountMax;
    m_arPattHandle = arPattCreateHandle2(m_patternSize, m_patternCountMax);
}

int ARTrackerSquare::patternCountMax() const
{
    return m_patternCountMax;
}

bool ARTrackerSquare::start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat)
{
    return start(paramLT, pixelFormat, NULL, AR_PIXEL_FORMAT_INVALID, NULL);
}

bool ARTrackerSquare::start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4])
{
    if (!paramLT0 || pixelFormat0 == AR_PIXEL_FORMAT_INVALID || (paramLT1 && (pixelFormat0 == AR_PIXEL_FORMAT_INVALID || !transL2R))) return false;
    
    // Create AR handle
    if ((m_arHandle0 = arCreateHandle(paramLT0)) == NULL) {
        ARLOGe("arCreateHandle\n");
        goto bail;
    }
    
    // Set the pixel format
    arSetPixelFormat(m_arHandle0, pixelFormat0);
    
    arPattAttach(m_arHandle0, m_arPattHandle);
    
    // Set initial configuration. One call for each configuration option.
    arSetLabelingThresh(m_arHandle0, m_threshold);
    arSetLabelingThreshMode(m_arHandle0, m_thresholdMode);
    arSetImageProcMode(m_arHandle0, m_imageProcMode);
    arSetDebugMode(m_arHandle0, m_debugMode);
    arSetLabelingMode(m_arHandle0, m_labelingMode);
    arSetPattRatio(m_arHandle0, m_pattRatio);
    arSetPatternDetectionMode(m_arHandle0, m_patternDetectionMode);
    arSetMatrixCodeType(m_arHandle0, m_matrixCodeType);
    
    if (paramLT1) {
        // Create AR handle
        if ((m_arHandle1 = arCreateHandle(paramLT1)) == NULL) {
            ARLOGe("arCreateHandle\n");
            goto bail1;
        }
        
        // Set the pixel format
        arSetPixelFormat(m_arHandle1, pixelFormat1);
        
        arPattAttach(m_arHandle1, m_arPattHandle);
        
        // Set initial configuration. One call for each configuration option.
        arSetLabelingThresh(m_arHandle1, m_threshold);
        arSetLabelingThreshMode(m_arHandle1, m_thresholdMode);
        arSetImageProcMode(m_arHandle1, m_imageProcMode);
        arSetDebugMode(m_arHandle1, m_debugMode);
        arSetLabelingMode(m_arHandle1, m_labelingMode);
        arSetPattRatio(m_arHandle1, m_pattRatio);
        arSetPatternDetectionMode(m_arHandle1, m_patternDetectionMode);
        arSetMatrixCodeType(m_arHandle1, m_matrixCodeType);
    }
    
    if (!paramLT1) {
        // Create 3D handle
        if ((m_ar3DHandle = ar3DCreateHandle(&paramLT0->param)) == NULL) {
            ARLOGe("ar3DCreateHandle\n");
            goto bail2;
        }
    } else{
        memcpy(m_transL2R, transL2R, sizeof(ARdouble)*12);
        m_ar3DStereoHandle = ar3DStereoCreateHandle(&paramLT0->param, &paramLT1->param, AR_TRANS_MAT_IDENTITY, m_transL2R);
        if (!m_ar3DStereoHandle) {
            ARLOGe("ar3DStereoCreateHandle\n");
            goto bail2;
        }
    }
    
    ARLOGd("ARTrackerSquare::start() done.\n");
    return true;
    
bail2:
    arDeleteHandle(m_arHandle1);
    m_arHandle1 = NULL;
bail1:
    arDeleteHandle(m_arHandle0);
    m_arHandle0 = NULL;
bail:
    return false;
}

bool ARTrackerSquare::wantsUpdate()
{
    return (!m_trackables.empty() || m_matrixModeAutoCreateNewTrackables);
}

bool ARTrackerSquare::update(AR2VideoBufferT *buff)
{
    return update(buff, NULL);
}

bool ARTrackerSquare::update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1)
{
    ARMarkerInfo *markerInfo0 = NULL;
    ARMarkerInfo *markerInfo1 = NULL;
    int markerNum0 = 0;
    int markerNum1 = 0;

    ARLOGd("ARX::ARTrackerSquare::update()\n");

    if (!buff0 || !m_arHandle0 || (buff1 && !m_arHandle1)) return false;

    if (arDetectMarker(m_arHandle0, buff0) < 0) {
        ARLOGe("arDetectMarker().\n");
        return false;
    }
    markerInfo0 = arGetMarker(m_arHandle0);
    markerNum0 = arGetMarkerNum(m_arHandle0);
    if (buff1) {
        if (arDetectMarker(m_arHandle1, buff1) < 0) {
            ARLOGe("arDetectMarker().\n");
            return false;
        }
        markerInfo1 = arGetMarker(m_arHandle1);
        markerNum1 = arGetMarkerNum(m_arHandle1);
    }

    // Update square markers.
    bool success = true;
    if (!buff1) {
        for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
            if ((*it)->type == ARTrackable::SINGLE) {
                success &= (std::static_pointer_cast<ARTrackableSquare>(*it))->updateWithDetectedMarkers(markerInfo0, markerNum0, m_ar3DHandle);
            } else if ((*it)->type == ARTrackable::MULTI) {
                success &= (std::static_pointer_cast<ARTrackableMultiSquare>(*it))->updateWithDetectedMarkers(markerInfo0, markerNum0, m_ar3DHandle);
            } else if ((*it)->type == ARTrackable::MULTI_AUTO) {
                success &= (std::static_pointer_cast<ARTrackableMultiSquareAuto>(*it))->updateWithDetectedMarkers(markerInfo0, markerNum0, m_arHandle0->xsize, m_arHandle0->ysize, m_ar3DHandle);
            }
        }
    } else {
        for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
            if ((*it)->type == ARTrackable::SINGLE) {
                success &= (std::static_pointer_cast<ARTrackableSquare>(*it))->updateWithDetectedMarkersStereo(markerInfo0, markerNum0, markerInfo1, markerNum1, m_ar3DStereoHandle, m_transL2R);
            } else if ((*it)->type == ARTrackable::MULTI) {
                success &= (std::static_pointer_cast<ARTrackableMultiSquare>(*it))->updateWithDetectedMarkersStereo(markerInfo0, markerNum0, markerInfo1, markerNum1, m_ar3DStereoHandle, m_transL2R);
            } else if ((*it)->type == ARTrackable::MULTI_AUTO) {
                success &= (std::static_pointer_cast<ARTrackableMultiSquareAuto>(*it))->updateWithDetectedMarkersStereo(markerInfo0, markerNum0, m_arHandle0->xsize, m_arHandle0->ysize, markerInfo1, markerNum1, m_arHandle1->xsize, m_arHandle1->ysize, m_ar3DStereoHandle, m_transL2R);
            }
        }
    }

    // If the user wants unmatched markers to be added as new trackables, and we're doing barcode (matrix code)
    // detection, look for unmatched valid barcode markers, and add them.
    if (m_matrixModeAutoCreateNewTrackables) {
        std::vector<std::shared_ptr<ARTrackableSquare>> newTrackables;
        int pattDectectionMode = arGetPatternDetectionMode(m_arHandle0);
        if (pattDectectionMode == AR_MATRIX_CODE_DETECTION || pattDectectionMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDectectionMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX) {
            for (int i = 0; i < markerNum0; i++) {
                if (markerInfo0[i].idMatrix >= 0 && !markerInfo0[i].matched) {
                    uint64_t globalID = m_matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID ? markerInfo0[i].globalID : 0;
                    ARTrackableSquare *t = new ARTrackableSquare();
                    if (!t->initWithBarcode(globalID ? 0 : markerInfo0[i].idMatrix, m_matrixModeAutoCreateNewTrackablesDefaultWidth, globalID)) {
                        ARLOGe("Error initing new auto-detected barcode marker.", globalID);
                        delete t;
                    } else {
                        newTrackables.push_back(std::shared_ptr<ARTrackableSquare>(t));
                    }
                }
            }
        }
        if (buff1) {
            pattDectectionMode = arGetPatternDetectionMode(m_arHandle1);
            if (pattDectectionMode == AR_MATRIX_CODE_DETECTION || pattDectectionMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDectectionMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX) {
                for (int i = 0; i < markerNum1; i++) {
                    if (markerInfo1[i].idMatrix >= 0 && !markerInfo1[i].matched) {
                        uint64_t globalID = m_matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID ? markerInfo1[i].globalID : 0;
                        // Need to check that we haven't already just added it (e.g. present in both stereo views).
                        if (std::find_if(newTrackables.begin(), newTrackables.end(), [&](std::shared_ptr<ARTrackableSquare> t) { return globalID ? t->globalID == globalID : t->patt_id == markerInfo1[i].idPatt; } ) == newTrackables.end()) {
                            ARTrackableSquare *t = new ARTrackableSquare();
                            if (!t->initWithBarcode(globalID ? 0 : markerInfo1[i].idMatrix, m_matrixModeAutoCreateNewTrackablesDefaultWidth, globalID)) {
                                ARLOGe("Error initing new auto-detected barcode marker.", globalID);
                                delete t;
                            } else {
                                newTrackables.push_back(std::shared_ptr<ARTrackableSquare>(t));
                            }
                        }
                    }
                }
            }
        }
        for (std::shared_ptr<ARTrackableSquare> trackable : newTrackables) {
            m_trackables.push_back(trackable);
            if (m_matrixModeAutoCreateNewTrackablesCallback) {
                (*m_matrixModeAutoCreateNewTrackablesCallback)(*trackable);
            }
        }
    }

    return true;
}

bool ARTrackerSquare::stop()
{
    //ARLOGd("Cleaning up artoolkitX handles.\n");
    if (m_ar3DHandle) {
        ar3DDeleteHandle(&m_ar3DHandle); // Sets ar3DHandle0 to NULL.
    }
    if (m_ar3DStereoHandle) {
        ar3DStereoDeleteHandle(&m_ar3DStereoHandle); // Sets ar3DStereoHandle to NULL.
    }
    
    if (m_arHandle0) {
        arPattDetach(m_arHandle0);
        arDeleteHandle(m_arHandle0);
        m_arHandle0 = NULL;
    }
    
    if (m_arHandle1) {
        arPattDetach(m_arHandle1);
        arDeleteHandle(m_arHandle1);
        m_arHandle1 = NULL;
    }

    return true;
}
void ARTrackerSquare::terminate()
{
    if (m_arPattHandle) {
        arPattDeleteHandle(m_arPattHandle);
        m_arPattHandle = NULL;
    }
}

int ARTrackerSquare::newTrackable(std::vector<std::string> config)
{
    // First token is trackable type.
    if (config.at(0).compare("single") == 0) {
        
        // Token 2 is path to pattern.
        if (config.size() < 2) {
            ARLOGe("Pattern marker config. requires path to pattern.\n");
            return ARTrackable::NO_ID;
        }
        
        // Token 3 is marker width.
        if (config.size() < 3) {
            ARLOGe("Pattern marker config. requires marker width.\n");
            return ARTrackable::NO_ID;
        }
        ARdouble width;
#ifdef ARDOUBLE_IS_FLOAT
        width = strtof(config.at(2).c_str(), NULL);
#else
        width = strtod(config.at(2).c_str(), NULL);
#endif
        if (width == 0.0f) {
            ARLOGe("Pattern marker config. specified with invalid width parameter ('%s').\n", config.at(2).c_str());
            return ARTrackable::NO_ID;
        }
        
        ARLOGi("Creating ARTrackableSquare with pattern='%s', width=%f.\n", config.at(1).c_str(), width);
        ARTrackableSquare *ret = new ARTrackableSquare();
        if (!ret->initWithPatternFile(config.at(1).c_str(), width, m_arPattHandle)) {
            // Marker failed to load, or was not added.
            delete ret;
            return ARTrackable::NO_ID;
        }

        m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
        return ret->UID;
        
    } else if (config.at(0).compare("single_buffer") == 0) {
        
        // Token 2 is marker width.
        if (config.size() < 2) {
            ARLOGe("Pattern marker from buffer config. requires marker width.\n");
            return ARTrackable::NO_ID;
        }
        ARdouble width;
#ifdef ARDOUBLE_IS_FLOAT
        width = strtof(config.at(1).c_str(), NULL);
#else
        width = strtod(config.at(1).c_str(), NULL);
#endif
        if (width == 0.0f) {
            ARLOGe("Pattern marker from buffer config. specified with invalid height parameter ('%s').\n", config.at(1).c_str());
            return ARTrackable::NO_ID;
        }
        
        // Token 3 is buffer.
        if (config.size() < 3) {
            ARLOGe("Pattern marker from buffer config. requires buffer in config.\n");
            return ARTrackable::NO_ID;
        }
        if (config.at(2).compare(0, 7, "buffer=") != 0) {
            ARLOGe("Pattern marker from buffer config. specified with invalid buffer parameter.\n");
            return ARTrackable::NO_ID;
        }
        const char *bufferStart = config.at(2).c_str() + 7;
        
        ARTrackableSquare *ret = new ARTrackableSquare();
        if (!ret->initWithPatternFromBuffer(bufferStart, width, m_arPattHandle)) {
            // Marker failed to load, or was not added
            delete ret;
            return ARTrackable::NO_ID;
        }

        m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
        return ret->UID;
        
    } else if (config.at(0).compare("single_barcode") == 0) {
        
        // Token 2 is barcode ID.
        if (config.size() < 2) {
            ARLOGe("Barcode marker config. requires barcode ID.\n");
            return ARTrackable::NO_ID;
        }
        long barcodeID = strtol(config.at(1).c_str(), NULL, 0);
        if (barcodeID < 0 || (barcodeID == 0 && (errno == EINVAL || errno == ERANGE))) {
            ARLOGe("Barcode marker config. specified with invalid ID parameter ('%s').\n", config.at(1).c_str());
            return ARTrackable::NO_ID;
        }
        
        // Token 3 is marker width.
        if (config.size() < 3) {
            ARLOGe("Barcode marker config. requires marker width.\n");
            return ARTrackable::NO_ID;
        }
        ARdouble width;
#ifdef ARDOUBLE_IS_FLOAT
        width = strtof(config.at(2).c_str(), NULL);
#else
        width = strtod(config.at(2).c_str(), NULL);
#endif
        if (width == 0.0f) {
            ARLOGe("Barcode marker config. specified with invalid width parameter ('%s').\n", config.at(2).c_str());
            return ARTrackable::NO_ID;
        }
        
        ARTrackableSquare *ret = new ARTrackableSquare();
        if (!ret->initWithBarcode((int)barcodeID, width)) {
            // Marker failed to load, or was not added
            delete ret;
            return ARTrackable::NO_ID;
        }

        m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
        return ret->UID;
        
    } else if (config.at(0).compare("multi") == 0) {
        
        // Token 2 is path to config.
        if (config.size() < 2) {
            ARLOGe("Multimarker config. requires path to multi config file.\n");
            return ARTrackable::NO_ID;
        }
        
        ARTrackableMultiSquare *ret = new ARTrackableMultiSquare();
        if (!ret->load(config.at(1).c_str(), m_arPattHandle)) {
            // Marker failed to load, or was not added
            delete ret;
            return ARTrackable::NO_ID;
        }

        m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
        return ret->UID;
        
    } else if (config.at(0).compare("multi_auto") == 0) {
        
        // Token 2 is origin marker barcode ID.
        if (config.size() < 2) {
            ARLOGe("Multimarker auto config. requires base marker ID.\n");
            return ARTrackable::NO_ID;
        }
        long originMarkerUID = strtol(config.at(1).c_str(), NULL, 0);
        if (originMarkerUID < 0 || (originMarkerUID == 0 && (errno == EINVAL || errno == ERANGE))) {
            ARLOGe("Multimarker auto config. specified with invalid origin marker UID parameter ('%s').\n", config.at(1).c_str());
            return ARTrackable::NO_ID;
        }

        // Token 3 is marker width.
        if (config.size() < 3) {
            ARLOGe("Multimarker auto config. requires marker width.\n");
            return ARTrackable::NO_ID;
        }
        ARdouble width;
#ifdef ARDOUBLE_IS_FLOAT
        width = strtof(config.at(2).c_str(), NULL);
#else
        width = strtod(config.at(2).c_str(), NULL);
#endif
        if (width == 0.0f) {
            ARLOGe("Multimarker auto config. specified with invalid width parameter ('%s').\n", config.at(2).c_str());
            return ARTrackable::NO_ID;
        }
        
        ARTrackableMultiSquareAuto *ret = new ARTrackableMultiSquareAuto();
        if (!ret->initWithOriginMarkerUID((int)originMarkerUID, width)) {
            // Marker failed to load, or was not added
            delete ret;
            return ARTrackable::NO_ID;
        }

        m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
        return ret->UID;

    }

    return ARTrackable::NO_ID;
}

unsigned int ARTrackerSquare::countTrackables()
{
    return (unsigned int)m_trackables.size();
}

std::shared_ptr<ARTrackable> ARTrackerSquare::getTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return std::shared_ptr<ARTrackable>();
    }
    return *ti;
}

std::vector<std::shared_ptr<ARTrackable>> ARTrackerSquare::getAllTrackables()
{
    return std::vector<std::shared_ptr<ARTrackable>>(m_trackables);
}

bool ARTrackerSquare::deleteTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return false;
    }
    m_trackables.erase(ti);
    return true;
}

void ARTrackerSquare::deleteAllTrackables()
{
    m_trackables.clear();
}

// ----------------------------------------------------------------------------------------------------
#pragma mark Square tracker debug texture
// ----------------------------------------------------------------------------------------------------

bool ARTrackerSquare::updateDebugTextureRGBA32(const int videoSourceIndex, uint32_t* buffer)
{
#ifdef AR_DISABLE_LABELING_DEBUG_MODE
    ARLOGe("Debug texture not supported.");
    return false;
#else
    // Check everything is valid.
    if (!buffer) return false;
    ARHandle *arHandle = (videoSourceIndex == 1 ? m_arHandle1 : m_arHandle0);
    if (!arHandle) return false;
    if (!arHandle->labelInfo.bwImage) return false;
    
    uint8_t *src;
    uint32_t* dest = buffer;
    int h = arHandle->ysize;
    if (arGetImageProcMode(arHandle) == AR_IMAGE_PROC_FIELD_IMAGE) {
        int wdiv2 = arHandle->xsize >> 1;
        for (int y = 0; y < h; y++) {
            src = &(arHandle->labelInfo.bwImage[(h >> 1) * wdiv2]);
            for (int x = 0; x < wdiv2; x++) {
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                dest++;
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                dest++;
                src++;
            }
        }
    } else {
        src = arHandle->labelInfo.bwImage;
        int w = arHandle->xsize;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                src++;
                dest++;
            }
        }
        
    }
    return true;
#endif
}

