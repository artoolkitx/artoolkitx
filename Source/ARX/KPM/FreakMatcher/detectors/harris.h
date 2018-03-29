//
//  harris.h
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

#include <cstddef>
#include <vector>
#include "utils/point.h"

namespace vision {

    /**
     * Allocate memory for Harris interest point detection.
     */
    void AllocHarris(float** S,
                     short** Ixx,
                     short** Iyy,
                     short** Ixy,
                     int** Gxx,
                     int** Gyy,
                     int** Gxy,
                     int chunk_size,
                     int width,
                     int height);
    
    /**
     * Release Harris memory.
     */
    void ReleaseHarris(float** S, int** Gxx);
    
    /**
     * Compute the Harris stength image.
     */
    void ComputeHarrisStengthImage(float* S,
                                   const unsigned char* src,
                                   short* Ixx,
                                   short* Iyy,
                                   short* Ixy,
                                   int* Gxx,
                                   int* Gyy,
                                   int* Gxy,
                                   int width,
                                   int height,
                                   int step,
                                   int chunk_size,
                                   float k);
    
#if __ARM_NEON__
    
    /**
     * Compute the Harris strength image.
     * This function contains NEON-optimized code that provide better performance than 'regular' version.
     */
    void ComputeHarrisStrengthImageNeon(float* S,
                                       const unsigned char* src,
                                       short* Ixx,
                                       short* Iyy,
                                       short* Ixy,
                                       int* Gxx,
                                       int* Gyy,
                                       int* Gxy,
                                       int width,
                                       int height,
                                       int step,
                                       int chunk_size,
                                       float k);
#endif
    
    /**
     * Prune Harris corners to a maximum number.
     */
    void PruneHarrisCorners(std::vector<std::vector<std::vector<std::pair<float, size_t> > > >& buckets,
                            std::vector<Point2d<int> >& outPoints,
                            const std::vector<Point2d<int> >& inPoints,
                            const std::vector<float>& scores,
                            int num_buckets_X,
                            int num_buckets_Y,
                            int width,
                            int height,
                            int max_points);
    
    /**
     * Perform non-max suppression on the Harris strength image.
     */
    void HarrisNonmaxSuppression3x3(std::vector<Point2d<int> >& points,
                                    std::vector<float>& scores,
                                    const float* S,
                                    int width,
                                    int height,
                                    int step,
                                    float tr);
    void HarrisNonmaxSuppression5x5(std::vector<Point2d<int> >& points,
                                    std::vector<float>& scores,
                                    const float* S,
                                    int width,
                                    int height,
                                    int step,
                                    float tr);

    /**
     * Refine corners.
     */
    void RefineHarrisCorners(std::vector<Point2d<float> >& outPoints,
                             const std::vector<Point2d<int> >& inPoints,
                             const float* S,
                             int width,
                             int height,
                             int step);

} // vision