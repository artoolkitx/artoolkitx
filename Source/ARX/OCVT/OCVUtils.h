/*
 *  OCVUtils.h
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

#ifndef OCV_UTILS_H
#define OCV_UTILS_H
#include "OCVConfig.h"

std::vector<cv::Point2f> Points(std::vector<cv::KeyPoint> keypoints)
{
    std::vector<cv::Point2f> res;
    for(unsigned i = 0; i < keypoints.size(); i++) {
        res.push_back(keypoints[i].pt);
    }
    return res;
}

//Method for calculating and validating a homography matrix from a set of corresponding points.
HomographyInfo GetHomographyInliers(std::vector<cv::Point2f> pts1, std::vector<cv::Point2f> pts2)
{
    cv::Mat inlier_mask, homography;
    std::vector<cv::DMatch> inlier_matches;
    if(pts1.size() >= 4) {
        homography = findHomography(pts1, pts2,
                                    cv::RANSAC, ransac_thresh, inlier_mask);
    }
    
    //Failed to find a homography
    if(pts1.size() < 4 || homography.empty()) {
        return HomographyInfo();
    }
    
    const double det = homography.at<double>(0, 0) * homography.at<double>(1, 1) - homography.at<double>(1, 0) * homography.at<double>(0, 1);
    if (det < 0)
        return HomographyInfo();
    
    const double N1 = sqrt(homography.at<double>(0, 0) * homography.at<double>(0, 0) + homography.at<double>(1, 0) * homography.at<double>(1, 0));
    if (N1 > 4 || N1 < 0.1)
        return HomographyInfo();
    
    const double N2 = sqrt(homography.at<double>(0, 1) * homography.at<double>(0, 1) + homography.at<double>(1, 1) * homography.at<double>(1, 1));
    if (N2 > 4 || N2 < 0.1)
        return HomographyInfo();
    
    const double N3 = sqrt(homography.at<double>(2, 0) * homography.at<double>(2, 0) + homography.at<double>(2, 1) * homography.at<double>(2, 1));
    if (N3 > 0.002)
        return HomographyInfo();
    
    std::vector<uchar> status;
    int linliers = 0;
    for(int i = 0; i < pts1.size(); i++) {
        if((int)inlier_mask.at<uchar>(i,0)==1) {
            status.push_back((uchar)1);
            inlier_matches.push_back(cv::DMatch(i, i, 0));
            linliers++;
        }
        else {
            status.push_back((uchar)0);
        }
    }
    //Return homography and corresponding inlier point sets
    return HomographyInfo(homography, status, inlier_matches);
}

#endif
