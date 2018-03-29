/*
 * 1394-Based Digital Camera Control Library
 *
 * Color conversion functions, including Bayer pattern decoding
 *
 * Written by Damien Douxchamps and Frederic Devernay
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dc1394/log.h>

#ifndef __DC1394_CONVERSIONS_H__
#define __DC1394_CONVERSIONS_H__

/*! \file dc1394/conversions.h
    \brief functions to convert video formats
    \author Damien Douxchamps: coding
    \author Frederic Devernay: coding
    \author Peter Antoniac: documentation maintainer

    More details soon
*/

#define restrict __restrict

/**
 * A list of de-mosaicing techniques for Bayer-patterns.
 *
 * The speed of the techniques can vary greatly, as well as their quality.
 */
typedef enum {
    DC1394_BAYER_METHOD_NEAREST=0,
    DC1394_BAYER_METHOD_SIMPLE,
    DC1394_BAYER_METHOD_BILINEAR,
    DC1394_BAYER_METHOD_HQLINEAR,
    DC1394_BAYER_METHOD_DOWNSAMPLE,
    DC1394_BAYER_METHOD_EDGESENSE,
    DC1394_BAYER_METHOD_VNG,
    DC1394_BAYER_METHOD_AHD
} dc1394bayer_method_t;
#define DC1394_BAYER_METHOD_MIN      DC1394_BAYER_METHOD_NEAREST
#define DC1394_BAYER_METHOD_MAX      DC1394_BAYER_METHOD_AHD
#define DC1394_BAYER_METHOD_NUM     (DC1394_BAYER_METHOD_MAX-DC1394_BAYER_METHOD_MIN+1)

/**
 * A list of known stereo-in-normal-video modes used by manufacturers like Point Grey Research and Videre Design.
 */
typedef enum {
    DC1394_STEREO_METHOD_INTERLACED=0,
    DC1394_STEREO_METHOD_FIELD
} dc1394stereo_method_t;
#define DC1394_STEREO_METHOD_MIN     DC1394_STEREO_METHOD_INTERLACED
#define DC1394_STEREO_METHOD_MAX     DC1394_STEREO_METHOD_FIELD
#define DC1394_STEREO_METHOD_NUM    (DC1394_STEREO_METHOD_MAX-DC1394_STEREO_METHOD_MIN+1)


// color conversion functions from Bart Nabbe.
// corrected by Damien: bad coeficients in YUV2RGB
#define YUV2RGB(y, u, v, r, g, b) {\
  r = y + ((v*1436) >> 10);\
  g = y - ((u*352 + v*731) >> 10);\
  b = y + ((u*1814) >> 10);\
  r = r < 0 ? 0 : r;\
  g = g < 0 ? 0 : g;\
  b = b < 0 ? 0 : b;\
  r = r > 255 ? 255 : r;\
  g = g > 255 ? 255 : g;\
  b = b > 255 ? 255 : b; }


#define RGB2YUV(r, g, b, y, u, v) {\
  y = (306*r + 601*g + 117*b)  >> 10;\
  u = ((-172*r - 340*g + 512*b) >> 10)  + 128;\
  v = ((512*r - 429*g - 83*b) >> 10) + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v; }

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************
 *  CONVERSION FUNCTIONS TO YUV422, MONO8 and RGB8
 **********************************************************************/

/**
 * Converts an image buffer to YUV422
 */
dc1394error_t
dc1394_convert_to_YUV422(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height, uint32_t byte_order,
                         dc1394color_coding_t source_coding, uint32_t bits);

/**
 * Converts an image buffer to MONO8
 */
dc1394error_t
dc1394_convert_to_MONO8(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height, uint32_t byte_order,
                        dc1394color_coding_t source_coding, uint32_t bits);

/**
 * Converts an image buffer to RGB8
 */
dc1394error_t
dc1394_convert_to_RGB8(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height, uint32_t byte_order,
                       dc1394color_coding_t source_coding, uint32_t bits);

/**********************************************************************
 *  CONVERSION FUNCTIONS FOR STEREO IMAGES
 **********************************************************************/

/**
 * changes a 16bit stereo image (8bit/channel) into two 8bit images on top of each other
 */
dc1394error_t
dc1394_deinterlace_stereo(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height);

/************************************************************************************************
 *                                                                                              *
 *      Color conversion functions for cameras that can output raw Bayer pattern images (color  *
 *  codings DC1394_COLOR_CODING_RAW8 and DC1394_COLOR_CODING_RAW16).                            *
 *                                                                                              *
 *  Credits and sources:                                                                        *
 *  - Nearest Neighbor : OpenCV library                                                         *
 *  - Bilinear         : OpenCV library                                                         *
 *  - HQLinear         : High-Quality Linear Interpolation For Demosaicing Of Bayer-Patterned   *
 *                       Color Images, by Henrique S. Malvar, Li-wei He, and Ross Cutler,       *
 *                       in Proceedings of the ICASSP'04 Conference.                            *
 *  - Edge Sense II    : Laroche, Claude A. "Apparatus and method for adaptively interpolating  *
 *                       a full color image utilizing chrominance gradients"                    *
 *                       U.S. Patent 5,373,322. Based on the code found on the website          *
 *                       http://www-ise.stanford.edu/~tingchen/ Converted to C and adapted to   *
 *                       all four elementary patterns.                                          *
 *  - Downsample       : "Known to the Ancients"                                                *
 *  - Simple           : Implemented from the information found in the manual of Allied Vision  *
 *                       Technologies (AVT) cameras.                                            *
 *  - VNG              : Variable Number of Gradients, a method described in                    *
 *                       http://www-ise.stanford.edu/~tingchen/algodep/vargra.html              *
 *                       Sources import from DCRAW by Frederic Devernay. DCRAW is a RAW         *
 *                       converter program by Dave Coffin. URL:                                 *
 *                       http://www.cybercom.net/~dcoffin/dcraw/                                *
 *  - AHD              : Adaptive Homogeneity-Directed Demosaicing Algorithm, by K. Hirakawa    *
 *                       and T.W. Parks, IEEE Transactions on Image Processing, Vol. 14, Nr. 3, *
 *                       March 2005, pp. 360 - 369.                                             *
 *                                                                                              *
 ************************************************************************************************/

/**
 * Perform de-mosaicing on an 8-bit image buffer
 */
dc1394error_t
dc1394_bayer_decoding_8bit(const uint8_t *bayer, uint8_t *rgb,
                           uint32_t width, uint32_t height, dc1394color_filter_t tile,
                           dc1394bayer_method_t method);

/**
 * Perform de-mosaicing on an 16-bit image buffer
 */
dc1394error_t
dc1394_bayer_decoding_16bit(const uint16_t *bayer, uint16_t *rgb,
                            uint32_t width, uint32_t height, dc1394color_filter_t tile,
                            dc1394bayer_method_t method, uint32_t bits);


/**********************************************************************************
 *  Frame based conversions
 **********************************************************************************/

/**
 * Converts the format of a video frame.
 *
 * To set the format of the output, simply set the values of the corresponding fields in the output frame
 */
dc1394error_t
dc1394_convert_frames(dc1394video_frame_t *in, dc1394video_frame_t *out);

/**
 * De-mosaicing of a Bayer-encoded video frame
 *
 * To set the format of the output, simply set the values of the corresponding fields in the output frame
 * @param in is a pointer to the bayer video frame that is to be converted
 * @param out is a pointer to the frame to be converted to.  If there is memory allocated to the image field, 
 *      then it will be adjusted accordingly by this function.  If there is no memory allocated to the image
 *      field, then ensure that out->image == NULL and out->allocated_image_bytes == 0
 * @param method is the bayer method to interpolate the frame.
 */
dc1394error_t
dc1394_debayer_frames(dc1394video_frame_t *in, dc1394video_frame_t *out, dc1394bayer_method_t method);

/**
 * De-interlacing of stereo data for cideo frames
 *
 * To set the format of the output, simply set the values of the corresponding fields in the output frame
 */
dc1394error_t
dc1394_deinterlace_stereo_frames(dc1394video_frame_t *in, dc1394video_frame_t *out, dc1394stereo_method_t method);

#ifdef __cplusplus
}
#endif

#endif /* _DC1394_CONVERSIONS_H */


