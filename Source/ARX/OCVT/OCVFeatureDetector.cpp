/*
 *  OCVFeatureDetector.cpp
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

#include "OCVFeatureDetector.h"

OCVFeatureDetector::OCVFeatureDetector()
{
}

void OCVFeatureDetector::SetFeatureDetector(int detectorType)
{
    switch (detectorType) {
        case 1:
            CreateORBFeatureDetector();
            break;
        case 2:
            CreateBriskFeatureDetector();
            break;
        case 3:
            CreateKazeFeatureDetector();
            break;
        default:
            CreateAkazeFeatureDetector();
            break;
    }
}

void OCVFeatureDetector::CreateAkazeFeatureDetector()
{
    _featureDetector = cv::AKAZE::create();
    _matcher = cv::BFMatcher::create();
}

void OCVFeatureDetector::CreateBriskFeatureDetector()
{
    _featureDetector = cv::BRISK::create();
    _matcher = cv::BFMatcher::create();
}

void OCVFeatureDetector::CreateKazeFeatureDetector()
{
    _featureDetector = cv::KAZE::create();
    _matcher = cv::BFMatcher::create();
}

void OCVFeatureDetector::CreateORBFeatureDetector()
{
    _featureDetector = cv::ORB::create();
    _matcher = cv::BFMatcher::create();
}

bool OCVFeatureDetector::AddDescriptorsToDictionary(int id, cv::Mat descriptors)
{
    if ( _visualDictionary.find(id) == _visualDictionary.end() ) {
        _visualDictionary.insert(std::pair<int,cv::Mat>(id, descriptors));
        return true;
    }
    return false;
}

std::vector<cv::KeyPoint> OCVFeatureDetector::DetectFeatures(cv::Mat frame, cv::Mat mask)
{
    std::vector<cv::KeyPoint> kp;
    _featureDetector->detect(frame, kp, mask);
    return kp;
}

cv::Mat OCVFeatureDetector::CalcDescriptors(cv::Mat frame, std::vector<cv::KeyPoint> kp)
{
    cv::Mat desc;
    _featureDetector->compute(frame, kp, desc);
    return desc;
}

std::vector< std::vector<cv::DMatch> >  OCVFeatureDetector::MatchFeatures(cv::Mat first_desc, cv::Mat desc)
{
    std::vector< std::vector<cv::DMatch> > matches;
    _matcher->knnMatch(first_desc, desc, matches, 2);
    return matches;
}

