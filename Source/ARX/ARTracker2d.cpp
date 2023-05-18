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
 *  Author(s): Philip Lamb, Daniel Bell.
 *
 */

#include <ARX/ARTracker2d.h>

#if HAVE_2D
#include <ARX/ARTrackable2d.h>
#include "trackingSub.h"
#include <ARX/OCVT/PlanarTracker.h>
#include <algorithm>

ARTracker2d::ARTracker2d() :
m_pixelFormat(AR_PIXEL_FORMAT_INVALID),
m_sizeX(0),
m_sizeY(0),
m_trackables(),
m_videoSourceIsStereo(false),
m_2DTrackerDataLoaded(false),
m_2DTrackerDetectedImageCount(0),
m_2DTracker(NULL),
m_running(false),
m_pageCount(0),
m_threaded(true),
m_trackingThread(NULL),
m_trackingBuffcopy(NULL)
{
}

ARTracker2d::~ARTracker2d()
{
    stop();
    terminate();
}

bool ARTracker2d::initialize()
{
    if (!m_2DTracker) {
        m_2DTracker = std::make_shared<PlanarTracker>(PlanarTracker());
    }
    
    return true;
}

void ARTracker2d::setMaxMarkersToTrack(int maximumNumberOfMarkersToTrack)
{
    m_2DTracker->SetMaximumNumberOfMarkersToTrack(maximumNumberOfMarkersToTrack);
}

int ARTracker2d::getMaxMarkersToTrack() const
{
    return m_2DTracker->GetMaximumNumberOfMarkersToTrack();
}

bool ARTracker2d::threaded(void) const
{
    return m_threaded;
}

void ARTracker2d::setThreaded(bool threaded)
{
    if (threaded && !m_threaded) {
        m_threaded = threaded;
        if (m_running) {
            m_trackingBuffcopy = (ARUint8 *)malloc(m_sizeX * m_sizeY);
            m_trackingThread = threadInit(0, this, trackingWorker);
        }
    } else if (!threaded && m_threaded) {
        m_threaded = threaded;
        if (m_trackingThread) {
            threadWaitQuit(m_trackingThread);
            threadFree(&m_trackingThread);
        }
        if (m_trackingBuffcopy) {
            free(m_trackingBuffcopy);
            m_trackingBuffcopy = NULL;
        }
    }
}

bool ARTracker2d::start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat)
{
    if (!paramLT || pixelFormat == AR_PIXEL_FORMAT_INVALID) return false;
    m_pixelFormat = pixelFormat;
    m_sizeX = paramLT->param.xsize;
    m_sizeY = paramLT->param.ysize;

    ARLOGd("ARTracker2d::start(): Camera parameters: frame %dx%d, intrinsics:", paramLT->param.xsize, paramLT->param.ysize);
    for (int i = 0; i < 3; i++) {
        ARLOGd("[% 5.3f % 5.3f % 5.3f] [% 6.1f]\n", paramLT->param.mat[i][0], paramLT->param.mat[i][1], paramLT->param.mat[i][2], paramLT->param.mat[i][3]);
    }

    m_2DTracker->Initialise(paramLT->param);
    
    m_videoSourceIsStereo = false;
    m_running = true;

    if (m_threaded) {
        m_trackingBuffcopy = (ARUint8 *)malloc(m_sizeX * m_sizeY);
        m_trackingThread = threadInit(0, this, trackingWorker);
    }

    ARLOGd("ARTracker2d::start(): done.\n");
    return (true);
}

bool ARTracker2d::start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4])
{
    if (!paramLT1 || pixelFormat1 == AR_PIXEL_FORMAT_INVALID || !transL2R) return false;

    if (!start(paramLT0, pixelFormat0)) return false;

    m_videoSourceIsStereo = true;

    return true;
}

bool ARTracker2d::unloadTwoDData(void)
{
    if (m_2DTrackerDataLoaded) {
        m_2DTracker->RemoveAllMarkers();
        m_2DTrackerDataLoaded = false;
        m_pageCount = 0;
    }
    return true;
}

bool ARTracker2d::loadTwoDData()
{
    // If data was already loaded, unload previously loaded data.
    if (m_2DTrackerDataLoaded) {
        ARLOGi("Reloading 2D data.\n");
        unloadTwoDData();
    } else {
        ARLOGi("Loading 2D data.\n");
    }
    for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
        std::shared_ptr<ARTrackable2d> t = std::static_pointer_cast<ARTrackable2d>(*it);
        t->pageNo = m_pageCount;
        // N.B.: PlanarTracker::AddMarker takes a copy of the image data.
        m_2DTracker->AddMarker(t->m_refImage, t->datasetPathname, t->m_refImageX, t->m_refImageY, t->UID, t->TwoDScale());
        ARLOGi("'%s' assigned page no. %d.\n", t->datasetPathname, t->pageNo);
        m_pageCount++; // For 2D tracker, no fixed upper limit on number of trackables that can be loaded.
    }
    
    m_2DTrackerDataLoaded = true;
    
    ARLOGi("Loading of 2D data complete.\n");
    return true;
}

bool ARTracker2d::isRunning()
{
    return m_running;
}

void ARTracker2d::updateTrackablesFromTracker()
{
    // Loop through all loaded 2D targets and match against tracking results.
    for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
        std::shared_ptr<ARTrackable2d> t = std::static_pointer_cast<ARTrackable2d>(*it);
        float transMat[3][4];
        if (m_2DTracker->GetTrackablePose(t->UID, transMat)) {
            ARdouble *transL2R = (m_videoSourceIsStereo ? (ARdouble *)m_transL2R : NULL);
            bool success = t->updateWithTwoDResults(transMat, (ARdouble (*)[4])transL2R);
            m_2DTrackerDetectedImageCount++;
        } else {
            t->updateWithTwoDResults(NULL, NULL);
        }
    }
}

bool ARTracker2d::update(AR2VideoBufferT *buff)
{
    ARLOGd("ARX::ARTracker2d::update()\n");
    if (!buff) return false;

    // Late loading of data now that we have image width and height.
    if (!m_2DTrackerDataLoaded) {
        if (!loadTwoDData()) {
            ARLOGe("Error loading 2D image tracker data.\n");
            return false;
        }
        m_2DTrackerDetectedImageCount = 0;
    }

    if (!m_threaded) {
        m_2DTracker->ProcessFrameData(buff->buffLuma);
        updateTrackablesFromTracker();
    } else {
        // First, see if a frane has been completely processed.
        if (threadGetStatus(m_trackingThread)) {
            threadEndWait(m_trackingThread); // We know from status above that worker has already finished, so this just resets it.
            updateTrackablesFromTracker();
        }

        // If corner finder worker thread is ready and waiting, submit the new image.
        if (!threadGetBusyStatus(m_trackingThread)) {
            // The calling routine expects us to have finished with the frame, so we need to
            // make a copy of it for OCVT.
            memcpy(m_trackingBuffcopy, buff->buffLuma, m_sizeX * m_sizeY);
            // Kick off tracking. The results will be collected on a subsequent cycle.
            threadStartSignal(m_trackingThread);
        }
    }
    return true;
}

// Worker thread.
// static
void *ARTracker2d::trackingWorker(THREAD_HANDLE_T *threadHandle)
{
#ifdef DEBUG
    ARLOGi("Start tracking thread.\n");
#endif

    ARTracker2d *tracker2D = (ARTracker2d *)threadGetArg(threadHandle);

    while (threadStartWait(threadHandle) == 0) {
        // Do tracking.
        tracker2D->m_2DTracker->ProcessFrameData(tracker2D->m_trackingBuffcopy);
        threadEndSignal(threadHandle);
    }

#ifdef DEBUG
    ARLOGi("End tracking thread.\n");
#endif
    return (NULL);
}

bool ARTracker2d::wantsUpdate()
{
    return !m_trackables.empty();
}

bool ARTracker2d::update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1)
{
    return update(buff0);
}

bool ARTracker2d::stop()
{
    if (m_running) {
        if (m_threaded) {
            if (m_trackingThread) {
                threadWaitQuit(m_trackingThread);
                threadFree(&m_trackingThread);
            }
            if (m_trackingBuffcopy) {
                free(m_trackingBuffcopy);
                m_trackingBuffcopy = NULL;
            }
        }
        unloadTwoDData();
        m_running = false;
    }

    return true;
}

void ARTracker2d::terminate()
{
}

int ARTracker2d::newTrackable(std::vector<std::string> config)
{
    // Minimum config length.
    if (config.size() < 1) {
        ARLOGe("Trackable config. must contain at least trackable type.\n");
        return ARTrackable::NO_ID;
    }
    
    // First token is trackable type.
    if (config.at(0).compare("2d") != 0) {
        return ARTrackable::NO_ID;
    }
    
    // Second token is path to 2D data.
    if (config.size() < 2) {
        ARLOGe("2D config. requires path to 2D data.\n");
        return ARTrackable::NO_ID;
    }
    
    // Optional 3rd parameter: scale.
    float scale = 0.0f;
    if (config.size() > 2) {
        scale = strtof(config.at(2).c_str(), NULL);
        if (scale == 0.0f) {
            ARLOGw("2D config. specified with invalid scale parameter ('%s'). Ignoring.\n", config.at(2).c_str());
        }
    }
    
    ARTrackable2d *ret = new ARTrackable2d();
    if (scale != 0.0f) ret->setTwoDScale(scale);
    bool ok = ret->load(config.at(1).c_str());
    if (!ok) {
        // Marker failed to load, or was not added
        delete ret;
        return ARTrackable::NO_ID;
    }

    m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
    // Trigger reload on next tracker update.
    unloadTwoDData();

    return ret->UID;
}

unsigned int ARTracker2d::countTrackables()
{
    return (unsigned int)m_trackables.size();
}

std::shared_ptr<ARTrackable> ARTracker2d::getTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return std::shared_ptr<ARTrackable>();
    }
    return *ti;
}

std::vector<std::shared_ptr<ARTrackable>> ARTracker2d::getAllTrackables()
{
    return std::vector<std::shared_ptr<ARTrackable>>(m_trackables);
}

bool ARTracker2d::deleteTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return false;
    }
    m_trackables.erase(ti);
    unloadTwoDData();
    return true;
}

void ARTracker2d::deleteAllTrackables()
{
    m_trackables.clear();
    unloadTwoDData();
}

bool ARTracker2d::loadImageDatabase(std::string fileName)
{
    deleteAllTrackables();

    if (!m_2DTracker->LoadTrackableDatabase(fileName)) {
        return false;
    } else {
        std::vector<int> loadedIds = m_2DTracker->GetImageIds();
        for (int i = 0; i < loadedIds.size(); i++) {
            //Get loaded image infomation
            int loadedId = loadedIds[i];
            TrackedImageInfo info = m_2DTracker->GetTrackableImageInfo(loadedId);
            
            //Create trackable from loaded info
            std::shared_ptr<ARTrackable2d> t = std::make_shared<ARTrackable2d>();
            t->load2DData(info.fileName.c_str(), info.imageData, info.width, info.height);
            t->setTwoDScale(info.scale);
            t->pageNo = i;
            //Change ID to the Trackable generated UID
            m_2DTracker->ChangeImageId(loadedId, t->UID);
            m_trackables.push_back(t);
        }
    }
    
    if (m_trackables.size() == 0) {
        return false;
    }
    
    m_2DTrackerDataLoaded = true;
    m_pageCount = (int)m_trackables.size();
    return true;
}

bool ARTracker2d::saveImageDatabase(std::string filename)
{
    return m_2DTracker->SaveTrackableDatabase(filename);
}

void ARTracker2d::setDetectorType(int detectorType)
{
    if (unloadTwoDData()) {
        m_2DTracker->SetFeatureDetector(detectorType);
    }
}

int ARTracker2d::getDetectorType(void)
{
    return m_2DTracker->GetFeatureDetector();
}

#endif // HAVE_2D

