//
//  feature_store.h
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

#include <vector>
#include "feature_point.h"

//#include <boost/serialization/serialization.hpp>
//#include <boost/serialization/vector.hpp>

namespace vision {

    /**
     * Represents a container for features and point information.
     */
    class BinaryFeatureStore {
    public:
        
        BinaryFeatureStore(int bytesPerFeature)
        : mNumBytesPerFeature(bytesPerFeature) {}
        BinaryFeatureStore()
        : mNumBytesPerFeature(0) {}
        ~BinaryFeatureStore() {}
    
        /**
         * Resize the feature store to hold NUMFEATURES.
         */
        inline void resize(size_t numFeatures) {
            mFeatures.resize(mNumBytesPerFeature*numFeatures, 0);
            mPoints.resize(numFeatures);
        }
        
        /**
         * @return Number of features.
         */
        inline size_t size() const {
            return mPoints.size();
        }
        
        /**
         * Set number of bytes per feature.
         */
        inline void setNumBytesPerFeature(int bytesPerFeature) { mNumBytesPerFeature = bytesPerFeature; }
        
        /**
         * @return Number of bytes per feature
         */
        inline int numBytesPerFeature() const { return mNumBytesPerFeature; }
        
        /**
         * @return Vector of features
         */
        inline std::vector<unsigned char>& features() { return mFeatures; }
        inline const std::vector<unsigned char>& features() const { return mFeatures; }
        
        /**
         * @return Specific feature with an index
         */
        inline unsigned char* feature(size_t i) { return &mFeatures[i*mNumBytesPerFeature]; }
        inline const unsigned char* feature(size_t i) const { return &mFeatures[i*mNumBytesPerFeature]; }
        
        /**
         * @return Vector of feature points
         */
        inline std::vector<FeaturePoint>& points() { return mPoints; }
        inline const std::vector<FeaturePoint>& points() const { return mPoints; }
        
        /**
         * @return Specific point with an index
         */
        inline FeaturePoint& point(size_t i) { return mPoints[i]; }
        inline const FeaturePoint& point(size_t i) const { return mPoints[i]; }
    
        /**
         * Copy a feature store.
         */
        void copy(const BinaryFeatureStore& store) {
            mNumBytesPerFeature = store.mNumBytesPerFeature;
            mFeatures = store.mFeatures;
            mPoints = store.mPoints;
        }
        
        //
        // Serialization
        //
        
        /*template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & mNumBytesPerFeature;
            ar & mFeatures;
            ar & mPoints;
        }*/
        
    private:
    
        // Number of bytes per feature
        int mNumBytesPerFeature;
        
        // Vector of features
        std::vector<unsigned char> mFeatures;
    
        // Vector of feature points
        std::vector<FeaturePoint> mPoints;
    };

} // vision