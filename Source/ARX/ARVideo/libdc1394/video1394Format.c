/*
 *  video1394V2Private.h
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
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Kiyoshi Kiyokawa, Hirokazu Kato, Philip Lamb
 *
 */
/*
 *   Video capture subrutine for Linux/libdc1394 devices
 *   author: Kiyoshi Kiyokawa (kiyo@crl.go.jp)
 *           Hirokazu Kato (kato@sys.im.hiroshima-cu.ac.jp)
 *
 *   Revision: 1.0   Date: 2002/01/01
 */

#include <ARX/ARVideo/video.h>

#ifdef ARVIDEO_INPUT_LIBDC1394
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "video1394Private.h"

static void ar2Video1394FormatRGB           (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatYUV411        (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatYUV411Half    (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatYUV422        (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMono          (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColor1    (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColor2    (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColor3    (ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColorHalf1(ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColorHalf2(ARUint8 *src, ARUint8 *dst, int width, int height);
static void ar2Video1394FormatMonoColorHalf3(ARUint8 *src, ARUint8 *dst, int width, int height);

void ar2Video1394FormatConversion(ARUint8 *src, ARUint8 *dst, int mode, int width, int height)
{
    if (mode == AR_VIDEO_1394_MODE_1600x1200_RGB
        || mode == AR_VIDEO_1394_MODE_1600x900_RGB
        || mode == AR_VIDEO_1394_MODE_1280x1024_RGB
        || mode == AR_VIDEO_1394_MODE_1280x960_RGB
        || mode == AR_VIDEO_1394_MODE_1280x720_RGB
        || mode == AR_VIDEO_1394_MODE_1024x768_RGB
        || mode == AR_VIDEO_1394_MODE_640x480_RGB) {
        ar2Video1394FormatRGB(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_640x480_YUV411) {
        ar2Video1394FormatYUV411(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_640x480_YUV411_HALF) {
        ar2Video1394FormatYUV411Half(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_320x240_YUV422
        || mode == AR_VIDEO_1394_MODE_640x480_YUV422) {
        ar2Video1394FormatYUV422(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF) {
        ar2Video1394FormatMonoColorHalf1(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2) {
        ar2Video1394FormatMonoColorHalf2(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3) {
        ar2Video1394FormatMonoColorHalf3(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_1600x1200_MONO
        || mode == AR_VIDEO_1394_MODE_1600x900_MONO
        || mode == AR_VIDEO_1394_MODE_1280x1024_MONO
        || mode == AR_VIDEO_1394_MODE_1280x960_MONO
        || mode == AR_VIDEO_1394_MODE_1280x720_MONO
        || mode == AR_VIDEO_1394_MODE_1024x768_MONO
        || mode == AR_VIDEO_1394_MODE_640x480_MONO) {
        ar2Video1394FormatMono(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_1600x900_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_1280x960_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_1280x720_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR
        || mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR) {
        ar2Video1394FormatMonoColor1(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2
        || mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR2) {
        ar2Video1394FormatMonoColor2(src, dst, width, height);
        return;
    } else if (mode == AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3
        || mode == AR_VIDEO_1394_MODE_640x480_MONO_COLOR3) {
        ar2Video1394FormatMonoColor3(src, dst, width, height);
        return;
    }
    
    return;
}

static void ar2Video1394FormatRGB(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    memcpy(dst, src, width*height*3);
    return;
}

static void ar2Video1394FormatYUV411(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf, *buf2;
    register int U, V, R, G, B, V2, U5, UV;
    register int Y0, Y1, Y2, Y3;
    register int i;
    
    buf  = dst;
    buf2 = src;
    for (i = height * width / 4; i; i--) {
        U   = ((ARUint8)*buf2++ - 128) * 0.354;
        U5  = 5*U;
        Y0  = (ARUint8)*buf2++;
        Y1  = (ARUint8)*buf2++;
        V   = ((ARUint8)*buf2++ - 128) * 0.707;
        V2  = 2*V;
        Y2  = (ARUint8)*buf2++;
        Y3  = (ARUint8)*buf2++;
        UV  = - U - V;
        
        // Original equations
        // R = Y           + 1.402 V
        // G = Y - 0.344 U - 0.714 V
        // B = Y + 1.772 U
        R = Y0 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y0 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y0 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
        
        //---
        R = Y1 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y1 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y1 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
        
        //---
        R = Y2 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y2 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y2 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
        
        //---
        R = Y3 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y3 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y3 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
    }
    
    return;
}

static void ar2Video1394FormatYUV411Half(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf, *buf2;
    register int U, V, R, G, B, V2, U5, UV;
    register int Y0, Y1, Y2, Y3;
    register int i, j;
    
    buf  = dst;
    buf2 = src;
    for (j = 0; j < height / 2; j++) {
        for (i = 0; i < width / 4; i++) {
            U   = ((ARUint8)*buf2++ - 128) * 0.354;
            U5  = 5*U;
            Y0  = (ARUint8)*buf2++;
            Y1  = (ARUint8)*buf2++;
            V   = ((ARUint8)*buf2++ - 128) * 0.707;
            V2  = 2*V;
            Y2  = (ARUint8)*buf2++;
            Y3  = (ARUint8)*buf2++;
            UV  = - U - V;
            
            // Original equations
            // R = Y           + 1.402 V
            // G = Y - 0.344 U - 0.714 V
            // B = Y + 1.772 U
            R = Y0 + V2;
            if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
            
            G = Y0 + UV;
            if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
            
            B = Y0 + U5;
            if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
            
            *buf++ = (ARUint8)R;
            *buf++ = (ARUint8)G;
            *buf++ = (ARUint8)B;
            
            //---
            R = Y1 + V2;
            if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
            
            G = Y1 + UV;
            if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
            
            B = Y1 + U5;
            if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
            
            *buf++ = (ARUint8)R;
            *buf++ = (ARUint8)G;
            *buf++ = (ARUint8)B;
            
            //---
            R = Y2 + V2;
            if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
            
            G = Y2 + UV;
            if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
            
            B = Y2 + U5;
            if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
            
            *buf++ = (ARUint8)R;
            *buf++ = (ARUint8)G;
            *buf++ = (ARUint8)B;
            
            //---
            R = Y3 + V2;
            if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
            
            G = Y3 + UV;
            if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
            
            B = Y3 + U5;
            if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
            
            *buf++ = (ARUint8)R;
            *buf++ = (ARUint8)G;
            *buf++ = (ARUint8)B;
        }
        buf2 += (width / 4) * 6;
        buf  +=  width * 3;
    }
    
    return;
}

static void ar2Video1394FormatYUV422(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf, *buf2;
    register int U, V, R, G, B, V2, U5, UV;
    register int Y0, Y1;
    register int i;
    
    buf  = dst;
    buf2 = src;
    for (i = height * width / 2; i; i--) {
        U   = ((ARUint8)*buf2++ - 128) * 0.354;
        U5  = 5*U;
        Y0  = (ARUint8)*buf2++;
        V   = ((ARUint8)*buf2++ - 128) * 0.707;
        V2  = 2*V;
        Y1  = (ARUint8)*buf2++;
        UV  = - U - V;
        
        //---
        R = Y0 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y0 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y0 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
        
        //---
        R = Y1 + V2;
        if ((R >> 8) > 0) R = 255; else if (R < 0) R = 0;
        
        G = Y1 + UV;
        if ((G >> 8) > 0) G = 255; else if (G < 0) G = 0;
        
        B = Y1 + U5;
        if ((B >> 8) > 0) B = 255; else if (B < 0) B = 0;
        
        *buf++ = (ARUint8)R;
        *buf++ = (ARUint8)G;
        *buf++ = (ARUint8)B;
    }
    
    return;
}

static void ar2Video1394FormatMono(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    memcpy(dst, src, width*height);
    return;
}

/*  RGRGRGRGRGRGRG */
/*  GBGBGBGBGBGBGB */
/*  RGRGRGRGRGRGRG */
/*  GBGBGBGBGBGBGB */
static void ar2Video1394FormatMonoColor1(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *p2;
    *buf++ = *(p2+1);
    *buf++ = *(p3+1);
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        *buf++ = *p3;
        p2++;
        p3++;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        p2++;
        p3++;
    }
    *buf++ = *(p2-1);
    *buf++ = *p2;
    *buf++ = *p3;
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        *buf++ = *(p2+1);
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = ((int)*(p1-1) + (int)*(p3-1))/2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        p1++;
        p2++;
        p3++;
        
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = ((int)*(p1+1) + (int)*(p3+1))/2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            p1++;
            p2++;
            p3++;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = *(p2-1);
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        p1++;
        p2++;
        p3++;
    }
    
    *buf++ = *p1;
    *buf++ = *p2;
    *buf++ = *(p2+1);
    p1++;
    p2++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p1-1) + (int)*(p1+1))/2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        p1++;
        p2++;
        *buf++ = *p1;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        p1++;
        p2++;
    }
    *buf++ = *(p1-1);
    *buf++ = *(p2-1);
    *buf++ = *p2;
    
    return;
}

/*  GRGRGRGRGRGRGR */
/*  BGBGBGBGBGBGBG */
/*  GRGRGRGRGRGRGR */
/*  BGBGBGBGBGBGBG */
static void ar2Video1394FormatMonoColor2(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *(p2+1);
    *buf++ = *p2;
    *buf++ = *p3;
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        p2++;
        p3++;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        *buf++ = *p3;
        p2++;
        p3++;
    }
    *buf++ = *p2;
    *buf++ = *p3;
    *buf++ = *(p3-1);
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        *buf++ = ((int)*(p1+1) + (int)*(p3+1))/2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*p1 + (int)*p3)/2;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        *buf++ = (int)*(p2-1);
        p1++;
        p2++;
        p3++;
        
        *buf++ = *(p2+1);
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = ((int)*(p1-1) + (int)*(p3-1))/2;
        p1++;
        p2++;
        p3++;
    }
    
    *buf++ = *(p1+1);
    *buf++ = *p1;
    *buf++ = *p2;
    p1++;
    p2++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = *p1;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        p1++;
        p2++;
        *buf++ = ((int)*(p1-1) + (int)*(p1+1))/2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        p1++;
        p2++;
    }
    *buf++ = *(p1-1);
    *buf++ = *p2;
    *buf++ = *(p2-1);
    
    return;
}

/*  GBGBGBGBGBGBGB */
/*  RGRGRGRGRGRGRG */
/*  GBGBGBGBGBGBGB */
/*  RGRGRGRGRGRGRG */
static void ar2Video1394FormatMonoColor3(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *p3;
    *buf++ = *p2;
    *buf++ = *(p2+1);
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        p2++;
        p3++;
        *buf++ = *p3;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        p2++;
        p3++;
    }
    *buf++ = *(p3-1);
    *buf++ = *(p2-1);
    *buf++ = *p2;
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = ((int)*(p1+1) + (int)*(p3+1))/2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            p1++;
            p2++;
            p3++;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = *(p2-1);
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        p1++;
        p2++;
        p3++;
        
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        *buf++ = *(p2+1);
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = ((int)*(p1-1) + (int)*(p3-1))/2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        p1++;
        p2++;
        p3++;
    }
    
    *buf++ = *p2;
    *buf++ = *(p2+1);
    *buf++ = *(p1+1);
    p1++;
    p2++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        *buf++ = *p1;
        p1++;
        p2++;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = ((int)*(p1-1) + (int)*(p1+1))/2;
        p1++;
        p2++;
    }
    *buf++ = *(p2-1);
    *buf++ = *p2;
    *buf++ = *p1;
    
    return;
}

static void ar2Video1394FormatMonoColorHalf1(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *p2;
    *buf++ = *(p2+1);
    *buf++ = *(p3+1);
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        *buf++ = *p3;
        p2++;
        p3++;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        p2++;
        p3++;
    }
    *buf++ = *(p2-1);
    *buf++ = *p2;
    *buf++ = *p3;
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        p1 += width;
        p2 += width;
        p3 += width;
        buf += width * 3;
        
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = ((int)*(p1+1) + (int)*(p3+1))/2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            p1++;
            p2++;
            p3++;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = *(p2-1);
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        p1++;
        p2++;
        p3++;
    }
    
    return;
}

static void ar2Video1394FormatMonoColorHalf2(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *(p2+1);
    *buf++ = *p2;
    *buf++ = *p3;
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        p2++;
        p3++;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        *buf++ = *p3;
        p2++;
        p3++;
    }
    *buf++ = *p2;
    *buf++ = *(p2-1);
    *buf++ = *(p3-1);
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        p1 += width;
        p2 += width;
        p3 += width;
        buf += width * 3;
        
        *buf++ = *(p2+1);
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = *p2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = ((int)*(p1-1) + (int)*(p3-1))/2;
        p1++;
        p2++;
        p3++;
    }
    
    return;
}

static void ar2Video1394FormatMonoColorHalf3(ARUint8 *src, ARUint8 *dst, int width, int height)
{
    register ARUint8 *buf;
    register ARUint8 *p1, *p2, *p3;
    register int i, j;
    
    buf  = dst;
    p2 = src;
    p3 = p2 + width;
    
    *buf++ = *p3;
    *buf++ = *p2;
    *buf++ = *(p2+1);
    p2++;
    p3++;
    for (i = width/2 - 1; i; i--) {
        *buf++ = ((int)*(p3-1) + (int)*(p3+1))/2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        *buf++ = *p2;
        p2++;
        p3++;
        *buf++ = *p3;
        *buf++ = *p2;
        *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
        p2++;
        p3++;
    }
    *buf++ = *(p3-1);
    *buf++ = *(p2-1);
    *buf++ = *p2;
    p2++;
    p3++;
    p1 = src;
    
    for (j = height/2 - 1; j; j--) {
        p1 += width;
        p2 += width;
        p3 += width;
        buf += width * 3;
        
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        *buf++ = *(p2+1);
        p1++;
        p2++;
        p3++;
        for (i = width/2 - 1; i; i--) {
            *buf++ = ((int)*(p1-1) + (int)*(p1+1) + (int)*(p3-1) + (int)*(p3+1))/4;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            *buf++ = *p2;
            p1++;
            p2++;
            p3++;
            *buf++ = ((int)*p1 + (int)*p3)/2;
            *buf++ = *p2;
            *buf++ = ((int)*(p2-1) + (int)*(p2+1))/2;
            p1++;
            p2++;
            p3++;
        }
        *buf++ = ((int)*(p1-1) + (int)*(p3-1))/2;
        *buf++ = ((int)*p1 + (int)*p3)/2;
        *buf++ = *p2;
        p1++;
        p2++;
        p3++;
    }
    
    return;
}

#endif // ARVIDEO_INPUT_LIBDC1394
