//
//  image_utils.cpp
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

#include "image.h"
#include <cmath>

#ifdef USE_OPENCV
#  include <opencv2/opencv.hpp>
#endif

namespace vision {

    class ImageUtils{
        
    public:
        static bool convertType(Image& dst, const Image& src, float scale) {
            if(src.type() == IMAGE_F32 && dst.type() == IMAGE_UINT8) {
                for(int i = 0; i < src.height(); i++) {
                    for(int j = 0; j < src.width(); j++) {
                        dst.get<unsigned char>(i)[j] = (unsigned char)(std::abs(src.get<float>(i)[j])*scale);
                    }
                }
                return true;
            }
            return false;
        }
        
#ifdef USE_OPENCV
        static cv::Mat toOpenCV(Image& im) {
            int type = -1;
            
            switch(im.type()) {
                case vision::IMAGE_UINT8:
                    type = CV_8UC((int)im.channels());
                    break;
                case vision::IMAGE_F32:
                    type = CV_32FC((int)im.channels());
                    break;
                default:
                    return cv::Mat();
            }
            
            cv::Mat opencv_im((int)im.height(),
                              (int)im.width(),
                              type,
                              im.get(),
                              im.step());
            
            return opencv_im;
        }
        
        static Image toImage(cv::Mat& im) {
            switch(im.type()) {
                case CV_8U:
                    break;
                default:
                    return Image();
            };
            
            return Image(im.data, IMAGE_UINT8, im.cols, im.rows, (int)im.step, im.channels());
        }
#endif
        
    };
    
} // vision