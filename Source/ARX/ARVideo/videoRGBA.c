/*
 *  videoRGBA.c
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
 *  Author(s): Philip Lamb.
 *
 */
#include <stdbool.h>
#include <ARX/ARVideo/videoRGBA.h>

#if HAVE_ARM_NEON || HAVE_ARM64_NEON
#  include <arm_neon.h>
#  ifdef ANDROID
#    include "cpu-features.h"
#  endif
#endif

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
#define CLAMP(x,r1,r2) (MIN(MAX(x,r1),r2))

#if defined(ANDROID) && HAVE_ARM_NEON
int gHaveARMv7aWithNEON = -1;
#endif

#if HAVE_ARM_NEON
static void YCbCr422BiPlanarToRGBA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCbCr, int rowBytes, int rows)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     // Setup factors etc.
                     "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
                     "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
                     "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
                     "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
                     "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         r4,  %3          \n" // Save arg 3 (rowBytes) in r4 for later use in pointer arithmetic.
                     "    lsr         r6,  %4,  #1     \n" // Save arg 4 (rows) divided by 2 in r6 for loop count.
                     "0:                               \n"
                     "    lsr         r5,  %3,  #4     \n" // Save arg 3 (rowBytes) divided by 16 in r5 for loop count.
                     "1:                               \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 CbCr pixels, de-interleaving Cb into d10 (q5[0]), Cr into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cr to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cb to 16 bit in q5 (d10-d11) (overwriting).
                     // Calculate red, then scale width by 2.
                     "    vmul.s16    q8,  q0,  q6     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmov        q9,  q8          \n" // Copy into q9 (d18-d19).
                     "    vzip.16     q8,  q9          \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    vmul.s16    q10, q1,  q6     \n"
                     "    vmla.s16    q10, q2,  q5     \n" // G is now signed 16 bit in q10 (d20-d21).
                     "    vmov        q11, q10         \n" // Copy into q11 (d22-d23).
                     "    vzip.16     q10, q11         \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    vmul.s16    q12, q3,  q5     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmov        q13, q12         \n" // Copy into q13 (d26-d27).
                     "    vzip.16     q12, q13         \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    vld1.8      {d10,d11}, [%1]  \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]). N.B. No post-increment.
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,  %0,  #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    vld1.8      {d10,d11}, [%1]! \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]).
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,  %1,  r4     \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,  %0,  r4, LSL #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        r5,  r5,  #1     \n" // Decrement iteration count.
                     "    bne         1b               \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        r6,  r6,  #1     \n" // Decrement iteration count.
                     "    bne         0b               \n" // Repeat unil iteration count is not zero.
                     "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved.
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCbCr), "r"(rowBytes), "r"(rows)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void YCrCb422BiPlanarToRGBA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCrCb, int rowBytes, int rows)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     // Setup factors etc.
                     "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
                     "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
                     "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
                     "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
                     "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         r4,  %3          \n" // Save arg 3 (rowBytes) in r4 for later use in pointer arithmetic.
                     "    lsr         r6,  %4,  #1     \n" // Save arg 4 (rows) divided by 2 in r6 for loop count.
                     "0:                               \n"
                     "    lsr         r5,  %3,  #4     \n" // Save arg 3 (rowBytes) divided by 16 in r5 for loop count.
                     "1:                               \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 CrCb pixels, de-interleaving Cr into d10 (q5[0]), Cb into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cb to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cr to 16 bit in q5 (d10-d11) (overwriting).
                     // Calculate red, then scale width by 2.
                     "    vmul.s16    q8,  q0,  q5     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmov        q9,  q8          \n" // Copy into q9 (d18-d19).
                     "    vzip.16     q8,  q9          \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    vmul.s16    q10, q1,  q5     \n"
                     "    vmla.s16    q10, q2,  q6     \n" // G is now signed 16 bit in q10 (d20-d21).
                     "    vmov        q11, q10         \n" // Copy into q11 (d22-d23).
                     "    vzip.16     q10, q11         \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    vmul.s16    q12, q3,  q6     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmov        q13, q12         \n" // Copy into q13 (d26-d27).
                     "    vzip.16     q12, q13         \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    vld1.8      {d10,d11}, [%1]  \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]). N.B. No post-increment.
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,  %0,  #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    vld1.8      {d10,d11}, [%1]! \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]).
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,  %1,  r4     \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,  %0,  r4, LSL #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        r5,  r5,  #1     \n" // Decrement iteration count.
                     "    bne         1b               \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        r6,  r6,  #1     \n" // Decrement iteration count.
                     "    bne         0b               \n" // Repeat until iteration count is not zero.
                     "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved.
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCrCb), "r"(rowBytes), "r"(rows)
                     : "cc", "r4", "r5", "r6"
                     );
}
#elif HAVE_ARM64_NEON

static void YCbCr422BiPlanarToRGBA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCbCr, int32_t rowBytes, int32_t rows)
{
    __asm__ volatile(// Push SIMD registers that should be preserved (d8-d15).
                     "    sub         sp,     sp,     #64    \n"
                     "    stp         d8,     d9,     [sp,#0] \n"
                     "    stp         d10,    d11,    [sp,#16] \n"
                     "    stp         d12,    d13,    [sp,#32] \n"
                     "    stp         d14,    d15,    [sp,#48] \n"
                     // Setup factors etc.
                     "    movi        v0.8h,  #179           \n" // Load v0 with 8 copies of the 16 LSBs of R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvni        v1.8h,  #90            \n" // Load v1 with 8 copies of the 16 LSBs of G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvni        v2.8h,  #43            \n" // Load v2 with 8 copies of the 16 LSBs of G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    movi        v3.8h,  #227           \n" // Load v3 with 8 copies of the 16 LSBs of B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    movi        v4.8b,  #0x80          \n" // Load v4 with 8 copies of the 8 LSBs of (XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128).
                     "    movi        v31.8b, #0xFF          \n" // Load v31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         w4,     %w3            \n" // Save arg 3 (rowBytes) in w4 for later use in pointer arithmetic.
                     "    lsr         w6,     %w4,    #1     \n" // Save arg 4 (rows) divided by 2 in w6 for loop count.
                     "0:                                     \n"
                     "    lsr         w5,     %w3,    #4     \n" // Save arg 3 (rowBytes) divided by 16 in w5 for loop count.
                     "1:                                     \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    ld2         {v10.8b, v11.8b}, [%2],#16 \n" // Load 8 CbCr pixels, de-interleaving Cb into lower half of v10, Cr into lower half of v11.
                     "    eor         v10.8b, v10.8b, v4.8b  \n" // Subtract 128 from Cb. Result is signed.
                     "    eor         v11.8b, v11.8b, v4.8b  \n" // Subtract 128 from Cr. Result is signed.
                     "    sxtl        v5.8h,  v10.8b         \n" // Sign-extend Cb to 16 bit in v5.
                     "    sxtl        v6.8h,  v11.8b         \n" // Sign-extend Cr to 16 bit in v6.
                     // Calculate red, then scale width by 2.
                     "    mul         v7.8h,  v0.8h,  v6.8h  \n" // R is now signed 16 bit in v4.
                     "    zip1        v8.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v9.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    mul         v7.8h,  v1.8h,  v6.8h  \n"
                     "    mla         v7.8h,  v2.8h,  v5.8h  \n" // G is now signed 16 bit in v4.
                     "    zip1        v10.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v11.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    mul         v7.8h,  v3.8h,  v5.8h  \n" // B is now signed 16 bit in v4.
                     "    zip1        v12.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v13.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    ld1         {v5.16b}, [%1]         \n" // Load 16 Y pixels into v5. N.B. No post-increment.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,     %0,     #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    ld1         {v5.16b}, [%1],#16     \n" // Load 16 Y pixels into v5.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,     %1,     w4, UXTW   \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,     %0,     w4, UXTW #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        w5,     w5,     #1     \n" // Decrement iteration count.
                     "    b.ne        1b                     \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        w6,     w6,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     // Pop saved SIMD registers.
                     "    ldp         d8,     d9,     [sp,#0]  \n"
                     "    ldp         d10,    d11,    [sp,#16] \n"
                     "    ldp         d12,    d13,    [sp,#32] \n"
                     "    ldp         d14,    d15,    [sp,#48] \n"
                     "    add         sp,     sp,     #64      \n"
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCbCr), "r"(rowBytes), "r"(rows)
                     : "cc", "w4", "w5", "w6"
                     );
}

static void YCrCb422BiPlanarToRGBA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCrCb, int32_t rowBytes, int32_t rows)
{
    __asm__ volatile(// Push SIMD registers that should be preserved (d8-d15).
                     "    sub         sp,     sp,     #64    \n"
                     "    stp         d8,     d9,     [sp,#0] \n"
                     "    stp         d10,    d11,    [sp,#16] \n"
                     "    stp         d12,    d13,    [sp,#32] \n"
                     "    stp         d14,    d15,    [sp,#48] \n"
                     // Setup factors etc.
                     "    movi        v0.8h,  #179           \n" // Load v0 with 8 copies of the 16 LSBs of R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvni        v1.8h,  #90            \n" // Load v1 with 8 copies of the 16 LSBs of G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvni        v2.8h,  #43            \n" // Load v2 with 8 copies of the 16 LSBs of G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    movi        v3.8h,  #227           \n" // Load v3 with 8 copies of the 16 LSBs of B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    movi        v4.8b,  #0x80          \n" // Load v4 with 8 copies of the 8 LSBs of (XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128).
                     "    movi        v31.8b, #0xFF          \n" // Load v31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         w4,     %w3            \n" // Save arg 3 (rowBytes) in w4 for later use in pointer arithmetic.
                     "    lsr         w6,     %w4,    #1     \n" // Save arg 4 (rows) divided by 2 in w6 for loop count.
                     "0:                                     \n"
                     "    lsr         w5,     %w3,    #4     \n" // Save arg 3 (rowBytes) divided by 16 in w5 for loop count.
                     "1:                                     \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    ld2         {v10.8b, v11.8b}, [%2],#16 \n" // Load 8 CrCb pixels, de-interleaving Cr into lower half of v10, Cb into lower half of v11.
                     "    eor         v10.8b, v10.8b, v4.8b  \n" // Subtract 128 from Cr. Result is signed.
                     "    eor         v11.8b, v11.8b, v4.8b  \n" // Subtract 128 from Cb. Result is signed.
                     "    sxtl        v5.8h,  v10.8b         \n" // Sign-extend Cr to 16 bit in v5.
                     "    sxtl        v6.8h,  v11.8b         \n" // Sign-extend Cb to 16 bit in v6.
                     // Calculate red, then scale width by 2.
                     "    mul         v7.8h,  v0.8h,  v5.8h  \n" // R is now signed 16 bit in v4.
                     "    zip1        v8.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v9.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    mul         v7.8h,  v1.8h,  v5.8h  \n"
                     "    mla         v7.8h,  v2.8h,  v6.8h  \n" // G is now signed 16 bit in v4.
                     "    zip1        v10.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v11.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    mul         v7.8h,  v3.8h,  v6.8h  \n" // B is now signed 16 bit in v4.
                     "    zip1        v12.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v13.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    ld1         {v5.16b}, [%1]         \n" // Load 16 Y pixels into v5. N.B. No post-increment.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,     %0,     #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    ld1         {v5.16b}, [%1],#16     \n" // Load 16 Y pixels into v5.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,     %1,     w4, UXTW   \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,     %0,     w4, UXTW #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        w5,     w5,     #1     \n" // Decrement iteration count.
                     "    b.ne        1b                     \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        w6,     w6,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat unil iteration count is not zero.
                     // Pop saved SIMD registers.
                     "    ldp         d8,     d9,     [sp,#0]  \n"
                     "    ldp         d10,    d11,    [sp,#16] \n"
                     "    ldp         d12,    d13,    [sp,#32] \n"
                     "    ldp         d14,    d15,    [sp,#48] \n"
                     "    add         sp,     sp,     #64      \n"
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCrCb), "r"(rowBytes), "r"(rows)
                     : "cc", "w4", "w5", "w6"
                     );
}
#endif

int videoRGBA(uint32_t *destRGBA, AR2VideoBufferT *source, int width, int height, AR_PIXEL_FORMAT pixelFormat)
{
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
    bool fastPath;
#endif
    
    if (!destRGBA || !source || width <= 0 || height <= 0 || pixelFormat == AR_PIXEL_FORMAT_INVALID) return (-1); // Sanity check.

#if HAVE_ARM_NEON || HAVE_ARM64_NEON
    if (width % 16 != 0 || height % 2 != 0) {
        fastPath = false;
    } else {
#  if defined(ANDROID) && HAVE_ARM_NEON
        if (gHaveARMv7aWithNEON == -1) {
            // Not all Android devices with ARMv7 are guaranteed to have NEON, so check.
            uint64_t features = android_getCpuFeatures();
            gHaveARMv7aWithNEON = (features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON);
            ARLOGi("videoRGBA() will%s use ARM NEON acceleration.\n", (gHaveARMv7aWithNEON ? "" : " not"));
        }
        fastPath = gHaveARMv7aWithNEON;
#  else
        fastPath = true;
#  endif
    }
#endif
    
    int pixelSize = arUtilGetPixelSize(pixelFormat);
    switch (pixelFormat) {
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_BGR:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 0) << 16) | (*(inp + 1) << 8) | (*(inp + 2));
#else
                    *outp = (*(inp + 2) << 24) | (*(inp + 1) << 16) | (*(inp + 0) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_RGB:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 2) << 16) | (*(inp + 1) << 8) | (*(inp + 0));
#else
                    *outp = (*(inp + 0) << 24) | (*(inp + 1) << 16) | (*(inp + 2) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ARGB:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 3) << 16) | (*(inp + 2) << 8) | (*(inp + 1));
#else
                    *outp = (*(inp + 1) << 24) | (*(inp + 2) << 16) | (*(inp + 3) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ABGR:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 1) << 16) | (*(inp + 2) << 8) | (*(inp + 3));
#else
                    *outp = (*(inp + 3) << 24) | (*(inp + 2) << 16) | (*(inp + 1) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*inp << 16) | (*inp << 8) | *inp;
#else
                    *outp = (*inp << 24) | (*inp << 16) | (*inp << 8) | 0xff;
#endif
                    inp++;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 1) & 0x1f) << 19) | ((*(inp + 1) & 0xe0) << 5) | ((*(inp + 0) & 0x07) << 13) | (*(inp + 0) & 0xf8);
#else
                    *outp = ((*(inp + 0) & 0xf8) << 24) | ((*(inp + 0) & 0x07) << 21) | ((*(inp + 1) & 0xe0) << 13) | ((*(inp + 1) & 0x1f) << 11) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 1) & 0x3e) << 18) | ((*(inp + 1) & 0xc0) << 5) | ((*(inp + 0) & 0x07) << 13) | (*(inp + 0) & 0xf8);
#else
                    *outp = ((*(inp + 0) & 0xf8) << 24) | ((*(inp + 0) & 0x07) << 21) | ((*(inp + 1) & 0xc0) << 13) | ((*(inp + 1) & 0x3e) << 10) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 1) & 0xf0) << 16) | ((*(inp + 0) & 0x0f) << 12) | (*(inp + 0) & 0xf0);
#else
                    *outp = ((*(inp + 0) & 0xf0) << 24) | ((*(inp + 0) & 0x0f) << 20) | ((*(inp + 1) & 0xf0) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_420f:
        {
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            if (fastPath) {
                YCbCr422BiPlanarToRGBA_ARM_neon_asm((uint8_t *)destRGBA, source->bufPlanes[0], source->bufPlanes[1], width, height);
            } else {
#endif
                ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
                ARUint8 *pCbCr = source->bufPlanes[1];
                uint8_t *outp0 = (uint8_t *)destRGBA;
                uint8_t *outp1 = ((uint8_t *)destRGBA) + width*4;
                int wd2 = width >> 1;
                int hd2 = height >> 1;
                for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                    for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                        int16_t Cb, Cr;
                        int16_t Y0, Y1, R, G, B;
                        uint8_t R0, R1, G0, G1, B0, B1;
                        Cb = ((int16_t)(*(pCbCr++))) - 128;
                        Cr = ((int16_t)(*(pCbCr++))) - 128;
                        R = (        179*Cr) >> 7;
                        G = (-44*Cb - 91*Cr) >> 7;
                        B = (227*Cb        ) >> 7;
                        Y0 = *(pY0++);
                        Y1 = *(pY0++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = R0;
                        *(outp0++) = G0;
                        *(outp0++) = B0;
                        *(outp0++) = 255;
                        *(outp1++) = R1;
                        *(outp1++) = G1;
                        *(outp1++) = B1;
                        *(outp1++) = 255;
                        Y0 = *(pY1++);
                        Y1 = *(pY1++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = R0;
                        *(outp0++) = G0;
                        *(outp0++) = B0;
                        *(outp0++) = 255;
                        *(outp1++) = R1;
                        *(outp1++) = G1;
                        *(outp1++) = B1;
                        *(outp1++) = 255;
                    }
                    pY0 += width;
                    pY1 += width;
                    outp0 += width*4;
                    outp1 += width*4;
                }
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            }
#endif
        }
            break;
        case AR_PIXEL_FORMAT_NV21:
        {
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            if (fastPath) {
                YCrCb422BiPlanarToRGBA_ARM_neon_asm((uint8_t *)destRGBA, source->bufPlanes[0], source->bufPlanes[1], width, height);
            } else {
#endif
                ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
                ARUint8 *pCrCb = source->bufPlanes[1];
                uint8_t *outp0 = (uint8_t *)destRGBA;
                uint8_t *outp1 = ((uint8_t *)destRGBA) + width*4;
                int wd2 = width >> 1;
                int hd2 = height >> 1;
                for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                    for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                        int16_t Cb, Cr;
                        int16_t Y0, Y1, R, G, B;
                        uint8_t R0, R1, G0, G1, B0, B1;
                        Cr = ((int16_t)(*(pCrCb++))) - 128;
                        Cb = ((int16_t)(*(pCrCb++))) - 128;
                        R = (        179*Cr) >> 7;
                        G = (-44*Cb - 91*Cr) >> 7;
                        B = (227*Cb        ) >> 7;
                        Y0 = *(pY0++);
                        Y1 = *(pY0++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = R0;
                        *(outp0++) = G0;
                        *(outp0++) = B0;
                        *(outp0++) = 255;
                        *(outp1++) = R1;
                        *(outp1++) = G1;
                        *(outp1++) = B1;
                        *(outp1++) = 255;
                        Y0 = *(pY1++);
                        Y1 = *(pY1++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = R0;
                        *(outp0++) = G0;
                        *(outp0++) = B0;
                        *(outp0++) = 255;
                        *(outp1++) = R1;
                        *(outp1++) = G1;
                        *(outp1++) = B1;
                        *(outp1++) = 255;
                    }
                    pY0 += width;
                    pY1 += width;
                    outp0 += width*4;
                    outp1 += width*4;
                }
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            }
#endif
        }
            break;
        case AR_PIXEL_FORMAT_420v:
        {
            ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
            ARUint8 *pCbCr = source->bufPlanes[1];
            uint8_t *outp0 = (uint8_t *)destRGBA;
            uint8_t *outp1 = ((uint8_t *)destRGBA) + width*4;
            int wd2 = width >> 1;
            int hd2 = height >> 1;
            for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int16_t Cb, Cr;
                    int16_t Y0, Y1, R, G, B;
                    uint8_t R0, R1, G0, G1, B0, B1;
                    Cb = ((int16_t)(*(pCbCr++))) - 128;
                    Cr = ((int16_t)(*(pCbCr++))) - 128;
                    R = (         204*Cr) >> 7;
                    G = (-50*Cb - 104*Cr) >> 7;
                    B = (258*Cb         ) >> 7;
                    Y0 = (149*((int16_t)(*(pY0++))) - 16) >> 7;
                    Y1 = (149*((int16_t)(*(pY0++))) - 16) >> 7;
                    R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                    R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                    G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                    G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                    B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                    B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                    *(outp0++) = R0;
                    *(outp0++) = G0;
                    *(outp0++) = B0;
                    *(outp0++) = 255;
                    *(outp1++) = R1;
                    *(outp1++) = G1;
                    *(outp1++) = B1;
                    *(outp1++) = 255;
                    Y0 = (149*((int16_t)(*(pY1++))) - 16) >> 7;
                    Y1 = (149*((int16_t)(*(pY1++))) - 16) >> 7;
                    R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                    R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                    G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                    G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                    B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                    B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                    *(outp0++) = R0;
                    *(outp0++) = G0;
                    *(outp0++) = B0;
                    *(outp0++) = 255;
                    *(outp1++) = R1;
                    *(outp1++) = G1;
                    *(outp1++) = B1;
                    *(outp1++) = 255;
                }
                pY0 += width;
                pY1 += width;
                outp0 += width*4;
                outp1 += width*4;
            }
        }
        case AR_PIXEL_FORMAT_2vuy:
        {
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                int wd2 = width >> 1;
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int B0, G0, R0;
                    float Cb =      (float)(inp[0] - 128); // Byte 0 of each 4-byte block for both even- and odd-numbered columns.
                    float Y0prime = (float)(inp[1] - 16);  // Byte 1 of each 4-byte block.
                    float Cr =      (float)(inp[2] - 128); // Byte 2 of each 4-byte block for both even- and odd-numbered columns.
                    float Y1prime = (float)(inp[3] - 16);  // Byte 3 of each 4-byte block.
                    inp += pixelSize * 2;
                    // Conversion from Poynton's color FAQ http://www.poynton.com.
                    B0 = (int)(298.082f*Y0prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y0prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y0prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(B0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(R0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(R0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(B0, 0, 255)) << 8) | 0xff;
#endif
                    B0 = (int)(298.082f*Y1prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y1prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y1prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(B0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(R0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(R0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(B0, 0, 255)) << 8) | 0xff;
#endif
                }
            }
        }
            break;
        case AR_PIXEL_FORMAT_yuvs:
        {
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destRGBA[width*y];
                int wd2 = width >> 1;
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int B0, G0, R0;
                    float Y0prime = (float)(inp[0] - 16);  // Byte 0 of each 4-byte block.
                    float Cb =      (float)(inp[1] - 128); // Byte 1 of each 4-byte block for both even- and odd-numbered columns.
                    float Y1prime = (float)(inp[2] - 16);  // Byte 2 of each 4-byte block.
                    float Cr =      (float)(inp[3] - 128); // Byte 3 of each 4-byte block for both even- and odd-numbered columns.
                    inp += pixelSize * 2;
                    // Conversion from Poynton's color FAQ http://www.poynton.com.
                    B0 = (int)(298.082f*Y0prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y0prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y0prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(B0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(R0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(R0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(B0, 0, 255)) << 8) | 0xff;
#endif
                    B0 = (int)(298.082f*Y1prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y1prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y1prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(B0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(R0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(R0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(B0, 0, 255)) << 8) | 0xff;
#endif
                }
            }
        }
            break;
       default:
            return -1;
            break;
    }
    
    return 0;
    
}

#if HAVE_ARM_NEON
static void YCbCr422BiPlanarToBGRA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCbCr, int rowBytes, int rows)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     // Setup factors etc.
                     "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
                     "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
                     "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
                     "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
                     "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         r4,  %3          \n" // Save arg 3 (rowBytes) in r4 for later use in pointer arithmetic.
                     "    lsr         r6,  %4,  #1     \n" // Save arg 4 (rows) divided by 2 in r6 for loop count.
                     "0:                               \n"
                     "    lsr         r5,  %3,  #4     \n" // Save arg 3 (rowBytes) divided by 16 in r5 for loop count.
                     "1:                               \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 CbCr pixels, de-interleaving Cb into d10 (q5[0]), Cr into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cr to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cb to 16 bit in q5 (d10-d11) (overwriting).
                     // Calculate red, then scale width by 2.
                     "    vmul.s16    q8,  q0,  q6     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmov        q9,  q8          \n" // Copy into q9 (d18-d19).
                     "    vzip.16     q8,  q9          \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    vmul.s16    q10, q1,  q6     \n"
                     "    vmla.s16    q10, q2,  q5     \n" // G is now signed 16 bit in q10 (d20-d21).
                     "    vmov        q11, q10         \n" // Copy into q11 (d22-d23).
                     "    vzip.16     q10, q11         \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    vmul.s16    q12, q3,  q5     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmov        q13, q12         \n" // Copy into q13 (d26-d27).
                     "    vzip.16     q12, q13         \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    vld1.8      {d10,d11}, [%1]  \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]). N.B. No post-increment.
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,  %0,  #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    vld1.8      {d10,d11}, [%1]! \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]).
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,  %1,  r4     \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,  %0,  r4, LSL #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        r5,  r5,  #1     \n" // Decrement iteration count.
                     "    bne         1b               \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        r6,  r6,  #1     \n" // Decrement iteration count.
                     "    bne         0b               \n" // Repeat unil iteration count is not zero.
                     "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved.
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCbCr), "r"(rowBytes), "r"(rows)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void YCrCb422BiPlanarToBGRA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCrCb, int rowBytes, int rows)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     // Setup factors etc.
                     "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
                     "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
                     "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
                     "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
                     "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         r4,  %3          \n" // Save arg 3 (rowBytes) in r4 for later use in pointer arithmetic.
                     "    lsr         r6,  %4,  #1     \n" // Save arg 4 (rows) divided by 2 in r6 for loop count.
                     "0:                               \n"
                     "    lsr         r5,  %3,  #4     \n" // Save arg 3 (rowBytes) divided by 16 in r5 for loop count.
                     "1:                               \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 CrCb pixels, de-interleaving Cr into d10 (q5[0]), Cb into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cb to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cr to 16 bit in q5 (d10-d11) (overwriting).
                     // Calculate red, then scale width by 2.
                     "    vmul.s16    q8,  q0,  q5     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmov        q9,  q8          \n" // Copy into q9 (d18-d19).
                     "    vzip.16     q8,  q9          \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    vmul.s16    q10, q1,  q5     \n"
                     "    vmla.s16    q10, q2,  q6     \n" // G is now signed 16 bit in q10 (d20-d21).
                     "    vmov        q11, q10         \n" // Copy into q11 (d22-d23).
                     "    vzip.16     q10, q11         \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    vmul.s16    q12, q3,  q6     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmov        q13, q12         \n" // Copy into q13 (d26-d27).
                     "    vzip.16     q12, q13         \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    vld1.8      {d10,d11}, [%1]  \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]). N.B. No post-increment.
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,  %0,  #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    vld1.8      {d10,d11}, [%1]! \n" // Load 16 Y pixels into d10 (q5[0]) and d11 (q5[1]).
                     "    vshll.u8    q6,  d11, #7     \n" // Multiply second 8 pixels by 128, store in q6 (d12-d13).
                     "    vshll.u8    q5,  d10, #7     \n" // Multiply first 8 pixels by 128, store in q5 (d10-d11) (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    vqadd.s16    q7,  q5,  q8    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q10   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q5,  q12   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    vqadd.s16    q7,  q6,  q9    \n" // Y+R
                     "    vqshrun.s16 d30, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q11   \n" // Y+G
                     "    vqshrun.s16 d29, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vqadd.s16    q7,  q6,  q13   \n" // Y+B
                     "    vqshrun.s16 d28, q7,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,  %1,  r4     \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,  %0,  r4, LSL #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        r5,  r5,  #1     \n" // Decrement iteration count.
                     "    bne         1b               \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,  %1,  r4     \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,  %0,  r4, LSL #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        r6,  r6,  #1     \n" // Decrement iteration count.
                     "    bne         0b               \n" // Repeat until iteration count is not zero.
                     "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved.
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCrCb), "r"(rowBytes), "r"(rows)
                     : "cc", "r4", "r5", "r6"
                     );
}
#elif HAVE_ARM64_NEON

static void YCbCr422BiPlanarToBGRA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCbCr, int32_t rowBytes, int32_t rows)
{
    __asm__ volatile(// Push SIMD registers that should be preserved (d8-d15).
                     "    sub         sp,     sp,     #64    \n"
                     "    stp         d8,     d9,     [sp,#0] \n"
                     "    stp         d10,    d11,    [sp,#16] \n"
                     "    stp         d12,    d13,    [sp,#32] \n"
                     "    stp         d14,    d15,    [sp,#48] \n"
                     // Setup factors etc.
                     "    movi        v0.8h,  #179           \n" // Load v0 with 8 copies of the 16 LSBs of R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvni        v1.8h,  #90            \n" // Load v1 with 8 copies of the 16 LSBs of G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvni        v2.8h,  #43            \n" // Load v2 with 8 copies of the 16 LSBs of G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    movi        v3.8h,  #227           \n" // Load v3 with 8 copies of the 16 LSBs of B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    movi        v4.8b,  #0x80          \n" // Load v4 with 8 copies of the 8 LSBs of (XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128).
                     "    movi        v31.8b, #0xFF          \n" // Load v31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         w4,     %w3            \n" // Save arg 3 (rowBytes) in w4 for later use in pointer arithmetic.
                     "    lsr         w6,     %w4,    #1     \n" // Save arg 4 (rows) divided by 2 in w6 for loop count.
                     "0:                                     \n"
                     "    lsr         w5,     %w3,    #4     \n" // Save arg 3 (rowBytes) divided by 16 in w5 for loop count.
                     "1:                                     \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    ld2         {v10.8b, v11.8b}, [%2],#16 \n" // Load 8 CbCr pixels, de-interleaving Cb into lower half of v10, Cr into lower half of v11.
                     "    eor         v10.8b, v10.8b, v4.8b  \n" // Subtract 128 from Cb. Result is signed.
                     "    eor         v11.8b, v11.8b, v4.8b  \n" // Subtract 128 from Cr. Result is signed.
                     "    sxtl        v5.8h,  v10.8b         \n" // Sign-extend Cb to 16 bit in v5.
                     "    sxtl        v6.8h,  v11.8b         \n" // Sign-extend Cr to 16 bit in v6.
                     // Calculate red, then scale width by 2.
                     "    mul         v7.8h,  v0.8h,  v6.8h  \n" // R is now signed 16 bit in v4.
                     "    zip1        v8.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v9.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    mul         v7.8h,  v1.8h,  v6.8h  \n"
                     "    mla         v7.8h,  v2.8h,  v5.8h  \n" // G is now signed 16 bit in v4.
                     "    zip1        v10.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v11.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    mul         v7.8h,  v3.8h,  v5.8h  \n" // B is now signed 16 bit in v4.
                     "    zip1        v12.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v13.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    ld1         {v5.16b}, [%1]         \n" // Load 16 Y pixels into v5. N.B. No post-increment.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,     %0,     #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    ld1         {v5.16b}, [%1],#16     \n" // Load 16 Y pixels into v5.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,     %1,     w4, UXTW   \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,     %0,     w4, UXTW #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        w5,     w5,     #1     \n" // Decrement iteration count.
                     "    b.ne        1b                     \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        w6,     w6,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat until iteration count is not zero.
                     // Pop saved SIMD registers.
                     "    ldp         d8,     d9,     [sp,#0]  \n"
                     "    ldp         d10,    d11,    [sp,#16] \n"
                     "    ldp         d12,    d13,    [sp,#32] \n"
                     "    ldp         d14,    d15,    [sp,#48] \n"
                     "    add         sp,     sp,     #64      \n"
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCbCr), "r"(rowBytes), "r"(rows)
                     : "cc", "w4", "w5", "w6"
                     );
}

static void YCrCb422BiPlanarToBGRA_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict srcY, uint8_t * __restrict srcCrCb, int32_t rowBytes, int32_t rows)
{
    __asm__ volatile(// Push SIMD registers that should be preserved (d8-d15).
                     "    sub         sp,     sp,     #64    \n"
                     "    stp         d8,     d9,     [sp,#0] \n"
                     "    stp         d10,    d11,    [sp,#16] \n"
                     "    stp         d12,    d13,    [sp,#32] \n"
                     "    stp         d14,    d15,    [sp,#48] \n"
                     // Setup factors etc.
                     "    movi        v0.8h,  #179           \n" // Load v0 with 8 copies of the 16 LSBs of R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
                     "    mvni        v1.8h,  #90            \n" // Load v1 with 8 copies of the 16 LSBs of G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
                     "    mvni        v2.8h,  #43            \n" // Load v2 with 8 copies of the 16 LSBs of G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    movi        v3.8h,  #227           \n" // Load v3 with 8 copies of the 16 LSBs of B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    movi        v4.8b,  #0x80          \n" // Load v4 with 8 copies of the 8 LSBs of (XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128).
                     "    movi        v31.8b, #0xFF          \n" // Load v31 (A channel of destRGBA) with FF.
                     // Setup loop-related stuff.
                     "    mov         w4,     %w3            \n" // Save arg 3 (rowBytes) in w4 for later use in pointer arithmetic.
                     "    lsr         w6,     %w4,    #1     \n" // Save arg 4 (rows) divided by 2 in w6 for loop count.
                     "0:                                     \n"
                     "    lsr         w5,     %w3,    #4     \n" // Save arg 3 (rowBytes) divided by 16 in w5 for loop count.
                     "1:                                     \n"
                     // Read 8 CbCr pixels, convert to 16-bit.
                     "    ld2         {v10.8b, v11.8b}, [%2],#16 \n" // Load 8 CrCb pixels, de-interleaving Cr into lower half of v10, Cb into lower half of v11.
                     "    eor         v10.8b, v10.8b, v4.8b  \n" // Subtract 128 from Cr. Result is signed.
                     "    eor         v11.8b, v11.8b, v4.8b  \n" // Subtract 128 from Cb. Result is signed.
                     "    sxtl        v5.8h,  v10.8b         \n" // Sign-extend Cr to 16 bit in v5.
                     "    sxtl        v6.8h,  v11.8b         \n" // Sign-extend Cb to 16 bit in v6.
                     // Calculate red, then scale width by 2.
                     "    mul         v7.8h,  v0.8h,  v5.8h  \n" // R is now signed 16 bit in v4.
                     "    zip1        v8.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v9.8h,  v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate green, then scale width by 2.
                     "    mul         v7.8h,  v1.8h,  v5.8h  \n"
                     "    mla         v7.8h,  v2.8h,  v6.8h  \n" // G is now signed 16 bit in v4.
                     "    zip1        v10.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v11.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Calculate blue, then scale width by 2.
                     "    mul         v7.8h,  v3.8h,  v6.8h  \n" // B is now signed 16 bit in v4.
                     "    zip1        v12.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     "    zip2        v13.8h, v7.8h,  v7.8h  \n" // Interleave the original and the copy.
                     // Read 16 Y from first row.
                     "    ld1         {v5.16b}, [%1]         \n" // Load 16 Y pixels into v5. N.B. No post-increment.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of first row done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to move to next row.
                     "    sub         %0,     %0,     #64    \n" // Back up dest by 16 pixels/64 bytes.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes.
                     // Read 16 Y from second row.
                     "    ld1         {v5.16b}, [%1],#16     \n" // Load 16 Y pixels into v5.
                     "    ushll2      v6.8h,  v5.16b, #7     \n" // Multiply second 8 pixels by 128, store in v6.
                     "    ushll       v5.8h,  v5.8b,  #7     \n" // Multiply first 8 pixels by 128, store in v5 (overwriting).
                     // Add luma and chroma values. First 8 pixels.
                     "    sqadd       v7.8h,  v5.8h,  v8.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v10.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v5.8h,  v12.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // Add luma and chroma values. Second 8 pixels.
                     "    sqadd       v7.8h,  v6.8h,  v9.8h  \n" // Y+R
                     "    sqshrun     v30.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v11.8h \n" // Y+G
                     "    sqshrun     v29.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    sqadd       v7.8h,  v6.8h,  v13.8h \n" // Y+B
                     "    sqshrun     v28.8b, v7.8h,  #7     \n" // Divide by 128, and clamp to range [0, 255].
                     "    st4         {v28.8b,v29.8b,v30.8b,v31.8b}, [%0],#32 \n" // Interleave.
                     // 16 pixels of second row done.
                     "    sub         %1,     %1,     w4, UXTW   \n" // Back up srcY by rowBytes to move to previous row.
                     "    sub         %0,     %0,     w4, UXTW #2 \n" // Back up dest by 4xrowBytes.
                     "    subs        w5,     w5,     #1     \n" // Decrement iteration count.
                     "    b.ne        1b                     \n" // Repeat unil iteration count is not zero.
                     // Two rows done.
                     "    add         %1,     %1,     w4, UXTW   \n" // Advance srcY by rowBytes to skip row we've already done.
                     "    add         %0,     %0,     w4, UXTW #2 \n" // Advance dest by 4xrowBytes to skip row we've already done.
                     "    subs        w6,     w6,     #1     \n" // Decrement iteration count.
                     "    b.ne        0b                     \n" // Repeat unil iteration count is not zero.
                     // Pop saved SIMD registers.
                     "    ldp         d8,     d9,     [sp,#0]  \n"
                     "    ldp         d10,    d11,    [sp,#16] \n"
                     "    ldp         d12,    d13,    [sp,#32] \n"
                     "    ldp         d14,    d15,    [sp,#48] \n"
                     "    add         sp,     sp,     #64      \n"
                     :
                     : "r"(dest), "r"(srcY), "r"(srcCrCb), "r"(rowBytes), "r"(rows)
                     : "cc", "w4", "w5", "w6"
                     );
}
#endif

int videoBGRA(uint32_t *destBGRA, AR2VideoBufferT *source, int width, int height, AR_PIXEL_FORMAT pixelFormat)
{
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
    bool fastPath;
#endif
    
    if (!destBGRA || !source || width <= 0 || height <= 0 || pixelFormat == AR_PIXEL_FORMAT_INVALID) return (-1); // Sanity check.
    
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
    if (width % 16 != 0 || height % 2 != 0) {
        fastPath = false;
    } else {
#  if defined(ANDROID) && HAVE_ARM_NEON
        if (gHaveARMv7aWithNEON == -1) {
            // Not all Android devices with ARMv7 are guaranteed to have NEON, so check.
            uint64_t features = android_getCpuFeatures();
            gHaveARMv7aWithNEON = (features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON);
            ARLOGi("videoRGBA() will%s use ARM NEON acceleration.\n", (gHaveARMv7aWithNEON ? "" : " not"));
        }
        fastPath = gHaveARMv7aWithNEON;
#  else
        fastPath = true;
#  endif
    }
#endif
    
    int pixelSize = arUtilGetPixelSize(pixelFormat);
    switch (pixelFormat) {
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_BGR:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 2) << 16) | (*(inp + 1) << 8) | (*(inp + 0));
#else
                    *outp = (*(inp + 0) << 24) | (*(inp + 1) << 16) | (*(inp + 2) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_RGB:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 0) << 16) | (*(inp + 1) << 8) | (*(inp + 2));
#else
                    *outp = (*(inp + 2) << 24) | (*(inp + 1) << 16) | (*(inp + 0) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ARGB:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 1) << 16) | (*(inp + 2) << 8) | (*(inp + 3));
#else
                    *outp = (*(inp + 3) << 24) | (*(inp + 2) << 16) | (*(inp + 1) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ABGR:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 3) << 16) | (*(inp + 2) << 8) | (*(inp + 1));
#else
                    *outp = (*(inp + 1) << 24) | (*(inp + 2) << 16) | (*(inp + 3) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*inp << 16) | (*inp << 8) | *inp;
#else
                    *outp = (*inp << 24) | (*inp << 16) | (*inp << 8) | 0xff;
#endif
                    inp++;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 0) & 0xf8) << 19) | ((*(inp + 1) & 0xe0) << 5) | ((*(inp + 0) & 0x07) << 13) | (*(inp + 1) & 0x1f);
#else
                    *outp = ((*(inp + 1) & 0x1f) << 24) | ((*(inp + 0) & 0x07) << 21) | ((*(inp + 1) & 0xe0) << 13) | ((*(inp + 0) & 0xf8) << 11) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 0) & 0xf8) << 18) | ((*(inp + 1) & 0xc0) << 5) | ((*(inp + 0) & 0x07) << 13) | (*(inp + 1) & 0x3e);
#else
                    *outp = ((*(inp + 1) & 0x3e) << 24) | ((*(inp + 0) & 0x07) << 21) | ((*(inp + 1) & 0xc0) << 13) | ((*(inp + 0) & 0xf8) << 10) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                for (int pixelsToGo = width; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | ((*(inp + 0) & 0xf0) << 16) | ((*(inp + 0) & 0x0f) << 12) | (*(inp + 1) & 0xf0);
#else
                    *outp = ((*(inp + 1) & 0xf0) << 24) | ((*(inp + 0) & 0x0f) << 20) | ((*(inp + 0) & 0xf0) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_420f:
        {
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            if (fastPath) {
                YCbCr422BiPlanarToBGRA_ARM_neon_asm((uint8_t *)destBGRA, source->bufPlanes[0], source->bufPlanes[1], width, height);
            } else {
#endif
                ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
                ARUint8 *pCbCr = source->bufPlanes[1];
                uint8_t *outp0 = (uint8_t *)destBGRA;
                uint8_t *outp1 = ((uint8_t *)destBGRA) + width*4;
                int wd2 = width >> 1;
                int hd2 = height >> 1;
                for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                    for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                        int16_t Cb, Cr;
                        int16_t Y0, Y1, R, G, B;
                        uint8_t R0, R1, G0, G1, B0, B1;
                        Cb = ((int16_t)(*(pCbCr++))) - 128;
                        Cr = ((int16_t)(*(pCbCr++))) - 128;
                        R = (        179*Cr) >> 7;
                        G = (-44*Cb - 91*Cr) >> 7;
                        B = (227*Cb        ) >> 7;
                        Y0 = *(pY0++);
                        Y1 = *(pY0++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = B0;
                        *(outp0++) = G0;
                        *(outp0++) = R0;
                        *(outp0++) = 255;
                        *(outp1++) = B1;
                        *(outp1++) = G1;
                        *(outp1++) = R1;
                        *(outp1++) = 255;
                        Y0 = *(pY1++);
                        Y1 = *(pY1++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = B0;
                        *(outp0++) = G0;
                        *(outp0++) = R0;
                        *(outp0++) = 255;
                        *(outp1++) = B1;
                        *(outp1++) = G1;
                        *(outp1++) = R1;
                        *(outp1++) = 255;
                    }
                    pY0 += width;
                    pY1 += width;
                    outp0 += width*4;
                    outp1 += width*4;
                }
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            }
#endif
        }
            break;
        case AR_PIXEL_FORMAT_NV21:
        {
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            if (fastPath) {
                YCrCb422BiPlanarToBGRA_ARM_neon_asm((uint8_t *)destBGRA, source->bufPlanes[0], source->bufPlanes[1], width, height);
            } else {
#endif
                ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
                ARUint8 *pCrCb = source->bufPlanes[1];
                uint8_t *outp0 = (uint8_t *)destBGRA;
                uint8_t *outp1 = ((uint8_t *)destBGRA) + width*4;
                int wd2 = width >> 1;
                int hd2 = height >> 1;
                for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                    for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                        int16_t Cb, Cr;
                        int16_t Y0, Y1, R, G, B;
                        uint8_t R0, R1, G0, G1, B0, B1;
                        Cr = ((int16_t)(*(pCrCb++))) - 128;
                        Cb = ((int16_t)(*(pCrCb++))) - 128;
                        R = (        179*Cr) >> 7;
                        G = (-44*Cb - 91*Cr) >> 7;
                        B = (227*Cb        ) >> 7;
                        Y0 = *(pY0++);
                        Y1 = *(pY0++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = B0;
                        *(outp0++) = G0;
                        *(outp0++) = R0;
                        *(outp0++) = 255;
                        *(outp1++) = B1;
                        *(outp1++) = G1;
                        *(outp1++) = R1;
                        *(outp1++) = 255;
                        Y0 = *(pY1++);
                        Y1 = *(pY1++);
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                        *(outp0++) = B0;
                        *(outp0++) = G0;
                        *(outp0++) = R0;
                        *(outp0++) = 255;
                        *(outp1++) = B1;
                        *(outp1++) = G1;
                        *(outp1++) = R1;
                        *(outp1++) = 255;
                    }
                    pY0 += width;
                    pY1 += width;
                    outp0 += width*4;
                    outp1 += width*4;
                }
#if HAVE_ARM_NEON || HAVE_ARM64_NEON
            }
#endif
        }
            break;
        case AR_PIXEL_FORMAT_420v:
        {
            ARUint8 *pY0 = source->bufPlanes[0], *pY1 = source->bufPlanes[0] + width;
            ARUint8 *pCbCr = source->bufPlanes[1];
            uint8_t *outp0 = (uint8_t *)destBGRA;
            uint8_t *outp1 = ((uint8_t *)destBGRA) + width*4;
            int wd2 = width >> 1;
            int hd2 = height >> 1;
            for (int y = 0; y < hd2; y++) { // Groups of two pixels.
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int16_t Cb, Cr;
                    int16_t Y0, Y1, R, G, B;
                    uint8_t R0, R1, G0, G1, B0, B1;
                    Cb = ((int16_t)(*(pCbCr++))) - 128;
                    Cr = ((int16_t)(*(pCbCr++))) - 128;
                    R = (         204*Cr) >> 7;
                    G = (-50*Cb - 104*Cr) >> 7;
                    B = (258*Cb         ) >> 7;
                    Y0 = (149*((int16_t)(*(pY0++))) - 16) >> 7;
                    Y1 = (149*((int16_t)(*(pY0++))) - 16) >> 7;
                    R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                    R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                    G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                    G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                    B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                    B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                    *(outp0++) = B0;
                    *(outp0++) = G0;
                    *(outp0++) = R0;
                    *(outp0++) = 255;
                    *(outp1++) = B1;
                    *(outp1++) = G1;
                    *(outp1++) = R1;
                    *(outp1++) = 255;
                    Y0 = (149*((int16_t)(*(pY1++))) - 16) >> 7;
                    Y1 = (149*((int16_t)(*(pY1++))) - 16) >> 7;
                    R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                    R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                    G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                    G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                    B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                    B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
                    *(outp0++) = B0;
                    *(outp0++) = G0;
                    *(outp0++) = R0;
                    *(outp0++) = 255;
                    *(outp1++) = B1;
                    *(outp1++) = G1;
                    *(outp1++) = R1;
                    *(outp1++) = 255;
                }
                pY0 += width;
                pY1 += width;
                outp0 += width*4;
                outp1 += width*4;
            }
        }
            break;
        case AR_PIXEL_FORMAT_2vuy:
        {
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                int wd2 = width >> 1;
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int B0, G0, R0;
                    float Cb =      (float)(inp[0] - 128); // Byte 0 of each 4-byte block for both even- and odd-numbered columns.
                    float Y0prime = (float)(inp[1] - 16);  // Byte 1 of each 4-byte block.
                    float Cr =      (float)(inp[2] - 128); // Byte 2 of each 4-byte block for both even- and odd-numbered columns.
                    float Y1prime = (float)(inp[3] - 16);  // Byte 3 of each 4-byte block.
                    inp += pixelSize * 2;
                    // Conversion from Poynton's color FAQ http://www.poynton.com.
                    B0 = (int)(298.082f*Y0prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y0prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y0prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(R0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(B0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(B0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(R0, 0, 255)) << 8) | 0xff;
#endif
                    B0 = (int)(298.082f*Y1prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y1prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y1prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(R0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(B0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(B0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(R0, 0, 255)) << 8) | 0xff;
#endif
                }
            }
        }
            break;
        case AR_PIXEL_FORMAT_yuvs:
        {
            for (int y = 0; y < height; y++) {
                ARUint8 *inp = &(source->buff[width*y*pixelSize]);
                uint32_t *outp = &destBGRA[width*y];
                int wd2 = width >> 1;
                for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                    int B0, G0, R0;
                    float Y0prime = (float)(inp[0] - 16);  // Byte 0 of each 4-byte block.
                    float Cb =      (float)(inp[1] - 128); // Byte 1 of each 4-byte block for both even- and odd-numbered columns.
                    float Y1prime = (float)(inp[2] - 16);  // Byte 2 of each 4-byte block.
                    float Cr =      (float)(inp[3] - 128); // Byte 3 of each 4-byte block for both even- and odd-numbered columns.
                    inp += pixelSize * 2;
                    // Conversion from Poynton's color FAQ http://www.poynton.com.
                    B0 = (int)(298.082f*Y0prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y0prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y0prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(R0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(B0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(B0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(R0, 0, 255)) << 8) | 0xff;
#endif
                    B0 = (int)(298.082f*Y1prime + 516.411f*Cb              ) >> 8;
                    G0 = (int)(298.082f*Y1prime - 100.291f*Cb - 208.120f*Cr) >> 8;
                    R0 = (int)(298.082f*Y1prime               + 408.583f*Cr) >> 8;
#ifdef AR_LITTLE_ENDIAN
                    *(outp++) = 0xff000000 | (((uint8_t)CLAMP(R0, 0, 255)) << 16) | (((uint8_t)CLAMP(G0, 0, 255)) << 8) | ((uint8_t)CLAMP(B0, 0, 255));
#else
                    *(outp++) = (((uint8_t)CLAMP(B0, 0, 255)) << 24) | (((uint8_t)CLAMP(G0, 0, 255)) << 16) | (((uint8_t)CLAMP(R0, 0, 255)) << 8) | 0xff;
#endif
                }
            }
        }
            break;
        default:
            return -1;
            break;
    }
    
    return 0;
    
}
