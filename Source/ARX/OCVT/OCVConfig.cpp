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

int minRequiredDetectedFeatures = 50;
int markerTemplateWidth = 15;
int maxLevel = 3; ///< Maximum number of levels in optical flow image pyramid.
const cv::Size subPixWinSize(10,10);
const cv::Size winSize(31,31);
cv::TermCriteria termcrit(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,20,0.03);
const int MAX_COUNT = 300;
int searchRadius = 15;
int match_method = cv::TM_SQDIFF_NORMED;
int featureDetectPyramidLevel = 2; ///> Scale factor applied to image pyramid to determine image to perform feature matching upon.
int defaultDetectorType = 0;
const double nn_match_ratio = 0.8f; ///< Nearest-neighbour matching ratio
const double ransac_thresh = 2.5f; ///< RANSAC inlier threshold
cv::RNG rng( 0xFFFFFFFF );
int harrisBorder = 10;
