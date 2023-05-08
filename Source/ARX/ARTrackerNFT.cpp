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

#include <ARX/ARTrackerNFT.h>

#if HAVE_NFT
#include <ARX/ARTrackableNFT.h>
#include "trackingSub.h"
#include <algorithm>

ARTrackerNFT::ARTrackerNFT() :
    m_trackables(),
    m_videoSourceIsStereo(false),
    m_nftMultiMode(false),
    m_kpmRequired(true),
    m_kpmBusy(false),
    trackingThreadHandle(NULL),
    m_ar2Handle(NULL),
    m_kpmHandle(NULL),
    m_surfaceSet{NULL},
    m_pageCount(0)
{
}

ARTrackerNFT::~ARTrackerNFT()
{
    terminate();
}

bool ARTrackerNFT::initialize()
{
    return true;
}

void ARTrackerNFT::setNFTMultiMode(bool on)
{
    m_nftMultiMode = on;
}

bool ARTrackerNFT::NFTMultiMode() const
{
    return m_nftMultiMode;
}

bool ARTrackerNFT::start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat)
{
    if (!paramLT || pixelFormat == AR_PIXEL_FORMAT_INVALID) return false;

    // KPM init.
    m_kpmHandle = kpmCreateHandle(paramLT);
    if (!m_kpmHandle) {
        ARLOGe("kpmCreateHandle\n");
        return (false);
    }
    //kpmSetProcMode( m_kpmHandle, KpmProcHalfSize );
    
    // AR2 init.
    if (!(m_ar2Handle = ar2CreateHandle(paramLT, AR_PIXEL_FORMAT_MONO, AR2_TRACKING_DEFAULT_THREAD_NUM))) { // Since we're guaranteed to have luma available, we'll use it as it is the optimal case.
        ARLOGe("ar2CreateHandle\n");
        kpmDeleteHandle(&m_kpmHandle);
        return (false);
    }
    if (threadGetCPU() <= 1) {
        ARLOGi("Using NFT tracking settings for a single CPU.\n");
        // Settings for devices with single-core CPUs.
        ar2SetTrackingThresh( m_ar2Handle, 5.0 );
        ar2SetSimThresh( m_ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(m_ar2Handle, 16);
        ar2SetSearchSize(m_ar2Handle, 6);
        ar2SetTemplateSize1(m_ar2Handle, 6);
        ar2SetTemplateSize2(m_ar2Handle, 6);
    } else {
        ARLOGi("Using NFT tracking settings for more than one CPU.\n");
        // Settings for devices with dual/multi-core CPUs.
        ar2SetTrackingThresh( m_ar2Handle, 5.0 );
        ar2SetSimThresh( m_ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(m_ar2Handle, 16);
        ar2SetSearchSize(m_ar2Handle, 12);
        ar2SetTemplateSize1(m_ar2Handle, 6);
        ar2SetTemplateSize2(m_ar2Handle, 6);
    }
    ARLOGd("ARTrackerNFT::start(): done.\n");
    return (true);
}

bool ARTrackerNFT::start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4])
{
    if (!paramLT1 || pixelFormat1 == AR_PIXEL_FORMAT_INVALID || !transL2R) return false;
    
    memcpy(m_transL2R, transL2R, sizeof(ARdouble)*12);
    m_videoSourceIsStereo = true;
    
    return start(paramLT0, pixelFormat0);
}

bool ARTrackerNFT::unloadNFTData(void)
{
    int i;
    
    if (trackingThreadHandle) {
        ARLOGi("Stopping NFT tracking thread.\n");
        trackingInitQuit(&trackingThreadHandle);
        m_kpmBusy = false;
    }
    for (i = 0; i < PAGES_MAX; i++) m_surfaceSet[i] = NULL; // Discard weak-references.
    m_kpmRequired = true;
    m_pageCount = 0;
    
    return true;
}

bool ARTrackerNFT::loadNFTData()
{
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    if (trackingThreadHandle) {
        ARLOGi("Reloading NFT data.\n");
        unloadNFTData();
    } else {
        ARLOGi("Loading NFT data.\n");
    }
    
    KpmRefDataSet *refDataSet = NULL;
    
    for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
        std::shared_ptr<ARTrackableNFT> t = std::static_pointer_cast<ARTrackableNFT>(*it);
        // Load KPM data.
        KpmRefDataSet *refDataSet2;
        ARLOGi("Reading '%s.fset3'.\n", t->datasetPathname);
        if (kpmLoadRefDataSet(t->datasetPathname, "fset3", &refDataSet2) < 0) {
            ARLOGe("Error reading KPM data from '%s.fset3'.\n", t->datasetPathname);
            t->pageNo = -1;
            continue;
        }
        t->pageNo = m_pageCount;
        ARLOGi("  Assigned page no. %d.\n", t->pageNo);
        if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, t->pageNo) < 0) {
            ARLOGe("kpmChangePageNoOfRefDataSet\n");
            exit(-1);
        }
        if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
            ARLOGe("kpmMergeRefDataSet\n");
            exit(-1);
        }
        ARLOGi("Done.\n");

        // For convenience, create a weak reference to the AR2 data.
        m_surfaceSet[t->pageNo] = t->surfaceSet;

        m_pageCount++;
        if (m_pageCount == PAGES_MAX) {
            ARLOGe("Maximum number of NFT pages (%d) loaded.\n", PAGES_MAX);
            break;
        }
    }
    if (kpmSetRefDataSet(m_kpmHandle, refDataSet) < 0) {
        ARLOGe("kpmSetRefDataSet\n");
        exit(-1);
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    // Start the KPM tracking thread.
    ARLOGi("Starting NFT tracking thread.\n");
    trackingThreadHandle = trackingInitInit(m_kpmHandle);
    if (!trackingThreadHandle) {
        ARLOGe("trackingInitInit()\n");
        return false;
    }
    
    ARLOGi("Loading of NFT data complete.\n");
    return true;
}

bool ARTrackerNFT::isRunning()
{
    return (bool)(m_kpmHandle && m_ar2Handle);
}

bool ARTrackerNFT::wantsUpdate()
{
    return !m_trackables.empty();
}

bool ARTrackerNFT::update(AR2VideoBufferT *buff)
{
    ARLOGd("ARX::ARTrackerNFT::update()\n");

    if (!m_kpmHandle || !m_ar2Handle) return false;
    
    if (!trackingThreadHandle) {
        loadNFTData();
        if (!trackingThreadHandle) {
            ARLOGe("Unable to load NFT data.\n");
            return false;
        }
    }
    
    if (trackingThreadHandle) {
        
        // Do KPM tracking.
        float err;
        float trackingTrans[3][4];
        
        if (m_kpmRequired) {
            if (!m_kpmBusy) {
                trackingInitStart(trackingThreadHandle, buff->buffLuma);
                m_kpmBusy = true;
            } else {
                int ret;
                int pageNo;
                ret = trackingInitGetResult(trackingThreadHandle, trackingTrans, &pageNo);
                if (ret != 0) {
                    m_kpmBusy = false;
                    if (ret == 1) {
                        if (pageNo >= 0 && pageNo < PAGES_MAX) {
                            if (m_surfaceSet[pageNo]->contNum < 1) {
                                ARLOGd("Detected page %d.\n", pageNo);
                                ar2SetInitTrans(m_surfaceSet[pageNo], trackingTrans); // Sets surfaceSet[page]->contNum = 1.
                            }
                        } else {
                            ARLOGe("Detected page with bad page number %d.\n", pageNo);
                        }
                    } else /*if (ret < 0)*/ {
                        ARLOGd("No page detected.\n");
                    }
                }
            }
        }
        
        // Do AR2 tracking and update NFT markers.
        int page = 0;
        int pagesTracked = 0;
        bool success = true;
        ARdouble *transL2R = (m_videoSourceIsStereo ? (ARdouble *)m_transL2R : NULL);
        
        for (std::vector<std::shared_ptr<ARTrackable>>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
            std::shared_ptr<ARTrackableNFT> t = std::static_pointer_cast<ARTrackableNFT>(*it);

            if (m_surfaceSet[page]->contNum > 0) {
                if (ar2Tracking(m_ar2Handle, m_surfaceSet[page], buff->buffLuma, trackingTrans, &err) < 0) {
                    ARLOGd("Tracking lost on page %d.\n", page);
                    success &= t->updateWithNFTResults(-1, NULL, NULL);
                } else {
                    ARLOGd("Tracked page %d (pos = {% 4f, % 4f, % 4f}).\n", page, trackingTrans[0][3], trackingTrans[1][3], trackingTrans[2][3]);
                    success &= t->updateWithNFTResults(page, trackingTrans, (ARdouble (*)[4])transL2R);
                    pagesTracked++;
                }
            }

            page++;
        }
        
        m_kpmRequired = (pagesTracked < (m_nftMultiMode ? page : 1));
        
    } // trackingThreadHandle

    return true;
}

bool ARTrackerNFT::update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1)
{
    return update(buff0);
}

bool ARTrackerNFT::stop()
{
    // Tracking thread is holding a reference to the camera parameters. Closing the
    // video source will dispose of the camera parameters, thus invalidating this reference.
    // So must stop tracking before closing the video source.
    if (trackingThreadHandle) {
        unloadNFTData();
    }

    // NFT cleanup.
    //ARLOGd("Cleaning up artoolkitX NFT handles.\n");
    if (m_ar2Handle) {
        ar2DeleteHandle(&m_ar2Handle); // Sets m_ar2Handle to NULL.
    }
    if (m_kpmHandle) {
        kpmDeleteHandle(&m_kpmHandle); // Sets m_kpmHandle to NULL.
    }
    
    m_videoSourceIsStereo = false;
    
    return true;
}

void ARTrackerNFT::terminate()
{
    
}

int ARTrackerNFT::newTrackable(std::vector<std::string> config)
{
    // Minimum config length.
    if (config.size() < 1) {
        ARLOGe("Trackable config. must contain at least trackable type.\n");
        return ARTrackable::NO_ID;
    }
    
    // First token is trackable type.
    if (config.at(0).compare("nft") != 0) {
        return ARTrackable::NO_ID;
    }
    
    // Second token is path to NFT data.
    if (config.size() < 2) {
        ARLOGe("NFT config. requires path to NFT data.\n");
        return ARTrackable::NO_ID;
    }
    
    // Optional 3rd parameter: scale.
    float scale = 0.0f;
    if (config.size() > 2) {
        scale = strtof(config.at(2).c_str(), NULL);
        if (scale == 0.0f) {
            ARLOGw("NFT config. specified with invalid scale parameter ('%s'). Ignoring.\n", config.at(2).c_str());
        }
    }
    
    ARTrackableNFT *ret = new ARTrackableNFT();
    if (scale != 0.0f) ret->setNFTScale(scale);
    bool ok = ret->load(config.at(1).c_str());
    if (!ok) {
        // Marker failed to load, or was not added
        delete ret;
        return ARTrackable::NO_ID;
    }

    m_trackables.push_back(std::shared_ptr<ARTrackable>(ret));
    // Trigger reload on next tracker update.
    unloadNFTData();

    return ret->UID;
}

unsigned int ARTrackerNFT::countTrackables()
{
    return (unsigned int)m_trackables.size();
}

std::shared_ptr<ARTrackable> ARTrackerNFT::getTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return std::shared_ptr<ARTrackable>();
    }
    return *ti;
}

std::vector<std::shared_ptr<ARTrackable>> ARTrackerNFT::getAllTrackables()
{
    return std::vector<std::shared_ptr<ARTrackable>>(m_trackables);
}

bool ARTrackerNFT::deleteTrackable(int UID)
{
    auto ti = std::find_if(m_trackables.begin(), m_trackables.end(), [&](std::shared_ptr<ARTrackable> t) { return t->UID == UID; } );
    if (ti == m_trackables.end()) {
        return false;
    }
    m_trackables.erase(ti);
    unloadNFTData();
    return true;
}

void ARTrackerNFT::deleteAllTrackables()
{
    m_trackables.clear();
    unloadNFTData();
}


#endif // HAVE_NFT
