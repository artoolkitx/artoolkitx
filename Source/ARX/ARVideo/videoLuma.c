/*
 *  videoLuma.c
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
 *
 *  Author(s): Philip Lamb
 *
 */

#include <ARX/ARVideo/video.h>

#include <stdlib.h>
#if defined(ANDROID)
#  define AR_PAGE_ALIGNED_ALLOC(s) memalign(4096,s)
#  define AR_PAGE_ALIGNED_FREE(p) free(p)
#elif defined(_WIN32)
#  define AR_PAGE_ALIGNED_ALLOC(s) _aligned_malloc(s, 4096)
#  define AR_PAGE_ALIGNED_FREE(p) _aligned_free(p)
#else
#  define AR_PAGE_ALIGNED_ALLOC(s) valloc(s)
#  define AR_PAGE_ALIGNED_FREE(p) free(p)
#endif

#if HAVE_ARM_NEON || HAVE_ARM64_NEON
#  include <arm_neon.h>
#endif
#if HAVE_INTEL_SIMD
#  include <emmintrin.h> // SSE2.
#  include <pmmintrin.h> // SSE3.
#  include <tmmintrin.h> // SSSE3.
#endif
#if defined(ANDROID) && (HAVE_ARM_NEON || HAVE_INTEL_SIMD)
#  include "cpu-features.h"
#endif

// CCIR 601 recommended values. See http://www.poynton.com/notes/colour_and_gamma/ColorFAQ.html#RTFToC11 .
#define R8_CCIR601 77
#define G8_CCIR601 150
#define B8_CCIR601 29

struct _ARVideoLumaInfo {
    int xsize;
    int ysize;
    int buffSize;
    AR_PIXEL_FORMAT pixFormat;
#if HAVE_ARM_NEON || HAVE_ARM64_NEON || HAVE_INTEL_SIMD
    int fastPath;
#endif
    ARUint8 *__restrict buff;
};

#if HAVE_ARM_NEON || HAVE_ARM64_NEON
static void arVideoLumaBGRAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaRGBAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaABGRtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaARGBtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
#elif HAVE_INTEL_SIMD
static void arVideoLumaBGRAtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaRGBAtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaABGRtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
static void arVideoLumaARGBtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels);
#endif


ARVideoLumaInfo *arVideoLumaInit(int xsize, int ysize, AR_PIXEL_FORMAT pixFormat)
{
    ARVideoLumaInfo *vli;
    
    vli = (ARVideoLumaInfo *)calloc(1, sizeof(ARVideoLumaInfo));
    if (!vli) {
        ARLOGe("Out of memory!!\n");
        return (NULL);
    }
    vli->xsize = xsize;
    vli->ysize = ysize;
    vli->buffSize = xsize*ysize;
	vli->buff = (ARUint8 *)AR_PAGE_ALIGNED_ALLOC(vli->buffSize);
    if (!vli->buff) {
        ARLOGe("Out of memory!!\n");
        free(vli);
        return (NULL);
    }
    vli->pixFormat = pixFormat;
    
    // Accelerated RGB to luma conversion.
#if HAVE_ARM_NEON || HAVE_ARM64_NEON || HAVE_INTEL_SIMD
    vli->fastPath = (xsize * ysize % 8 == 0
                     && (pixFormat == AR_PIXEL_FORMAT_RGBA
                         || pixFormat == AR_PIXEL_FORMAT_BGRA
                         || pixFormat == AR_PIXEL_FORMAT_ABGR
                         ||pixFormat == AR_PIXEL_FORMAT_ARGB
                         )
                     );
    // Under Windows, Linux and OS X, we assume a minimum of Intel Core2, which satifisfies the requirement for SSE2, SSE3 and SSSE3 support.
    // Under iOS, we assume a minimum of ARMv7a with NEON.
#  if defined(ANDROID) && (HAVE_ARM_NEON || HAVE_INTEL_SIMD)
    // Not all Android devices with ARMv7 CPUs are guaranteed to have NEON, so check.
    // Also, need to check Android devices with x86 and x86_64 CPUs for appropriate level of SIMD support.
    uint64_t features = android_getCpuFeatures();
    vli->fastPath = vli->fastPath &&
                                    (((features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON)) ||
                                     ((features & ANDROID_CPU_FAMILY_X86 || features & ANDROID_CPU_FAMILY_X86_64) && (features & ANDROID_CPU_X86_FEATURE_SSSE3))); // Can also test for: ANDROID_CPU_X86_FEATURE_POPCNT, ANDROID_CPU_X86_FEATURE_SSE4_1, ANDROID_CPU_X86_FEATURE_SSE4_2, ANDROID_CPU_X86_FEATURE_MOVBE.
#  endif
    // Debug output.
#  if HAVE_ARM_NEON || HAVE_ARM64_NEON
    if (vli->fastPath) ARLOGi("arVideoLuma will use ARM NEON acceleration.\n");
	else ARLOGd("arVideoLuma will NOT use ARM NEON acceleration.\n");
#  elif HAVE_INTEL_SIMD
    if (vli->fastPath) ARLOGi("arVideoLuma will use Intel SIMD acceleration.\n");
	else ARLOGd("arVideoLuma will NOT use Intel SIMD acceleration.\n");
#  endif
#endif

    return (vli);
}

int arVideoLumaFinal(ARVideoLumaInfo **vli_p)
{
    if (!vli_p) return (-1);
    if (!*vli_p) return (0);
    
	AR_PAGE_ALIGNED_FREE((*vli_p)->buff);
    free(*vli_p);
    *vli_p = NULL;
    
    return (0);
}

ARUint8 *__restrict arVideoLuma(ARVideoLumaInfo *vli, const ARUint8 *__restrict dataPtr)
{
    unsigned int p, q;
    
    AR_PIXEL_FORMAT pixFormat = vli->pixFormat;
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
    if (vli->fastPath) {
        if (pixFormat == AR_PIXEL_FORMAT_BGRA) {
            arVideoLumaBGRAtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA) {
            arVideoLumaRGBAtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_ABGR) {
            arVideoLumaABGRtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else /*(pixFormat == AR_PIXEL_FORMAT_ARGB)*/ {
            arVideoLumaARGBtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        }
        return (vli->buff);
    }
#  elif HAVE_INTEL_SIMD
    if (vli->fastPath) {
        if (pixFormat == AR_PIXEL_FORMAT_BGRA) {
            arVideoLumaBGRAtoL_Intel_simd_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA) {
            arVideoLumaRGBAtoL_Intel_simd_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_ABGR) {
            arVideoLumaABGRtoL_Intel_simd_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else /*(pixFormat == AR_PIXEL_FORMAT_ARGB)*/ {
            arVideoLumaARGBtoL_Intel_simd_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        }
        return (vli->buff);
    }
#endif
    if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21) {
        memcpy(vli->buff, dataPtr, vli->buffSize);
    } else {
        q = 0;
        if (pixFormat == AR_PIXEL_FORMAT_RGBA) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + B8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_BGRA) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + R8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_ARGB) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 1] + G8_CCIR601*dataPtr[q + 2] + B8_CCIR601*dataPtr[q + 3]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_ABGR) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 1] + G8_CCIR601*dataPtr[q + 2] + R8_CCIR601*dataPtr[q + 3]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGB) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + B8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 3;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_BGR) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + R8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 3;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_yuvs) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = dataPtr[q + 0];
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_2vuy) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = dataPtr[q + 1];
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGB_565) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf8) + 4) + G8_CCIR601*(((dataPtr[q + 0] & 0x07) << 5) + ((dataPtr[q + 1] & 0xe0) >> 3) + 2) + B8_CCIR601*(((dataPtr[q + 1] & 0x1f) << 3) + 4)) >> 8;
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA_5551) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf8) + 4) + G8_CCIR601*(((dataPtr[q + 0] & 0x07) << 5) + ((dataPtr[q + 1] & 0xc0) >> 3) + 2) + B8_CCIR601*(((dataPtr[q + 1] & 0x3e) << 2) + 4)) >> 8;
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA_4444) {
            for (p = 0; p < ((unsigned int)vli->buffSize); p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf0) + 8) + G8_CCIR601*(((dataPtr[q + 0] & 0x0f) << 4) + 8) + B8_CCIR601*((dataPtr[q + 1] & 0xf0) + 8)) >> 8;
                q += 2;
            }
        } else {
            ARLOGe("Error: Unsupported pixel format passed to arVideoLuma().\n");
            return (NULL);
        }
    }
    return (vli->buff);
}

//
// Methods from http://computer-vision-talks.com/2011/02/a-very-fast-bgra-to-grayscale-conversion-on-iphone/
//
#if 0
static void arVideoLumaBGRAtoL_ARM_neon(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    int i;
    uint8x8_t rfac = vdup_n_u8 (R8_CCIR601);
    uint8x8_t gfac = vdup_n_u8 (G8_CCIR601);
    uint8x8_t bfac = vdup_n_u8 (B8_CCIR601);
    int n = numPixels / 8;
    
    // Convert per eight pixels.
    for (i = 0; i < n; i++) {
        uint16x8_t  temp;
        uint8x8x4_t rgb  = vld4_u8 (src);
        uint8x8_t result;
        
        temp = vmull_u8 (rgb.val[0],      bfac);
        temp = vmlal_u8 (temp,rgb.val[1], gfac);
        temp = vmlal_u8 (temp,rgb.val[2], rfac);
        
        result = vshrn_n_u16 (temp, 8);
        vst1_u8 (dest, result);
        src  += 8*4;
        dest += 8;
    }
}
#endif

#ifdef HAVE_ARM_NEON

static void arVideoLumaBGRAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #29          \n" // Blue channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #77          \n" // Red channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // B into d0, G into d1, R into d2, A into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d0, d4       \n"
                     "vmlal.u8    q7, d1, d5       \n"
                     "vmlal.u8    q7, d2, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaRGBAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #77          \n" // Red channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #29          \n" // Blue channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // R into d0, G into d1, B into d2, A into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d0, d4       \n"
                     "vmlal.u8    q7, d1, d5       \n"
                     "vmlal.u8    q7, d2, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaABGRtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #29          \n" // Blue channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #77          \n" // Red channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // A into d0, B into d1, G into d2, R into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d1, d4       \n"
                     "vmlal.u8    q7, d2, d5       \n"
                     "vmlal.u8    q7, d3, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaARGBtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #77          \n" // Red channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #29          \n" // Blue channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // A into d0, R into d1, G into d2, B into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d1, d4       \n"
                     "vmlal.u8    q7, d2, d5       \n"
                     "vmlal.u8    q7, d3, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

#elif HAVE_ARM64_NEON

static void arVideoLumaBGRAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile(// Setup factors etc.
                     "    lsr         w4,     %w2,    #3     \n" // Save arg 2 (numPixels) divided by 8 in w4 for later use in loop count.
                     "    movi        v0.8b,  #29            \n" // Load v0 with 8 copies of the 8 LSBs of L B factor =  0.114. 29/256 =  0.113 (29 = 0x001d).
                     "    movi        v1.8b,  #150           \n" // Load v1 with 8 copies of the 8 LSBs of L G factor =  0.587. 150/256 =  0.586 (150 = 0x0096).
                     "    movi        v2.8b,  #77            \n" // Load v2 with 8 copies of the 8 LSBs of L R factor =  0.299. 77/256 =  0.301 (77 = 0x004d).
                     "0:						             \n"
                     // Read 8 BGRA pixels.
                     "    ld4         {v3.8b - v6.8b}, [%1],#32 \n" // Load 8 BGRA pixels, de-interleaving B into lower half of v3, G into lower half of v4, R into lower half of v5, A into lower half of v6.
                     // Do the weight average.
                     "    umull       v7.8h,  v0.8b,  v3.8b  \n"
                     "    umlal       v7.8h,  v1.8b,  v4.8b  \n"
                     "    umlal       v7.8h,  v2.8b,  v5.8b  \n"
                     // Shift and store.
                     "    shrn        v7.8b,  v7.8h,  #8     \n" // Divide by 256, and clamp to range [0, 255].
                     "    st1         {v7.8b}, [%0],#8       \n"
                     "    subs        w4,     w4,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "w4"
                     );
}

static void arVideoLumaRGBAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile(// Setup factors etc.
                     "    lsr         w4,     %w2,    #3     \n" // Save arg 2 (numPixels) divided by 8 in w4 for later use in loop count.
                     "    movi        v0.8b,  #29            \n" // Load v0 with 8 copies of the 8 LSBs of L B factor =  0.114. 29/256 =  0.113 (29 = 0x001d).
                     "    movi        v1.8b,  #150           \n" // Load v1 with 8 copies of the 8 LSBs of L G factor =  0.587. 150/256 =  0.586 (150 = 0x0096).
                     "    movi        v2.8b,  #77            \n" // Load v2 with 8 copies of the 8 LSBs of L R factor =  0.299. 77/256 =  0.301 (77 = 0x004d).
                     "0:						             \n"
                     // Read 8 BGRA pixels.
                     "    ld4         {v3.8b - v6.8b}, [%1],#32 \n" // Load 8 RGBA pixels, de-interleaving R into lower half of v3, G into lower half of v4, B into lower half of v5, A into lower half of v6.
                     // Do the weight average.
                     "    umull       v7.8h,  v2.8b,  v3.8b  \n"
                     "    umlal       v7.8h,  v1.8b,  v4.8b  \n"
                     "    umlal       v7.8h,  v0.8b,  v5.8b  \n"
                     // Shift and store.
                     "    shrn        v7.8b,  v7.8h,  #8     \n" // Divide by 256, and clamp to range [0, 255].
                     "    st1         {v7.8b}, [%0],#8       \n"
                     "    subs        w4,     w4,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "w4"
                     );
}

static void arVideoLumaABGRtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile(// Setup factors etc.
                     "    lsr         w4,     %w2,    #3     \n" // Save arg 2 (numPixels) divided by 8 in w4 for later use in loop count.
                     "    movi        v0.8b,  #29            \n" // Load v0 with 8 copies of the 8 LSBs of L B factor =  0.114. 29/256 =  0.113 (29 = 0x001d).
                     "    movi        v1.8b,  #150           \n" // Load v1 with 8 copies of the 8 LSBs of L G factor =  0.587. 150/256 =  0.586 (150 = 0x0096).
                     "    movi        v2.8b,  #77            \n" // Load v2 with 8 copies of the 8 LSBs of L R factor =  0.299. 77/256 =  0.301 (77 = 0x004d).
                     "0:						             \n"
                     // Read 8 BGRA pixels.
                     "    ld4         {v3.8b - v6.8b}, [%1],#32 \n" // Load 8 ABGR pixels, de-interleaving A into lower half of v3, B into lower half of v4, G into lower half of v5, R into lower half of v6.
                     // Do the weight average.
                     "    umull       v7.8h,  v0.8b,  v4.8b  \n"
                     "    umlal       v7.8h,  v1.8b,  v5.8b  \n"
                     "    umlal       v7.8h,  v2.8b,  v6.8b  \n"
                     // Shift and store.
                     "    shrn        v7.8b,  v7.8h,  #8     \n" // Divide by 256, and clamp to range [0, 255].
                     "    st1         {v7.8b}, [%0],#8       \n"
                     "    subs        w4,     w4,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "w4"
                     );
}

static void arVideoLumaARGBtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __asm__ volatile(// Setup factors etc.
                     "    lsr         w4,     %w2,    #3     \n" // Save arg 2 (numPixels) divided by 8 in w4 for later use in loop count.
                     "    movi        v0.8b,  #29            \n" // Load v0 with 8 copies of the 8 LSBs of L B factor =  0.114. 29/256 =  0.113 (29 = 0x001d).
                     "    movi        v1.8b,  #150           \n" // Load v1 with 8 copies of the 8 LSBs of L G factor =  0.587. 150/256 =  0.586 (150 = 0x0096).
                     "    movi        v2.8b,  #77            \n" // Load v2 with 8 copies of the 8 LSBs of L R factor =  0.299. 77/256 =  0.301 (77 = 0x004d).
                     "0:						             \n"
                     // Read 8 BGRA pixels.
                     "    ld4         {v3.8b - v6.8b}, [%1],#32 \n" // Load 8 ARGB pixels, de-interleaving A into lower half of v3, R into lower half of v4, G into lower half of v5, B into lower half of v6.
                     // Do the weight average.
                     "    umull       v7.8h,  v2.8b,  v4.8b  \n"
                     "    umlal       v7.8h,  v1.8b,  v5.8b  \n"
                     "    umlal       v7.8h,  v0.8b,  v6.8b  \n"
                     // Shift and store.
                     "    shrn        v7.8b,  v7.8h,  #8     \n" // Divide by 256, and clamp to range [0, 255].
                     "    st1         {v7.8b}, [%0],#8       \n"
                     "    subs        w4,     w4,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "w4"
                     );
}

#elif HAVE_INTEL_SIMD

static void arVideoLumaBGRAtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __m128i *pin = (__m128i *)src;
    uint32_t *pout = (uint32_t *)dest;
    int numPixelsDiv8 = numPixels >> 3;
    __m128i RGBScale = _mm_set_epi16(0, R8_CCIR601, G8_CCIR601, B8_CCIR601, 0, R8_CCIR601, G8_CCIR601, B8_CCIR601); // RGBScale = 000000[R8_CCIR601]00[G8_CCIR601]00[B8_CCIR601]000000[R8_CCIR601]00[G8_CCIR601]00[B8_CCIR601].

    do {
        __m128i pixels0_3 = _mm_load_si128(pin++); // pixels0_3 = [A3][R3][G3][B3][A2][R2][G2][B2][A1][R1][G1][B1][A0][R0][G0][B0].
        __m128i pixels4_7 = _mm_load_si128(pin++); // pixels4_7 = [A7][R7][G7][B7][A6][R6][G6][B6][A5][R5][G5][B5][A4][R4][G4][B4].
        
        __m128i pixels0_3_l = _mm_unpacklo_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_l = 00[A1]00[R1]00[G1]00[B1]00[A0]00[R0]00[G0]00[B0].
        __m128i pixels0_3_h = _mm_unpackhi_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_h = 00[A3]00[R3]00[G3]00[B3]00[A2]00[R2]00[G2]00[B2].
        __m128i pixels4_7_l = _mm_unpacklo_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_l = 00[A5]00[R5]00[G5]00[B5]00[A4]00[R4]00[G4]00[B4].
        __m128i pixels4_7_h = _mm_unpackhi_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_h = 00[A7]00[R7]00[G7]00[B7]00[A6]00[R6]00[G6]00[B6].
        
        __m128i y0_3_l = _mm_madd_epi16(pixels0_3_l, RGBScale);
        __m128i y0_3_h = _mm_madd_epi16(pixels0_3_h, RGBScale);
        __m128i y4_7_l = _mm_madd_epi16(pixels4_7_l, RGBScale);
        __m128i y4_7_h = _mm_madd_epi16(pixels4_7_h, RGBScale);
        __m128i y0_3 = _mm_hadd_epi32(y0_3_l, y0_3_h);
        __m128i y4_7 = _mm_hadd_epi32(y4_7_l, y4_7_h);
        
        y0_3 = _mm_srli_epi32(y0_3, 8);
        y4_7 = _mm_srli_epi32(y4_7, 8);
        y0_3 = _mm_packs_epi32(y0_3, y0_3);
        y4_7 = _mm_packs_epi32(y4_7, y4_7);
        y0_3 = _mm_packus_epi16(y0_3, y0_3);
        y4_7 = _mm_packus_epi16(y4_7, y4_7);

        *pout++ = _mm_cvtsi128_si32(y0_3);
        *pout++ = _mm_cvtsi128_si32(y4_7);
        
        numPixelsDiv8--;
    } while (numPixelsDiv8);
}

static void arVideoLumaRGBAtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __m128i *pin = (__m128i *)src;
    uint32_t *pout = (uint32_t *)dest;
    int numPixelsDiv8 = numPixels >> 3;
    __m128i RGBScale = _mm_set_epi16(0, B8_CCIR601, G8_CCIR601, R8_CCIR601, 0, B8_CCIR601, G8_CCIR601, R8_CCIR601); // RGBScale = 000000[B8_CCIR601]00[G8_CCIR601]00[R8_CCIR601]000000[B8_CCIR601]00[G8_CCIR601]00[R8_CCIR601].
    
    do {
        __m128i pixels0_3 = _mm_load_si128(pin++); // pixels0_3 = [A3][B3][G3][R3][A2][B2][G2][R2][A1][B1][G1][R1][A0][B0][G0][R0].
        __m128i pixels4_7 = _mm_load_si128(pin++); // pixels4_7 = [A7][B7][G7][R7][A6][B6][G6][R6][A5][B5][G5][R5][A4][B4][G4][R4].
        
        __m128i pixels0_3_l = _mm_unpacklo_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_l = 00[A1]00[B1]00[G1]00[R1]00[A0]00[B0]00[G0]00[R0].
        __m128i pixels0_3_h = _mm_unpackhi_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_h = 00[A3]00[B3]00[G3]00[R3]00[A2]00[B2]00[G2]00[R2].
        __m128i pixels4_7_l = _mm_unpacklo_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_l = 00[A5]00[B5]00[G5]00[R5]00[A4]00[B4]00[G4]00[R4].
        __m128i pixels4_7_h = _mm_unpackhi_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_h = 00[A7]00[B7]00[G7]00[R7]00[A6]00[B6]00[G6]00[R6].
        
        __m128i y0_3_l = _mm_madd_epi16(pixels0_3_l, RGBScale);
        __m128i y0_3_h = _mm_madd_epi16(pixels0_3_h, RGBScale);
        __m128i y4_7_l = _mm_madd_epi16(pixels4_7_l, RGBScale);
        __m128i y4_7_h = _mm_madd_epi16(pixels4_7_h, RGBScale);
        __m128i y0_3 = _mm_hadd_epi32(y0_3_l, y0_3_h);
        __m128i y4_7 = _mm_hadd_epi32(y4_7_l, y4_7_h);
        
        y0_3 = _mm_srli_epi32(y0_3, 8);
        y4_7 = _mm_srli_epi32(y4_7, 8);
        y0_3 = _mm_packs_epi32(y0_3, y0_3);
        y4_7 = _mm_packs_epi32(y4_7, y4_7);
        y0_3 = _mm_packus_epi16(y0_3, y0_3);
        y4_7 = _mm_packus_epi16(y4_7, y4_7);
        
        *pout++ = _mm_cvtsi128_si32(y0_3);
        *pout++ = _mm_cvtsi128_si32(y4_7);
        
        numPixelsDiv8--;
    } while (numPixelsDiv8);
}

static void arVideoLumaABGRtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __m128i *pin = (__m128i *)src;
    uint32_t *pout = (uint32_t *)dest;
    int numPixelsDiv8 = numPixels >> 3;
    __m128i RGBScale = _mm_set_epi16(R8_CCIR601, G8_CCIR601, B8_CCIR601, 0, R8_CCIR601, G8_CCIR601, B8_CCIR601, 0); // RGBScale = 00[R8_CCIR601]00[G8_CCIR601]00[B8_CCIR601]000000[R8_CCIR601]00[G8_CCIR601]00[B8_CCIR601]0000.
    
    do {
        __m128i pixels0_3 = _mm_load_si128(pin++); // pixels0_3 = [R3][G3][B3][A3][R2][G2][B2][A2][R1][G1][B1][A1][R0][G0][B0][A0].
        __m128i pixels4_7 = _mm_load_si128(pin++); // pixels4_7 = [R7][G7][B7][A7][R6][G6][B6][A6][R5][G5][B5][A5][R4][G4][B4][A4].
        
        __m128i pixels0_3_l = _mm_unpacklo_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_l = 00[R1]00[G1]00[B1]00[A1]00[R0]00[G0]00[B0]00[A0].
        __m128i pixels0_3_h = _mm_unpackhi_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_h = 00[R3]00[G3]00[B3]00[A3]00[R2]00[G2]00[B2]00[A2].
        __m128i pixels4_7_l = _mm_unpacklo_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_l = 00[R5]00[G5]00[B5]00[A5]00[R4]00[G4]00[B4]00[A4].
        __m128i pixels4_7_h = _mm_unpackhi_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_h = 00[R7]00[G7]00[B7]00[A7]00[R6]00[G6]00[B6]00[A6].
        
        __m128i y0_3_l = _mm_madd_epi16(pixels0_3_l, RGBScale);
        __m128i y0_3_h = _mm_madd_epi16(pixels0_3_h, RGBScale);
        __m128i y4_7_l = _mm_madd_epi16(pixels4_7_l, RGBScale);
        __m128i y4_7_h = _mm_madd_epi16(pixels4_7_h, RGBScale);
        __m128i y0_3 = _mm_hadd_epi32(y0_3_l, y0_3_h);
        __m128i y4_7 = _mm_hadd_epi32(y4_7_l, y4_7_h);
        
        y0_3 = _mm_srli_epi32(y0_3, 8);
        y4_7 = _mm_srli_epi32(y4_7, 8);
        y0_3 = _mm_packs_epi32(y0_3, y0_3);
        y4_7 = _mm_packs_epi32(y4_7, y4_7);
        y0_3 = _mm_packus_epi16(y0_3, y0_3);
        y4_7 = _mm_packus_epi16(y4_7, y4_7);
        
        *pout++ = _mm_cvtsi128_si32(y0_3);
        *pout++ = _mm_cvtsi128_si32(y4_7);
        
        numPixelsDiv8--;
    } while (numPixelsDiv8);
}

static void arVideoLumaARGBtoL_Intel_simd_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int32_t numPixels)
{
    __m128i *pin = (__m128i *)src;
    uint32_t *pout = (uint32_t *)dest;
    int numPixelsDiv8 = numPixels >> 3;
    __m128i RGBScale = _mm_set_epi16(B8_CCIR601, G8_CCIR601, R8_CCIR601, 0, B8_CCIR601, G8_CCIR601, R8_CCIR601, 0); // RGBScale = 00[B8_CCIR601]00[G8_CCIR601]00[R8_CCIR601]000000[B8_CCIR601]00[G8_CCIR601]00[R8_CCIR601]0000.
    
    do {
        __m128i pixels0_3 = _mm_load_si128(pin++); // pixels0_3 = [B3][G3][R3][A3][B2][G2][R2][A2][B1][G1][R1][A1][B0][G0][R0][A0].
        __m128i pixels4_7 = _mm_load_si128(pin++); // pixels4_7 = [B7][G7][R7][A7][B6][G6][R6][A6][B5][G5][R5][A5][B4][G4][R4][A4].
        
        __m128i pixels0_3_l = _mm_unpacklo_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_l = 00[B1]00[G1]00[R1]00[A1]00[B0]00[G0]00[R0]00[A0].
        __m128i pixels0_3_h = _mm_unpackhi_epi8(pixels0_3, _mm_setzero_si128()); // pixels0_3_h = 00[B3]00[G3]00[R3]00[A3]00[B2]00[G2]00[R2]00[A2].
        __m128i pixels4_7_l = _mm_unpacklo_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_l = 00[B5]00[G5]00[R5]00[A5]00[B4]00[G4]00[R4]00[A4].
        __m128i pixels4_7_h = _mm_unpackhi_epi8(pixels4_7, _mm_setzero_si128()); // pixels4_7_h = 00[B7]00[G7]00[R7]00[A7]00[B6]00[G6]00[R6]00[A6].
        
        __m128i y0_3_l = _mm_madd_epi16(pixels0_3_l, RGBScale);
        __m128i y0_3_h = _mm_madd_epi16(pixels0_3_h, RGBScale);
        __m128i y4_7_l = _mm_madd_epi16(pixels4_7_l, RGBScale);
        __m128i y4_7_h = _mm_madd_epi16(pixels4_7_h, RGBScale);
        __m128i y0_3 = _mm_hadd_epi32(y0_3_l, y0_3_h);
        __m128i y4_7 = _mm_hadd_epi32(y4_7_l, y4_7_h);
        
        y0_3 = _mm_srli_epi32(y0_3, 8);
        y4_7 = _mm_srli_epi32(y4_7, 8);
        y0_3 = _mm_packs_epi32(y0_3, y0_3);
        y4_7 = _mm_packs_epi32(y4_7, y4_7);
        y0_3 = _mm_packus_epi16(y0_3, y0_3);
        y4_7 = _mm_packus_epi16(y4_7, y4_7);
        
        *pout++ = _mm_cvtsi128_si32(y0_3);
        *pout++ = _mm_cvtsi128_si32(y4_7);
        
        numPixelsDiv8--;
    } while (numPixelsDiv8);
}

#endif // HAVE_ARM_NEON|HAVE_ARM64_NEON|HAVE_INTEL_SIMD

