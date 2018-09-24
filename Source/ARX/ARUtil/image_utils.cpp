/*
 *  image_utils.cpp
 *  artoolkitX
 *
 *  Implements a basic image loading/saving system.
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *
 *  Author(s): Daniel Bell
 *
 */

#include <ARX/ARUtil/image_utils.h>
 
#include <memory>

#ifndef STBI_INCLUDE_STB_IMAGE_H
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#endif
#endif

std::string getFileExtension(const std::string& filename) {
    return filename.find_last_of(".") != std::string::npos ? filename.substr(filename.find_last_of(".")) : "";
}

bool ReadImageFromFile(const char* fileName, std::shared_ptr<unsigned char> &refImage, int *cols, int *rows, int *nc, bool forceMono)
{
    if (!fileName || !cols || !rows) return false;
    std::string ext = getFileExtension(fileName);
    try {
        unsigned char* data = stbi_load(fileName, cols, rows, nc, forceMono ? 1 : 0);
        if (data) {
            refImage.reset(data, free);
            return true;
        } else {
            return false;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

bool WriteImageTofile(unsigned char* data, int width, int height, int stride, std::string fileName, bool colourImage)
{
    if (!data) return false;
    std::string ext = getFileExtension(fileName);
    if (ext==".png") {
        if (colourImage) {
            return stbi_write_png(fileName.c_str(), (int) width, (int) height, (int) 3, data, (int) stride);
        } else {
            return stbi_write_png(fileName.c_str(), (int) width, (int) height, (int) 1, data, (int) stride);
        }
    } else if(ext==".bmp") {
        if (colourImage) {
            return stbi_write_bmp(fileName.c_str(), (int) width, (int) height, (int) 3, data);
        } else {
            return stbi_write_bmp(fileName.c_str(), (int) width, (int) height, (int) 1, data);
        }
    }
    return false;
}

void ReleaseSTBImage(unsigned char* data)
{
    stbi_image_free(data);
}
