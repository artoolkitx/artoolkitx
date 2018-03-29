/*
 *  OCVFeatureDetector.h
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

#ifndef OCVFEATUREDETECTOR_H
#define OCVFEATUREDETECTOR_H
#include <opencv2/features2d.hpp>
#include <vector>
#include <map>

class OCVFeatureDetector
{
public:
    OCVFeatureDetector();
    
    bool AddDescriptorsToDictionary(int id, cv::Mat descriptors);
    
    std::vector<cv::KeyPoint> DetectFeatures(cv::Mat frame, cv::Mat mask);
    
    cv::Mat CalcDescriptors(cv::Mat frame, std::vector<cv::KeyPoint> kp);
    
    std::vector< std::vector<cv::DMatch> >  MatchFeatures(cv::Mat first_desc, cv::Mat desc);
    
    void SetFeatureDetector(int detectorType);
    
private:
    void CreateAkazeFeatureDetector();
    void CreateORBFeatureDetector();
    void CreateKazeFeatureDetector();
    void CreateBriskFeatureDetector();
    
    std::map<int, cv::Mat> _visualDictionary;
    cv::Ptr<cv::DescriptorMatcher> _matcher;
    cv::Ptr<cv::Feature2D> _featureDetector;
    float _akaze_thresh;
};

#endif //OCVFEATUREDETECTOR
