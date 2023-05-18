/*
 *  ARTracker2d.h
 *  artoolkitX
 *
 *  A C++ class implementing the artoolkitX 2d fiducial marker tracker.
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


#ifndef ARTRACKER2D_H
#define ARTRACKER2D_H

#include <ARX/AR/config.h>
#if HAVE_2D

#include <ARX/ARTrackable2d.h>
#include <ARX/ARTrackerVideo.h>
#include <ARX/AR2/tracking.h>
#include <ARX/OCVT/PlanarTracker.h>

class ARTracker2d : public ARTrackerVideo {
public:
    ARTracker2d();
    ~ARTracker2d();
    
    ARTrackerType type() const override {
        return ARTrackerType::TEXTURE2D_FIDUCIAL;
    }
    
    std::vector<std::string> trackableConfigurations() const override {
        std::vector<std::string> sv;
        sv.push_back("2d");
        return sv;
    }
    
    bool initialize() override;
    
    void setMaxMarkersToTrack(int maximumNumberOfMarkersToTrack);
    int getMaxMarkersToTrack() const;
    
    bool start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat) override;
    bool start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4]) override;
    bool isRunning() override;
    bool wantsUpdate() override;
    bool update(AR2VideoBufferT *buff) override;
    bool update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1) override;
    bool stop() override;
    void terminate() override;
    
    int newTrackable(std::vector<std::string> config) override;
    unsigned int countTrackables() override;
    std::shared_ptr<ARTrackable> getTrackable(int UID) override;
    std::vector<std::shared_ptr<ARTrackable>> getAllTrackables() override;
    bool deleteTrackable(int UID) override;
    void deleteAllTrackables() override;

    bool loadImageDatabase(std::string filename);
    bool saveImageDatabase(std::string filename);
    
    void setDetectorType(int detectorType);
    int getDetectorType(void);

    bool threaded(void) const;
    void setThreaded(bool threaded);

private:

    AR_PIXEL_FORMAT m_pixelFormat;
    int m_sizeX;
    int m_sizeY;

    std::vector<std::shared_ptr<ARTrackable>> m_trackables;
    bool m_videoSourceIsStereo;
    bool m_2DTrackerDataLoaded;
    int m_2DTrackerDetectedImageCount;
    std::shared_ptr<PlanarTracker> m_2DTracker;
    // 2d data.
    ARdouble m_transL2R[3][4];          ///< For stereo tracking, transformation matrix from left camera to right camera.
    
    bool m_running;
    bool unloadTwoDData();
    bool loadTwoDData();
    void updateTrackablesFromTracker();
    int m_pageCount;                    ///< Number of loaded pages.

    bool m_threaded;
    THREAD_HANDLE_T     *m_trackingThread;
    static void *trackingWorker(THREAD_HANDLE_T *threadHandle);
    ARUint8 *m_trackingBuffcopy;
};

#endif // HAVE_2D
#endif // !ARTRACKER2D_H

