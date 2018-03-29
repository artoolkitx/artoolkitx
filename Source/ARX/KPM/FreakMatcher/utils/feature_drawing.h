//
//  feature_drawing.cpp
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#pragma once


#include <matchers/feature_point.h>
#include <matchers/feature_store.h>
#include <matchers/feature_matcher.h>
#include <matchers/freak.h>

#ifdef USE_OPENCV
#  include <opencv2/opencv.hpp>
#  include <opencv2/highgui/highgui.hpp>
#endif

#include <math/indexing.h>
#include <math/matrix.h>
#include <math/rand.h>
#include <math/homography.h>
#include <math/linear_algebra.h>
#include <utils/point.h>
#include <framework/image_utils.h>

namespace vision{

template<typename T>
inline void RotationMatrix2x2(T R[4], T angle) {
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    R[0] = c;	R[1] = -s;
    R[2] = s;	R[3] = c;
}

void DrawPolygon(cv::Mat& outImg, const std::vector<cv::Point2f>& points, const cv::Scalar& color) {
    int npts = (int)points.size();
    std::vector<cv::Point> cvpoints(points.size());
    for(size_t i = 0; i < points.size(); i++) {
        cvpoints[i].x = round(points[i].x);
        cvpoints[i].y = round(points[i].y);
    }
    const cv::Point* pts = (cv::Point*)&cvpoints[0];
    cv::polylines(outImg, &pts, &npts, 1, 1, color);
}

void RotatePoints(std::vector<cv::Point2f>& xp,
                  const std::vector<cv::Point2f>& x,
                  const cv::Point& center,
                  float angle) {
    float R[4];
    float xx[2];
    float xxp[2];
    
    RotationMatrix2x2(R, angle);
    
    xp.resize(x.size());
    for(size_t i = 0; i < x.size(); i++) {
        xx[0] = x[i].x-center.x;
        xx[1] = x[i].y-center.y;
        Multiply_2x2_2x1(xxp, R, xx);
        xp[i].x = xxp[0]+center.x;
        xp[i].y = xxp[1]+center.y;
    }
}
}

namespace vision {
    
    void MakeCircleTestImage(Image& im) {
        im.alloc(IMAGE_UINT8, 512, 512, 512, 1);
        cv::Mat cvim = ImageUtils::toOpenCV(im);
        
        int seed = 1234;
        
        for(int i = 0; i < 20; i++) {
            int x, y, r;
            x = FastRandom(seed)%im.width();
            y = FastRandom(seed)%im.height();
            r = FastRandom(seed)%30;
            cv::circle(cvim, cv::Point(x, y), r*2, cv::Scalar(255, 255, 255), -1);
        }
    }
    
    void DrawSquareFeature(cv::Mat& outImg, const FeaturePoint& p, const cv::Scalar& color) {
        float radius = max2<float>(p.scale, 1);
        
        std::vector<cv::Point2f> x(4);
        std::vector<cv::Point2f> xp(4);
        
        cv::Point2f center(p.x, p.y);
        
        // Compute four corners for drawing rotated square
        x[0].x = p.x+radius;
        x[0].y = p.y+radius;
        x[1].x = p.x-radius;
        x[1].y = p.y+radius;
        x[2].x = p.x-radius;
        x[2].y = p.y-radius;
        x[3].x = p.x+radius;
        x[3].y = p.y-radius;
        
        // Rotate the bounding square
        RotatePoints(xp, x, center, p.angle);
        
        // Draw rotated bounding box (via polygon drawing)
        DrawPolygon(outImg, xp, color);
    }
    
    void DrawHomography(cv::Mat& dst, const float H[9], int width, int height, const  cv::Scalar& color) {
        float xp[4];
        float yp[4];
    
        MultiplyPointHomographyInhomogenous<float>(xp[0], yp[0], H, 0, 0);
        MultiplyPointHomographyInhomogenous<float>(xp[1], yp[1], H, width, 0);
        MultiplyPointHomographyInhomogenous<float>(xp[2], yp[2], H, width, height);
        MultiplyPointHomographyInhomogenous<float>(xp[3], yp[3], H, 0, height);
        
        cv::line(dst, cv::Point(xp[0], yp[0]), cv::Point(xp[1], yp[1]), color, 2);
        cv::line(dst, cv::Point(xp[1], yp[1]), cv::Point(xp[2], yp[2]), color, 2);
        cv::line(dst, cv::Point(xp[2], yp[2]), cv::Point(xp[3], yp[3]), color, 2);
        cv::line(dst, cv::Point(xp[3], yp[3]), cv::Point(xp[0], yp[0]), color, 2);
    }
    
    void DrawMatches(cv::Mat& dst,
                     const cv::Mat& ref,
                     const cv::Mat& ins,
                     const std::vector<FeaturePoint>& refPoints,
                     const std::vector<FeaturePoint>& insPoints,
                     const matches_t& matches,
                     const float H[9],
                     int refWidth,
                     int refHeight) {
        
        cv::Mat dstIns;
        cv::Mat dstRef;
        
        cv::Size size(ref.cols+ins.cols, MAX(ref.rows, ins.rows));
        
        dst.create(size, CV_MAKETYPE(ref.depth(), ref.channels()));
        dst = cv::Scalar(0);
        
        dstIns = dst(cv::Rect(0, 0, ins.cols, ins.rows));
        dstRef = dst(cv::Rect(ins.cols, 0, ref.cols, ref.rows));
        
        ins.copyTo(dstIns);
        ref.copyTo(dstRef);
        
        float Hinv[9];
        if(H != NULL) {
            if(!MatrixInverse3x3(Hinv, H, 0.f)) {
                ASSERT(0, "Failed to invert homography");
            }
        }
        
        int seed = 1234;
        for(size_t i = 0; i < matches.size(); i++) {
            const FeaturePoint& refFeature = refPoints[matches[i].ref];
            const FeaturePoint& insFeature = insPoints[matches[i].ins];
            
            int r = FastRandom(seed)%256;
            int g = FastRandom(seed)%256;
            int b = FastRandom(seed)%256;
            
            DrawSquareFeature(dstRef, refFeature, cv::Scalar(r,g,b));
            DrawSquareFeature(dstIns, insFeature, cv::Scalar(r,g,b));
            
            if(H != NULL) {
                float xp1, yp1;
               
                MultiplyPointHomographyInhomogenous(xp1, yp1, H, refFeature.x, refFeature.y);
                DrawSquareFeature(dstIns, FeaturePoint(xp1, yp1, insFeature.angle, insFeature.scale, insFeature.maxima), cv::Scalar(255, 0, 0));
                
                MultiplyPointHomographyInhomogenous(xp1, yp1, Hinv, insFeature.x, insFeature.y);
                DrawSquareFeature(dstRef, FeaturePoint(xp1, yp1, refFeature.angle, refFeature.scale, refFeature.maxima), cv::Scalar(255, 0, 0));
            }
            
            cv::line(dst,
                     cv::Point(insFeature.x, insFeature.y),
                     cv::Point(refFeature.x+dstIns.cols, refFeature.y),
                     cv::Scalar(r,g,b));
        }
        
        if(H != NULL) {
            DrawHomography(dstIns, H, refWidth, refHeight, cv::Scalar(0, 255, 0));
        }
    }
    
    void DrawFeatures(cv::Mat& im, const std::vector<FeaturePoint>& features) {
        int seed = 1234;
        for(size_t i = 0; i < features.size(); i++) {
            int r = FastRandom(seed)%256;
            int g = FastRandom(seed)%256;
            int b = FastRandom(seed)%256;
            DrawSquareFeature(im, features[i], cv::Scalar(r,g,b));
        }
    }
    
    void DrawFreakFeature(cv::Mat& im,
                          const std::vector<FeaturePoint>& points,
                          const FREAKExtractor& extractor,
                          size_t i) {
#ifdef FREAK_DEBUG
        const Point2d<float>* pts0 = &extractor.mMappedPoints0[i*6];
        const Point2d<float>* pts1 = &extractor.mMappedPoints1[i*6];
        const Point2d<float>* pts2 = &extractor.mMappedPoints2[i*6];
        const Point2d<float>* pts3 = &extractor.mMappedPoints3[i*6];
        const Point2d<float>* pts4 = &extractor.mMappedPoints4[i*6];
        const Point2d<float>* pts5 = &extractor.mMappedPoints5[i*6];
        const Point2d<float>* ptsC = &extractor.mMappedPointsC[i];
        
        float s0 = extractor.mMappedS0[i];
        float s1 = extractor.mMappedS1[i];
        float s2 = extractor.mMappedS2[i];
        float s3 = extractor.mMappedS3[i];
        float s4 = extractor.mMappedS4[i];
        float s5 = extractor.mMappedS5[i];
        float sc = extractor.mMappedSC[i];
        
        for(int j = 0; j < 6; j++) {
            cv::circle(im, cv::Point2d(pts0[j].x, pts0[j].y), s0, CV_RGB(255, 0, 0));
            cv::circle(im, cv::Point2d(pts1[j].x, pts1[j].y), s1, CV_RGB(255, 0, 0));
            cv::circle(im, cv::Point2d(pts2[j].x, pts2[j].y), s2, CV_RGB(255, 0, 0));
            cv::circle(im, cv::Point2d(pts3[j].x, pts3[j].y), s3, CV_RGB(255, 0, 0));
            cv::circle(im, cv::Point2d(pts4[j].x, pts4[j].y), s4, CV_RGB(255, 0, 0));
            cv::circle(im, cv::Point2d(pts5[j].x, pts5[j].y), s5, CV_RGB(255, 0, 0));
        }
        cv::circle(im, cv::Point2d(ptsC[0].x, ptsC[0].y), sc, CV_RGB(255, 0, 0));
        
        cv::Point2f p(points[i].scale*10, 0);
        std::vector<cv::Point2f> vp;
        std::vector<cv::Point2f> vpp;
        vp.push_back(p);
        RotatePoints(vpp, vp, cv::Point(0,0), points[i].angle);
        
        cv::line(im, cv::Point(ptsC[0].x, ptsC[0].y), cv::Point(ptsC[0].x+vpp[0].x, ptsC[0].y+vpp[0].y), CV_RGB(0, 255, 0), 5);
        
        DrawSquareFeature(im, points[i], CV_RGB(0, 255, 255));
#endif
    }
    
    void DrawText(cv::Mat& im, const std::string& text, const cv::Point& p) {
        using namespace cv;
        
        int fontFace = FONT_HERSHEY_PLAIN;
        double fontScale = 1;
        int thickness = 1;
        
        putText(im, text, p, fontFace, fontScale, Scalar::all(255), thickness, 8);
    }
    
} // vision
