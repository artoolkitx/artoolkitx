/*
 *  arImageProc.c
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

#include <string.h> // memset(), memcpy()
#include <ARX/AR/arImageProc.h>
#if AR_IMAGEPROC_USE_VIMAGE
#  include <Accelerate/Accelerate.h>
#endif

ARImageProcInfo *arImageProcInit(const int xsize, const int ysize)
{
    ARImageProcInfo *ipi = (ARImageProcInfo *)malloc(sizeof(ARImageProcInfo));
    if (ipi) {
        ipi->image2 = NULL;
        ipi->imageX = xsize;
        ipi->imageY = ysize;
#if AR_IMAGEPROC_USE_VIMAGE
        ipi->tempBuffer = NULL;
#endif
    }
    return (ipi);
}

void arImageProcFinal(ARImageProcInfo *ipi)
{
    if (!ipi) return;
    if (ipi->image2) free (ipi->image2);
#if AR_IMAGEPROC_USE_VIMAGE
    if (ipi->tempBuffer) free (ipi->tempBuffer);
#endif
    free (ipi);
}

int arImageProcLumaHist(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr)
{
	if (!ipi || !dataPtr) return (-1);

#ifdef AR_IMAGEPROC_USE_VIMAGE
    vImage_Error err;
    vImage_Buffer buf = {(void *)dataPtr, ipi->imageY, ipi->imageX, ipi->imageX};
    if ((err = vImageHistogramCalculation_Planar8(&buf, ipi->histBins, 0)) != kvImageNoError) {
        ARLOGe("arImageProcLumaHist(): vImageHistogramCalculation_Planar8 error %ld.\n", err);
        return (-1);
    }
#else
    unsigned char *__restrict p;
    memset(ipi->histBins, 0, sizeof(ipi->histBins));
    for (p = (unsigned char *__restrict)dataPtr; p < dataPtr + ipi->imageX*ipi->imageY; p++) ipi->histBins[*p]++;
#endif // AR_IMAGEPROC_USE_VIMAGE
    
    return (0);
}

unsigned char *arImageProcGetHistImage(ARImageProcInfo *ipi)
{
    int i, j, y;
    unsigned long peak = 0;
    float scalef;
    
    if (!ipi) return (NULL);
    
    unsigned char *buf = (unsigned char *)calloc(1, 256*256*sizeof(unsigned char));
    if (buf) {
        for (i = 0; i < 256; i++) if (ipi->histBins[i] > peak) peak = ipi->histBins[i]; // Find value of mode.
        scalef = 256.0f / (float)peak;
        for (i = 0; i < 256; i++) {
            y = (int)((float)ipi->histBins[i] * scalef);
            if (y > 256) y = 256; // Safety in case of FP rounding errors etc.
            for (j = 0; j < y; j++) buf[256*j + i] = 0xff;
        }
    }
    return (buf);
}

int arImageProcLumaHistAndCDF(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr)
{
    unsigned long cdfCurrent;
	unsigned char i;

	int ret = arImageProcLumaHist(ipi, dataPtr);
    if (ret < 0) return (ret);

    cdfCurrent = 0;
    i = 0;
    do {
        ipi->cdfBins[i] = cdfCurrent + ipi->histBins[i];
        cdfCurrent = ipi->cdfBins[i];
        i++;
    } while (i != 0);
    return (0);
}

int arImageProcLumaHistAndCDFAndPercentile(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, const float percentile, unsigned char *value_p)
{
	int ret;
	unsigned int requiredCD;
	unsigned char i, j;

    if (percentile < 0.0f || percentile > 1.0f) return (-1);
    
    ret = arImageProcLumaHistAndCDF(ipi, dataPtr);
    if (ret < 0) return (ret);
    
    requiredCD = (unsigned int)(ipi->imageX * ipi->imageY * percentile);
    i = 0;
    while (ipi->cdfBins[i] < requiredCD) i++; // cdfBins[i] >= requiredCD
    j = i;
    while (ipi->cdfBins[j] == requiredCD) j++; // cdfBins[j] > requiredCD    
    *value_p = (unsigned char)((i + j) / 2);
    return (0);
}

int arImageProcLumaHistAndCDFAndMedian(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, unsigned char *value_p)
{
    return (arImageProcLumaHistAndCDFAndPercentile(ipi, dataPtr, 0.5f, value_p));
}

int arImageProcLumaHistAndOtsu(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, unsigned char *value_p)
{
    int ret;
    unsigned char i;
    
    ret = arImageProcLumaHist(ipi, dataPtr);
    if (ret < 0) return (ret);
    
    float sum = 0.0f;
    i = 1;
    do {
        sum += ipi->histBins[i] * i;
        i++;
    } while (i != 0);
    
    float count = (float)(ipi->imageX * ipi->imageY);
    float sumB = 0.0f;
    float wB = 0.0f;
    float wF = 0.0f;
    float varMax = 0.0f;
    unsigned char threshold = 0;
    i = 0;
    do {
        wB += ipi->histBins[i];          // Weight background.
        if (wB != 0.0f) {
            wF = count - wB;                 // Weight foreground.
            if (wF == 0.0f) break;
            
            sumB += (float)(i * ipi->histBins[i]);
            
            float mB = sumB / wB;            // Mean background.
            float mF = (sum - sumB) / wF;    // Mean foreground.
            
            // Calculate between-class variance.
            float varBetween = wB * wF * (mB - mF) * (mB - mF);
            
            // Check if new maximum found.
            if (varBetween > varMax) {
                varMax = varBetween;
                threshold = i;
            }
        }
        i++;
    } while (i != 0);
    
    *value_p = threshold;
    return (0);
}

int arImageProcLumaHistAndBoxFilterWithBias(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr, const int boxSize, const int bias)
{
    int ret, i;
#if !AR_IMAGEPROC_USE_VIMAGE
    int j, kernelSizeHalf;
#endif
    
    ret = arImageProcLumaHist(ipi, dataPtr);
    if (ret < 0) return (ret);
    
    if (!ipi->image2) {
        ipi->image2 = (unsigned char *)malloc(ipi->imageX * ipi->imageY * sizeof(unsigned char));
        if (!ipi->image2) return (-1);
    }
#if AR_IMAGEPROC_USE_VIMAGE
    vImage_Error err;
    vImage_Buffer src = {(void *)dataPtr, ipi->imageY, ipi->imageX, ipi->imageX};
    vImage_Buffer dest = {ipi->image2, ipi->imageY, ipi->imageX, ipi->imageX};
    if (!ipi->tempBuffer) {
        // Request size of buffer, and allocate.
        if ((err = vImageBoxConvolve_Planar8(&src, &dest, NULL, 0, 0, boxSize, boxSize, '\0', kvImageTruncateKernel | kvImageGetTempBufferSize)) < 0) return (-1);
        if (!(ipi->tempBuffer = malloc(err))) return (-1);
    }
    err = vImageBoxConvolve_Planar8(&src, &dest, ipi->tempBuffer, 0, 0, boxSize, boxSize, '\0', kvImageTruncateKernel);
    if (err != kvImageNoError) {
        ARLOGe("Error %ld in vImageBoxConvolve_Planar8().\n", err);
        return (-1);
    }
#else
    kernelSizeHalf = boxSize >> 1;
    for (j = 0; j < ipi->imageY; j++) {
        for (i = 0; i < ipi->imageX; i++) {
            int val, count, kernel_i, kernel_j, ii, jj;
            val = count = 0;
            for (kernel_j = -kernelSizeHalf; kernel_j <= kernelSizeHalf; kernel_j++) {
                jj = j + kernel_j;
                if (jj < 0 || jj >= ipi->imageY) continue;
                for (kernel_i = -kernelSizeHalf; kernel_i <= kernelSizeHalf; kernel_i++) {
                    ii = i + kernel_i;
                    if (ii < 0 || ii >= ipi->imageX) continue;
                    val += dataPtr[ii + jj*(ipi->imageX)];
                    count++;
                }
            }
            ipi->image2[i + j*(ipi->imageX)] = val / count;
        }
    }
#endif
    if (bias) for (i = 0; i < ipi->imageX*ipi->imageY; i++) ipi->image2[i] += bias;
    return (0);
}

int arImageProcLumaHistAndCDFAndLevels(ARImageProcInfo *ipi, const ARUint8 *__restrict dataPtr)
{
	unsigned char l;
	unsigned int maxCD;

    int ret = arImageProcLumaHistAndCDF(ipi, dataPtr);
    if (ret < 0) return (ret);

    // Find min and max values.
    l = 0;
    while (ipi->cdfBins[l] == 0) l++;
    ipi->min = l;
    maxCD = ipi->imageX*ipi->imageY;
    while (ipi->cdfBins[l] < maxCD) l++;
    ipi->max = l;
    
    return (0);
}
