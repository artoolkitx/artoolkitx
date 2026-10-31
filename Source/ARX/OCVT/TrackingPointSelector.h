/*
 *  TrackingPointSelector.h
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

#ifndef TRACKINGPOINTSELECTOR_H
#define TRACKINGPOINTSELECTOR_H
#include <vector>
#include <map>
#include "OCVConfig.h"
#include "TrackedPoint.h"

/**
    @brief Class used to manage selection of tracking points based on image templates (i.e. unique pixel patches).</brief>
 */
class TrackingPointSelector
{
public:
    OCV_EXTERN TrackingPointSelector();
    
    /// @param pts Location of Harris corners in the image.
    /// @param width Width of the image in which pts are measured.
    /// @param height Height of the image in which pts are measured.
    /// @param width0 Width of the image at level 0 of the image pyramid. If larger than width (i.e. width does not represent level 0), the output of all Get*Features*() functions will be scaled to level 0 image dimensions.
    /// @param height0 Height of the image at level 0 of the image pyramid. If larger than height (i.e. height does not represent level 0, the output of all Get*Features*() functions will be scaled to level 0 image dimensions.
    OCV_EXTERN TrackingPointSelector(std::vector<cv::Point2f> pts, int width, int height, int markerTemplateWidth, int width0, int height0);
    
    void DistributeBins(int markerTemplateWidth);
    
    void UpdatePointStatus(std::vector<uchar> status);

    /**
     @brief Signal that the next call to GetInitialFeatures should return a new selection.
     */
    void ResetSelection();
    
    /**
     @brief If reset, then selects an initial random template from each bin for tracking,
        and returns this set. If not reset then returns the same set as GetTrackedFeatures.
     */
    std::vector<cv::Point2f> GetInitialFeatures();
    
    std::vector<cv::Point2f> GetTrackedFeatures();
    
    std::vector<cv::Point3f> GetTrackedFeatures3d();
    
    /// @brief Gets the projected location in the video frame of the currently tracked features,
    /// when projected via the supplied homography.
    /// @param 3x3 cv::Mat (of type CV_64FC1, i.e. double) containing the homography.
    std::vector<cv::Point2f> GetTrackedFeaturesWarped(const cv::Mat& homography);

    /// Get all points from all bins that are candidates for selection.
    OCV_EXTERN std::vector<cv::Point2f> GetAllFeatures();
    
    OCV_EXTERN void CleanUp();

private:
    bool _reset;
    int _width;
    int _height;
    std::vector<cv::Point2f> _pts;
    std::map<int, std::vector<TrackedPoint> > trackingPointBin;
    std::vector<TrackedPoint> _selectedPts;
    cv::Vec2f _scalef;
    
    void ScaleFeatures(std::vector<cv::Point2f>& features);
    void ScaleFeatures3d(std::vector<cv::Point3f>& features3d);
};
#endif //TRACKINGPOINTSELECTOR
