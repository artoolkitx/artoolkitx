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

TrackingPointSelector::TrackingPointSelector(std::vector<cv::Point2f> pts, int width, int height, int markerTemplateWidth)
{
    _pts = pts;
    DistributeBins(width, height, markerTemplateWidth);
}

void TrackingPointSelector::DistributeBins(int width, int height, int markerTemplateWidth)
{
    int numberOfBins = 10;

    // Split width and height dimensions into 10 bins each, for total of 100 bins.
    int totalXBins = width/numberOfBins;
    int totalYBins = height/numberOfBins;
    // Init empty bins.
    for(int i=0; i<(numberOfBins * numberOfBins) ; i++) {
        trackingPointBin.insert(std::pair<int, std::vector<TrackedPoint> >(i, std::vector<TrackedPoint>()));
    }
    
    // Iterate the points and add points to each bin.
    for(int i=0, id=0; i<_pts.size(); i++) {
        int bx = (int)_pts[i].x/totalXBins;
        int by = (int)_pts[i].y/totalYBins;
        int index = bx + (by * numberOfBins);
        
        cv::Rect templateRoi = cv::Rect(_pts[i].x-markerTemplateWidth, _pts[i].y-markerTemplateWidth, markerTemplateWidth*2, markerTemplateWidth*2);
        bool is_inside = (templateRoi & cv::Rect(0, 0, width, height)) == templateRoi; // templateRoi must not intersect image boundary.
        if(is_inside) {
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
    
void TrackingPointSelector::SetHomography(cv::Mat newHomography)
{
    homography = newHomography;
}
    
cv::Mat TrackingPointSelector::GetHomography()
{
    return homography;
}
    
void TrackingPointSelector::UpdatePointStatus(std::vector<uchar> status)
{
    int index = 0;
    for(std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if(it->tracking) {
            it->SetTracking((int)status[index++]);
        }
    }
}
    
void TrackingPointSelector::SelectPoints()
{
    _selectedPts.clear();
    for(auto &track : trackingPointBin) {
        for(auto &trackPt : track.second) {
            trackPt.SetSelected(false);
            trackPt.SetTracking(false);
        }
    }
    for(auto &track : trackingPointBin) {
        if(track.second.size()>0) {
            //Get a random idex for this track and choose that point from the points bin
            int tIndex = rng.uniform(0, static_cast<int>(track.second.size()));
            track.second[tIndex].SetSelected(true);
            track.second[tIndex].SetTracking(true);
            _selectedPts.push_back(track.second[tIndex]);
        }
    }
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetSelectedFeatures()
{
    std::vector<cv::Point2f> selectedPoints;
    for(std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if(it->IsSelected()) {
            selectedPoints.push_back(it->pt);
        }
    }
    return selectedPoints;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetTrackedFeatures()
{
    std::vector<cv::Point2f> selectedPoints;
    for(std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if(it->IsTracking()) {
            selectedPoints.push_back(it->pt);
        }
    }
    return selectedPoints;
}
    
std::vector<cv::Point3f> TrackingPointSelector::GetSelectedFeatures3d()
{
    std::vector<cv::Point3f> selectedPoints;
    for(std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if(it->IsTracking()) {
            selectedPoints.push_back(it->pt3d);
        }
    }
    return selectedPoints;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetSelectedFeaturesWarped()
{
    std::vector<cv::Point2f> warpedPoints;
    std::vector<cv::Point2f> selectedPoints;
    for(std::vector<TrackedPoint>::iterator it = _selectedPts.begin(); it != _selectedPts.end(); ++it) {
        if(it->IsTracking()) {
            selectedPoints.push_back(it->pt);
        }
    }
    perspectiveTransform(selectedPoints, warpedPoints, homography);
    return warpedPoints;
}
    
std::vector<cv::Point2f> TrackingPointSelector::GetAllFeatures()
{
    std::vector<cv::Point2f> allBinnedPoints;
    for(auto &track : trackingPointBin) {
        for(auto &trackPt : track.second) {
            allBinnedPoints.push_back(trackPt.pt);
        }
    }
    return allBinnedPoints;
}

void TrackingPointSelector::CleanUp()
{
    _selectedPts.clear();
    _pts.clear();
    trackingPointBin.clear();
    homography.release();
}
