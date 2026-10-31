/*
 *  TrackingPointSelector.cpp
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
 *  Author(s): Philip Lamb, Daniel Bell.
 *
 */

#include "TrackingPointSelector.h"

TrackingPointSelector::TrackingPointSelector()
{
}

TrackingPointSelector::TrackingPointSelector(std::vector<cv::Point2f> pts, int width, int height, int markerTemplateWidth, int width0, int height0) :
    _reset(false),
    _pts(pts),
    _width(width),
    _height(height),
    _scalef(cv::Vec2f((float)width0/(float)width, (float)height0/(float)height))
{
    DistributeBins(markerTemplateWidth);
}

/**
    @brief Iterates over \_pts, the location of Harris corners in the image, and for each one,
    provided it doesn't intersect the image border, creates a TrackedPoint representing a template
    with a serially-increasing id from 0, and put it into the trackingPointsBin structure
    (a vector of pairs of (binIndex, trackingPoint)).
 */
void TrackingPointSelector::DistributeBins(int markerTemplateWidth)
{
    int numberOfBins = 10;

    // Split width and height dimensions into 10 bins each, for total of 100 bins.
    int totalXBins = _width/numberOfBins;
    int totalYBins = _height/numberOfBins;
    // Init empty bins.
    for (int i = 0; i < (numberOfBins * numberOfBins); i++) {
        trackingPointBin.insert(std::pair<int, std::vector<TrackedPoint> >(i, std::vector<TrackedPoint>()));
    }
    
    // Iterate the points and add points to each bin.
    for (int i = 0, id = 0; i < _pts.size(); i++) {
        int bx = (int)_pts[i].x/totalXBins;
        int by = (int)_pts[i].y/totalYBins;
        int index = bx + (by * numberOfBins);
        
        cv::Rect templateRoi = cv::Rect(_pts[i].x - markerTemplateWidth, _pts[i].y - markerTemplateWidth, markerTemplateWidth*2, markerTemplateWidth*2);
        bool is_inside = (templateRoi & cv::Rect(0, 0, _width, _height)) == templateRoi; // templateRoi must not intersect image boundary.
        if (is_inside) {
            TrackedPoint newPt;
            newPt.id = id;
            newPt.pt = _pts[i];
            newPt.pt3d = cv::Point3f(_pts[i].x, _pts[i].y, 0);
            newPt.markerRoi = templateRoi;
            trackingPointBin[index].push_back(newPt);
            id++;
        }
    }
}
    
void TrackingPointSelector::UpdatePointStatus(std::vector<uchar> status)
{
    int index = 0;
    for (std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if (it->tracking) {
            it->SetTracking((int)status[index++]);
        }
    }
}

void TrackingPointSelector::ResetSelection()
{
    _reset = true;
}

void TrackingPointSelector::ScaleFeatures(std::vector<cv::Point2f>& features)
{
    if (_scalef[0] == 1.0f && _scalef[1] == 1.0f) return;
    for (auto &f : features) {
        f.x *= _scalef[0];
        f.y *= _scalef[1];
    }
}

void TrackingPointSelector::ScaleFeatures3d(std::vector<cv::Point3f>& features3d)
{
    if (_scalef[0] == 1.0f && _scalef[1] == 1.0f) return;
    for (auto &f : features3d) {
        f.x *= _scalef[0];
        f.y *= _scalef[1];
    }
}

std::vector<cv::Point2f> TrackingPointSelector::GetInitialFeatures()
{
    if (!_reset) return GetTrackedFeatures();
    _reset = false;
    
    // Reset state of all points to not selected and not tracking.
    _selectedPts.clear();
    for (auto &bin : trackingPointBin) {
        for (auto &trackPt : bin.second) {
            trackPt.SetSelected(false);
            trackPt.SetTracking(false);
        }
    }
    
    // Selects a random template from each bin for tracking.
    std::vector<cv::Point2f> ret;
    for (auto &bin : trackingPointBin) {
        size_t pointCount = bin.second.size();
        if (pointCount > 0) { // If there are points in the bin.
            // Select a random point from the bin.
            int tIndex = pointCount > 1 ? rng.uniform(0, static_cast<int>(bin.second.size())) : 0;
            bin.second[tIndex].SetSelected(true);
            bin.second[tIndex].SetTracking(true);
            _selectedPts.push_back(bin.second[tIndex]);
            
            ret.push_back(bin.second[tIndex].pt);
        }
    }
    
    ScaleFeatures(ret);
    return ret;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetTrackedFeatures()
{
    std::vector<cv::Point2f> selectedPoints;
    for (std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if (it->IsTracking()) {
            selectedPoints.push_back(it->pt);
        }
    }
    ScaleFeatures(selectedPoints);
    return selectedPoints;
}
    
std::vector<cv::Point3f> TrackingPointSelector::GetTrackedFeatures3d()
{
    std::vector<cv::Point3f> selectedPoints;
    for (std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if (it->IsTracking()) {
            selectedPoints.push_back(it->pt3d);
        }
    }
    ScaleFeatures3d(selectedPoints);
    return selectedPoints;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetTrackedFeaturesWarped(const cv::Mat& homography)
{
    std::vector<cv::Point2f> selectedPoints = GetTrackedFeatures();
    std::vector<cv::Point2f> warpedPoints;
    perspectiveTransform(selectedPoints, warpedPoints, homography);
    return warpedPoints;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetAllFeatures()
{
    std::vector<cv::Point2f> allBinnedPoints;
    for (auto &track : trackingPointBin) {
        for (auto &trackPt : track.second) {
            allBinnedPoints.push_back(trackPt.pt);
        }
    }
    ScaleFeatures(allBinnedPoints);
    return allBinnedPoints;
}

void TrackingPointSelector::CleanUp()
{
    _selectedPts.clear();
    _pts.clear();
    trackingPointBin.clear();
}
