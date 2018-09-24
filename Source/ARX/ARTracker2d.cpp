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

ARTracker2d::ARTracker2d() :
m_videoSourceIsStereo(false),
m_2DTrackerDataLoaded(false),
m_2DTrackerDetectedImageCount(0),
m_2DTracker(NULL),
m_running(false)
{
}

ARTracker2d::~ARTracker2d()
{
    terminate();
}

bool ARTracker2d::initialize()
{
    if (!m_2DTracker) {
        m_2DTracker = std::make_shared<PlanarTracker>(PlanarTracker());
    }
    
    return true;
}

void ARTracker2d::setTwoDMultiMode(bool on)
{
    m_twoDMultiMode = on;
}

bool ARTracker2d::TwoDMultiMode() const
{
    return m_twoDMultiMode;
}

bool ARTracker2d::start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat)
{
    if (!paramLT || pixelFormat == AR_PIXEL_FORMAT_INVALID) return false;
    
    m_cameraXSize = paramLT->param.xsize;
    m_cameraYSize = paramLT->param.ysize;

    m_2DTracker->Initialise(m_cameraXSize,m_cameraYSize, paramLT->param.mat);
    
    m_videoSourceIsStereo = false;
    m_running = true;

    ARLOGd("ARTracker2d::start(): done.\n");
    return (true);
}

bool ARTracker2d::start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4])
{
    if (!paramLT1 || pixelFormat1 == AR_PIXEL_FORMAT_INVALID || !transL2R) return false;
    
    m_videoSourceIsStereo = true;
    m_running = true;
    
    return start(paramLT0, pixelFormat0);
}

bool ARTracker2d::unloadTwoDData(void)
{
    m_2DTracker->RemoveAllMarkers();
    m_2DTrackerDataLoaded = false;
    return true;
}

bool ARTracker2d::loadTwoDData(std::vector<ARTrackable *>& trackables)
{
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    if (m_2DTrackerDataLoaded) {
        ARLOGi("Reloading 2D data.\n");
        unloadTwoDData();
    } else {
        ARLOGi("Loading 2D data.\n");
    }
    int pageCount = 0;
    for (std::vector<ARTrackable *>::iterator it = trackables.begin(); it != trackables.end(); ++it) {
        if ((*it)->type == ARTrackable::TwoD) {
            ((ARTrackable2d *)(*it))->pageNo = pageCount;
            // N.B.: AddMarker takes a copy of the image data.
            m_2DTracker->AddMarker(((ARTrackable2d *)(*it))->m_refImage.get(),((ARTrackable2d *)(*it))->datasetPathname,((ARTrackable2d *)(*it))->m_refImageX,((ARTrackable2d *)(*it))->m_refImageY,((ARTrackable2d *)(*it))->UID, ((ARTrackable2d *)(*it))->TwoDScale());
            ARLOGi("'%s' assigned page no. %d.\n", ((ARTrackable2d *)(*it))->datasetPathname, pageCount);
            pageCount++;
        }
    }
    
    m_2DTrackerDataLoaded = true;
    
    ARLOGi("Loading of 2D data complete.\n");
    return true;
}

bool ARTracker2d::isRunning()
{
    return m_running;
}

bool ARTracker2d::update(AR2VideoBufferT *buff, std::vector<ARTrackable *>& trackables)
{
    ARLOGd("ARX::ARTracker2d::update()\n");
    // Late loading of data now that we have image width and height.
    if (!m_2DTrackerDataLoaded) {
        if (!loadTwoDData(trackables)) {
            ARLOGe("Error loading 2D image tracker data.\n");
            return false;
        }
    }
    
    m_2DTracker->ProcessFrameData(buff->buff);
    // Loop through all loaded 2D targets and match against tracking results.
    m_2DTrackerDetectedImageCount = 0;
    for (std::vector<ARTrackable *>::iterator it = trackables.begin(); it != trackables.end(); ++it) {
        if ((*it)->type == ARTrackable::TwoD) {
            ARTrackable2d *trackable2D = static_cast<ARTrackable2d *>(*it);
            bool trackable2DFound = false;
            if (m_2DTracker->IsTrackableVisible(trackable2D->UID)) {
                float* transMat = m_2DTracker->GetTrackablePose(trackable2D->UID);
                if (transMat) {
                    ARdouble *transL2R = (m_videoSourceIsStereo ? (ARdouble *)m_transL2R : NULL);
                    bool success = ((ARTrackable2d *)(*it))->updateWithTwoDResults(trackable2D->pageNo, (float (*)[4])transMat, (ARdouble (*)[4])transL2R);
                    m_2DTrackerDetectedImageCount++;
                    trackable2DFound = true;
                } else {
                    trackable2D->updateWithTwoDResults(-1, NULL, NULL);
                }
            } else {
                trackable2D->updateWithTwoDResults(-1, NULL, NULL);
            }
        }
    }
    return true;
}

bool ARTracker2d::update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1, std::vector<ARTrackable *>& trackables)
{
    return update(buff0, trackables);
}

bool ARTracker2d::stop()
{
    // Tracking thread is holding a reference to the camera parameters. Closing the
    // video source will dispose of the camera parameters, thus invalidating this reference.
    // So must stop tracking before closing the video source.
    if (m_2DTrackerDataLoaded) {
        unloadTwoDData();
    }
    m_videoSourceIsStereo = false;
    
    return true;
}

void ARTracker2d::terminate()
{
    
}

ARTrackable *ARTracker2d::newTrackable(std::vector<std::string> config)
{
    // Minimum config length.
    if (config.size() < 1) {
        ARLOGe("Trackable config. must contain at least trackable type.\n");
        return nullptr;
    }
    
    // First token is trackable type.
    if (config.at(0).compare("2d") != 0) {
        return nullptr;
    }
    
    // Second token is path to 2D data.
    if (config.size() < 2) {
        ARLOGe("2D config. requires path to 2D data.\n");
        return nullptr;
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
        ret = nullptr;
    } else {
        // Trigger reload on next tracker update.
        unloadTwoDData();
    }
    return ret;
}

void ARTracker2d::deleteTrackable(ARTrackable **trackable_p)
{
    if (!trackable_p || !(*trackable_p)) return;
    if ((*trackable_p)->type != ARTrackable::TwoD) return;

    m_2DTracker->RemoveAllMarkers();
    delete (*trackable_p);
    (*trackable_p) = NULL;
    
    unloadTwoDData();
}

std::vector<ARTrackable*> ARTracker2d::loadImageDatabase(std::string fileName)
{
    std::vector<ARTrackable*> loadedTrackables;
    if (m_2DTracker->LoadTrackableDatabase(fileName)) {
        std::vector<int> loadedIds = m_2DTracker->GetImageIds();
        for (int i = 0; i < loadedIds.size(); i++) {
            //Get loaded image infomation
            int loadedId = loadedIds[i];
            TrackedImageInfo info = m_2DTracker->GetTrackableImageInfo(loadedId);
            
            //Create trackable from loaded info
            ARTrackable2d *ret = new ARTrackable2d();
            ret->load2DData(info.fileName.c_str(), info.imageData, info.width, info.height);
            ret->setTwoDScale(info.scale);
            ret->pageNo = i;
            //Change ID to the Trackable generated UID
            m_2DTracker->ChangeImageId(loadedId, ret->UID);
            loadedTrackables.push_back(ret);
        }
    }
    
    if (loadedTrackables.size() > 0) {
        m_2DTrackerDataLoaded = true;
    }
    return loadedTrackables;
}

bool ARTracker2d::saveImageDatabase(std::string filename)
{
    return m_2DTracker->SaveTrackableDatabase(filename);
}

void ARTracker2d::setDetectorType(int detectorType)
{
    if(unloadTwoDData()) {
        m_2DTracker->SetFeatureDetector(detectorType);
    }
}
#endif // HAVE_2D

