//
//  keyframe.h
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

#include "feature_store.h"
#include "binary_hierarchical_clustering.h"

namespace vision {
    
    template<int NUM_BYTES_PER_FEATURE>
    class Keyframe {
    public:
        
        typedef Keyframe<NUM_BYTES_PER_FEATURE> keyframe_t;
        typedef BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE> index_t;
        
        Keyframe() : mWidth(0), mHeight(0) {}
        ~Keyframe() {}
        
        /**
         * Get/Set image width.
         */
        inline void setWidth(int width) { mWidth = width; }
        inline int width() const { return mWidth; }
        
        /**
         * Get/Set image height.
         */
        inline void setHeight(int height) { mHeight = height; }
        inline int height() const { return mHeight; }
        
        /**
         * @return Feature store.
         */
        inline BinaryFeatureStore& store() { return mStore; }
        inline const BinaryFeatureStore& store() const { return mStore; }
        
        /**
         * @return Index over the features.
         */
        inline const index_t& index() const { return mIndex; }
        
        /**
         * Build an index for the features.
         */
        void buildIndex();
        
        /**
         * Copy a keyframe.
         */
        void copy(const keyframe_t& keyframe) {
            mWidth = keyframe.mWidth;
            mHeight = keyframe.mHeight;
            mStore.copy(keyframe.store());
        }
        
        //
        // Serialization
        //
        
        /*template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & mWidth;
            ar & mHeight;
            ar & mStore;
        }*/
        
    private:
        
        // Image width and height
        int mWidth;
        int mHeight;
        
        // Feature store
        BinaryFeatureStore mStore;
        
        // Feature index
        index_t mIndex;
        
    }; // Keyframe
    
    template<int NUM_BYTES_PER_FEATURE>
    void Keyframe<NUM_BYTES_PER_FEATURE>::buildIndex() {
        mIndex.setNumHypotheses(128);
        mIndex.setNumCenters(8);
        mIndex.setMaxNodesToPop(8);
        mIndex.setMinFeaturesPerNode(16);
        mIndex.build(&mStore.features()[0], (int)mStore.size());
    }
    
} // vision