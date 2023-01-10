//
//  image.h
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

#include "exception.h"
#include "error.h"
#include <memory>

namespace vision {
    
    /**
     * Enumeration for the different image types.
     */
    enum ImageType {
        IMAGE_UNKNOWN = 0,
        IMAGE_UINT8,
        IMAGE_F32
    };
    
    /**
     * Type of image step.
     */
    enum StepType {
        AUTO_STEP = -1
    };
    
    /**
     * Represents images that used in the framework.
     */
    class Image {
    public:
        
        Image();
        Image(Image& image);
        Image(const Image& image);
        Image(ImageType type,
              size_t width,
              size_t height,
              int step,
              size_t channels);
        Image(unsigned char* data,
              ImageType type,
              size_t width,
              size_t height,
              int step,
              size_t channels);
        ~Image();
        
        /**
         * Allocate the image data.
         */
        void alloc(ImageType type,
                   size_t width,
                   size_t height,
                   int step,
                   size_t channels);
        
        /**
         * Release the memory associated with the image.
         */
        void release();
        
        /**
         * Get a pointer to the image data.
         */
        inline unsigned char* get() {
            return mData.get();
        }
        inline const unsigned char* get() const {
            return mData.get();
        }
        
        /**
         * Get a pointer to the image data via template functions.
         */
        template<typename T>
        inline T* get() {
            return (T*)(mData.get());
        }
        template<typename T>
        inline const T* get() const {
            return (const T*)(mData.get());
        }
        template<typename T>
        inline T* get(size_t row) {
            ASSERT(row < mHeight, "row out of bounds");
            return (T*)(mData.get() + row*mStep);
        }
        template<typename T>
        inline const T* get(size_t row) const {
            ASSERT(row < mHeight, "row out of bounds");
            return (const T*)(mData.get() + row*mStep);
        }
        
        /**
         * Get the image type.
         */
        inline ImageType type() const { return mType; }
        
        /**
         * Set the dimensions of the image.
         */
        inline void setWidth(size_t width) { mWidth = width; }
        inline void setHeight(size_t height) { mHeight = height; }
        inline void setStep(size_t step) { mStep = step; }
        inline void setChannels(size_t channels) { mChannels = channels; }
        
        /**
         * Get the dimensions of the image.
         */
        inline size_t width() const { return mWidth; }
        inline size_t height() const { return mHeight; }
        inline size_t step() const { return mStep; }
        inline size_t channels() const { return mChannels; }
        inline size_t size() const { return mSize; }
        
        /**
         * Copy functions.
         */
        void deepCopy(const Image& image);
        void shallowCopy(const Image& image);
        
        /**
         * Copy an image by doing a shallow copy.
         */
        Image& operator=(Image& image);
        Image& operator=(const Image& image);
        
        /**
         * Calculate the size of a single unit.
         */
        static size_t calculate_unit_size(ImageType type);
        
    private:
        
        // Image type
        ImageType mType;
        
        // Dimensions
        size_t mWidth;
        size_t mHeight;
        size_t mStep;
        size_t mChannels;
        
        // Size of the allocated image
        size_t mSize;
        
        // Image data
        std::shared_ptr<unsigned char> mData;
    };
    
} // vision
