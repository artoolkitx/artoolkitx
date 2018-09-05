/*
 *  ARController.cpp
 *  artoolkitX
 *
 *  A C++ class encapsulating core controller functionality of artoolkitX.
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

#include <ARX/ARController.h>
#include <ARX/Error.h>
#include <ARX/ARG/mtx.h>
#ifdef __APPLE__
#  include <syslog.h>
#endif
#if HAVE_NFT
#  include "trackingSub.h"
#endif
#include <ARX/AR/paramGL.h>

#include <stdarg.h>

#include <algorithm>
#include <string>
#include <sstream>


// ----------------------------------------------------------------------------------------------------
#pragma mark  Singleton, constructor, destructor
// ----------------------------------------------------------------------------------------------------

ARController::ARController() :
    state(NOTHING_INITIALISED),
    versionString(NULL),
    m_videoSource0(NULL),
    m_videoSource1(NULL),
    m_videoSourceIsStereo(false),
    m_updateFrameStamp0({0,0}),
    m_updateFrameStamp1({0,0}),
    m_arVideoViews{NULL},
    m_trackables(),
    doSquareMarkerDetection(false),
#if HAVE_NFT
    doNFTMarkerDetection(false),
#endif
#if HAVE_2D
    doTwoDMarkerDetection(false),
#endif
    m_error(ARX_ERROR_NONE)
{
}

ARController::~ARController()
{
	shutdown();
    if (versionString) free(versionString);
}

const char* ARController::getARToolKitVersion()
{
    if (!versionString) arGetVersion(&versionString);
	return versionString;
}

void ARController::setError(int error)
{
    if (m_error == ARX_ERROR_NONE) {
        m_error = error;
    }
}

int ARController::getError()
{
    int temp = m_error;
    if (temp != ARX_ERROR_NONE) {
        m_error = ARX_ERROR_NONE;
    }
    return temp;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  artoolkitX lifecycle functions
// ----------------------------------------------------------------------------------------------------

bool ARController::initialiseBase()
{
    ARLOGd("ARX::ARController::initialiseBase(...)\n");
	if (state != NOTHING_INITIALISED) {
        ARLOGe("Initialise called while already initialised. Will finish first.\n");
        if (!shutdown()) {
            return false;
        }
	}
    
    char *versionString = NULL;
    arGetVersion(&versionString);
	ARLOGi("artoolkitX v%s initalised.\n", versionString);
    free(versionString);

    // At present, trackers are hard-coded. In the future, we'll dynamically
    // load trackers as required.
    m_squareTracker = std::shared_ptr<ARTrackerSquare>(new ARTrackerSquare);
    if (!m_squareTracker->initialize()) {
        ARLOGe("Error initialising square marker tracker.\n");
        goto bail;
    }
#if HAVE_NFT
    m_nftTracker = std::shared_ptr<ARTrackerNFT>(new ARTrackerNFT);
    if (!m_nftTracker->initialize()) {
        ARLOGe("Error initialising NFT marker tracker.\n");
         goto bail2;
    }
#endif

#if HAVE_2D
    m_twoDTracker = std::shared_ptr<ARTracker2d>(new ARTracker2d);
    if (!m_twoDTracker->initialize()) {
        ARLOGe("Error initialising NFT tracker.\n");
        goto bail2;
    }
#endif
    
	state = BASE_INITIALISED;

    ARLOGd("ARX::ARController::initialiseBase() done.\n");
	return true;

bail2:
    m_squareTracker.reset();
bail:
    return false;
}

bool ARController::startRunning(const char* vconf, const char* cparaName, const char* cparaBuff, const long cparaBuffLen)
{
	ARLOGi("Starting...\n");

	// Check for initialization before starting video
	if (state != BASE_INITIALISED) {
        ARLOGe("Start running called but base not initialised.\n");
		return false;
	}

	m_videoSource0 = new ARVideoSource;
	if (!m_videoSource0) {
        ARLOGe("No video source.\n");
		return false;
	}

	m_videoSource0->configure(vconf, false, cparaName, cparaBuff, cparaBuffLen);

    if (!m_videoSource0->open()) {
        if (m_videoSource0->getError() == ARX_ERROR_DEVICE_UNAVAILABLE) {
            ARLOGe("Video source unavailable.\n");
            setError(ARX_ERROR_DEVICE_UNAVAILABLE);
        } else {
            ARLOGe("Unable to open video source.\n");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        return false;
    }

    m_videoSourceIsStereo = false;
	state = WAITING_FOR_VIDEO;
    stateWaitingMessageLogged = false;

    ARLOGd("ARController::startRunning(): done.\n");
	return true;
}

bool ARController::startRunningStereo(const char* vconfL, const char* cparaNameL, const char* cparaBuffL, const long cparaBuffLenL,
                                      const char* vconfR, const char* cparaNameR, const char* cparaBuffR, const long cparaBuffLenR,
                                      const char* transL2RName, const char* transL2RBuff, const long transL2RBuffLen)
{
    ARLOGi("Starting... (stereo)\n");

	// Check for initialisation before starting video
	if (state != BASE_INITIALISED) {
        ARLOGe("Start running called but base not initialised.\n");
		return false;
	}

    // Load stereo parameters.
    if (transL2RName) {
        if (arParamLoadExt(transL2RName, m_transL2R) < 0) {
            ARLOGe("arParamLoadExt.\n");
            return false;
        }
    } else if (transL2RBuff && transL2RBuffLen > 0) {
        if (arParamLoadExtFromBuffer(transL2RBuff, transL2RBuffLen, m_transL2R) < 0) {
            ARLOGe("arParamLoadExtFromBuffer.\n");
            return false;
        }
    } else {
        ARLOGe("transL2R not specified.\n");
		return false;
    }
    //arUtilMatInv(m_transL2R, transR2L);
    arParamDispExt(m_transL2R);

	m_videoSource0 = new ARVideoSource;
	m_videoSource1 = new ARVideoSource;
	if (!m_videoSource0 || !m_videoSource1) {
        ARLOGe("No video sources.\n");
		return false;
	}

	m_videoSource0->configure(vconfL, false, cparaNameL, cparaBuffL, cparaBuffLenL);
	m_videoSource1->configure(vconfR, false, cparaNameR, cparaBuffR, cparaBuffLenR);

    if (!m_videoSource0->open()) {
        if (m_videoSource0->getError() == ARX_ERROR_DEVICE_UNAVAILABLE) {
            ARLOGe("Video source 0 unavailable.\n");
            setError(ARX_ERROR_DEVICE_UNAVAILABLE);
        } else {
            ARLOGe("Unable to open video source 1.\n");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        delete m_videoSource1;
        m_videoSource1 = NULL;
        return false;
    }
    if (!m_videoSource1->open()) {
        if (m_videoSource1->getError() == ARX_ERROR_DEVICE_UNAVAILABLE) {
            ARLOGe("Video source 1 unavailable.\n");
            setError(ARX_ERROR_DEVICE_UNAVAILABLE);
        } else {
            ARLOGe("Unable to open video source 1.\n");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        delete m_videoSource1;
        m_videoSource1 = NULL;
        return false;
    }

    m_videoSourceIsStereo = true;
	state = WAITING_FOR_VIDEO;
    stateWaitingMessageLogged = false;

    ARLOGd("ARController::startRunningStereo(): done.\n");
	return true;
}

bool ARController::capture()
{
    // First check there is a video source and it's open.
    if (!m_videoSource0 || !m_videoSource0->isOpen() || (m_videoSourceIsStereo && (!m_videoSource1 || !m_videoSource1->isOpen()))) {
        ARLOGe("No video source or video source is closed.\n");
        return false;
    }

    if (!m_videoSource0->captureFrame()) {
        return false;
    }

    if (m_videoSourceIsStereo) {
        if (!m_videoSource1->captureFrame()) {
            return false;
        }
    }

    return true;
}

bool ARController::updateTextureRGBA32(const int videoSourceIndex, uint32_t *buffer)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    if (!m_squareTracker->debugMode()) {
        ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
        if (!vs) return false;
        return vs->getFrameTextureRGBA32(buffer);
    } else {
        return m_squareTracker->updateDebugTextureRGBA32(videoSourceIndex, buffer);
    }
}

bool ARController::update()
{
    ARLOGd("ARX::ARController::update().\n");
    
	if (state != DETECTION_RUNNING) {
        if (state != WAITING_FOR_VIDEO) {
            // State is NOTHING_INITIALISED or BASE_INITIALISED.
            ARLOGe("Update called but not yet started.\n");
            return false;

        } else {

            // First check there is a video source and it's open.
            if (!m_videoSource0 || !m_videoSource0->isOpen() || (m_videoSourceIsStereo && (!m_videoSource1 || !m_videoSource1->isOpen()))) {
                ARLOGe("No video source or video source is closed.\n");
                return false;
            }

            // Video source is open, check whether we're waiting for it to start running.
            // If it's not running, return to caller now.
            if (!m_videoSource0->isRunning() || (m_videoSourceIsStereo && !m_videoSource1->isRunning())) {

                if (!stateWaitingMessageLogged) {
                    ARLOGi("Waiting for video.\n");
                    stateWaitingMessageLogged = true;
                }
                return true;
            }

            state = DETECTION_RUNNING;
        }
	}

    // Checkout frame(s).
    AR2VideoBufferT *image0, *image1 = NULL;
    image0 = m_videoSource0->checkoutFrameIfNewerThan(m_updateFrameStamp0);
    if (!image0) {
        return true;
    }
    m_updateFrameStamp0 = image0->time;
    if (m_videoSourceIsStereo) {
        image1 = m_videoSource1->checkoutFrameIfNewerThan(m_updateFrameStamp1);
        if (!image1) {
            m_videoSource0->checkinFrame(); // If we didn't checkout this frame, but we already checked out a frame from one or more other video sources, check those back in.
            return true;
        }
        m_updateFrameStamp1 = image1->time;
    }

    //
    // Tracker updates.
    //

    bool ret = true;

    if (doSquareMarkerDetection) {
        if (!m_squareTracker->isRunning()) {
            if (!m_videoSourceIsStereo) ret = m_squareTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat());
            else ret = m_squareTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat(), m_videoSource1->getCameraParameters(), m_videoSource1->getPixelFormat(), m_transL2R);
            if (!ret) goto done;
        }
        m_squareTracker->update(image0, image1, m_trackables);
    }
#if HAVE_NFT
    if (doNFTMarkerDetection) {
        if (!m_nftTracker->isRunning()) {
            if (!m_videoSourceIsStereo) ret = m_nftTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat());
            else ret = m_nftTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat(), m_videoSource1->getCameraParameters(), m_videoSource1->getPixelFormat(), m_transL2R);
            if (!ret) goto done;
       }
        m_nftTracker->update(image0, image1, m_trackables);
    }
#endif
#if HAVE_2D
    if (doTwoDMarkerDetection) {
        if (!m_twoDTracker->isRunning()) {
            if (!m_videoSourceIsStereo) ret = m_twoDTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat());
            else ret = m_twoDTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat(), m_videoSource1->getCameraParameters(), m_videoSource1->getPixelFormat(), m_transL2R);
            if (!ret) goto done;
        }
        m_twoDTracker->update(image0, image1, m_trackables);
    }
#endif
done:
    // Checkin frames.
    m_videoSource0->checkinFrame();
    if (m_videoSourceIsStereo) m_videoSource1->checkinFrame();

    ARLOGd("ARX::ARController::update(): done.\n");
    
    return ret;
}

bool ARController::stopRunning()
{
	ARLOGd("ARX::ARController::stopRunning()\n");
	if (state != DETECTION_RUNNING && state != WAITING_FOR_VIDEO) {
        ARLOGe("Stop running called but not running.\n");
		return false;
	}
    
    m_squareTracker->stop();
#if HAVE_NFT
    m_nftTracker->stop();
#endif
#if HAVE_2D
    m_twoDTracker->stop();
#endif
    if (m_videoSource0) {
        if (m_videoSource0->isOpen()) {
            m_videoSource0->close();
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
    }

    if (m_videoSource1) {
        if (m_videoSource1->isOpen()) {
            m_videoSource1->close();
        }
        delete m_videoSource1;
        m_videoSource1 = NULL;
    }

	state = BASE_INITIALISED;

    ARLOGd("ARX::ARController::stopRunning(): done.\n");
	return true;
}

bool ARController::shutdown()
{
    ARLOGd("ARX::ARController::shutdown()\n");
    do {
        switch (state) {
            case DETECTION_RUNNING:
            case WAITING_FOR_VIDEO:
                ARLOGd("ARX::ARController::shutdown(): DETECTION_RUNNING or WAITING_FOR_VIDEO, forcing stop.\n");
                stopRunning();
            break;

            case BASE_INITIALISED:
                if (countTrackables() > 0) {
                    ARLOGd("ARX::ARController::shutdown(): BASE_INITIALISED, cleaning up trackables.\n");
                    removeAllTrackables();
                }

                m_squareTracker->terminate();
                m_squareTracker.reset();
#if HAVE_NFT
                m_nftTracker->terminate();
                m_nftTracker.reset();
#endif
#if HAVE_2D
                m_twoDTracker->terminate();
                m_twoDTracker.reset();
#endif
                state = NOTHING_INITIALISED;
                // Fall though.
            case NOTHING_INITIALISED:
                ARLOGd("ARX::ARController::shutdown(): NOTHING_INITIALISED, complete\n");
            break;
        }
    } while (state != NOTHING_INITIALISED);

    ARLOGi("artoolkitX finished.\n");
	return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream drawing.
// ----------------------------------------------------------------------------------------------------

bool ARController::drawVideoInit(const int videoSourceIndex)
{
    ARLOGd("ARController::drawVideoInit(%d).\n", videoSourceIndex);
    
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;
    
    if (m_arVideoViews[videoSourceIndex]) {
        delete m_arVideoViews[videoSourceIndex];
        m_arVideoViews[videoSourceIndex] = NULL;
    }
    
    m_arVideoViews[videoSourceIndex] = new ARVideoView;
    if (!m_arVideoViews[videoSourceIndex]) {
        ARLOGe("Error allocated ARVideoView.\n");
        return false;
    }
    
    return true;
}

bool ARController::drawVideoSettings(const int videoSourceIndex, const int width, const int height, const bool rotate90, const bool flipH, const bool flipV, const ARVideoView::HorizontalAlignment hAlign, const ARVideoView::VerticalAlignment vAlign, const ARVideoView::ScalingMode scalingMode, int32_t viewport[4])
{
    ARLOGd("ARController::drawVideoSettings(%d, %d, %d, ...).\n", videoSourceIndex, width, height);
    
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    if (!m_arVideoViews[videoSourceIndex]) {
        ARLOGe("ARVideoView %d not inited.\n", videoSourceIndex);
        return false;
    }
    
    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!m_arVideoViews[videoSourceIndex]->initWithVideoSource(*vs, width, height)) {
        ARLOGe("Unable to init ARVideoView.\n");
        return false;
    }
    m_arVideoViews[videoSourceIndex]->setRotate90(rotate90);
    m_arVideoViews[videoSourceIndex]->setFlipH(flipH);
    m_arVideoViews[videoSourceIndex]->setFlipV(flipV);
    m_arVideoViews[videoSourceIndex]->setHorizontalAlignment(hAlign);
    m_arVideoViews[videoSourceIndex]->setVerticalAlignment(vAlign);
    m_arVideoViews[videoSourceIndex]->setScalingMode(scalingMode);
    if (viewport) {
        m_arVideoViews[videoSourceIndex]->getViewport(viewport);
    }
    
    return true;
}

bool ARController::drawVideo(const int videoSourceIndex)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    if (!m_arVideoViews[videoSourceIndex]) {
        ARLOGe("ARVideoView %d not inited.\n", videoSourceIndex);
        return false;
    }
    
    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    m_arVideoViews[videoSourceIndex]->draw(vs);
    
    return true;
}

bool ARController::drawVideoFinal(const int videoSourceIndex)
{
    ARLOGd("ARController::drawVideoFinal(%d).\n", videoSourceIndex);
    
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    if (!m_arVideoViews[videoSourceIndex]) {
        ARLOGe("ARVideoView %d not inited.\n", videoSourceIndex);
        return false;
    }
    
    delete m_arVideoViews[videoSourceIndex];
    m_arVideoViews[videoSourceIndex] = NULL;
    
    return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  State queries
// ----------------------------------------------------------------------------------------------------

bool ARController::projectionMatrix(const int videoSourceIndex, const ARdouble projectionNearPlane, const ARdouble projectionFarPlane, ARdouble proj[16])
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs || !vs->isOpen()) {
        ARLOGe("Error: projection matrix requested but no video source %d or video source is closed.\n", videoSourceIndex);
        return false;
    }
    if (!vs->isRunning()) {
        ARLOGe("Error: projection matrix requested but no video source %d not yet running.\n", videoSourceIndex);
        return false;
    }
    
    ARParamLT* paramLT = vs->getCameraParameters();
    if (!paramLT) {
        ARLOGe("Error: video source %d unable to supply camera parameters.\n", videoSourceIndex);
        return false;
    }
    
    arglCameraFrustumRH(&(paramLT->param), projectionNearPlane, projectionFarPlane, proj);
    ARLOGd("Computed projection matrix using near=%f far=%f.\n", projectionNearPlane, projectionFarPlane);
    return true;
}

bool ARController::isInited()
{
	// Check we are in a valid state to add a trackable (i.e. base initialisation has occurred)
	return (state != NOTHING_INITIALISED);
}

bool ARController::isRunning()
{
	return state == DETECTION_RUNNING;
}

bool ARController::videoParameters(const int videoSourceIndex, int *width, int *height, AR_PIXEL_FORMAT *pixelFormat)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) return false;

    if (width) *width = vs->getVideoWidth();
    if (height) *height = vs->getVideoHeight();
    if (pixelFormat) *pixelFormat = vs->getPixelFormat();

    return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Trackable list management functions.
// ----------------------------------------------------------------------------------------------------

int ARController::addTrackable(const std::string& cfgs)
{
	if (!isInited()) {
		ARLOGe("Error: Cannot add trackable. artoolkitX not initialised\n");
		return -1;
	}
    
    std::istringstream iss(cfgs);
    std::string token;
    std::vector<std::string> config;
    while (std::getline(iss, token, ';')) {
        config.push_back(token);
    }
    
    // First token is trackable type. Required.
    if (config.size() < 1) {
        ARLOGe("Error: invalid configuration string. Could not find trackable type.\n");
        return -1;
    }
    
    // Until we have a registry, have to manually request from all trackers.
    ARTrackable *trackable;
    if ((trackable = m_squareTracker->newTrackable(config)) != nullptr) {
#if HAVE_NFT
    } else if ((trackable = m_nftTracker->newTrackable(config)) != nullptr) {
#endif
#if HAVE_2D
    } else if ((trackable = m_twoDTracker->newTrackable(config)) != nullptr) {
#endif
    }
    if (!trackable) {
        ARLOGe("Error: Failed to load trackable.\n");
        return -1;
    }
    if (!addTrackable(trackable)) {
        return -1;
    }
    return trackable->UID;
}

// private
bool ARController::addTrackable(ARTrackable* trackable)
{
    ARLOGd("ARController::addTrackable(): called\n");
	if (!isInited()) {
        ARLOGe("Error: Cannot add trackable. artoolkitX not initialised.\n");
		return false;
	}

	if (!trackable) {
        ARLOGe("ARController::addTrackable(): NULL trackable.\n");
		return false;
	}

    m_trackables.push_back(trackable);

#if HAVE_NFT
    if (trackable->type == ARTrackable::NFT) {
        if (!doNFTMarkerDetection)
            ARLOGi("First NFT marker trackable added; enabling NFT marker tracker.\n");
        doNFTMarkerDetection = true;
    } else
#endif
#if HAVE_2D
    if (trackable->type == ARTrackable::TwoD) {
        if (!doTwoDMarkerDetection)
            ARLOGi("First 2D marker trackable added; enabling 2D marker tracker.\n");
        doTwoDMarkerDetection = true;
    } else
#endif
    if (trackable->type == ARTrackable::SINGLE || trackable->type == ARTrackable::MULTI || trackable->type == ARTrackable::MULTI_AUTO) {
        if (!doSquareMarkerDetection)
            ARLOGi("First square marker trackable added; enabling square marker tracker.\n");
        doSquareMarkerDetection = true;
    }

	ARLOGi("Added trackable (UID=%d), total trackables loaded: %d.\n", trackable->UID, countTrackables());
	return true;
}

bool ARController::removeTrackable(int UID)
{
    ARTrackable *trackable = findTrackable(UID);
    if (!trackable) {
        ARLOGe("ARController::removeTrackable(): could not find trackable (UID=%d).\n");
        return (false);
    }
	return removeTrackable(trackable);
}

// private
bool ARController::removeTrackable(ARTrackable* trackable)
{
    ARLOGd("ARController::removeTrackable(): called\n");
	if (!trackable) {
        ARLOGe("ARController::removeTrackable(): NULL trackable.\n");
		return false;
	}

	int UID = trackable->UID;
    std::vector<ARTrackable *>::iterator position = std::find(m_trackables.begin(), m_trackables.end(), trackable);
    bool found = (position != m_trackables.end());
    if (!found) {
        ARLOGe("ARController::removeTrackable(): Could not find trackable (UID=%d).\n", UID);
        return false;
    }

    // Until we have a registry, have to manually request from all trackers.
    m_squareTracker->deleteTrackable(&trackable);
#if HAVE_NFT
    m_nftTracker->deleteTrackable(&trackable);
#endif
#if HAVE_2D
    m_twoDTracker->deleteTrackable(&trackable);
#endif

    // Remove from our trackable list.
    m_trackables.erase(position);

    // Count each type of trackable so we know whether we can disable a given type of tracker.
    if (countTrackables(ARTrackable::SINGLE) + countTrackables(ARTrackable::MULTI) + countTrackables(ARTrackable::MULTI_AUTO) == 0) {
        if (doSquareMarkerDetection)
            ARLOGi("Last square marker removed; disabling square marker tracking.\n");
        doSquareMarkerDetection = false;
    }
#if HAVE_NFT
    if (countTrackables(ARTrackable::NFT) == 0) {
        if (doNFTMarkerDetection)
            ARLOGi("Last NFT marker removed; disabling NFT marker tracking.\n");
        doNFTMarkerDetection = false;
    }
#endif
#if HAVE_2D
    if (countTrackables(ARTrackable::TwoD) == 0) {
        if (doTwoDMarkerDetection)
            ARLOGi("Last 2D marker removed; disabling 2D marker tracking.\n");
        doTwoDMarkerDetection = false;
    }
#endif
    ARLOGi("Removed trackable (UID=%d), now %d trackables loaded\n", UID, countTrackables());

    return (found);
}

int ARController::removeAllTrackables()
{
	unsigned int count = countTrackables();
    
    for (std::vector<ARTrackable *>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
        m_squareTracker->deleteTrackable(&(*it));
#if HAVE_NFT
        m_nftTracker->deleteTrackable(&(*it));
#endif
#if HAVE_2D
        m_twoDTracker->deleteTrackable(&(*it));
#endif
    }
    m_trackables.clear();
    doSquareMarkerDetection = false;
#if HAVE_NFT
    doNFTMarkerDetection = false;
#endif
#if HAVE_2D
    doTwoDMarkerDetection = false;
#endif
	ARLOGi("Removed all %d trackables.\n", count);

	return count;
}

unsigned int ARController::countTrackables() const
{
	return ((unsigned int)m_trackables.size());
}

unsigned int ARController::countTrackables(ARTrackable::TrackableType trackableType) const
{
    int trackableCount = 0;
    for (int i = 0; i < m_trackables.size(); i++) {
        if (m_trackables[i]->type == trackableType) {
            trackableCount++;
        }
    }
    return trackableCount;
}

#if HAVE_2D
bool ARController::load2DTrackerImageDatabase(const char* databaseFileName)
{
    removeAllTrackables();
    std::vector<ARTrackable *> newTrackables = m_twoDTracker->loadImageDatabase(std::string(databaseFileName));
    if (newTrackables.size() == 0) {
        return false;
    }
    for (int i = 0; i < newTrackables.size(); i++) {
        addTrackable(newTrackables[i]);
    }
    return true;
}

bool ARController::save2DTrackerImageDatabase(const char* databaseFileName)
{
    if (m_twoDTracker->saveImageDatabase(std::string(databaseFileName))) {
        return true;
    }
    
    return false;
}
#endif // HAVE_2D

ARTrackable* ARController::getTrackableAtIndex(unsigned int index)
{
    if (index >= m_trackables.size()) return NULL;
    return m_trackables[index];
}

ARTrackable* ARController::findTrackable(int UID)
{

    std::vector<ARTrackable *>::const_iterator it = m_trackables.begin();
	while (it != m_trackables.end()) {
		if ((*it)->UID == UID) return (*it);
        ++it;
	}
	return NULL;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
// ----------------------------------------------------------------------------------------------------

bool ARController::loadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const long optical_param_buffLen, const ARdouble projectionNearPlane, const ARdouble projectionFarPlane, ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16], ARdouble p[16])
{
    if (!fovy_p || !aspect_p || !m) return false;

    // Load the optical parameters.
    if (optical_param_name) {
        if (arParamLoadOptical(optical_param_name, fovy_p, aspect_p, m) < 0) {
            ARLOGe("Error: loading optical parameters from file '%s'.\n", optical_param_name);
            return false;
        }
    } else if (optical_param_buff && optical_param_buffLen) {
        if (arParamLoadOpticalFromBuffer(optical_param_buff, optical_param_buffLen, fovy_p, aspect_p, m) < 0) {
            ARLOGe("Error: loading optical parameters from buffer.\n");
            return false;
        }
    } else return false;

#ifdef DEBUG
    ARPRINT("*** Optical parameters ***\n");
    arParamDispOptical(*fovy_p, *aspect_p, m);
#endif

    if (p) {
        // Create the OpenGL projection from the optical parameters.
        // We are using an optical see-through display, so
        // perspective is determined by its field of view and aspect ratio only.
        // This is the same calculation as performed by:
        // gluPerspective(fovy, aspect, nearPlane, farPlane);
#ifdef ARDOUBLE_IS_FLOAT
        mtxLoadIdentityf(p);
        mtxPerspectivef(p, *fovy_p, *aspect_p, projectionNearPlane, projectionFarPlane);
#else
        mtxLoadIdentityd(p);
        mtxPerspectived(p, *fovy_p, *aspect_p, projectionNearPlane, projectionFarPlane);
#endif
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Android-only API
// ----------------------------------------------------------------------------------------------------

#if ARX_TARGET_PLATFORM_ANDROID
jint ARController::androidVideoPushInit(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;
 
    int ret = -1;
    
    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);;
    if (!vs) {
        ARLOGe("ARController::androidVideoPushInit: no ARVideoSource.\n");
    } else {
        if (!vs->isOpen() || !vs->isRunning()) {
            ret = vs->androidVideoPushInit(env, obj, width, height, pixelFormat, camera_index, camera_face);
        } else {
            ARLOGe("ARController::androidVideoPushInit: ARVideoSource is either closed or already running.\n");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPush1(JNIEnv *env, jobject obj, jint videoSourceIndex, jbyteArray buf, jint bufSize)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPush1: no ARVideoSource.\n");
    } else {
        if (vs->isRunning()) {
            ret = vs->androidVideoPush1(env, obj, buf, bufSize);
        } else {
            ARLOGe("ARController::androidVideoPush1: ARVideoSource is not running.\n");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPush2(JNIEnv *env, jobject obj, jint videoSourceIndex,
                                     jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                                     jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                                     jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                                     jobject buf3, jint buf3PixelStride, jint buf3RowStride)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPush2: no ARVideoSource.\n");
    } else {
        if (vs->isRunning()) {
            ret = vs->androidVideoPush2(env, obj, buf0, buf0PixelStride, buf0RowStride, buf1, buf1PixelStride, buf1RowStride, buf2, buf2PixelStride, buf2RowStride, buf3, buf3PixelStride, buf3RowStride);
        } else {
            ARLOGe("ARController::androidVideoPush2: ARVideoSource is not running.\n");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPushFinal(JNIEnv *env, jobject obj, jint videoSourceIndex)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPushFinal: no ARVideoSource.\n");
    } else {
        if (vs->isOpen()) {
            ret = vs->androidVideoPushFinal(env, obj);
        } else {
            ARLOGe("ARController::androidVideoPushFinal: ARVideoSource is not open.\n");
        }
    }
done:

    return ret;
}
#endif // ARX_TARGET_PLATFORM_ANDROID

