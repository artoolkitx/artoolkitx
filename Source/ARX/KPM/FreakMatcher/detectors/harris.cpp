//
//  harris.cpp
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

#include "harris.h"
#include "harris-inline.h"
#include "math/math_io.h"
#include "math/linear_algebra.h"
#include "framework/error.h"
#include <algorithm>
#include <functional>

namespace vision {

    /**
     * Swap the pointers in a "rolling shutter" style.
     */
    template<typename T>
    inline void SwapPointers(T** pm2,
                             T** pm1,
                             T** p,
                             T** pp1,
                             T** pp2) {
        T* tmp = *pm2;
        *pm2 = *pm1;
        *pm1 = *p;
        *p   = *pp1;
        *pp1 = *pp2;
        *pp2 = tmp;
    }
    
    void AllocHarris(float** S,
                     short** Ixx,
                     short** Iyy,
                     short** Ixy,
                     int** Gxx,
                     int** Gyy,
                     int** Gxy,
                     int chunk_size,
                     int width,
                     int height) {

        ASSERT(width >= 1, "wp must be at least 1");
        ASSERT(height >= 1, "hp must be at least 1");
        
        *S = new float[width*height];
        
        short * Ibuf = new short[15*chunk_size];
        
        *Ixx = Ibuf;
        *Iyy = Ibuf + 5 * chunk_size;
        *Ixy = Ibuf + 10 * chunk_size;

        int * Gbuf = new int[15*(chunk_size-4)];
        
        *Gxx = Gbuf +  0*(chunk_size-4);
        *Gyy = Gbuf +  5*(chunk_size-4);
        *Gxy = Gbuf + 10*(chunk_size-4);
    }
    
    void ReleaseHarris(float** S, int** Gxx) {
        delete [] *S;
        delete [] *Gxx;
    }
    
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
                                   float k) {

        int width_minus_6 = width-6;
        int height_minus_6 = height-6;
        int chunk_size_minus_4 = chunk_size-4;
        
        //
        // Initialize row pointers for (Ix,Iy,Ixy,Gxx,Gyy,Gxy)
        //
        
        short* Ixx_pm2 = Ixx;
        short* Ixx_pm1 = Ixx_pm2+chunk_size;
        short* Ixx_p   = Ixx_pm1+chunk_size;
        short* Ixx_pp1 = Ixx_p+chunk_size;
        short* Ixx_pp2 = Ixx_pp1+chunk_size;
        
        short* Iyy_pm2 = Iyy;
        short* Iyy_pm1 = Iyy_pm2+chunk_size;
        short* Iyy_p   = Iyy_pm1+chunk_size;
        short* Iyy_pp1 = Iyy_p+chunk_size;
        short* Iyy_pp2 = Iyy_pp1+chunk_size;
        
        short* Ixy_pm2 = Ixy;
        short* Ixy_pm1 = Ixy_pm2+chunk_size;
        short* Ixy_p   = Ixy_pm1+chunk_size;
        short* Ixy_pp1 = Ixy_p+chunk_size;
        short* Ixy_pp2 = Ixy_pp1+chunk_size;
        
        int* Gxx_pm2 = Gxx;
        int* Gxx_pm1 = Gxx_pm2+chunk_size_minus_4;
        int* Gxx_p   = Gxx_pm1+chunk_size_minus_4;
        int* Gxx_pp1 = Gxx_p+chunk_size_minus_4;
        int* Gxx_pp2 = Gxx_pp1+chunk_size_minus_4;
        
        int* Gyy_pm2 = Gyy;
        int* Gyy_pm1 = Gyy_pm2+chunk_size_minus_4;
        int* Gyy_p   = Gyy_pm1+chunk_size_minus_4;
        int* Gyy_pp1 = Gyy_p+chunk_size_minus_4;
        int* Gyy_pp2 = Gyy_pp1+chunk_size_minus_4;
        
        int* Gxy_pm2 = Gxy;
        int* Gxy_pm1 = Gxy_pm2+chunk_size_minus_4;
        int* Gxy_p   = Gxy_pm1+chunk_size_minus_4;
        int* Gxy_pp1 = Gxy_p+chunk_size_minus_4;
        int* Gxy_pp2 = Gxy_pp1+chunk_size_minus_4;
        
        for(int i = 0; i < width_minus_6; i += chunk_size_minus_4) {
            
            // Calculate the chunk overlap - only the last chunk can have an overlap
            int overlap = ClipScalar((i+chunk_size)-(width-2), 0, chunk_size);
            
            // Shift the index by the overlap
            i = i-overlap;
            ASSERT(i+1+chunk_size < width, "Extending beyond the width of the image");
            
            // Get pointers to the images 
            const unsigned char* src_ptr = src+i+1;
            float* S_ptr = S+3*width+i+3;
            
            // Pointer to the first 3 rows of the soure image
            const unsigned char* pm1  = src_ptr;
            const unsigned char* p    = pm1+step;
            const unsigned char* pp1  = p+step;
            
            //
            // Compute (Ixx,Iyy,Ixy) on first 4 rows
            //
            ComputeDerivatives(Ixx_pm2, Iyy_pm2, Ixy_pm2, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivatives(Ixx_pm1, Iyy_pm1, Ixy_pm1, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivatives(Ixx_p,   Iyy_p,   Ixy_p,   pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivatives(Ixx_pp1, Iyy_pp1, Ixy_pp1, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            
            //
            // Apply horizontal weights on Ixx
            //
            HorizontalBinomial(Gxx_pm2, Ixx_pm2+2, chunk_size_minus_4);
            HorizontalBinomial(Gxx_pm1, Ixx_pm1+2, chunk_size_minus_4);
            HorizontalBinomial(Gxx_p,   Ixx_p+2,   chunk_size_minus_4);
            HorizontalBinomial(Gxx_pp1, Ixx_pp1+2, chunk_size_minus_4);

            //
            // Apply horizontal weights on Iyy
            //
            HorizontalBinomial(Gyy_pm2, Iyy_pm2+2, chunk_size_minus_4);
            HorizontalBinomial(Gyy_pm1, Iyy_pm1+2, chunk_size_minus_4);
            HorizontalBinomial(Gyy_p,   Iyy_p+2,   chunk_size_minus_4);
            HorizontalBinomial(Gyy_pp1, Iyy_pp1+2, chunk_size_minus_4);
            
            //
            // Apply horizontal weights on Ixy
            //
            HorizontalBinomial(Gxy_pm2, Ixy_pm2+2, chunk_size_minus_4);
            HorizontalBinomial(Gxy_pm1, Ixy_pm1+2, chunk_size_minus_4);
            HorizontalBinomial(Gxy_p,   Ixy_p+2,   chunk_size_minus_4);
            HorizontalBinomial(Gxy_pp1, Ixy_pp1+2, chunk_size_minus_4);
            
            for(int j = 0; j < height_minus_6; j++) {
                //
                // Compute (Ixx,Iyy,Ixy) and forward pointers
                //
                ComputeDerivatives(Ixx_pp2, Iyy_pp2, Ixy_pp2, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
                
                //
                // Apply horizontal weights
                //
                HorizontalBinomial(Gxx_pp2, Ixx_pp2+2, chunk_size_minus_4);
                HorizontalBinomial(Gyy_pp2, Iyy_pp2+2, chunk_size_minus_4);
                HorizontalBinomial(Gxy_pp2, Ixy_pp2+2, chunk_size_minus_4);
                
                //
                // Apply vertical weights and write back to (Gxx_pm2, Gyy_pm2, Gxy_pm2) to resuse memory
                //
                VerticalBinomial(Gxx_pm2, Gxx_pm2, Gxx_pm1, Gxx_p, Gxx_pp1, Gxx_pp2, chunk_size_minus_4);
                VerticalBinomial(Gyy_pm2, Gyy_pm2, Gyy_pm1, Gyy_p, Gyy_pp1, Gyy_pp2, chunk_size_minus_4);
                VerticalBinomial(Gxy_pm2, Gxy_pm2, Gxy_pm1, Gxy_p, Gxy_pp1, Gxy_pp2, chunk_size_minus_4);
                
                //
                // Compute the Harris stengths and forward pointer
                //
                HorizontalHarrisStengths(S_ptr, Gxx_pm2, Gyy_pm2, Gxy_pm2, chunk_size_minus_4, k); S_ptr += width;
                
                //
                // Swap the pointers in a rolling shutter style down the image
                //
                SwapPointers(&Ixx_pm2, &Ixx_pm1, &Ixx_p, &Ixx_pp1, &Ixx_pp2);
                SwapPointers(&Iyy_pm2, &Iyy_pm1, &Iyy_p, &Iyy_pp1, &Iyy_pp2);
                SwapPointers(&Ixy_pm2, &Ixy_pm1, &Ixy_p, &Ixy_pp1, &Ixy_pp2);
                SwapPointers(&Gxx_pm2, &Gxx_pm1, &Gxx_p, &Gxx_pp1, &Gxx_pp2);
                SwapPointers(&Gyy_pm2, &Gyy_pm1, &Gyy_p, &Gyy_pp1, &Gyy_pp2);
                SwapPointers(&Gxy_pm2, &Gxy_pm1, &Gxy_p, &Gxy_pp1, &Gxy_pp2);
            }
        }
    }
    
#if __ARM_NEON__
    
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
                                   float k) {
        
        int width_minus_6 = width-6;
        int height_minus_6 = height-6;
        int chunk_size_minus_4 = chunk_size-4;
        
        //
        // Initialize row pointers for (Ix,Iy,Ixy,Gxx,Gyy,Gxy)
        //
        
        short* Ixx_pm2 = Ixx;
        short* Ixx_pm1 = Ixx_pm2+chunk_size;
        short* Ixx_p   = Ixx_pm1+chunk_size;
        short* Ixx_pp1 = Ixx_p+chunk_size;
        short* Ixx_pp2 = Ixx_pp1+chunk_size;
        
        short* Iyy_pm2 = Iyy;
        short* Iyy_pm1 = Iyy_pm2+chunk_size;
        short* Iyy_p   = Iyy_pm1+chunk_size;
        short* Iyy_pp1 = Iyy_p+chunk_size;
        short* Iyy_pp2 = Iyy_pp1+chunk_size;
        
        short* Ixy_pm2 = Ixy;
        short* Ixy_pm1 = Ixy_pm2+chunk_size;
        short* Ixy_p   = Ixy_pm1+chunk_size;
        short* Ixy_pp1 = Ixy_p+chunk_size;
        short* Ixy_pp2 = Ixy_pp1+chunk_size;
        
        int* Gxx_pm2 = Gxx;
        int* Gxx_pm1 = Gxx_pm2+chunk_size_minus_4;
        int* Gxx_p   = Gxx_pm1+chunk_size_minus_4;
        int* Gxx_pp1 = Gxx_p+chunk_size_minus_4;
        int* Gxx_pp2 = Gxx_pp1+chunk_size_minus_4;
        
        int* Gyy_pm2 = Gyy;
        int* Gyy_pm1 = Gyy_pm2+chunk_size_minus_4;
        int* Gyy_p   = Gyy_pm1+chunk_size_minus_4;
        int* Gyy_pp1 = Gyy_p+chunk_size_minus_4;
        int* Gyy_pp2 = Gyy_pp1+chunk_size_minus_4;
        
        int* Gxy_pm2 = Gxy;
        int* Gxy_pm1 = Gxy_pm2+chunk_size_minus_4;
        int* Gxy_p   = Gxy_pm1+chunk_size_minus_4;
        int* Gxy_pp1 = Gxy_p+chunk_size_minus_4;
        int* Gxy_pp2 = Gxy_pp1+chunk_size_minus_4;
        
        for(int i = 0; i < width_minus_6; i += chunk_size_minus_4) {
            
            // Calculate the chunk overlap - only the last chunk can have an overlap
            int overlap = ClipScalar((i+chunk_size)-(width-2), 0, chunk_size);
            
            // Shift the index by the overlap
            i = i-overlap;
            ASSERT(i+1+chunk_size < width, "Extending beyond the width of the image");
            
            // Get pointers to the images
            const unsigned char* src_ptr = src+i+1;
            float* S_ptr = S+3*width+i+3;
            
            // Pointer to the first 3 rows of the soure image
            const unsigned char* pm1  = src_ptr;
            const unsigned char* p    = pm1+step;
            const unsigned char* pp1  = p+step;
            
            //
            // Compute (Ixx,Iyy,Ixy) on first 4 rows
            //
            ComputeDerivativesNeon(Ixx_pm2, Iyy_pm2, Ixy_pm2, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivativesNeon(Ixx_pm1, Iyy_pm1, Ixy_pm1, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivativesNeon(Ixx_p,   Iyy_p,   Ixy_p,   pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
            ComputeDerivativesNeon(Ixx_pp1, Iyy_pp1, Ixy_pp1, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;

            //
            // Apply horizontal weights on Ixx
            //
            HorizontalBinomialNeon(Gxx_pm2, Ixx_pm2+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gxx_pm1, Ixx_pm1+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gxx_p,   Ixx_p+2, chunk_size_minus_4  );
            HorizontalBinomialNeon(Gxx_pp1, Ixx_pp1+2, chunk_size_minus_4);
            
            //
            // Apply horizontal weights on Iyy
            //
            HorizontalBinomialNeon(Gyy_pm2, Iyy_pm2+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gyy_pm1, Iyy_pm1+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gyy_p,   Iyy_p+2, chunk_size_minus_4  );
            HorizontalBinomialNeon(Gyy_pp1, Iyy_pp1+2, chunk_size_minus_4);
            
            //
            // Apply horizontal weights on Ixy
            //
            HorizontalBinomialNeon(Gxy_pm2, Ixy_pm2+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gxy_pm1, Ixy_pm1+2, chunk_size_minus_4);
            HorizontalBinomialNeon(Gxy_p,   Ixy_p+2, chunk_size_minus_4  );
            HorizontalBinomialNeon(Gxy_pp1, Ixy_pp1+2, chunk_size_minus_4);
            
            for(int j = 0; j < height_minus_6; j++) {
                //
                // Compute (Ixx,Iyy,Ixy) and forward pointers
                //
                ComputeDerivativesNeon(Ixx_pp2, Iyy_pp2, Ixy_pp2, pm1, p, pp1, chunk_size); pm1 += step; p += step; pp1 += step;
                
                //
                // Apply horizontal weights
                //
                HorizontalBinomialNeon(Gxx_pp2, Ixx_pp2+2, chunk_size_minus_4);
                HorizontalBinomialNeon(Gyy_pp2, Iyy_pp2+2, chunk_size_minus_4);
                HorizontalBinomialNeon(Gxy_pp2, Ixy_pp2+2, chunk_size_minus_4);
                
                //
                // Apply vertical weights and write back to (Gxx_pm2, Gyy_pm2, Gxy_pm2) to resuse memory
                //
                VerticalBinomialNeon(Gxx_pm2, Gxx_pm2, Gxx_pm1, Gxx_p, Gxx_pp1, Gxx_pp2, chunk_size_minus_4);
                VerticalBinomialNeon(Gyy_pm2, Gyy_pm2, Gyy_pm1, Gyy_p, Gyy_pp1, Gyy_pp2, chunk_size_minus_4);
                VerticalBinomialNeon(Gxy_pm2, Gxy_pm2, Gxy_pm1, Gxy_p, Gxy_pp1, Gxy_pp2, chunk_size_minus_4);
                
                //
                // Compute the Harris stengths and forward pointer
                //
                HorizontalHarrisStengthsNeon(S_ptr, Gxx_pm2, Gyy_pm2, Gxy_pm2, chunk_size_minus_4, k); S_ptr += width;
                
                //
                // Swap the pointers in a rolling shutter style down the image
                //
                SwapPointers(&Ixx_pm2, &Ixx_pm1, &Ixx_p, &Ixx_pp1, &Ixx_pp2);
                SwapPointers(&Iyy_pm2, &Iyy_pm1, &Iyy_p, &Iyy_pp1, &Iyy_pp2);
                SwapPointers(&Ixy_pm2, &Ixy_pm1, &Ixy_p, &Ixy_pp1, &Ixy_pp2);
                SwapPointers(&Gxx_pm2, &Gxx_pm1, &Gxx_p, &Gxx_pp1, &Gxx_pp2);
                SwapPointers(&Gyy_pm2, &Gyy_pm1, &Gyy_p, &Gyy_pp1, &Gyy_pp2);
                SwapPointers(&Gxy_pm2, &Gxy_pm1, &Gxy_p, &Gxy_pp1, &Gxy_pp2);
            }
        }
    }
    
#endif
    
    void PruneHarrisCorners(std::vector<std::vector<std::vector<std::pair<float, size_t> > > >& buckets,
                            std::vector<Point2d<int> >& outPoints,
                            const std::vector<Point2d<int> >& inPoints,
                            const std::vector<float>& scores,
                            int num_buckets_X,
                            int num_buckets_Y,
                            int width,
                            int height,
                            int max_points) {
        
        int num_buckets = num_buckets_X*num_buckets_Y;
        int num_points_per_bucket = max_points/num_buckets;
        int dx = (int)std::ceil((float)width/num_buckets_X);
        int dy = (int)std::ceil((float)height/num_buckets_Y);
        
        //
        // Clear the previous state
        //
        outPoints.clear();
        outPoints.reserve(max_points);
        for(size_t i = 0; i < buckets.size(); i++) {
            for(size_t j = 0; j < buckets[i].size(); j++) {
                buckets[i][j].clear();
            }
        }
        
        //
        // Insert each features into a bucket
        //
        for(size_t i = 0; i < inPoints.size(); i++) {
            const Point2d<int>& p = inPoints[i];
            int binX = p.x/dx;
            int binY = p.y/dy;
            buckets[binX][binY].push_back(std::make_pair(scores[i], i));
        }
        
        //
        // Do a partial sort on the first N points of each bucket
        //
        for(size_t i = 0; i < buckets.size(); i++) {
            for(size_t j = 0; j < buckets[i].size(); j++) {
                std::vector<std::pair<float, size_t> >& bucket = buckets[i][j];
                size_t n = std::min<size_t>(bucket.size(), num_points_per_bucket);
                if(n == 0) {
                    continue;
                }
                std::nth_element(bucket.begin(),
                                 bucket.begin()+n,
                                 bucket.end(), std::greater<std::pair<float, size_t> >());
                
                DEBUG_BLOCK(
                    if(n > bucket.size()) {
                        ASSERT(bucket[0].first >= bucket[n].first, "nth_element failed");
                    }
                )
                
                for(size_t k = 0; k < n; k++) {
                    outPoints.push_back(inPoints[bucket[k].second]);
                }
            }
        }
    }

    void HarrisNonmaxSuppression3x3(std::vector<Point2d<int> >& points,
                                    std::vector<float>& scores,
                                    const float* S,
                                    int width,
                                    int height,
                                    int step,
                                    float tr) {
        
        int width_minus_8 = width-8;
        int height_minus_8 = height-8;
        
        points.clear();
        scores.clear();
        
        //
        // Get pointers to the first 3 rows (shift by 1 to right) for
        // 3x3 non-max window. There is a 3 pixel border around the stength
        // image from the Harris window for a total of 4 pixels.
        //
        const float* pm1 = (float*)((unsigned char*)S+step*3+sizeof(float)*4);
        const float* p   = (float*)((unsigned char*)pm1+step);
        const float* pp1 = (float*)((unsigned char*)p+step);
        
        for(int i = 0; i < height_minus_8; i++) {
            for(int j = 0; j < width_minus_8; j++) {
                const float& value = p[j];
                if(value < tr) {
                    continue;
                }
                
                if(/* pm1 */
                   value > pm1[j-2]  &&
                   value > pm1[j-1]  &&
                   value > pm1[j]    &&
                   value > pm1[j+1]  &&
                   value > pm1[j+2]  &&
                   /* p */
                   value > p[j-2]    &&
                   value > p[j-1]    &&
                   value > p[j+1]    &&
                   value > p[j+2]    &&
                   /* pp1 */
                   value > pp1[j-2]  &&
                   value > pp1[j-1]  &&
                   value > pp1[j]    &&
                   value > pp1[j+1]  &&
                   value > pp1[j+2]) {
                    
                    points.push_back(Point2d<int>(j+4, i+4));
                    scores.push_back(value);
                }
            }
            
            pm1 = (float*)((unsigned char*)pm1+step);
            p   = (float*)((unsigned char*)pm1+step);
            pp1 = (float*)((unsigned char*)p+step);
        }
    }
    
    void HarrisNonmaxSuppression5x5(std::vector<Point2d<int> >& points,
                                    std::vector<float>& scores,
                                    const float* S,
                                    int width,
                                    int height,
                                    int step,
                                    float tr) {
        
        int width_minus_10 = width-10;
        int height_minus_10 = height-10;
        
        points.clear();
        scores.clear();
        
        //
        // Get pointers to the first 5 rows (shift by 2 to right) for
        // 5x5 non-max window. There is a 3 pixel border around the stength
        // image from the Harris window for a total of 5 pixels.
        //
        const float* pm2 = (float*)((unsigned char*)S+step*3+sizeof(float)*5);
        const float* pm1 = (float*)((unsigned char*)pm2+step);
        const float* p   = (float*)((unsigned char*)pm1+step);
        const float* pp1 = (float*)((unsigned char*)p+step);
        const float* pp2 = (float*)((unsigned char*)pp1+step);
        
        for(int i = 0; i < height_minus_10; i++) {
            for(int j = 0; j < width_minus_10; j++) {
                const float& value = p[j];
                if(value < tr) {
                    continue;
                }
                
                if(/* pm2 */
                   value > pm2[j-2]  &&
                   value > pm2[j-1]  &&
                   value > pm2[j]    &&
                   value > pm2[j+1]  &&
                   value > pm2[j+2]  &&
                   /* pm1 */
                   value > pm1[j-2]  &&
                   value > pm1[j-1]  &&
                   value > pm1[j]    &&
                   value > pm1[j+1]  &&
                   value > pm1[j+2]  &&
                   /* p */
                   value > p[j-2]    &&
                   value > p[j-1]    &&
                   value > p[j+1]    &&
                   value > p[j+2]    &&
                   /* pp1 */
                   value > pp1[j-2]  &&
                   value > pp1[j-1]  &&
                   value > pp1[j]    &&
                   value > pp1[j+1]  &&
                   value > pp1[j+2]  &&
                   /* pp2 */
                   value > pp2[j-2]  &&
                   value > pp2[j-1]  &&
                   value > pp2[j]    &&
                   value > pp2[j+1]  &&
                   value > pp2[j+2]) {
                    
                    points.push_back(Point2d<int>(j+5, i+5));
                    scores.push_back(value);
                }
            }
            
            pm2 = (float*)((unsigned char*)pm2+step);
            pm1 = (float*)((unsigned char*)pm2+step);
            p   = (float*)((unsigned char*)pm1+step);
            pp1 = (float*)((unsigned char*)p+step);
            pp2 = (float*)((unsigned char*)pp1+step);
        }
    }
    
    void RefineHarrisCorners(std::vector<Point2d<float> >& outPoints,
                             const std::vector<Point2d<int> >& inPoints,
                             const float* S,
                             int width,
                             int height,
                             int step) {
        float A[4];
        float b[2];
        for(size_t i = 0; i < inPoints.size(); i++) {
            const Point2d<int>& point = inPoints[i];
            
            // Each pixel must have 1 pixel on each side
            if(point.x < 1 ||
               point.y < 1 ||
               point.x > width-2 ||
               point.y > height-2) {
                continue;
            }
            
            const float* pm1 = (float*)((unsigned char*)S+(point.y-1)*step+sizeof(float)*point.x);
            const float* p   = (float*)((unsigned char*)pm1+step);
            const float* pp1  = (float*)((unsigned char*)p+step);
            
            ASSERT(p[0] > pm1[-1], "Should be maxima");
            ASSERT(p[0] > pm1[0],  "Should be maxima");
            ASSERT(p[0] > pm1[1],  "Should be maxima");
            ASSERT(p[0] > p[-1],   "Should be maxima");
            ASSERT(p[0] > p[1],    "Should be maxima");
            ASSERT(p[0] > pp1[-1], "Should be maxima");
            ASSERT(p[0] > pp1[0],  "Should be maxima");
            ASSERT(p[0] > pp1[1],  "Should be maxima");
            
            // Cx,Cy
            b[0] = -(-p[-1]+p[1])*0.5f;
            b[1] = -(-pm1[0]+pp1[0])*0.5f;
            
            // Cxx,Cyy,Cxy
            A[0] = p[-1]-2*p[0]+p[1];
            A[1] = 0.25f*((pm1[-1] + pp1[1]) - (pm1[1] + pp1[-1]));
            A[2] = A[1];
            A[3] = pm1[0]-2*p[0]+pp1[0];
            
            float u[2];
            if(SolveLinearSystem2x2(u, A, b)) {
                // We dont' want to move pixels too far from the maxima
                // TODO: determine if it's best remove the point if it's too far, or leave it alone.
                if(std::abs(u[0]) <= 1.f && std::abs(u[1]) <= 1.f) {
                    outPoints.push_back(Point2d<float>(point.x+u[0], point.y+u[1]));
                } else {
                    outPoints.push_back(Point2d<float>(point.x, point.y));
                }
            }
            
            ASSERT_INF(u[0]);
            ASSERT_INF(u[1]);
        }
    }
    
} // vision