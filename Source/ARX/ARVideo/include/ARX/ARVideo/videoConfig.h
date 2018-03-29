/*
 *	videoConfig.h
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR_VIDEO_CONFIG_H
#define AR_VIDEO_CONFIG_H

#include <ARX/AR/config.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef ARVIDEO_INPUT_V4L2
#define   AR_VIDEO_V4L2_MODE_PAL              0
#define   AR_VIDEO_V4L2_MODE_NTSC             1
#define   AR_VIDEO_V4L2_MODE_SECAM            2
#define   AR_VIDEO_V4L2_DEFAULT_DEVICE        "/dev/video0"
#define   AR_VIDEO_V4L2_DEFAULT_WIDTH         640
#define   AR_VIDEO_V4L2_DEFAULT_HEIGHT        480
#define   AR_VIDEO_V4L2_DEFAULT_CHANNEL       0
#define   AR_VIDEO_V4L2_DEFAULT_MODE          AR_VIDEO_V4L2_MODE_NTSC
#define   AR_VIDEO_V4L2_DEFAULT_FORMAT_CONVERSION AR_PIXEL_FORMAT_BGRA // Options include AR_PIXEL_FORMAT_INVALID for no conversion, AR_PIXEL_FORMAT_BGRA, and AR_PIXEL_FORMAT_RGBA.
#endif


#ifdef ARVIDEO_INPUT_LIBDC1394
enum {
    AR_VIDEO_1394_MODE_320x240_YUV422 = 32,
    AR_VIDEO_1394_MODE_640x480_YUV411,
    AR_VIDEO_1394_MODE_640x480_YUV411_HALF,
    AR_VIDEO_1394_MODE_640x480_YUV422,
    AR_VIDEO_1394_MODE_640x480_RGB,
    AR_VIDEO_1394_MODE_640x480_MONO,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR2,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR3,
    AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_800x600_RGB,
    AR_VIDEO_1394_MODE_800x600_MONO,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR2,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR3,
    AR_VIDEO_1394_MODE_800x600_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1024x768_RGB,
    AR_VIDEO_1394_MODE_1024x768_MONO,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1024x768_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1280x720_RGB,
    AR_VIDEO_1394_MODE_1280x720_MONO,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1280x720_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1280x960_RGB,
    AR_VIDEO_1394_MODE_1280x960_MONO,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1280x960_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1280x1024_RGB,
    AR_VIDEO_1394_MODE_1280x1024_MONO,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1280x1024_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1600x900_RGB,
    AR_VIDEO_1394_MODE_1600x900_MONO,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1600x900_MONO_COLOR_HALF3,
    AR_VIDEO_1394_MODE_1600x1200_RGB,
    AR_VIDEO_1394_MODE_1600x1200_MONO,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR_HALF,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR2,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR_HALF2,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR3,
    AR_VIDEO_1394_MODE_1600x1200_MONO_COLOR_HALF3
};
#define   AR_VIDEO_1394_FRAME_RATE_1_875                1
#define   AR_VIDEO_1394_FRAME_RATE_3_75                 2
#define   AR_VIDEO_1394_FRAME_RATE_7_5                  3
#define   AR_VIDEO_1394_FRAME_RATE_15                   4
#define   AR_VIDEO_1394_FRAME_RATE_30                   5
#define   AR_VIDEO_1394_FRAME_RATE_60                   6
#define   AR_VIDEO_1394_FRAME_RATE_120                  7
#define   AR_VIDEO_1394_FRAME_RATE_240                  8
#define   AR_VIDEO_1394_SPEED_400                       1
#define   AR_VIDEO_1394_SPEED_800                       2

#if ARVIDEO_INPUT_LIBDC1394_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO
#  if defined(ARVIDEO_INPUT_LIBDC1394_USE_FLEA_XGA)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_1024x768_MONO
#  else
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_MONO
#  endif
#else
#  if defined(ARVIDEO_INPUT_LIBDC1394_USE_DRAGONFLY)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_MONO_COLOR
#  elif defined(ARVIDEO_INPUT_LIBDC1394_USE_DF_EXPRESS)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_MONO_COLOR2
#  elif defined(ARVIDEO_INPUT_LIBDC1394_USE_FLEA)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_MONO_COLOR2
#  elif defined(ARVIDEO_INPUT_LIBDC1394_USE_FLEA_XGA)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_1024x768_MONO_COLOR
#  elif defined(ARVIDEO_INPUT_LIBDC1394_USE_DFK21AF04)
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_MONO_COLOR3
#  else
#    define   AR_VIDEO_1394_DEFAULT_MODE               AR_VIDEO_1394_MODE_640x480_YUV411
#  endif
#endif
#define   AR_VIDEO_1394_DEFAULT_FRAME_RATE             AR_VIDEO_1394_FRAME_RATE_30
#define   AR_VIDEO_1394_DEFAULT_SPEED                  AR_VIDEO_1394_SPEED_400
#define   AR_VIDEO_1394_DEFAULT_PORT                   0
#endif

#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_CONFIG_H
