/*
 *  arImageProc.h
 *  artoolkitX
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

/*!
    @file arImageProc.h
    @brief   artoolkitX functions for 2D luminance image processing.
    @details
*/

#ifndef AR_IMAGEPROC_H
#define AR_IMAGEPROC_H

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#include <ARX/AR/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#  define AR_IMAGEPROC_USE_VIMAGE 1
#endif

/*!
    @brief Structure holding settings for an instance of the image-processing pipeline.
 */
struct _ARImageProcInfo {
    unsigned char *__restrict image2;   ///< Extra buffer, allocated as required.
    int imageX;                         ///< Width of image buffer.
    int imageY;                         ///< Height of image buffer.
    unsigned long histBins[256];        ///< Luminance histogram.
    unsigned long cdfBins[256];         ///< Luminance cumulative density function.
    unsigned char min;                  ///< Minimum luminance.
    unsigned char max;                  ///< Maximum luminance.
#if AR_IMAGEPROC_USE_VIMAGE
    void *tempBuffer;                   ///< Extra buffer when using macOS/iOS vImage framework.
#endif
};
typedef struct _ARImageProcInfo ARImageProcInfo;
    
#ifdef __cplusplus
}
#endif

#include <ARX/AR/ar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
    @brief Initialise image processing.
    @details
        This function creates the ARImageProcInfo structure required
        for other image processing functions. The size of the image
        that will be processed is fixed by this call.
    @param xsize Width of the images that will be processed, in pixels.
    @param ysize Height of the images that will be processed, in pixels.
    @result Pointer to the ARImageProcInfo structure. When processing
        is complete, this structure should be disposed of by calling
        arImageProcFinal.
    @see arImageProcFinal
 */
ARImageProcInfo *arImageProcInit(const int xsize, const int ysize);

/*!
    @brief Finish image processing and free memory.
    @details
        When processing is complete, the ARImageProcInfo structure should
        be disposed of by calling this function.
    @param ipi ARImageProcInfo structure to be disposed of, as created by arImageProcInit.
    @see arImageProcInit
 */
void arImageProcFinal(ARImageProcInfo *ipi);

/*!
    @brief Calculate luminance histogram.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
 
        On macOS and iOS, the calculation is accelerated using the Accelerate framework.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHist(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr);

/*!
    @brief Get luminance histogram as an image.
    @details 
        Returns a pointer to a buffer containing a 256x256 8-bit grayscale texture.
        The texture can be uploaded to an OpenGL texture with the calls:
        @code
            buf = arImageProcGetHistImage(ipi);
            if (buf) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);
                free(buf);
            }
        @endcode
        After uploading, the texture can be drawn as with any other OpenGL texture.
    @param ipi ARImageProcInfo structure holding the most recently processed histogram,
        as generated by arImageProcLumaHist etc.
    @result Pointer to the texture data.
 */
unsigned char *arImageProcGetHistImage(ARImageProcInfo *ipi);

/*!
    @brief Calculate image histogram and cumulative density function.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndCDF(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr);

/*!
    @brief Calculate image histogram, cumulative density function, and luminance value at a given histogram percentile.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndCDFAndPercentile(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, const float percentile, unsigned char *value_p);

/*!
    @brief Calculate image histogram, cumulative density function, and median luminance value.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndCDFAndMedian(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, unsigned char *value_p);

/*!
    @brief Calculate image histogram, and binarize image using Otsu's method for foreground/background separation.
 
        See http://en.wikipedia.org/wiki/Otsu's_method fore more information.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndOtsu(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, unsigned char *value_p);

/*!
    @brief Calculate image histogram, and box filter image.
    @details 
        See https://developer.apple.com/library/ios/documentation/Performance/Reference/vImage_convolution/
        On macOS and iOS, the calculation is accelerated using the Accelerate framework.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndBoxFilterWithBias(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, const int boxSize, const int bias);

/*!
    @brief Calculate image histogram, cumulative density function, and minimum and maximum luminance values.
    @param ipi ARImageProcInfo structure describing the format of the image
        to be processed, as created by arImageProcInit.
    @result 0 in case of success, or a value less than 0 in case of error.
 */
int arImageProcLumaHistAndCDFAndLevels(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr);

#ifdef __cplusplus
}
#endif

#endif // !AR_IMAGEPROC_H
