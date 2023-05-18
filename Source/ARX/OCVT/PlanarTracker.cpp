/*
 *  PlanarTracker.cpp
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

#include "PlanarTracker.h"

#include "OCVConfig.h"
#include "OCVFeatureDetector.h"
#include "HarrisDetector.h"
#include "TrackableInfo.h"
#include "HomographyInfo.h"
#include "OCVUtils.h"
#include <opencv2/video.hpp>
#include <iostream>
#include <algorithm>

class PlanarTracker::PlanarTrackerImpl
{
private:
    int _maxNumberOfMarkersToTrack;
    OCVFeatureDetector _featureDetector;
    HarrisDetector _harrisDetector;
    std::vector<cv::Mat> _pyramid, _prevPyramid;
    
    std::vector<TrackableInfo> _trackables;
    
    int _currentlyTrackedMarkers;
    int _resetCount;
    int _frameCount;
    int _frameSizeX;
    int _frameSizeY;
    cv::Mat _K;
    cv::Mat _distortionCoeff;

    int _selectedFeatureDetectorType;
public:
    PlanarTrackerImpl()
    {
        _maxNumberOfMarkersToTrack = 1;
        _featureDetector = OCVFeatureDetector();
        SetFeatureDetector(defaultDetectorType);
        _harrisDetector = HarrisDetector();
        _currentlyTrackedMarkers = 0;
        _frameCount = 0;
        _resetCount = 30;
        _frameSizeX = 0;
        _frameSizeY = 0;
        _K = cv::Mat();
        _distortionCoeff = cv::Mat();
    }
    
    void Initialise(ARParam cParam)
    {
        _frameSizeX = cParam.xsize;
        _frameSizeY = cParam.ysize;
        _K = cv::Mat(3,3, CV_64FC1);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                _K.at<double>(i,j) = (double)(cParam.mat[i][j]);
            }
        }

        if (cParam.dist_function_version == 5) {
            // k1,k2,p1,p2,k3,k4,k5,k6,s1,s2,s3,s4.
            _distortionCoeff = cv::Mat::zeros(12, 1, CV_64F);
            for (int i = 0; i < 12; i++) _distortionCoeff.at<double>(i) = cParam.dist_factor[i];
        } else if (cParam.dist_function_version == 4) {
            _distortionCoeff = cv::Mat::zeros(5, 1, CV_64F);
            // k1,k2,p1,p2, and k3=0.
            for (int i = 0; i < 4; i++) _distortionCoeff.at<double>(i) = cParam.dist_factor[i];
            _distortionCoeff.at<double>(4) = 0.0;
        } else {
            ARLOGw("Unsupported camera parameters.\n");
        }
    }
    
    cv::Mat CreateFeatureMask(cv::Mat frame)
    {
        cv::Mat featureMask;
        for (int i = 0; i < _trackables.size(); i++) {
            if (_trackables[i]._isDetected) {
                if (featureMask.empty()) {
                    //Only create mask if we have something to draw in it.
                    featureMask = cv::Mat::ones(frame.size(), CV_8UC1);
                }
                std::vector<std::vector<cv::Point> > contours(1);
                for (int j = 0; j < 4; j++) {
                    contours[0].push_back(cv::Point(_trackables[i]._bBoxTransformed[j].x/featureDetectPyramidLevel,_trackables[i]._bBoxTransformed[j].y/featureDetectPyramidLevel));
                }
                drawContours(featureMask, contours, 0, cv::Scalar(0), -1, 8);
            }
        }
        return featureMask;
    }
    
    bool CanDetectNewFeatures()
    {
        return (_currentlyTrackedMarkers < _maxNumberOfMarkersToTrack);
    }
    
    bool CanMatchNewFeatures(int detectedFeaturesSize)
    {
        return (detectedFeaturesSize>minRequiredDetectedFeatures);
    }
    
    void MatchFeatures(std::vector<cv::KeyPoint> newFrameFeatures, cv::Mat newFrameDescriptors)
    {
        int maxMatches = 0;
        int bestMatchIndex = -1;
        std::vector<cv::KeyPoint> finalMatched1, finalMatched2;
        for (int i = 0; i < _trackables.size(); i++) {
            if (!_trackables[i]._isDetected) {
                std::vector< std::vector<cv::DMatch> >  matches = _featureDetector.MatchFeatures(newFrameDescriptors, _trackables[i]._descriptors);
                if (matches.size()>minRequiredDetectedFeatures) {
                    std::vector<cv::KeyPoint> matched1, matched2;
                    std::vector<uchar> status;
                    int totalGoodMatches = 0;
                    for (unsigned int j = 0; j < matches.size(); j++) {
                        //Ratio Test for outlier removal, removes ambiguous matches.
                        if (matches[j][0].distance < nn_match_ratio * matches[j][1].distance) {
                            matched1.push_back(newFrameFeatures[matches[j][0].queryIdx]);
                            matched2.push_back(_trackables[i]._featurePoints[matches[j][0].trainIdx]);
                            status.push_back(1);
                            totalGoodMatches++;
                        } else {
                            status.push_back(0);
                        }
                    }
                    if (totalGoodMatches>maxMatches) {
                        finalMatched1 = matched1;
                        finalMatched2 = matched2;
                        maxMatches = totalGoodMatches;
                        bestMatchIndex = i;
                    }
                }
            }
        }
        
        if (maxMatches > 0) {
            for (int i = 0; i < finalMatched1.size(); i++) {
                finalMatched1[i].pt.x *=featureDetectPyramidLevel;
                finalMatched1[i].pt.y *=featureDetectPyramidLevel;
            }
            
            HomographyInfo homoInfo = GetHomographyInliers(Points(finalMatched2), Points(finalMatched1));
            if (homoInfo.validHomography) {
                //std::cout << "New marker detected" << std::endl;
                _trackables[bestMatchIndex]._trackSelection.SelectPoints();
                _trackables[bestMatchIndex]._trackSelection.SetHomography(homoInfo.homography);
                _trackables[bestMatchIndex]._isDetected = true;
                _trackables[bestMatchIndex]._resetTracks = true;
                
                perspectiveTransform(_trackables[bestMatchIndex]._bBox, _trackables[bestMatchIndex]._bBoxTransformed, homoInfo.homography);
                _currentlyTrackedMarkers++;
            }
        }
    }
    
    std::vector<cv::Point2f> SelectTrackablePoints(int trackableIndex)
    {
        if (_trackables[trackableIndex]._resetTracks) {
            _trackables[trackableIndex]._trackSelection.SelectPoints();
            _trackables[trackableIndex]._resetTracks = false;
            return _trackables[trackableIndex]._trackSelection.GetSelectedFeatures();
        } else {
            return _trackables[trackableIndex]._trackSelection.GetTrackedFeatures();
        }
    }
    
    void RunOpticalFlow(int trackableId, std::vector<cv::Point2f> trackablePoints, std::vector<cv::Point2f> trackablePointsWarped)
    {
        std::vector<cv::Point2f> flowResultPoints, trackablePointsWarpedResult;
        std::vector<uchar> statusFirstPass, statusSecondPass;
        std::vector<float> err;
        cv::calcOpticalFlowPyrLK(_prevPyramid, _pyramid, trackablePointsWarped, flowResultPoints, statusFirstPass, err, winSize, 3, termcrit, 0, 0.001);
        cv::calcOpticalFlowPyrLK(_pyramid, _prevPyramid, flowResultPoints, trackablePointsWarpedResult, statusSecondPass, err, winSize, 3, termcrit, 0, 0.001);
        
        int killed1 =0;
        std::vector<cv::Point2f> filteredTrackablePoints, filteredTrackedPoints;
        for (auto j = 0; j != flowResultPoints.size(); ++j) {
            if (!statusFirstPass[j] || !statusSecondPass[j]) {
                statusFirstPass[j] = (uchar)0;
                killed1++;
                continue;
            }
            filteredTrackablePoints.push_back(trackablePoints[j]);
            filteredTrackedPoints.push_back(flowResultPoints[j]);
        }
        if (UpdateTrackableHomography(trackableId, filteredTrackablePoints, filteredTrackedPoints)) {
            _trackables[trackableId]._isTracking = true;
        } else {
            _trackables[trackableId]._isDetected = false;
            _trackables[trackableId]._isTracking = false;
            _currentlyTrackedMarkers--;
        }
    }
    
    bool UpdateTrackableHomography(int trackableId, std::vector<cv::Point2f> matchedPoints1, std::vector<cv::Point2f> matchedPoints2)
    {
        if (matchedPoints1.size()>4) {
            HomographyInfo homoInfo = GetHomographyInliers(matchedPoints1, matchedPoints2);
            if (homoInfo.validHomography) {
                _trackables[trackableId]._trackSelection.UpdatePointStatus(homoInfo.status);
                _trackables[trackableId]._trackSelection.SetHomography(homoInfo.homography);
                perspectiveTransform(_trackables[trackableId]._bBox, _trackables[trackableId]._bBoxTransformed, homoInfo.homography);
                if (_frameCount > 1) {
                    _trackables[trackableId]._resetTracks = true;
                }
                return true;
            }
        }
        return false;
    }
    
    std::vector<cv::Point2f> GetVerticesFromPoint(cv::Point ptOrig, int width = markerTemplateWidth, int height = markerTemplateWidth)
    {
        std::vector<cv::Point2f> vertexPoints;
        vertexPoints.push_back(cv::Point2f(ptOrig.x - width/2, ptOrig.y - height/2));
        vertexPoints.push_back(cv::Point2f(ptOrig.x + width/2, ptOrig.y - height/2));
        vertexPoints.push_back(cv::Point2f(ptOrig.x + width/2, ptOrig.y + height/2));
        vertexPoints.push_back(cv::Point2f(ptOrig.x - width/2, ptOrig.y + height/2));
        return vertexPoints;
    }
    
    std::vector<cv::Point2f> GetVerticesFromTopCorner(int x, int y, int width, int height)
    {
        std::vector<cv::Point2f> vertexPoints;
        vertexPoints.push_back(cv::Point2f(x, y));
        vertexPoints.push_back(cv::Point2f(x + width, y));
        vertexPoints.push_back(cv::Point2f(x + width, y + height));
        vertexPoints.push_back(cv::Point2f(x, y + height));
        return vertexPoints;
    }
    
    cv::Rect GetTemplateRoi(cv::Point2f pt)
    {
        return cv::Rect(pt.x-(markerTemplateWidth/2), pt.y-(markerTemplateWidth/2), markerTemplateWidth, markerTemplateWidth);
    }
    
    bool IsRoiValidForFrame(cv::Rect frameRoi, cv::Rect roi)
    {
        return (roi & frameRoi) == roi;
    }
    
    cv::Rect InflateRoi(cv::Rect roi, int inflationFactor)
    {
        cv::Rect newRoi = roi;
        newRoi.x -= inflationFactor;
        newRoi.y -= inflationFactor;
        newRoi.width += 2 * inflationFactor;
        newRoi.height += 2 * inflationFactor;
        return newRoi;
    }
    
    std::vector<cv::Point2f> FloorVertexPoints(std::vector<cv::Point2f> vertexPoints)
    {
        std::vector<cv::Point2f> testVertexPoints = vertexPoints;
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        for (int k = 0; k < testVertexPoints.size(); k++) {
            if (testVertexPoints[k].x < minX) {
                minX=testVertexPoints[k].x;
            }
            if (testVertexPoints[k].y < minY) {
                minY=testVertexPoints[k].y;
            }
        }
        for(int k = 0; k < testVertexPoints.size(); k++) {
            testVertexPoints[k].x -= minX;
            testVertexPoints[k].y -= minY;
        }
        return testVertexPoints;
    }
    
    cv::Mat MatchTemplateToImage(cv::Mat searchImage, cv::Mat warpedTemplate)
    {
        int result_cols =  searchImage.cols - warpedTemplate.cols + 1;
        int result_rows = searchImage.rows - warpedTemplate.rows + 1;
        if (result_cols > 0 && result_rows > 0) {
            cv::Mat result;
            result.create( result_rows, result_cols, CV_32FC1 );
            
            double minVal; double maxVal;
            minMaxLoc(warpedTemplate, &minVal, &maxVal, 0, 0, cv::Mat());
            
            cv::Mat normSeatchROI;
            normalize(searchImage, normSeatchROI, minVal, maxVal, cv::NORM_MINMAX, -1, cv::Mat());
            /// Do the Matching and Normalize
            matchTemplate(normSeatchROI, warpedTemplate, result, match_method);
            return result;
        }
        else {
            //std::cout << "Results image too small" << std::endl;
            return cv::Mat();
        }
    }
    
    void RunTemplateMatching(cv::Mat frame, int trackableId)
    {
        //std::cout << "Starting template match" << std::endl;
        std::vector<cv::Point2f> finalTemplatePoints, finalTemplateMatchPoints;
        //Get a handle on the corresponding points from current image and the marker
        std::vector<cv::Point2f> trackablePoints = _trackables[trackableId]._trackSelection.GetTrackedFeatures();
        std::vector<cv::Point2f> trackablePointsWarped = _trackables[trackableId]._trackSelection.GetSelectedFeaturesWarped();
        //Create an empty result image - May be able to pre-initialize this container
        
        for(int j=0; j<trackablePointsWarped.size();j++) {
            auto pt = trackablePointsWarped[j];
            if (cv::pointPolygonTest( _trackables[trackableId]._bBoxTransformed, trackablePointsWarped[j], true )>0) {
                auto ptOrig = trackablePoints[j];
                
                cv::Rect templateRoi = GetTemplateRoi(pt);
                cv::Rect frameROI(0, 0, frame.cols, frame.rows);
                if (IsRoiValidForFrame(frameROI, templateRoi)) {
                    cv::Rect markerRoi(0, 0, _trackables[trackableId]._image.cols, _trackables[trackableId]._image.rows);
                    
                    std::vector<cv::Point2f> vertexPoints = GetVerticesFromPoint(ptOrig);
                    std::vector<cv::Point2f> vertexPointsResults;
                    perspectiveTransform(vertexPoints, vertexPointsResults, _trackables[trackableId]._trackSelection.GetHomography());
                    
                    cv::Rect srcBoundingBox = cv::boundingRect(cv::Mat(vertexPointsResults));
                    
                    vertexPoints.clear();
                    vertexPoints = GetVerticesFromTopCorner(srcBoundingBox.x, srcBoundingBox.y, srcBoundingBox.width, srcBoundingBox.height);
                    perspectiveTransform(vertexPoints, vertexPointsResults, _trackables[trackableId]._trackSelection.GetHomography().inv());
                    
                    std::vector<cv::Point2f> testVertexPoints = FloorVertexPoints(vertexPointsResults);
                    std::vector<cv::Point2f> finalWarpPoints = GetVerticesFromTopCorner(0, 0, srcBoundingBox.width, srcBoundingBox.height);
                    cv::Mat templateHomography = findHomography(testVertexPoints, finalWarpPoints, cv::RANSAC, ransac_thresh);
                    
                    if (!templateHomography.empty()) {
                        cv::Rect templateBoundingBox = cv::boundingRect(cv::Mat(vertexPointsResults));
                        cv::Rect searchROI = InflateRoi(templateRoi, searchRadius);
                        if (IsRoiValidForFrame(frameROI, searchROI)) {
                            searchROI = searchROI & frameROI;
                            templateBoundingBox = templateBoundingBox & markerRoi;
                            
                            if (templateBoundingBox.area() > 0 && searchROI.area() > templateBoundingBox.area()) {
                                cv::Mat searchImage = frame(searchROI);
                                cv::Mat templateImage = _trackables[trackableId]._image(templateBoundingBox);
                                cv::Mat warpedTemplate;
                                
                                warpPerspective(templateImage, warpedTemplate, templateHomography, srcBoundingBox.size());
                                cv::Mat matchResult =  MatchTemplateToImage(searchImage, warpedTemplate);
                                
                                if (!matchResult.empty()) {
                                    double minVal; double maxVal;
                                    cv::Point minLoc, maxLoc, matchLoc;
                                    minMaxLoc( matchResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
                                    if (minVal < 0.5) {
                                        matchLoc = minLoc;
                                        matchLoc.x+=searchROI.x + (warpedTemplate.cols/2);
                                        matchLoc.y+=searchROI.y + (warpedTemplate.rows/2);
                                        finalTemplatePoints.push_back(ptOrig);
                                        finalTemplateMatchPoints.push_back(matchLoc);
                                    }
                                }
                            }
                            else {
                                //std::cout << "ROIs not good" << std::endl;
                            }
                        }
                    }
                    else {
                        //std::cout << "Empty homography" << std::endl;
                    }
                }
            }
        }
        if (!UpdateTrackableHomography(trackableId, finalTemplatePoints, finalTemplateMatchPoints)) {
            _trackables[trackableId]._isTracking = false;
            _trackables[trackableId]._isDetected = false;
            _currentlyTrackedMarkers--;
        }
    }
    
    void BuildImagePyramid(cv::Mat frame)
    {
        cv::buildOpticalFlowPyramid(frame, _pyramid, winSize, maxLevel);
    }
    
    void SwapImagePyramid()
    {
        _pyramid.swap(_prevPyramid);
    }
    
    void ProcessFrameData(unsigned char * frame)
    {
        // Just wraps `frame` rather than copying it, i.e. `frame` must remain valid
        // for the duration of the call.
        cv::Mat newFrame(_frameSizeY, _frameSizeX, CV_8UC1, frame);
        ProcessFrame(newFrame);
        newFrame.release();
    }
    
    void ProcessFrame(cv::Mat frame)
    {
        //std::cout << "Building pyramid" << std::endl;
        BuildImagePyramid(frame);
        //std::cout << "Drawing detected markers to mask" << std::endl;
        if (CanDetectNewFeatures()) {
            //std::cout << "Detecting new features" << std::endl;
            cv::Mat detectionFrame;
            cv::pyrDown(frame, detectionFrame, cv::Size(frame.cols/featureDetectPyramidLevel, frame.rows/featureDetectPyramidLevel));
            cv::Mat featureMask = CreateFeatureMask(detectionFrame);
            std::vector<cv::KeyPoint> newFrameFeatures = _featureDetector.DetectFeatures(detectionFrame, featureMask);
            
            if (CanMatchNewFeatures(static_cast<int>(newFrameFeatures.size()))) {
                //std::cout << "Matching new features" << std::endl;
                cv::Mat newFrameDescriptors = _featureDetector.CalcDescriptors(detectionFrame, newFrameFeatures);
                MatchFeatures(newFrameFeatures, newFrameDescriptors);
            }
        }
        if (_frameCount>0)
        {
            if ((_currentlyTrackedMarkers>0) && (_prevPyramid.size()>0)) {
                //std::cout << "Begin tracking phase" << std::endl;
                for (int i = 0; i <_trackables.size(); i++) {
                    if (_trackables[i]._isDetected) {
                        std::vector<cv::Point2f> trackablePoints = SelectTrackablePoints(i);
                        std::vector<cv::Point2f> trackablePointsWarped = _trackables[i]._trackSelection.GetSelectedFeaturesWarped();
                        //std::cout << "Starting Optical Flow" << std::endl;
                        RunOpticalFlow(i, trackablePoints, trackablePointsWarped);
                        if (_trackables[i]._isTracking) {
                            //Refine optical flow with template match.
                            RunTemplateMatching(frame, i);
                        }
                    }
                }
            }
        }
        for (auto&& t : _trackables) {
            if (t._isDetected || t._isTracking) {
                
                std::vector<cv::Point2f> imgPoints = t._trackSelection.GetSelectedFeaturesWarped();
                std::vector<cv::Point3f> objPoints = t._trackSelection.GetSelectedFeatures3d();
                
                CameraPoseFromPoints(t._pose, objPoints, imgPoints);
            }
        }
        SwapImagePyramid();
        _frameCount++;
    }
    
    void RemoveAllMarkers()
    {
        for (auto&& t : _trackables) {
            t.CleanUp();
        }
        _trackables.clear();
    }
    
    bool SaveTrackableDatabase(std::string fileName)
    {
        bool success = false;
        cv::FileStorage fs;
        fs.open(fileName, cv::FileStorage::WRITE);
        if (fs.isOpened())
        {
            try {
                int totalTrackables = (int)_trackables.size();
                fs << "totalTrackables" << totalTrackables;
                fs << "featureType" << _selectedFeatureDetectorType;
                for (int i = 0; i <_trackables.size(); i++) {
                    std::string index = std::to_string(i);
                    fs << "trackableId" + index << _trackables[i]._id;
                    fs << "trackableFileName" + index << _trackables[i]._fileName;
                    fs << "trackableScale" + index << _trackables[i]._scale;
                    fs << "trackableImage" + index << _trackables[i]._image;
                    fs << "trackableWidth" + index << _trackables[i]._width;
                    fs << "trackableHeight" + index << _trackables[i]._height;
                    fs << "trackableDescriptors" + index << _trackables[i]._descriptors;
                    fs << "trackableFeaturePoints" + index << _trackables[i]._featurePoints;
                    fs << "trackableCornerPoints" + index << _trackables[i]._cornerPoints;
                }
                success = true;
            } catch (std::exception e) {
                ARLOGe("Error: Something went wrong while writing trackable database to path '%s'.\n", fileName.c_str());
            }
        }
        else
        {
            ARLOGe("Error: Could not create new trackable database at path '%s'.\n", fileName.c_str());
        }
        fs.release();
        return success;
    }
    
    bool LoadTrackableDatabase(std::string fileName)
    {
        bool success = false;
        cv::FileStorage fs;
        fs.open(fileName, cv::FileStorage::READ);

        if (fs.isOpened())
        {
            try {
                int numberOfTrackables = (int) fs["totalTrackables"];
                int featureType = defaultDetectorType;
                fs["featureType"] >> featureType;
                SetFeatureDetector(featureType);
                for(int i=0;i<numberOfTrackables; i++) {
                    TrackableInfo newTrackable;
                    std::string index = std::to_string(i);
                    fs["trackableId" + index] >> newTrackable._id;
                    fs["trackableFileName" + index] >> newTrackable._fileName;
                    fs["trackableScale" + index] >> newTrackable._scale;
                    fs["trackableImage" + index] >> newTrackable._image;
                    fs["trackableWidth" + index] >> newTrackable._width;
                    fs["trackableHeight" + index] >> newTrackable._height;
                    fs["trackableDescriptors" + index] >> newTrackable._descriptors;
                    fs["trackableFeaturePoints" + index] >> newTrackable._featurePoints;
                    fs["trackableCornerPoints" + index] >> newTrackable._cornerPoints;
                    newTrackable._bBox.push_back(cv::Point2f(0,0));
                    newTrackable._bBox.push_back(cv::Point2f(newTrackable._width, 0));
                    newTrackable._bBox.push_back(cv::Point2f(newTrackable._width, newTrackable._height));
                    newTrackable._bBox.push_back(cv::Point2f(0, newTrackable._height));
                    newTrackable._isTracking = false;
                    newTrackable._isDetected = false;
                    newTrackable._resetTracks = false;
                    newTrackable._trackSelection = TrackingPointSelector(newTrackable._cornerPoints, newTrackable._width, newTrackable._height, markerTemplateWidth);
                    _trackables.push_back(newTrackable);
                }
                success = true;
            } catch(std::exception e) {
                ARLOGe("Error: Something went wrong while reading trackable database from path '%s'.\n", fileName.c_str());
            }
        }
        else
        {
            ARLOGe("Error: Could not open trackable database from path '%s'.\n", fileName.c_str());
        }
        fs.release();
        return success;
    }
    
    void AddMarker(std::shared_ptr<unsigned char> buff, std::string fileName, int width, int height, int uid, float scale)
    {
        TrackableInfo newTrackable;
        // cv::Mat() wraps `buff` rather than copying it, but this is OK as we share ownership with caller via the shared_ptr.
        newTrackable._imageBuff = buff;
        newTrackable._image = cv::Mat(height, width, CV_8UC1, buff.get());
        if (!newTrackable._image.empty()) {
            newTrackable._id = uid;
            newTrackable._fileName = fileName;
            newTrackable._scale = scale;
            newTrackable._width = newTrackable._image.cols;
            newTrackable._height = newTrackable._image.rows;
            newTrackable._featurePoints = _featureDetector.DetectFeatures(newTrackable._image, cv::Mat());
            newTrackable._descriptors = _featureDetector.CalcDescriptors(newTrackable._image, newTrackable._featurePoints);
            newTrackable._cornerPoints = _harrisDetector.FindCorners(newTrackable._image);
            newTrackable._bBox.push_back(cv::Point2f(0,0));
            newTrackable._bBox.push_back(cv::Point2f(newTrackable._width, 0));
            newTrackable._bBox.push_back(cv::Point2f(newTrackable._width, newTrackable._height));
            newTrackable._bBox.push_back(cv::Point2f(0, newTrackable._height));
            newTrackable._isTracking = false;
            newTrackable._isDetected = false;
            newTrackable._resetTracks = false;
            newTrackable._trackSelection = TrackingPointSelector(newTrackable._cornerPoints, newTrackable._width, newTrackable._height, markerTemplateWidth);
            
            _trackables.push_back(newTrackable);
            ARLOGi("2D marker added.\n");
        }
    }

    bool GetTrackablePose(int trackableId, float transMat[3][4])
    {
        auto t = std::find_if(_trackables.begin(), _trackables.end(), [&](const TrackableInfo& e) { return e._id == trackableId; });
        if (t != _trackables.end()) {
            if (t->_isDetected || t->_isTracking) {
                cv::Mat poseOut;
                t->_pose.convertTo(poseOut, CV_32FC1);
                //std::cout << "poseOut: " << poseOut << std::endl;
                memcpy(transMat, poseOut.ptr<float>(0), 3*4*sizeof(float));
                return true;
            }
        }
        return false;
    }
    
    bool IsTrackableVisible(int trackableId)
    {
        auto t = std::find_if(_trackables.begin(), _trackables.end(), [&](const TrackableInfo& e) { return e._id == trackableId; });
        if (t != _trackables.end()) {
            return (t->_isDetected || t->_isTracking);
        }
        return false;
    }
    
    void CameraPoseFromPoints(cv::Mat& pose, std::vector<cv::Point3f> objPts, std::vector<cv::Point2f> imgPts)
    {
        cv::Mat rvec = cv::Mat::zeros(3, 1, CV_64FC1);          // output rotation vector
        cv::Mat tvec = cv::Mat::zeros(3, 1, CV_64FC1);          // output translation vector
        
        cv::solvePnPRansac(objPts, imgPts, _K, _distortionCoeff, rvec, tvec);
        
        cv::Mat rMat;
        Rodrigues(rvec,rMat);
        cv::hconcat(rMat,tvec, pose);
    }
    
    
    bool HasTrackables()
    {
        if (_trackables.size() > 0) {
            return true;
        }
        return false;
    }
    
    bool ChangeImageId(int prevId, int newId)
    {
        auto t = std::find_if(_trackables.begin(), _trackables.end(), [&](const TrackableInfo& e) { return e._id == prevId; });
        if (t != _trackables.end()) {
            t->_id = newId;
            return true;
        }
        return false;
    }

    std::vector<int> GetImageIds()
    {
        std::vector<int> imageIds;
        for (int i=0;i<_trackables.size(); i++) {
            imageIds.push_back(_trackables[i]._id);
        }
        return imageIds;
    }

    TrackedImageInfo GetTrackableImageInfo(int trackableId)
    {
        TrackedImageInfo info;
        auto t = std::find_if(_trackables.begin(), _trackables.end(), [&](const TrackableInfo& e) { return e._id == trackableId; });
        if (t != _trackables.end()) {
            info.uid = t->_id;
            info.scale = t->_scale;
            info.fileName = t->_fileName;
            // Copy the image data and use a shared_ptr to refer to it.
            unsigned char *data = (unsigned char *)malloc(t->_width * t->_height);
            memcpy(data, t->_image.ptr(), t->_width * t->_height);
            info.imageData.reset(data, free);
            info.width = t->_width;
            info.height = t->_height;
            info.fileName = t->_fileName;
        }
        return info;
    }
    
    void SetFeatureDetector(int detectorType)
    {
        _selectedFeatureDetectorType = detectorType;
        _featureDetector.SetFeatureDetector(detectorType);
    }

    int GetFeatureDetector(void)
    {
        return _selectedFeatureDetectorType;
    }

    void SetMaximumNumberOfMarkersToTrack(int maximumNumberOfMarkersToTrack)
    {
        if (_maxNumberOfMarkersToTrack > 0) {
            _maxNumberOfMarkersToTrack = maximumNumberOfMarkersToTrack;
        }
    }

    int GetMaximumNumberOfMarkersToTrack(void)
    {
        return _maxNumberOfMarkersToTrack;
    }
};

PlanarTracker::PlanarTracker() : _trackerImpl(new PlanarTrackerImpl())
{
}

PlanarTracker::~PlanarTracker() = default;
PlanarTracker::PlanarTracker(PlanarTracker&&) = default;
PlanarTracker& PlanarTracker::operator=(PlanarTracker&&) = default;

void PlanarTracker::Initialise(ARParam cParam)
{
    _trackerImpl->Initialise(cParam);
}

void PlanarTracker::ProcessFrameData(unsigned char * frame)
{
    _trackerImpl->ProcessFrameData(frame);
}

void PlanarTracker::RemoveAllMarkers()
{
    _trackerImpl->RemoveAllMarkers();
}

void PlanarTracker::AddMarker(std::shared_ptr<unsigned char> buff, std::string fileName, int width, int height, int uid, float scale)
{
    _trackerImpl->AddMarker(buff, fileName, width, height, uid, scale);
}

bool PlanarTracker::GetTrackablePose(int trackableId, float transMat[3][4])
{
    return _trackerImpl->GetTrackablePose(trackableId, transMat);
}

bool PlanarTracker::IsTrackableVisible(int trackableId)
{
    return _trackerImpl->IsTrackableVisible(trackableId);
}

bool PlanarTracker::LoadTrackableDatabase(std::string fileName)
{
    return _trackerImpl->LoadTrackableDatabase(fileName);
}
bool PlanarTracker::SaveTrackableDatabase(std::string fileName)
{
    return _trackerImpl->SaveTrackableDatabase(fileName);
}

bool PlanarTracker::ChangeImageId(int prevId, int newId)
{
    return _trackerImpl->ChangeImageId(prevId, newId);
}
std::vector<int> PlanarTracker::GetImageIds()
{
    return _trackerImpl->GetImageIds();
}

TrackedImageInfo PlanarTracker::GetTrackableImageInfo(int trackableId)
{
    return _trackerImpl->GetTrackableImageInfo(trackableId);
}

void PlanarTracker::SetFeatureDetector(int detectorType)
{
    _trackerImpl->SetFeatureDetector(detectorType);
}

int PlanarTracker::GetFeatureDetector(void)
{
    return _trackerImpl->GetFeatureDetector();
}

void PlanarTracker::SetMaximumNumberOfMarkersToTrack(int maximumNumberOfMarkersToTrack)
{
    _trackerImpl->SetMaximumNumberOfMarkersToTrack(maximumNumberOfMarkersToTrack);
}

int PlanarTracker::GetMaximumNumberOfMarkersToTrack(void)
{
    return _trackerImpl->GetMaximumNumberOfMarkersToTrack();
}
