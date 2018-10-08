/*
 *  arRefineCorners.cpp
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
 *  Copyright 2018 Dan Bell & Philip Lamb.
 *
 *  Author(s): Dan Bell, Philip Lamb.
 *
 */

#include "arRefineCorners.h"

//#define DEBUG_REFINECORNERS

#if !HAVE_OPENCV

void arRefineCorners(ARfloat vertex[4][2], const unsigned char *buff, int width, int height)
{
    // Do nothing.
}

#else

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <vector>

void arRefineCorners(float vertex[4][2], const unsigned char *buff, int width, int height)
{
    bool validCorners = true;
    cv::Rect rect = cv::Rect(1, 1, width - 1, height - 1);
    std::vector<cv::Point2f> corners;
    for (int i = 0; i < 4; i++) {
        if (!rect.contains(cv::Point2f(vertex[i][0], vertex[i][1]))) {
            validCorners = false;
            break;
        } else {
            corners.push_back(cv::Point2f(vertex[i][0], vertex[i][1]));
        }
    }
    if (validCorners) {
        cv::Mat src = cv::Mat(height, width, CV_8UC1, (void *)buff, width);
        cv::Size winSize = cv::Size(5, 5);
        cv::Size zeroZone = cv::Size(-1, -1);
        cv::TermCriteria criteria = cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
        
        // Calculate the refined corner locations.
        cv::cornerSubPix(src, corners, winSize, zeroZone, criteria);
        for (int i = 0; i < 4; i++) {
#ifdef DEBUG_REFINECORNERS
            if ((fabsf(vertex[i][0] - corners[i].x) > 0.1f) || (fabsf(vertex[i][1] - corners[i].y) > 0.1f)) {
                ARLOGd("arRefineCorners adjusted vertex %d from (%.1f, %.1f) to (%.1f, %.1f).\n", i, vertex[i][0], vertex[i][1], corners[i].x, corners[i].y);
            }
#endif
            vertex[i][0] = (ARdouble)corners[i].x;
            vertex[i][1] = (ARdouble)corners[i].y;
        }
        src.release();
    }
    corners.clear();
}

#endif // HAVE_OPENCV
