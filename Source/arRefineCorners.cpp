//
//  arRefineCorners.cpp
//  AR
//
//  Created by John Bell on 6/07/18.
//

#include <stdio.h>
#include "arRefineCorners.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
void arRefineCorners(double (*vertex)[4][2], unsigned char *buff, int width, int height)
{
    cv::Mat src = cv::Mat(height, width, cv::CV_8UC1, buff);
    std::vector<cv::Point2d> corners;
    for(int i=0; i<4; i++)
    {
        corners.push_back(cv::Point2d(vertex[i][0], vertex[i][1]));
    }
    
    cv::Size winSize = cv::Size( 5, 5 );
    cv::Size zeroZone = cv::Size( -1, -1 );
    cv::TermCriteria criteria = cv::TermCriteria( cv::CV_TERMCRIT_EPS + cv::CV_TERMCRIT_ITER, 40, 0.001 );
    
    /// Calculate the refined corner locations
    cv::cornerSubPix( src_gray, corners, winSize, zeroZone, criteria );
    for(int i=0; i<4; i++)
    {
        vertex[i][0] = corners.x;
        vertex[i][1] = corners.y;
    }
    src.release();
    corners.clear();
}
