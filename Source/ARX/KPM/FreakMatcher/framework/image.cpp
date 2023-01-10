//
//  image.cpp
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

#include "image.h"
#include "error.h"
#include <string.h>

using namespace vision;

// Array deleter for shared pointer
template<typename T>
class NullArrayDeleter {
public:
    void operator () (T* ptr) const
    {}
};

Image::Image()
: mType(IMAGE_UNKNOWN)
, mWidth(0)
, mHeight(0)
, mStep(0)
, mChannels(0)
, mSize(0) {
}

Image::Image(Image& image)
: mType(IMAGE_UNKNOWN)
, mWidth(0)
, mHeight(0)
, mStep(0)
, mChannels(0)
, mSize(0) {
    shallowCopy(image);
}

Image::Image(const Image& image)
: mType(IMAGE_UNKNOWN)
, mWidth(0)
, mHeight(0)
, mStep(0)
, mChannels(0)
, mSize(0) {
    shallowCopy(image);
}

Image::Image(ImageType type,
             size_t width,
             size_t height,
             int step,
             size_t channels)
: mType(IMAGE_UNKNOWN)
, mWidth(0)
, mHeight(0)
, mStep(0)
, mChannels(0)
, mSize(0) {
    alloc(type, width, height, step, channels);
}

Image::Image(unsigned char* data,
             ImageType type,
             size_t width,
             size_t height,
             int step,
             size_t channels) 
: mType(type)
, mWidth(width)
, mHeight(height)
, mChannels(channels)
, mSize(step*height)
, mData(data, NullArrayDeleter<unsigned char>()) {
    // Find the step size
    if(step < 0) {
        switch(step) {
            case AUTO_STEP:
            default:
                mStep = calculate_unit_size(type)*channels*width;
                break;
        };
    } else {
        mStep = step;
    }
}

Image::~Image() {}

void Image::alloc(ImageType type,
                  size_t width,
                  size_t height,
                  int step,
                  size_t channels) {
    size_t size;
    
    ASSERT(width > 0, "Width cannot be zero");
    ASSERT(height > 0, "Height cannot be zero");
    ASSERT(step >= width, "Step must be greater than or equal the width");
    ASSERT(channels > 0, "Number of channels cannot be zero");
    
    // Find the step size
    if(step < 0) {
        switch(step) {
            case AUTO_STEP:
            default:
                mStep = calculate_unit_size(type)*channels*width;
                break;
        };
    } else {
        mStep = step;
    }
    
    size = mStep*height;
    
    // Allocate image data
    if(mSize != size) {
        mData.reset(new unsigned char[size]);
        ASSERT(mData.get(), "Data pointer is NULL");
        
        if(mData.get() == NULL) {
            throw EXCEPTION("Unable to allocate image data");
        }
    }
    
    // Set variables
    mType       = type;
    mWidth      = width;
    mHeight     = height;
    mChannels   = channels;
    mSize       = size;
}

void Image::release() {
    // Reset variables
    mType       = IMAGE_UNKNOWN;
    mWidth      = 0;
    mHeight     = 0;
    mStep       = 0;
    mChannels   = 0;
    mSize       = 0;
    
    // Release the data
    mData.reset();
}

void Image::deepCopy(const Image& image) {
    // Allocate memory.
    // The ALLOC function will allocate memory if the
    // images are not the same size.
    alloc(image.type(),
          image.width(),
          image.height(),
          (int)image.step(),
          image.channels());
    
    // Copy the image data
    memcpy(mData.get(), image.mData.get(), image.size());
}

void Image::shallowCopy(const Image& image) {
    // Set variables
    mType       = image.mType;
    mWidth      = image.mWidth;
    mHeight     = image.mHeight;
    mStep       = image.mStep;
    mChannels   = image.mChannels;
    mSize       = image.mSize;
    
    // Shallow copy on the data
    mData       = image.mData;
}

Image& Image::operator=(Image& image) {
    shallowCopy(image);
    return *this;
}

Image& Image::operator=(const Image& image) {
    shallowCopy(image);
    return *this;
}

size_t Image::calculate_unit_size(ImageType type) {
    size_t size;
    
    switch(type) {
        case IMAGE_UINT8:
            size = 1;
            break;
        case IMAGE_F32:
            size = sizeof(float);
            break;
        default:
            throw EXCEPTION("Invalid image type");
    };
    
    return size;
}
