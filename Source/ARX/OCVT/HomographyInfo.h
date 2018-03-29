#ifndef HOMOGRAPHY_INFO_H
#define HOMOGRAPHY_INFO_H
#include "OCVConfig.h"

class HomographyInfo
{
public:
    HomographyInfo();
    
    HomographyInfo(cv::Mat hom, std::vector<uchar> newStatus, std::vector<cv::DMatch> matches);
    
    bool validHomography;
    cv::Mat homography;
    std::vector<uchar> status;
    std::vector<cv::DMatch> inlier_matches;
    
};

#endif
