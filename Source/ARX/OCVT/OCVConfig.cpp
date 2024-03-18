/*
 *  OCVConfig.cpp
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
 *
 *  Author(s): Daniel Bell.
 *
 */

#include "OCVConfig.h"

int minRequiredDetectedFeatures = 50; ///< Minimum number of detected features required to consider a target matched.
int markerTemplateWidth = 15;
int maxLevel = 3; ///< Maximum number of levels in optical flow image pyramid.
const cv::Size subPixWinSize(10,10);
const cv::Size winSize(31,31); ///< Window size to use in optical flow search.
cv::TermCriteria termcrit(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,20,0.03);
const int featureDetectMaxFeatures = 300;
int searchRadius = 15;
int match_method = cv::TM_SQDIFF_NORMED;
const cv::Size featureImageMinSize(640, 480); ///< Minimum size when downscaling incoming images used for feature tracking.
PlanarTracker::FeatureDetectorType defaultDetectorType = PlanarTracker::FeatureDetectorType::Akaze;
const double nn_match_ratio = 0.8f; ///< Nearest-neighbour matching ratio
const double ransac_thresh = 2.5f; ///< RANSAC inlier threshold
cv::RNG rng( 0xFFFFFFFF );
int harrisBorder = 10;
