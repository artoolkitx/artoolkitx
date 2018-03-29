//
//  freak.h
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

#include <detectors/gaussian_scale_space_pyramid.h>
#include <math/indexing.h>
#include <math/homography.h>
#include <math/math_io.h>
#include <utils/point.h>
#include <detectors/interpolate.h>
#include "feature_store.h"

namespace vision {
    
// DEFINE this to enable bilinar interpolation when sampling intensity values
#define FREAK_BILINEAR_SAMPLE
    
//#define FREAK_DEBUG
    
    /**
     * Implements the FREAK extractor.
     */
    class FREAKExtractor {
    public:
        
        struct receptor {
            receptor() {}
            receptor(float _x, float _y, float _s) : x(_x), y(_y), s(_s) {}
            float x, y, s;
        };
        
        FREAKExtractor();
        ~FREAKExtractor() {}
        
        /**
         * Get a set of tests for an 84 byte descriptor.
         */
        void layout84(std::vector<receptor>& receptors,
                      std::vector<std::vector<int> >& tests);
        
        /**
         * Extract a 96 byte descriptor.
         */
        void extract(BinaryFeatureStore& store,
                     const GaussianScaleSpacePyramid* pyramid,
                     const std::vector<FeaturePoint>& points);
        
#ifdef FREAK_DEBUG
        std::vector<Point2d<float> > mMappedPoints0;
        std::vector<Point2d<float> > mMappedPoints1;
        std::vector<Point2d<float> > mMappedPoints2;
        std::vector<Point2d<float> > mMappedPoints3;
        std::vector<Point2d<float> > mMappedPoints4;
        std::vector<Point2d<float> > mMappedPoints5;
        std::vector<Point2d<float> > mMappedPointsC;
        
        std::vector<float> mMappedS0;
        std::vector<float> mMappedS1;
        std::vector<float> mMappedS2;
        std::vector<float> mMappedS3;
        std::vector<float> mMappedS4;
        std::vector<float> mMappedS5;
        std::vector<float> mMappedSC;
#endif
        
    private:
        
        // Receptor locations
        float mPointRing0[12];
        float mPointRing1[12];
        float mPointRing2[12];
        float mPointRing3[12];
        float mPointRing4[12];
        float mPointRing5[12];
        
        // Sigma value
        float mSigmaCenter;
        float mSigmaRing0;
        float mSigmaRing1;
        float mSigmaRing2;
        float mSigmaRing3;
        float mSigmaRing4;
        float mSigmaRing5;
        
        // Scale expansion factor
        float mExpansionFactor;
        
    }; // FREAKExtractor

    /**
     * Sample a receptor given (x,y) given an image using bilinear interpolation.
     */
    inline float SampleReceptorBilinear(const Image& image,
                                float x,
                                float y) {
        x = ClipScalar<float>(x, 0, image.width()-2);
        y = ClipScalar<float>(y, 0, image.height()-2);
        return bilinear_interpolation<float>(image, x, y);
    }
    
    /**
     * Sample a receptor given (x,y) given an image using nearest neighbor.
     */
    inline float SampleReceptorNN(const Image& image,
                                        float x,
                                        float y) {
        x = ClipScalar<float>(x, 0, image.width()-1);
        y = ClipScalar<float>(y, 0, image.height()-1);
        return image.get<float>((int)y)[(int)x];
    }
    
    /**
     * Sample a receptor given (x,y) given an image.
     */
    inline float SampleReceptor(const Image& image,
                                float x,
                                float y) {
#ifdef FREAK_BILINEAR_SAMPLE
        return SampleReceptorBilinear(image, x, y);
#else
        return SampleReceptorNN(image, x, y);
#endif
    }
    
    /**
     * Sample a receptor given (x,y,octave,scale) and a pyramid.
     */
    inline float SampleReceptor(const GaussianScaleSpacePyramid* pyramid,
                                float x,
                                float y,
                                int octave,
                                int scale) {
        // Get the correct image from the pyramid
        const Image& image = pyramid->get(octave, scale);
        
        float xp, yp;
        // Downsample the point to the octave
        bilinear_downsample_point(xp, yp, x, y, octave);
        // Sample the receptor
        return SampleReceptor(image, xp, yp);
    }
    
    /**
     * Sample all the receptors from the pyramid given a single point.
     */
    inline bool SamplePyramidFREAK84(float samples[37],
                                     const GaussianScaleSpacePyramid* pyramid,
                                     const FeaturePoint& point,
                                     const float points_ring0[12],
                                     const float points_ring1[12],
                                     const float points_ring2[12],
                                     const float points_ring3[12],
                                     const float points_ring4[12],
                                     const float points_ring5[12],
                                     float sigma_center,
                                     float sigma_ring0,
                                     float sigma_ring1,
                                     float sigma_ring2,
                                     float sigma_ring3,
                                     float sigma_ring4,
                                     float sigma_ring5,
                                     float expansion_factor
#ifdef FREAK_DEBUG
                                     ,
                                     float mapped_ring0[12],
                                     float mapped_ring1[12],
                                     float mapped_ring2[12],
                                     float mapped_ring3[12],
                                     float mapped_ring4[12],
                                     float mapped_ring5[12],
                                     float mapped_center[2],
                                     float& mapped_s0,
                                     float& mapped_s1,
                                     float& mapped_s2,
                                     float& mapped_s3,
                                     float& mapped_s4,
                                     float& mapped_s5,
                                     float& mapped_sc
#endif  
                                     ) {
        float S[9];
        
        float c[2];
        float r0[2*6];
        float r1[2*6];
        float r2[2*6];
        float r3[2*6];
        float r4[2*6];
        float r5[2*6];
        
        float sc, s0, s1, s2, s3, s4, s5;
        
        int octave, scale;
        
        // Ensure the scale of the similarity transform is at least "1".
        float transform_scale = point.scale*expansion_factor;
        if(transform_scale < 1) {
            transform_scale = 1;
        }
        
        // Transformation from canonical test locations to image
        Similarity(S, point.x, point.y, point.angle, transform_scale);
        
        // Locate center points
        c[0] = S[2];
        c[1] = S[5];
        
        // Locate ring 0 points
        MultiplyPointSimilarityInhomogenous(r0,    S, points_ring0);
        MultiplyPointSimilarityInhomogenous(r0+2,  S, points_ring0+2);
        MultiplyPointSimilarityInhomogenous(r0+4,  S, points_ring0+4);
        MultiplyPointSimilarityInhomogenous(r0+6,  S, points_ring0+6);
        MultiplyPointSimilarityInhomogenous(r0+8,  S, points_ring0+8);
        MultiplyPointSimilarityInhomogenous(r0+10, S, points_ring0+10);
        
        // Locate ring 1 points
        MultiplyPointSimilarityInhomogenous(r1,    S, points_ring1);
        MultiplyPointSimilarityInhomogenous(r1+2,  S, points_ring1+2);
        MultiplyPointSimilarityInhomogenous(r1+4,  S, points_ring1+4);
        MultiplyPointSimilarityInhomogenous(r1+6,  S, points_ring1+6);
        MultiplyPointSimilarityInhomogenous(r1+8,  S, points_ring1+8);
        MultiplyPointSimilarityInhomogenous(r1+10, S, points_ring1+10);
        
        // Locate ring 2 points
        MultiplyPointSimilarityInhomogenous(r2,    S, points_ring2);
        MultiplyPointSimilarityInhomogenous(r2+2,  S, points_ring2+2);
        MultiplyPointSimilarityInhomogenous(r2+4,  S, points_ring2+4);
        MultiplyPointSimilarityInhomogenous(r2+6,  S, points_ring2+6);
        MultiplyPointSimilarityInhomogenous(r2+8,  S, points_ring2+8);
        MultiplyPointSimilarityInhomogenous(r2+10, S, points_ring2+10);
        
        // Locate ring 3 points
        MultiplyPointSimilarityInhomogenous(r3,    S, points_ring3);
        MultiplyPointSimilarityInhomogenous(r3+2,  S, points_ring3+2);
        MultiplyPointSimilarityInhomogenous(r3+4,  S, points_ring3+4);
        MultiplyPointSimilarityInhomogenous(r3+6,  S, points_ring3+6);
        MultiplyPointSimilarityInhomogenous(r3+8,  S, points_ring3+8);
        MultiplyPointSimilarityInhomogenous(r3+10, S, points_ring3+10);
        
        // Locate ring 4 points
        MultiplyPointSimilarityInhomogenous(r4,    S, points_ring4);
        MultiplyPointSimilarityInhomogenous(r4+2,  S, points_ring4+2);
        MultiplyPointSimilarityInhomogenous(r4+4,  S, points_ring4+4);
        MultiplyPointSimilarityInhomogenous(r4+6,  S, points_ring4+6);
        MultiplyPointSimilarityInhomogenous(r4+8,  S, points_ring4+8);
        MultiplyPointSimilarityInhomogenous(r4+10, S, points_ring4+10);
        
        // Locate ring 5 points
        MultiplyPointSimilarityInhomogenous(r5,    S, points_ring5);
        MultiplyPointSimilarityInhomogenous(r5+2,  S, points_ring5+2);
        MultiplyPointSimilarityInhomogenous(r5+4,  S, points_ring5+4);
        MultiplyPointSimilarityInhomogenous(r5+6,  S, points_ring5+6);
        MultiplyPointSimilarityInhomogenous(r5+8,  S, points_ring5+8);
        MultiplyPointSimilarityInhomogenous(r5+10, S, points_ring5+10);
        
        // Transfer all the SIGMA values to the image
        sc = sigma_center*transform_scale;
        s0 = sigma_ring0*transform_scale;
        s1 = sigma_ring1*transform_scale;
        s2 = sigma_ring2*transform_scale;
        s3 = sigma_ring3*transform_scale;
        s4 = sigma_ring4*transform_scale;
        s5 = sigma_ring5*transform_scale;
        
        //
        // Locate and sample ring 5
        //
        
        pyramid->locate(octave, scale, s5);
        samples[0] = SampleReceptor(pyramid, r5[0],  r5[1], octave, scale);
        samples[1] = SampleReceptor(pyramid, r5[2],  r5[3], octave, scale);
        samples[2] = SampleReceptor(pyramid, r5[4],  r5[5], octave, scale);
        samples[3] = SampleReceptor(pyramid, r5[6],  r5[7], octave, scale);
        samples[4] = SampleReceptor(pyramid, r5[8],  r5[9], octave, scale);
        samples[5] = SampleReceptor(pyramid, r5[10], r5[11], octave, scale);
        
        //
        // Locate and sample ring 4
        //
        
        pyramid->locate(octave, scale, s4);
        samples[6]  = SampleReceptor(pyramid, r4[0],  r4[1], octave, scale);
        samples[7]  = SampleReceptor(pyramid, r4[2],  r4[3], octave, scale);
        samples[8]  = SampleReceptor(pyramid, r4[4],  r4[5], octave, scale);
        samples[9]  = SampleReceptor(pyramid, r4[6],  r4[7], octave, scale);
        samples[10] = SampleReceptor(pyramid, r4[8],  r4[9], octave, scale);
        samples[11] = SampleReceptor(pyramid, r4[10], r4[11], octave, scale);
        
        //
        // Locate and sample ring 3
        //
        
        pyramid->locate(octave, scale, s3);
        samples[12] = SampleReceptor(pyramid, r3[0],  r3[1], octave, scale);
        samples[13] = SampleReceptor(pyramid, r3[2],  r3[3], octave, scale);
        samples[14] = SampleReceptor(pyramid, r3[4],  r3[5], octave, scale);
        samples[15] = SampleReceptor(pyramid, r3[6],  r3[7], octave, scale);
        samples[16] = SampleReceptor(pyramid, r3[8],  r3[9], octave, scale);
        samples[17] = SampleReceptor(pyramid, r3[10], r3[11], octave, scale);
        
        //
        // Locate and sample ring 2
        //

        pyramid->locate(octave, scale, s2);
        samples[18] = SampleReceptor(pyramid, r2[0],  r2[1], octave, scale);
        samples[19] = SampleReceptor(pyramid, r2[2],  r2[3], octave, scale);
        samples[20] = SampleReceptor(pyramid, r2[4],  r2[5], octave, scale);
        samples[21] = SampleReceptor(pyramid, r2[6],  r2[7], octave, scale);
        samples[22] = SampleReceptor(pyramid, r2[8],  r2[9], octave, scale);
        samples[23] = SampleReceptor(pyramid, r2[10], r2[11], octave, scale);
        
        //
        // Locate and sample ring 1
        //

        pyramid->locate(octave, scale, s1);
        samples[24] = SampleReceptor(pyramid, r1[0],  r1[1], octave, scale);
        samples[25] = SampleReceptor(pyramid, r1[2],  r1[3], octave, scale);
        samples[26] = SampleReceptor(pyramid, r1[4],  r1[5], octave, scale);
        samples[27] = SampleReceptor(pyramid, r1[6],  r1[7], octave, scale);
        samples[28] = SampleReceptor(pyramid, r1[8],  r1[9], octave, scale);
        samples[29] = SampleReceptor(pyramid, r1[10], r1[11], octave, scale);
        
        //
        // Locate and sample ring 0
        //
        
        pyramid->locate(octave, scale, s0);
        samples[30] = SampleReceptor(pyramid, r0[0],  r0[1], octave, scale);
        samples[31] = SampleReceptor(pyramid, r0[2],  r0[3], octave, scale);
        samples[32] = SampleReceptor(pyramid, r0[4],  r0[5], octave, scale);
        samples[33] = SampleReceptor(pyramid, r0[6],  r0[7], octave, scale);
        samples[34] = SampleReceptor(pyramid, r0[8],  r0[9], octave, scale);
        samples[35] = SampleReceptor(pyramid, r0[10], r0[11], octave, scale);
        
        //
        // Locate and sample center
        //
        
        pyramid->locate(octave, scale, sc);
        samples[36] = SampleReceptor(pyramid, c[0], c[1], octave, scale);
        
#ifdef FREAK_DEBUG
        CopyVector(mapped_ring5, r5, 12);
        CopyVector(mapped_ring4, r4, 12);
        CopyVector(mapped_ring3, r3, 12);
        CopyVector(mapped_ring2, r2, 12);
        CopyVector(mapped_ring1, r1, 12);
        CopyVector(mapped_ring0, r0, 12);
        CopyVector(mapped_center, c, 2);
        
        mapped_s5 = s5;
        mapped_s4 = s4;
        mapped_s3 = s3;
        mapped_s2 = s2;
        mapped_s1 = s1;
        mapped_s0 = s0;
        mapped_sc = sc;
#endif
    
        return true;
    }
    
    /**
     * Compute the descriptor given the 37 samples from each receptor.
     */
    inline void CompareFREAK84(unsigned char desc[84], const float samples[37]) {
        int pos = 0;
        ZeroVector(desc, 84);
        for(int i = 0; i < 37; i++) {
            for(int j = i+1; j < 37; j++) {
                bitstring_set_bit(desc, pos, samples[i] < samples[j]);
                pos++;
            }
        }
        ASSERT(pos == 666, "Position is not within range");
    }
    
    /**
     * Extract a descriptor from the pyramid for a single point.
     */
    inline bool ExtractFREAK84(unsigned char desc[84],
                               const GaussianScaleSpacePyramid* pyramid,
                               const FeaturePoint& point,
                               const float points_ring0[12],
                               const float points_ring1[12],
                               const float points_ring2[12],
                               const float points_ring3[12],
                               const float points_ring4[12],
                               const float points_ring5[12],
                               float sigma_center,
                               float sigma_ring0,
                               float sigma_ring1,
                               float sigma_ring2,
                               float sigma_ring3,
                               float sigma_ring4,
                               float sigma_ring5,
                               float expansion_factor
#ifdef FREAK_DEBUG
                               ,
                               float mapped_ring0[12],
                               float mapped_ring1[12],
                               float mapped_ring2[12],
                               float mapped_ring3[12],
                               float mapped_ring4[12],
                               float mapped_ring5[12],
                               float mapped_center[2],
                               float& mapped_s0,
                               float& mapped_s1,
                               float& mapped_s2,
                               float& mapped_s3,
                               float& mapped_s4,
                               float& mapped_s5,
                               float& mapped_sc
#endif  
                               ) {
        float samples[37];
        
        // Create samples
        if(!SamplePyramidFREAK84(samples,
                                 pyramid,
                                 point,
                                 points_ring0,
                                 points_ring1,
                                 points_ring2,
                                 points_ring3,
                                 points_ring4,
                                 points_ring5,
                                 sigma_center,
                                 sigma_ring0,
                                 sigma_ring1,
                                 sigma_ring2,
                                 sigma_ring3,
                                 sigma_ring4,
                                 sigma_ring5,
                                 expansion_factor
#ifdef FREAK_DEBUG
                                 ,
                                 mapped_ring0,
                                 mapped_ring1,
                                 mapped_ring2,
                                 mapped_ring3,
                                 mapped_ring4,
                                 mapped_ring5,
                                 mapped_center,
                                 mapped_s0,
                                 mapped_s1,
                                 mapped_s2,
                                 mapped_s3,
                                 mapped_s4,
                                 mapped_s5,
                                 mapped_sc
#endif
                                 )) {
            return false;
        }
        
        // Once samples are created compute descriptor
        CompareFREAK84(desc, samples);
        
        return true;
    }
    
    /**
     * Extract the descriptors for all the feature points.
     */
    inline void ExtractFREAK84(BinaryFeatureStore& store,
                               const GaussianScaleSpacePyramid* pyramid,
                               const std::vector<FeaturePoint>& points,
                               const float points_ring0[12],
                               const float points_ring1[12],
                               const float points_ring2[12],
                               const float points_ring3[12],
                               const float points_ring4[12],
                               const float points_ring5[12],
                               float sigma_center,
                               float sigma_ring0,
                               float sigma_ring1,
                               float sigma_ring2,
                               float sigma_ring3,
                               float sigma_ring4,
                               float sigma_ring5,
                               float expansion_factor
#ifdef FREAK_DEBUG
                               ,
                               std::vector<Point2d<float> >& mapped_ring0,
                               std::vector<Point2d<float> >& mapped_ring1,
                               std::vector<Point2d<float> >& mapped_ring2,
                               std::vector<Point2d<float> >& mapped_ring3,
                               std::vector<Point2d<float> >& mapped_ring4,
                               std::vector<Point2d<float> >& mapped_ring5,
                               std::vector<Point2d<float> >& mapped_ringC,
                               std::vector<float>& mapped_s0,
                               std::vector<float>& mapped_s1,
                               std::vector<float>& mapped_s2,
                               std::vector<float>& mapped_s3,
                               std::vector<float>& mapped_s4,
                               std::vector<float>& mapped_s5,
                               std::vector<float>& mapped_sc
#endif
                               
                               ) {
        ASSERT(pyramid, "Pyramid is NULL");
        ASSERT(store.size() == points.size(), "Feature store has not been allocated");
        size_t num_points = 0;
        for(size_t i = 0; i < points.size(); i++) {
            
#ifdef FREAK_DEBUG
            std::vector<Point2d<float> > tmp_p0(6);
            std::vector<Point2d<float> > tmp_p1(6);
            std::vector<Point2d<float> > tmp_p2(6);
            std::vector<Point2d<float> > tmp_p3(6);
            std::vector<Point2d<float> > tmp_p4(6);
            std::vector<Point2d<float> > tmp_p5(6);
            Point2d<float> tmp_pc;
            float tmp_s0;
            float tmp_s1;
            float tmp_s2;
            float tmp_s3;
            float tmp_s4;
            float tmp_s5;
            float tmp_sc;
#endif
            
            if(!ExtractFREAK84(store.feature(num_points),
                               pyramid,
                               points[i],
                               points_ring0,
                               points_ring1,
                               points_ring2,
                               points_ring3,
                               points_ring4,
                               points_ring5,
                               sigma_center,
                               sigma_ring0,
                               sigma_ring1,
                               sigma_ring2,
                               sigma_ring3,
                               sigma_ring4,
                               sigma_ring5,
                               expansion_factor
#ifdef FREAK_DEBUG
                               ,
                               (float*)&tmp_p0[0],
                               (float*)&tmp_p1[0],
                               (float*)&tmp_p2[0],
                               (float*)&tmp_p3[0],
                               (float*)&tmp_p4[0],
                               (float*)&tmp_p5[0],
                               (float*)&tmp_pc,
                               tmp_s0,
                               tmp_s1,
                               tmp_s2,
                               tmp_s3,
                               tmp_s4,
                               tmp_s5,
                               tmp_sc
#endif
                               )) {
                continue;
            }
#ifdef FREAK_DEBUG
            mapped_ring0.insert(mapped_ring0.end(), tmp_p0.begin(), tmp_p0.end());
            mapped_ring1.insert(mapped_ring1.end(), tmp_p1.begin(), tmp_p1.end());
            mapped_ring2.insert(mapped_ring2.end(), tmp_p2.begin(), tmp_p2.end());
            mapped_ring3.insert(mapped_ring3.end(), tmp_p3.begin(), tmp_p3.end());
            mapped_ring4.insert(mapped_ring4.end(), tmp_p4.begin(), tmp_p4.end());
            mapped_ring5.insert(mapped_ring5.end(), tmp_p5.begin(), tmp_p5.end());
            mapped_ringC.push_back(tmp_pc);
            
            mapped_s0.push_back(tmp_s0);
            mapped_s1.push_back(tmp_s1);
            mapped_s2.push_back(tmp_s2);
            mapped_s3.push_back(tmp_s3);
            mapped_s4.push_back(tmp_s4);
            mapped_s5.push_back(tmp_s5);
            mapped_sc.push_back(tmp_sc);
#endif
            
            store.point(num_points) = points[i];
            num_points++;
        }
        ASSERT(num_points == points.size(), "Should be same size");
        
        // Shrink store down to the number of valid points
        store.resize(num_points);
    }
    
} // vision