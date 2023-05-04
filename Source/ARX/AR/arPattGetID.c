/*
 *  arPattGetID.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 3.0
 * Date: 03/08/13
 *
 *******************************************************/

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#if DEBUG_PATT_GETID
#  ifndef __APPLE__
#    include <GL/gl.h>
#  else
#    include <OpenGL/gl.h>
#  endif
int  cnt = 1;
#endif

#ifndef MAX
#  define MAX(x,y) (x > y ? x : y)
#endif
#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif
#ifndef CLAMP
#  define CLAMP(x,r1,r2) (MIN(MAX(x,r1),r2))
#endif

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#  define _100_0 100.0f
#  define _30_0 30.f
#  define _10_0 10.0f
#  define _2_0 2.0f
#  define SQRT_3_0 1.7320508f
#  define _1_0 1.0f
#  define _0_5 0.5f
#  define _0_0 0.0f
#else
#  define SQRT sqrt
#  define _100_0 100.0
#  define _30_0 30.0
#  define _10_0 10.0
#  define _2_0 2.0
#  define SQRT_3_0 1.7320508
#  define _1_0 1.0
#  define _0_5 0.5
#  define _0_0 0.0
#endif

#define AR_GLOBAL_ID_OUTER_SIZE 14
#define AR_GLOBAL_ID_INNER_SIZE 3

//#define DEBUG_BCH

static void   get_cpara( ARdouble world[4][2], ARdouble vertex[4][2],
                         ARdouble para[3][3] );
static int    pattern_match( ARPattHandle *pattHandle, int mode, ARUint8 *data, int size,
                             int *code, int *dir, ARdouble *cf );
static int    decode_bch(const AR_MATRIX_CODE_TYPE matrixCodeType, const uint64_t in, uint8_t recd127[127], uint64_t *out_p);
static int    get_matrix_code( ARUint8 *data, int size, int *code_out_p, int *dir, ARdouble *cf, const AR_MATRIX_CODE_TYPE matrixCodeType, int *errorCorrected );
static int    get_global_id_code( ARUint8 *data, uint64_t *code_out_p, int *dir, ARdouble *cf, int *errorCorrected );

#if !AR_DISABLE_NON_CORE_FNS
int arPattGetID( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
                 ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat,
                 int *x_coord, int *y_coord, int *vertex, ARdouble pattRatio,
                 int *code, int *dir, ARdouble *cf, const AR_MATRIX_CODE_TYPE matrixCodeType )
{
    ARUint8 ext_patt1[AR_PATT_SIZE1_MAX*AR_PATT_SIZE1_MAX*3]; // Holds unwarped pattern extracted from image for template matching.
    ARUint8 ext_patt2[AR_PATT_SIZE2_MAX*AR_PATT_SIZE2_MAX]; // Holds unwarped pattern extracted from image for matrix-code matching.
    int errorCodeMtx, errorCodePatt;

    // Matrix code detection pass.
    if( pattDetectMode == AR_MATRIX_CODE_DETECTION
     || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX
     || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
        arPattGetImage(imageProcMode, AR_MATRIX_CODE_DETECTION, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, (matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK)*AR_PATT_SAMPLE_FACTOR2,
                       image, xsize, ysize, pixelFormat, x_coord, y_coord, vertex, pattRatio, ext_patt2);
#if DEBUG_PATT_GETID
        glPixelZoom( 4.0f, -4.0f);
        glRasterPos3f( 0.0f, (matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK)*4.0f*cnt, 1.0f );
        glDrawPixels( matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt2 );
        glPixelZoom( 1.0f, 1.0f);
        cnt++;
#endif
        errorCodeMtx = get_matrix_code(ext_patt2, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, code, dir, cf, matrixCodeType, NULL);
    } else errorCodeMtx = 1;

    // Template matching pass.
    if( pattDetectMode == AR_TEMPLATE_MATCHING_COLOR
     || pattDetectMode == AR_TEMPLATE_MATCHING_MONO
     || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX
     || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
        if (!pattHandle) {
            *code = 0;
            *dir  = 0;
            *cf   = -_1_0;
            errorCodePatt = -1;
        } else {
            if (pattDetectMode == AR_TEMPLATE_MATCHING_COLOR || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX) {
                arPattGetImage(imageProcMode, AR_TEMPLATE_MATCHING_COLOR, pattHandle->pattSize, pattHandle->pattSize*AR_PATT_SAMPLE_FACTOR1,
                               image, xsize, ysize, pixelFormat, x_coord, y_coord, vertex, pattRatio, ext_patt1);
                errorCodePatt = pattern_match(pattHandle, AR_TEMPLATE_MATCHING_COLOR, ext_patt1, pattHandle->pattSize, code, dir, cf);
            } else {
                arPattGetImage(imageProcMode, AR_TEMPLATE_MATCHING_MONO, pattHandle->pattSize, pattHandle->pattSize*AR_PATT_SAMPLE_FACTOR1,
                               image, xsize, ysize, pixelFormat, x_coord, y_coord, vertex, pattRatio, ext_patt1);
                errorCodePatt = pattern_match(pattHandle, AR_TEMPLATE_MATCHING_MONO, ext_patt1, pattHandle->pattSize, code, dir, cf);
            }
#if DEBUG_PATT_GETID
            glPixelZoom( 4.0f, -4.0f);
            glRasterPos3f( 0.0f, pattHandle->pattSize*4.0f*cnt, 1.0f );
            if( pattDetectMode == AR_TEMPLATE_MATCHING_COLOR || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX)
                glDrawPixels( pattHandle->pattSize, pattHandle->pattSize, GL_BGR, GL_UNSIGNED_BYTE, ext_patt1 );
            else
                glDrawPixels( pattHandle->pattSize, pattHandle->pattSize, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt1 );
            glPixelZoom( 1.0f, 1.0f);
            cnt++;
#endif
        }
    } else errorCodePatt = 1;

    if (errorCodeMtx == 1) return (errorCodePatt);
    else if (errorCodePatt == 1) return (errorCodeMtx);
    else if (errorCodeMtx < 0 && errorCodePatt < 0) return (errorCodePatt);
    else return (0);
}

int arPattGetID2( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
                 ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *paramLTf, ARdouble vertex[4][2], ARdouble pattRatio,
                 int *codePatt, int *dirPatt, ARdouble *cfPatt, int *codeMatrix, int *dirMatrix, ARdouble *cfMatrix,
                 const AR_MATRIX_CODE_TYPE matrixCodeType )
{
    return (arPattGetIDGlobal(pattHandle, imageProcMode, pattDetectMode, image, xsize, ysize, pixelFormat, paramLTf, vertex, pattRatio,
                              codePatt, dirPatt, cfPatt, codeMatrix, dirMatrix, cfMatrix,
                              matrixCodeType, NULL, NULL));
}
#endif // !AR_DISABLE_NON_CORE_FNS

int arPattGetIDGlobal( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
                      ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *paramLTf, ARdouble vertex[4][2], ARdouble pattRatio,
                      int *codePatt, int *dirPatt, ARdouble *cfPatt, int *codeMatrix, int *dirMatrix, ARdouble *cfMatrix,
                      const AR_MATRIX_CODE_TYPE matrixCodeType, int *errorCorrected, uint64_t *codeGlobalID_p )
{
    ARUint8 ext_patt[MAX(AR_PATT_SIZE1_MAX,AR_PATT_SIZE2_MAX)*MAX(AR_PATT_SIZE1_MAX,AR_PATT_SIZE2_MAX)*3]; // Holds unwarped pattern extracted from image.
    int errorCodeMtx, errorCodePatt;
    uint64_t codeGlobalID;

    // Matrix code detection pass.
    if( pattDetectMode == AR_MATRIX_CODE_DETECTION
       || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX
       || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
        if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID) {
            if (arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, AR_GLOBAL_ID_OUTER_SIZE, AR_GLOBAL_ID_OUTER_SIZE * AR_PATT_SAMPLE_FACTOR2,
                                image, xsize, ysize, pixelFormat, paramLTf, vertex, (((ARdouble)AR_GLOBAL_ID_OUTER_SIZE)/((ARdouble)(AR_GLOBAL_ID_OUTER_SIZE + 2))), ext_patt) < 0) {
                errorCodeMtx = -6;
                *codeMatrix = -1;
            } else {
                errorCodeMtx = get_global_id_code(ext_patt, &codeGlobalID, dirMatrix, cfMatrix, errorCorrected);
                
                if (errorCodeMtx < 0) {
                    *codeMatrix = -1;
                } else if (codeGlobalID == 0 || codeGlobalID == UINT64_MAX) { // Heuristic-based elimination of frequently misrecognised codes.
                    errorCodeMtx = -5;
                    *codeMatrix = -1;
                } else {
                    if ((codeGlobalID & 0xffffffff80000000ULL) == 0ULL) *codeMatrix = (int)(codeGlobalID & 0x7fffffffULL); // If upper 33 bits are zero, return lower 31 bits as regular matrix code as well.
                    else *codeMatrix = 0; // otherwise, regular matrix code = 0;
                    if (codeGlobalID_p) *codeGlobalID_p = codeGlobalID;
                }
            }
        } else {
            if (arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, (matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK) * AR_PATT_SAMPLE_FACTOR2,
                                image, xsize, ysize, pixelFormat, paramLTf, vertex, pattRatio, ext_patt) < 0) {
                errorCodeMtx = -6;
                *codeMatrix = -1;
            } else {
#if DEBUG_PATT_GETID
                glPixelZoom( 4.0f, -4.0f);
                glRasterPos3f( 0.0f, (matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK)*4.0f*cnt, 1.0f );
                glDrawPixels( matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt );
                glPixelZoom( 1.0f, 1.0f);
                cnt++;
#endif
                errorCodeMtx = get_matrix_code(ext_patt, matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK, codeMatrix, dirMatrix, cfMatrix, matrixCodeType, errorCorrected);
                if (codeGlobalID_p) *codeGlobalID_p = 0ULL;
            }
        }
    } else errorCodeMtx = 1;
    
    // Template matching pass.
    if( pattDetectMode == AR_TEMPLATE_MATCHING_COLOR
       || pattDetectMode == AR_TEMPLATE_MATCHING_MONO
       || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX
       || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
        if (!pattHandle) {
            errorCodePatt = -1;
            *codePatt = -1;
        } else {
            if (pattDetectMode == AR_TEMPLATE_MATCHING_COLOR || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX) {
                if (arPattGetImage2(imageProcMode, AR_TEMPLATE_MATCHING_COLOR, pattHandle->pattSize, pattHandle->pattSize*AR_PATT_SAMPLE_FACTOR1,
                                    image, xsize, ysize, pixelFormat, paramLTf, vertex, pattRatio, ext_patt) < 0) {
                    errorCodePatt = -6;
                    *codePatt = -1;
                } else {
                    errorCodePatt = pattern_match(pattHandle, AR_TEMPLATE_MATCHING_COLOR, ext_patt, pattHandle->pattSize, codePatt, dirPatt, cfPatt);
#if DEBUG_PATT_GETID
                    glPixelZoom( 4.0f, -4.0f);
                    glRasterPos3f( 0.0f, pattHandle->pattSize*4.0f*cnt, 1.0f );
                    glDrawPixels( pattHandle->pattSize, pattHandle->pattSize, GL_BGR, GL_UNSIGNED_BYTE, ext_patt );
                    glPixelZoom( 1.0f, 1.0f);
                    cnt++;
#endif
                }
            } else {
                if (arPattGetImage2(imageProcMode, AR_TEMPLATE_MATCHING_MONO, pattHandle->pattSize, pattHandle->pattSize*AR_PATT_SAMPLE_FACTOR1,
                                    image, xsize, ysize, pixelFormat, paramLTf, vertex, pattRatio, ext_patt) < 0) {
                    errorCodePatt = -6;
                    *codePatt = -1;
                } else {
                    errorCodePatt = pattern_match(pattHandle, AR_TEMPLATE_MATCHING_MONO, ext_patt, pattHandle->pattSize, codePatt, dirPatt, cfPatt);
#if DEBUG_PATT_GETID
                    glPixelZoom( 4.0f, -4.0f);
                    glRasterPos3f( 0.0f, pattHandle->pattSize*4.0f*cnt, 1.0f );
                    glDrawPixels( pattHandle->pattSize, pattHandle->pattSize, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt );
                    glPixelZoom( 1.0f, 1.0f);
                    cnt++;
#endif
                }
            }
        }
    } else errorCodePatt = 1;
    
    if (errorCodeMtx == 1) return (errorCodePatt);                           // pattern-mode only.
    else if (errorCodePatt == 1) return (errorCodeMtx);                      // matrix-mode only.
    else if (errorCodeMtx < 0 && errorCodePatt < 0) return (errorCodePatt);  // if mixed mode and errors in both modes, return error from pattern mode.
    else return (0);
    
}

#if !AR_DISABLE_NON_CORE_FNS
int arPattGetImage( int imageProcMode, int pattDetectMode, int patt_size, int sample_size,
                    ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, int *x_coord, int *y_coord, int *vertex,
                    ARdouble pattRatio, ARUint8 *ext_patt )
{
    ARUint32 *ext_patt2;
    ARdouble    world[4][2];
    ARdouble    local[4][2];
    ARdouble    para[3][3];
    ARdouble    d, xw, yw;
    ARdouble    pattRatio1, pattRatio2;
    int       xc, yc;
    int       xdiv, ydiv;
    int       xdiv2, ydiv2;
    int       lx1, lx2, ly1, ly2, lxPatt, lyPatt;
    int       i, j;

    world[0][0] = _100_0;
    world[0][1] = _100_0;
    world[1][0] = _100_0 + _10_0;
    world[1][1] = _100_0;
    world[2][0] = _100_0 + _10_0;
    world[2][1] = _100_0 + _10_0;
    world[3][0] = _100_0;
    world[3][1] = _100_0 + _10_0;
    for( i = 0; i < 4; i++ ) {
        local[i][0] = x_coord[vertex[i]];
        local[i][1] = y_coord[vertex[i]];
    }
    get_cpara( world, local, para );

	// The square roots of lx1, lx2, ly1, and ly2 are the lengths of the sides of the polygon.
    lx1 = (int)((local[0][0] - local[1][0])*(local[0][0] - local[1][0])
        + (local[0][1] - local[1][1])*(local[0][1] - local[1][1]));
    lx2 = (int)((local[2][0] - local[3][0])*(local[2][0] - local[3][0])
        + (local[2][1] - local[3][1])*(local[2][1] - local[3][1]));
    ly1 = (int)((local[1][0] - local[2][0])*(local[1][0] - local[2][0])
        + (local[1][1] - local[2][1])*(local[1][1] - local[2][1]));
    ly2 = (int)((local[3][0] - local[0][0])*(local[3][0] - local[0][0])
        + (local[3][1] - local[0][1])*(local[3][1] - local[0][1]));
    if( lx2 > lx1 ) lx1 = lx2;
    if( ly2 > ly1 ) ly1 = ly2;
    lxPatt = (int)(lx1*pattRatio*pattRatio);
    lyPatt = (int)(ly1*pattRatio*pattRatio);
    xdiv2 = patt_size;
    ydiv2 = patt_size;
    if( imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE ) {
        while( xdiv2*xdiv2 < lxPatt && xdiv2 < sample_size ) xdiv2*=2; // while (xdiv2 < ((sqrt(lx1) * pattRatio)) xdiv2*=2;
        while( ydiv2*ydiv2 < lyPatt && ydiv2 < sample_size ) ydiv2*=2; // while (ydiv2 < ((sqrt(ly1) * pattRatio)) ydiv2*=2;
    }
    else {
        while( xdiv2*xdiv2*4 < lxPatt && xdiv2 < sample_size ) xdiv2*=2;
        while( ydiv2*ydiv2*4 < lyPatt && ydiv2 < sample_size ) ydiv2*=2;
    }
    if( xdiv2 > sample_size ) xdiv2 = sample_size;
    if( ydiv2 > sample_size ) ydiv2 = sample_size;

    xdiv = xdiv2/patt_size;
    ydiv = ydiv2/patt_size;
    pattRatio1 = (_1_0 - pattRatio)/_2_0 * _10_0; // borderSize * 10.0
    pattRatio2 = pattRatio * _10_0;

    if( pattDetectMode == AR_TEMPLATE_MATCHING_COLOR ) {
        arMallocClear( ext_patt2, ARUint32, patt_size*patt_size*3 );

        if( pixelFormat == AR_PIXEL_FORMAT_RGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*3+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*3+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*3+0];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_BGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*3+0];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*3+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*3+2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+0];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_BGRA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+0];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ABGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+3];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f || pixelFormat == AR_PIXEL_FORMAT_NV21 ) {
            // N.B.: caller asked for colour matching, but we can/will only supply mono.
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[yc*xsize+xc];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[yc*xsize+xc];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[yc*xsize+xc];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ARGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+3];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+1];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_2vuy ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        float Cb =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 0] - 128); // Byte 0 of each 4-byte block for both even- and odd-numbered columns.
                        float Yprime = (float)(image[(yc*xsize +            xc)*2 + 1] - 16);  // Byte 1 of each 4-byte block for even-numbered columns, byte 3 for odd-numbered columns.
                        float Cr =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 2] - 128); // Byte 2 of each 4-byte block for both even- and odd-numbered columns.
						// Conversion from Poynton's color FAQ http://www.poynton.com.
                        int B0 = (int)(298.082f*Yprime + 516.411f*Cb              ) >> 8;
                        int G0 = (int)(298.082f*Yprime - 100.291f*Cb - 208.120f*Cr) >> 8;
                        int R0 = (int)(298.082f*Yprime               + 408.583f*Cr) >> 8;
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += CLAMP(B0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += CLAMP(G0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += CLAMP(R0, 0, 255);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_yuvs ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        float Yprime = (float)(image[(yc*xsize +            xc)*2 + 0] - 16);  // Byte 0 of each 4-byte block for even-numbered columns, byte 2 for odd-numbered columns.
                        float Cb =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 1] - 128); // Byte 1 of each 4-byte block for both even- and odd-numbered columns.
                        float Cr =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 3] - 128); // Byte 3 of each 4-byte block for both even- and odd-numbered columns.
						// Conversion from Poynton's color FAQ http://www.poynton.com.
                        int B0 = (int)(298.082f*Yprime + 516.411f*Cb              ) >> 8;
                        int G0 = (int)(298.082f*Yprime - 100.291f*Cb - 208.120f*Cr) >> 8;
                        int R0 = (int)(298.082f*Yprime               + 408.583f*Cr) >> 8;
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += CLAMP(B0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += CLAMP(G0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += CLAMP(R0, 0, 255);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGB_565 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=                                            (((image[(yc*xsize+xc)*2+1] & 0x1f) << 3) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xe0) >> 3) + 0x02);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_5551 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=                                            (((image[(yc*xsize+xc)*2+1] & 0x3e) << 2) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xc0) >> 3) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_4444 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=  ((image[(yc*xsize+xc)*2+1] & 0xf0) + 0x08);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x0f) << 4) + 0x08);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf0) + 0x08);
                    }
                }
            }
        }
        else {
            ARLOGe("Error: unsupported pixel format.\n");
            goto bail;
        }

        for( i = 0; i < patt_size*patt_size*3; i++ ) {
            ext_patt[i] = ext_patt2[i] / (xdiv*ydiv);
        }

        free( ext_patt2 );
    }
    else { // pattDetectMode != AR_TEMPLATE_MATCHING_COLOR
        arMallocClear( ext_patt2, ARUint32, patt_size*patt_size );

        if( pixelFormat == AR_PIXEL_FORMAT_RGB || pixelFormat == AR_PIXEL_FORMAT_BGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*3+0]
                                  + image[(yc*xsize+xc)*3+1]
                                  + image[(yc*xsize+xc)*3+2] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA || pixelFormat == AR_PIXEL_FORMAT_BGRA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*4+0]
                                  + image[(yc*xsize+xc)*4+1]
                                  + image[(yc*xsize+xc)*4+2] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ABGR || pixelFormat == AR_PIXEL_FORMAT_ARGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*4+1]
                                  + image[(yc*xsize+xc)*4+2]
                                  + image[(yc*xsize+xc)*4+3] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f || pixelFormat == AR_PIXEL_FORMAT_NV21 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[yc*xsize+xc];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_2vuy ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[(yc*xsize+xc)*2+1];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_yuvs ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[(yc*xsize+xc)*2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGB_565 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (   ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04)
                            + (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xe0) >> 3) + 0x02)
                            + (((image[(yc*xsize+xc)*2+1] & 0x1f) << 3) + 0x04) )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_5551 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (    ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04)
                             + (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xc0) >> 3) + 0x04)
                             + (((image[(yc*xsize+xc)*2+1] & 0x3e) << 2) + 0x04) )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_4444 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc = (int)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc = (int)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((xc+1)/2)*2;
                        yc = ((yc+1)/2)*2;
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (    ((image[(yc*xsize+xc)*2+0] & 0xf0) + 0x08)
                             + (((image[(yc*xsize+xc)*2+0] & 0x0f) << 4) + 0x08)
                             +  ((image[(yc*xsize+xc)*2+1] & 0xf0) + 0x08) )/3;
                    }
                }
            }
        }
        else {
            ARLOGe("Error: unsupported pixel format.\n");
            goto bail;
        }
        
        for( i = 0; i < patt_size*patt_size; i++ ) {
            ext_patt[i] = ext_patt2[i] / (xdiv*ydiv);
        }

        free( ext_patt2 );
    }

    return 0;
    
bail:
    free(ext_patt2);
    return -1;
}

#endif // !AR_DISABLE_NON_CORE_FNS

int arPattGetImage2( int imageProcMode, int pattDetectMode, int patt_size, int sample_size,
                     ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *paramLTf,
                     ARdouble vertex[4][2], ARdouble pattRatio, ARUint8 *ext_patt)
{
    ARUint32 *ext_patt2;
    ARdouble  world[4][2];
    ARdouble  local[4][2];
    ARdouble  para[3][3];
    ARdouble  d, xw, yw;
    float     xc2, yc2;
    ARdouble  pattRatio1, pattRatio2;
    int       xc, yc;
    int       xdiv, ydiv;
    int       xdiv2, ydiv2;
    int       lx1, lx2, ly1, ly2, lxPatt, lyPatt;
    int       i, j;

    world[0][0] = _100_0;
    world[0][1] = _100_0;
    world[1][0] = _100_0 + _10_0;
    world[1][1] = _100_0;
    world[2][0] = _100_0 + _10_0;
    world[2][1] = _100_0 + _10_0;
    world[3][0] = _100_0;
    world[3][1] = _100_0 + _10_0;
    for( i = 0; i < 4; i++ ) {
        local[i][0] = vertex[i][0];
        local[i][1] = vertex[i][1];
    }
    get_cpara( world, local, para );

	// The square roots of lx1, lx2, ly1, and ly2 are the lengths of the sides of the polygon.
    lx1 = (int)((local[0][0] - local[1][0])*(local[0][0] - local[1][0])
              + (local[0][1] - local[1][1])*(local[0][1] - local[1][1]));
    lx2 = (int)((local[2][0] - local[3][0])*(local[2][0] - local[3][0])
              + (local[2][1] - local[3][1])*(local[2][1] - local[3][1]));
    ly1 = (int)((local[1][0] - local[2][0])*(local[1][0] - local[2][0])
              + (local[1][1] - local[2][1])*(local[1][1] - local[2][1]));
    ly2 = (int)((local[3][0] - local[0][0])*(local[3][0] - local[0][0])
              + (local[3][1] - local[0][1])*(local[3][1] - local[0][1]));
    
    // Take the longest two adjacent sides, and calculate the length of those sides which is pattern space (actually the square of the length).
    if( lx2 > lx1 ) lx1 = lx2;
    if( ly2 > ly1 ) ly1 = ly2;
    lxPatt = (int)(lx1*pattRatio*pattRatio);
    lyPatt = (int)(ly1*pattRatio*pattRatio);
    
    // Work out how many samples ("divisions") to take of the pattern space. Start with the pattern size itself,
    // but scale up by factors of two, until the number of divisions exceeds the number of pixels in the pattern space
    // on that side, or we reach the maximum sample size.
    xdiv2 = patt_size;
    ydiv2 = patt_size;
    if( imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE ) {
        while( xdiv2*xdiv2 < lxPatt && xdiv2 < sample_size ) xdiv2*=2; // i.e. while (xdiv2 < ((sqrt(lx1) * pattRatio)) xdiv2*=2;
        while( ydiv2*ydiv2 < lyPatt && ydiv2 < sample_size ) ydiv2*=2; // i.e. while (ydiv2 < ((sqrt(ly1) * pattRatio)) ydiv2*=2;
    }
    else {
        while( xdiv2*xdiv2*4 < lxPatt && xdiv2 < sample_size ) xdiv2*=2;
        while( ydiv2*ydiv2*4 < lyPatt && ydiv2 < sample_size ) ydiv2*=2;
    }
    if( xdiv2 > sample_size ) xdiv2 = sample_size;
    if( ydiv2 > sample_size ) ydiv2 = sample_size;

    xdiv = xdiv2/patt_size;
    ydiv = ydiv2/patt_size;
    pattRatio1 = (_1_0 - pattRatio)/_2_0 * _10_0; // borderSize * 10.0
    pattRatio2 = pattRatio * _10_0;

    if( pattDetectMode == AR_TEMPLATE_MATCHING_COLOR ) {
        arMallocClear( ext_patt2, ARUint32, patt_size*patt_size*3 );

        if( pixelFormat == AR_PIXEL_FORMAT_RGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*3+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*3+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*3+0];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_BGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*3+0];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*3+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*3+2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+0];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_BGRA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+0];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ABGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+1];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+3];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f || pixelFormat == AR_PIXEL_FORMAT_NV21 ) {
            // N.B.: caller asked for colour matching, but we can/will only supply mono.
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[yc*xsize+xc];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[yc*xsize+xc];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[yc*xsize+xc];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ARGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += image[(yc*xsize+xc)*4+3];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += image[(yc*xsize+xc)*4+2];
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += image[(yc*xsize+xc)*4+1];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_2vuy ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        float Cb =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 0] - 128); // Byte 0 of each 4-byte block for both even- and odd-numbered columns.
                        float Yprime = (float)(image[(yc*xsize +            xc)*2 + 1] - 16);  // Byte 1 of each 4-byte block for even-numbered columns, byte 3 for odd-numbered columns.
                        float Cr =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 2] - 128); // Byte 2 of each 4-byte block for both even- and odd-numbered columns.
						// Conversion from Poynton's color FAQ http://www.poynton.com.
                        int B0 = (int)(298.082f*Yprime + 516.411f*Cb              ) >> 8;
                        int G0 = (int)(298.082f*Yprime - 100.291f*Cb - 208.120f*Cr) >> 8;
                        int R0 = (int)(298.082f*Yprime               + 408.583f*Cr) >> 8;
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += CLAMP(B0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += CLAMP(G0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += CLAMP(R0, 0, 255);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_yuvs ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        float Yprime = (float)(image[(yc*xsize +            xc)*2 + 0] - 16);  // Byte 0 of each 4-byte block for even-numbered columns, byte 2 for odd-numbered columns.
                        float Cb =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 1] - 128); // Byte 1 of each 4-byte block for both even- and odd-numbered columns.
                        float Cr =     (float)(image[(yc*xsize + (xc & 0xFFFE))*2 + 3] - 128); // Byte 3 of each 4-byte block for both even- and odd-numbered columns.
						// Conversion from Poynton's color FAQ http://www.poynton.com.
                        int B0 = (int)(298.082f*Yprime + 516.411f*Cb              ) >> 8;
                        int G0 = (int)(298.082f*Yprime - 100.291f*Cb - 208.120f*Cr) >> 8;
                        int R0 = (int)(298.082f*Yprime               + 408.583f*Cr) >> 8;
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] += CLAMP(B0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += CLAMP(G0, 0, 255);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] += CLAMP(R0, 0, 255);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGB_565 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=                                            (((image[(yc*xsize+xc)*2+1] & 0x1f) << 3) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xe0) >> 3) + 0x02);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_5551 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=                                            (((image[(yc*xsize+xc)*2+1] & 0x3e) << 2) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xc0) >> 3) + 0x04);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04);
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_4444 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+0] +=  ((image[(yc*xsize+xc)*2+1] & 0xf0) + 0x08);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+1] += (((image[(yc*xsize+xc)*2+0] & 0x0f) << 4) + 0x08);
                        ext_patt2[((j/ydiv)*patt_size+(i/xdiv))*3+2] +=  ((image[(yc*xsize+xc)*2+0] & 0xf0) + 0x08);
                    }
                }
            }
        }
        else {
            ARLOGe("Error: unsupported pixel format.\n");
            goto bail;
        }

        for( i = 0; i < patt_size*patt_size*3; i++ ) {
            ext_patt[i] = ext_patt2[i] / (xdiv*ydiv);
        }

        free( ext_patt2 );
    }
    else { // !AR_TEMPLATE_MATCHING_COLOR
        arMallocClear( ext_patt2, ARUint32, patt_size*patt_size );

        if( pixelFormat == AR_PIXEL_FORMAT_RGB || pixelFormat == AR_PIXEL_FORMAT_BGR ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*3+0]
                                  + image[(yc*xsize+xc)*3+1]
                                  + image[(yc*xsize+xc)*3+2] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA || pixelFormat == AR_PIXEL_FORMAT_BGRA ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*4+0]
                                  + image[(yc*xsize+xc)*4+1]
                                  + image[(yc*xsize+xc)*4+2] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_ABGR || pixelFormat == AR_PIXEL_FORMAT_ARGB ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                             += (   image[(yc*xsize+xc)*4+1]
                                  + image[(yc*xsize+xc)*4+2]
                                  + image[(yc*xsize+xc)*4+3] )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f || pixelFormat == AR_PIXEL_FORMAT_NV21 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[yc*xsize+xc];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_2vuy ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[(yc*xsize+xc)*2+1];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_yuvs ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)] += image[(yc*xsize+xc)*2];
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGB_565 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (   ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04)
                            + (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xe0) >> 3) + 0x02)
                            + (((image[(yc*xsize+xc)*2+1] & 0x1f) << 3) + 0x04) )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_5551 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (    ((image[(yc*xsize+xc)*2+0] & 0xf8) + 0x04)
                            + (((image[(yc*xsize+xc)*2+0] & 0x07) << 5) + ((image[(yc*xsize+xc)*2+1] & 0xc0) >> 3) + 0x04)
                            + (((image[(yc*xsize+xc)*2+1] & 0x3e) << 2) + 0x04) )/3;
                    }
                }
            }
        }
        else if( pixelFormat == AR_PIXEL_FORMAT_RGBA_4444 ) {
            for( j = 0; j < ydiv2; j++ ) {
                yw = (_100_0+pattRatio1) + pattRatio2 * (j+_0_5) / (ARdouble)ydiv2;
                for( i = 0; i < xdiv2; i++ ) {
                    xw = (_100_0+pattRatio1) + pattRatio2 * (i+_0_5) / (ARdouble)xdiv2;
                    d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                    if( d == 0 ) goto bail;
                    xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                    yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                    arParamIdeal2ObservLTf( paramLTf, xc2, yc2, &xc2, &yc2 );
                    //arParamIdeal2Observ( dist_factor, xc2, yc2, &xc2, &yc2, dist_function_version );
                    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
                        xc = ((int)(xc2+1.0f)/2)*2;
                        yc = ((int)(yc2+1.0f)/2)*2;
                    }
                    else {
                        xc = (int)(xc2+0.5f);
                        yc = (int)(yc2+0.5f);
                    }
                    if( xc >= 0 && xc < xsize && yc >= 0 && yc < ysize ) {
                        ext_patt2[(j/ydiv)*patt_size+(i/xdiv)]
                        += (    ((image[(yc*xsize+xc)*2+0] & 0xf0) + 0x08)
                            + (((image[(yc*xsize+xc)*2+0] & 0x0f) << 4) + 0x08)
                            +  ((image[(yc*xsize+xc)*2+1] & 0xf0) + 0x08) )/3;
                    }
                }
            }
        }
        else {
            ARLOGe("Error: unsupported pixel format.\n");
            goto bail;
        }

        for( i = 0; i < patt_size*patt_size; i++ ) {
            ext_patt[i] = ext_patt2[i] / (xdiv*ydiv);
        }

        free( ext_patt2 );
    }

    return 0;
    
bail:
    free(ext_patt2);
    return -1;
}

int arPattGetImage3( ARHandle *arHandle, int markerNo, ARUint8 *image, ARPattRectInfo *rect, int xsize, int ysize,
                     int overSampleScale, ARUint8 *outImage )
{
    ARUint32 *tempImage;
    ARdouble  world[4][2];
    ARdouble  local[4][2];
    ARdouble  para[3][3];
    int       xdiv, ydiv;
    int       xdiv2, ydiv2;
    ARdouble  d, xw, yw;
    float     xc2, yc2;
    int       xc, yc;
    int       xsize2, ysize2;
    int       i, j;

    xsize2 = arHandle->xsize;
    ysize2 = arHandle->ysize;
    world[0][0] = _100_0;
    world[0][1] = _100_0;
    world[1][0] = _100_0 + _10_0;
    world[1][1] = _100_0;
    world[2][0] = _100_0 + _10_0;
    world[2][1] = _100_0 + _10_0;
    world[3][0] = _100_0;
    world[3][1] = _100_0 + _10_0;
    for( i = 0; i < 4; i++ ) {
        local[i][0] = arHandle->markerInfo[markerNo].vertex[i][0];
        local[i][1] = arHandle->markerInfo[markerNo].vertex[i][1];
    }
    get_cpara( world, local, para );

    xdiv = overSampleScale;
    ydiv = overSampleScale;
    xdiv2 = xsize*xdiv;
    ydiv2 = ysize*xdiv;

    if( arHandle->arPixelFormat == AR_PIXEL_FORMAT_RGB
     || arHandle->arPixelFormat == AR_PIXEL_FORMAT_BGR ) {
        arMallocClear(tempImage, ARUint32, xsize*ysize*3);
        for( j = 0; j < ydiv2; j++ ) {
            yw = _100_0 + _10_0 * (rect->topLeftY + (rect->bottomRightY - rect->topLeftY) * (j+_0_5) / (float)ydiv2);
            for( i = 0; i < xdiv2; i++ ) {
                xw = _100_0 + _10_0 * (rect->topLeftX + (rect->bottomRightX - rect->topLeftX) * (i+_0_5) / (float)xdiv2);
                d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                if( d == 0 ) { free(tempImage); return -1; }
                xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                arParamIdeal2ObservLTf( &(arHandle->arParamLT->paramLTf), xc2, yc2, &xc2, &yc2 );
                //arParamIdeal2Observ( arHandle->arParam.dist_factor, xc2, yc2, &xc2, &yc2, arHandle->arParam.dist_function_version );
                xc = (int)(xc2+0.5f);
                yc = (int)(yc2+0.5f);
                if( xc < 0 || xc >= xsize2 || yc < 0 || yc >= ysize2 ) { free(tempImage); return -1; }
                tempImage[((j/ydiv)*xsize+(i/xdiv))*3+0] += image[(yc*xsize2+xc)*3+0];
                tempImage[((j/ydiv)*xsize+(i/xdiv))*3+1] += image[(yc*xsize2+xc)*3+1];
                tempImage[((j/ydiv)*xsize+(i/xdiv))*3+2] += image[(yc*xsize2+xc)*3+2];
            }
        }
        for( i = 0; i < xsize*ysize*3; i++ ) {
            outImage[i] = tempImage[i] / (xdiv*ydiv);
        }
    }
    else if( arHandle->arPixelFormat == AR_PIXEL_FORMAT_RGBA // Assume alpha is 255.
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_BGRA
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_ARGB
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_ABGR ) {
        arMallocClear(tempImage, ARUint32, xsize*ysize*4);
        for( j = 0; j < ydiv2; j++ ) {
            yw = _100_0 + _10_0 * (rect->topLeftY + (rect->bottomRightY - rect->topLeftY) * (j+_0_5) / (float)ydiv2);
            for( i = 0; i < xdiv2; i++ ) {
                xw = _100_0 + _10_0 * (rect->topLeftX + (rect->bottomRightX - rect->topLeftX) * (i+_0_5) / (float)xdiv2);
                d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                if( d == 0 ) { free(tempImage); return -1; }
                xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                arParamIdeal2ObservLTf( &(arHandle->arParamLT->paramLTf), xc2, yc2, &xc2, &yc2 );
                //arParamIdeal2Observ( arHandle->arParam.dist_factor, xc2, yc2, &xc2, &yc2, arHandle->arParam.dist_function_version );
                xc = (int)(xc2+0.5f);
                yc = (int)(yc2+0.5f);
                if( xc < 0 || xc >= xsize2 || yc < 0 || yc >= ysize2 ) { free(tempImage); return -1; }
                tempImage[((j/ydiv)*xsize+(i/xdiv))*4+0] += image[(yc*xsize2+xc)*4+0];
                tempImage[((j/ydiv)*xsize+(i/xdiv))*4+1] += image[(yc*xsize2+xc)*4+1];
                tempImage[((j/ydiv)*xsize+(i/xdiv))*4+2] += image[(yc*xsize2+xc)*4+2];
                tempImage[((j/ydiv)*xsize+(i/xdiv))*4+3] += image[(yc*xsize2+xc)*4+3];
            }
        }
        for( i = 0; i < xsize*ysize*4; i++ ) {
            outImage[i] = tempImage[i] / (xdiv*ydiv);
        }
    }
    else if( arHandle->arPixelFormat == AR_PIXEL_FORMAT_MONO
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_420v
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_420f
          || arHandle->arPixelFormat == AR_PIXEL_FORMAT_NV21 ) {
        arMallocClear(tempImage, ARUint32, xsize*ysize);
        for( j = 0; j < ydiv2; j++ ) {
            yw = _100_0 + _10_0 * (rect->topLeftY + (rect->bottomRightY - rect->topLeftY) * (j+_0_5) / (float)ydiv2);
            for( i = 0; i < xdiv2; i++ ) {
                xw = _100_0 + _10_0 * (rect->topLeftX + (rect->bottomRightX - rect->topLeftX) * (i+_0_5) / (float)xdiv2);
                d = para[2][0]*xw + para[2][1]*yw + para[2][2];
                if( d == 0 ) { free(tempImage); return -1; }
                xc2 = (float)((para[0][0]*xw + para[0][1]*yw + para[0][2])/d);
                yc2 = (float)((para[1][0]*xw + para[1][1]*yw + para[1][2])/d);
                arParamIdeal2ObservLTf( &(arHandle->arParamLT->paramLTf), xc2, yc2, &xc2, &yc2 );
                //arParamIdeal2Observ( arHandle->arParam.dist_factor, xc2, yc2, &xc2, &yc2, arHandle->arParam.dist_function_version );
                xc = (int)(xc2+0.5f);
                yc = (int)(yc2+0.5f);
                if( xc < 0 || xc >= xsize2 || yc < 0 || yc >= ysize2 ) { free(tempImage); return -1; }
                tempImage[(j/ydiv)*xsize+(i/xdiv)] += image[yc*xsize2+xc];
            }
        }
        for( i = 0; i < xsize*ysize; i++ ) {
            outImage[i] = tempImage[i] / (xdiv*ydiv);
        }
    }
    else exit(0);

    free( tempImage );

    return 0;
}

static void get_cpara( ARdouble world[4][2], ARdouble vertex[4][2],
                       ARdouble para[3][3] )
{
    ARMat   *a, *b, *c;
    int     i;

    a = arMatrixAlloc( 8, 8 );
    b = arMatrixAlloc( 8, 1 );
    c = arMatrixAlloc( 8, 1 );
    for( i = 0; i < 4; i++ ) {
        a->m[i*16+0]  = world[i][0];
        a->m[i*16+1]  = world[i][1];
        a->m[i*16+2]  = _1_0;
        a->m[i*16+3]  = _0_0;
        a->m[i*16+4]  = _0_0;
        a->m[i*16+5]  = _0_0;
        a->m[i*16+6]  = -world[i][0] * vertex[i][0];
        a->m[i*16+7]  = -world[i][1] * vertex[i][0];
        a->m[i*16+8]  = _0_0;
        a->m[i*16+9]  = _0_0;
        a->m[i*16+10] = _0_0;
        a->m[i*16+11] = world[i][0];
        a->m[i*16+12] = world[i][1];
        a->m[i*16+13] = _1_0;
        a->m[i*16+14] = -world[i][0] * vertex[i][1];
        a->m[i*16+15] = -world[i][1] * vertex[i][1];
        b->m[i*2+0] = vertex[i][0];
        b->m[i*2+1] = vertex[i][1];
    }
    arMatrixSelfInv( a );
    arMatrixMul( c, a, b );
    for( i = 0; i < 2; i++ ) {
        para[i][0] = c->m[i*3+0];
        para[i][1] = c->m[i*3+1];
        para[i][2] = c->m[i*3+2];
    }
    para[2][0] = c->m[2*3+0];
    para[2][1] = c->m[2*3+1];
    para[2][2] = _1_0;
    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
}

static int pattern_match( ARPattHandle *pattHandle, int mode, ARUint8 *data, int size, int *code, int *dir, ARdouble *cf )
{
    int   *input = NULL;
    int    sum, ave;
    int    res1, res2;
    int    i, j, k, l;
    ARdouble datapow;
    ARdouble sum2, max;

    if ( pattHandle == NULL || 0 >= size ) {
        *code = 0;
        *dir  = 0;
        *cf   = -_1_0;
        return -1;
    }

    if ( mode == AR_TEMPLATE_MATCHING_COLOR ) {
        const int SIZE_SQD_X3 = size*size*3;
        
        arMalloc( input, int, SIZE_SQD_X3 );
        if ( NULL == input ) {
            return -1;
        }

        sum = ave = 0;
        for (i=0; i < SIZE_SQD_X3; i++) {
            ave += (255-data[i]);
        }
        ave /= (SIZE_SQD_X3);

        for (i=0; i < SIZE_SQD_X3; i++) {
            input[i] = (255-data[i]) - ave;
            sum += input[i]*input[i];
        }

        datapow = SQRT( (ARdouble)sum );
        //if( datapow == 0.0 ) {
        if ( datapow/(size*SQRT_3_0) < AR_PATT_CONTRAST_THRESH1 ) {
            *code = 0;
            *dir  = 0;
            *cf   = -_1_0;
            free( input );
            return -2; // Insufficient contrast.
        }

        res1 = res2 = -1;
        k = -1; // Best match in search space.
        max = _0_0;
        for ( l = 0; l < pattHandle->patt_num; l++ ) { // Consider the whole search space.
            k++;
            while( pattHandle->pattf[k] == 0 ) k++; // No pattern at this slot.
            if( pattHandle->pattf[k] == 2 ) continue; // Pattern at this slot is deactivated.
            for( j = 0; j < 4; j++ ) { // The 4 rotated variants of the pattern.
                sum = 0;
                for(i=0; i < SIZE_SQD_X3; i++)
                    sum += input[i] * pattHandle->patt[k * 4 + j][i]; // Correlation operation.
                sum2 = sum / pattHandle->pattpow[k*4 + j] / datapow;
                if( sum2 > max ) { max = sum2; res1 = j; res2 = k; }
            }
        }
        *dir  = res1;
        *code = res2;
        *cf   = max;

        free( input );
        return 0;
    }
    else if ( mode == AR_TEMPLATE_MATCHING_MONO ) {
        const int SIZE_SQD = size*size;

        arMalloc( input, int, SIZE_SQD );
        if ( NULL == input ) {
            return -1;
        }
        
        sum = ave = 0;
        for ( i=0; i < SIZE_SQD; i++ ) {
            ave += (255-data[i]);
        }
        ave /= (SIZE_SQD);

        for ( i=0; i < SIZE_SQD; i++ ) {
            input[i] = (255-data[i]) - ave;
            sum += input[i] * input[i];
        }

        datapow = SQRT( (ARdouble)sum );
        //if( datapow == 0.0 ) {
        if ( datapow/size < AR_PATT_CONTRAST_THRESH1 ) {
            *code = 0;
            *dir  = 0;
            *cf   = -_1_0;
            free( input );
            return -2; // Insufficient contrast.
        }

        res1 = res2 = -1;
        k = -1;
        max = _0_0;
        for ( l = 0; l < pattHandle->patt_num; l++ ) {
            k++;
            while ( pattHandle->pattf[k] == 0 ) k++;
            if ( pattHandle->pattf[k] == 2 ) continue;
            for ( j = 0; j < 4; j++ ) {
                sum = 0;
                for ( i=0; i < SIZE_SQD; i++ )
                    sum += input[i] * (pattHandle->pattBW[k * 4 + j][i]);
                sum2 = sum / pattHandle->pattpowBW[k * 4 + j] / datapow;
                if ( sum2 > max ) { max = sum2; res1 = j; res2 = k; }
            }
        }
        *dir  = res1;
        *code = res2;
        *cf   = max;

        free( input );
        return 0;
    }
    else {
        return -1;
    }
}

static int decode_bch(const AR_MATRIX_CODE_TYPE matrixCodeType, const uint64_t in, uint8_t recd127[127], uint64_t *out_p)
{
    uint64_t in_bitwise;
    uint8_t *recd;
    uint64_t out_bit;
    int t, n, length, k;
    uint8_t recd64[64];
    const int *alpha_to, *index_of;
    const int bch_15_alpha_to[15] = {1, 2, 4, 8, 3, 6, 12, 11, 5, 10, 7, 14, 15, 13, 9};
    const int bch_15_index_of[16] = {-1, 0, 1, 4, 2, 8, 5, 10, 3, 14, 9, 7, 6, 13, 11, 12};
    const int bch_31_alpha_to[31] = {1, 2, 4, 8, 16, 5, 10, 20, 13, 26, 17, 7, 14, 28, 29, 31, 27, 19, 3, 6, 12, 24, 21, 15, 30, 25, 23, 11, 22, 9, 18};
    const int bch_31_index_of[32] = {-1, 0, 1, 18, 2, 5, 19, 11, 3, 29, 6, 27, 20, 8, 12, 23, 4, 10, 30, 17, 7, 22, 28, 26, 21, 25, 9, 16, 13, 14, 24, 15};
    const int bch_127_alpha_to[127] = {1, 2, 4, 8, 16, 32, 64, 3, 6, 12, 24, 48, 96, 67, 5, 10, 20, 40, 80, 35, 70, 15, 30, 60, 120, 115, 101, 73, 17, 34, 68, 11, 22, 44, 88, 51, 102, 79, 29, 58, 116, 107, 85, 41, 82, 39, 78, 31, 62, 124, 123, 117, 105, 81, 33, 66, 7, 14, 28, 56, 112, 99, 69, 9, 18, 36, 72, 19, 38, 76, 27, 54, 108, 91, 53, 106, 87, 45, 90, 55, 110, 95, 61, 122, 119, 109, 89, 49, 98, 71, 13, 26, 52, 104, 83, 37, 74, 23, 46, 92, 59, 118, 111, 93, 57, 114, 103, 77, 25, 50, 100, 75, 21, 42, 84, 43, 86, 47, 94, 63, 126, 127, 125, 121, 113, 97, 65};
    const int bch_127_index_of[128] = {-1, 0, 1, 7, 2, 14, 8, 56, 3, 63, 15, 31, 9, 90, 57, 21, 4, 28, 64, 67, 16, 112, 32, 97, 10, 108, 91, 70, 58, 38, 22, 47, 5, 54, 29, 19, 65, 95, 68, 45, 17, 43, 113, 115, 33, 77, 98, 117, 11, 87, 109, 35, 92, 74, 71, 79, 59, 104, 39, 100, 23, 82, 48, 119, 6, 126, 55, 13, 30, 62, 20, 89, 66, 27, 96, 111, 69, 107, 46, 37, 18, 53, 44, 94, 114, 42, 116, 76, 34, 86, 78, 73, 99, 103, 118, 81, 12, 125, 88, 61, 110, 26, 36, 106, 93, 52, 75, 41, 72, 85, 80, 102, 60, 124, 105, 25, 40, 51, 101, 84, 24, 123, 83, 50, 49, 122, 120, 121};
    int i, j, u, q, t2, count = 0, syn_error = 0;
	int elp[20][18], d[20], l[20], u_lu[20], s[19], loc[127], reg[10]; // int elp[t2 + 2, t2], d[t2 + 2], l[t2 + 2], u_lu[t2 + 2], s[t2 + 1], loc[n], reg[t + 1].
    
    if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_9_3 || matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_5_5 || matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_12_5 || matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_7_7) {
        if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_9_3 || matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_5_5) {
            if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_9_3) {
                t = 1; k = 9;
            } else { // matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_5_5
                t = 2; k = 5;
            }
            n = 15;
            length = 13;
            alpha_to = bch_15_alpha_to;
            index_of = bch_15_index_of;
        } else { // matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_12_5 || matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_7_7
            if (matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_12_5) {
                t = 2; k = 12;
            } else { // matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_7_7
                t = 3; k = 7;
            }
            n = 31;
            length = 22;
            alpha_to = bch_31_alpha_to;
            index_of = bch_31_index_of;
        }
        // Unpack input into recd64[]. recd64[0] is least significant bit.
        in_bitwise = in;
        for (i = 0; i < length; i++) {
            recd64[i] = (uint8_t)(in_bitwise & 1);
            in_bitwise = in_bitwise >> 1;
        }
        recd = recd64;
    } else if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID) {
        t = 9; k = 64;
        n = 127;
        length = 120;
        alpha_to = bch_127_alpha_to;
        index_of = bch_127_index_of;
        recd = recd127;
    } else {
#ifdef DEBUG_BCH
        ARLOGe("Error: unsupported BCH code.\n");
#endif
        return (-1); // Unsupported code.
    }
    
    
    /*
     * Simon Rockliff's implementation of Berlekamp's algorithm.
     * Copyright (c) 1994-7,  Robert Morelos-Zaragoza. All rights reserved.
     *
     * Assume we have received bits in recd[i], i=0..(n-1).
     *
     * Compute the 2*t syndromes by substituting alpha^i into rec(X) and
     * evaluating, storing the syndromes in s[i], i=1..2t (leave s[0] zero) .
     * Then we use the Berlekamp algorithm to find the error location polynomial
     * elp[i].
     *
     * If the degree of the elp is >t, then we cannot correct all the errors, and
     * we have detected an uncorrectable error pattern. We output the information
     * bits uncorrected.
     *
     * If the degree of elp is <=t, we substitute alpha^i , i=1..n into the elp
     * to get the roots, hence the inverse roots, the error location numbers.
     * This step is usually called "Chien's search".
     *
     * If the number of errors located is not equal the degree of the elp, then
     * the decoder assumes that there are more than t errors and cannot correct
     * them, only detect them. We output the information bits uncorrected.
     *
     * t = error correcting capability (max. no. of errors the code corrects)
     * length = length of the BCH code
     * n = 2**m - 1 = size of the multiplicative group of GF(2**m)
     * alpha_to [] = log table of GF(2**m) 
     * index_of[] = antilog table of GF(2**m)
     * recd[] = coefficients of the received polynomial 
     */
	t2 = 2 * t;
    
	/* first form the syndromes */
	for (i = 1; i <= t2; i++) {
		s[i] = 0;
		for (j = 0; j < length; j++) {
			if (recd[j] != 0) s[i] ^= alpha_to[(i * j) % n];
        }
		if (s[i] != 0) syn_error = 1; /* set error flag if non-zero syndrome */
		s[i] = index_of[s[i]]; /* convert syndrome from polynomial form to index form  */
	}
    
	if (syn_error) {	/* if there are errors, try to correct them */
		/*
		 * Compute the error location polynomial via the Berlekamp
		 * iterative algorithm. Following the terminology of Lin and
		 * Costello's book :   d[u] is the 'mu'th discrepancy, where
		 * u='mu'+1 and 'mu' (the Greek letter!) is the step number
		 * ranging from -1 to 2*t (see L&C),  l[u] is the degree of
		 * the elp at that step, and u_l[u] is the difference between
		 * the step number and the degree of the elp. 
		 */
		/* initialise table entries */
		d[0] = 0;			/* index form */
		d[1] = s[1];		/* index form */
		elp[0][0] = 0;		/* index form */
		elp[1][0] = 1;		/* polynomial form */
		for (i = 1; i < t2; i++) {
			elp[0][i] = -1;	/* index form */
			elp[1][i] = 0;	/* polynomial form */
		}
		l[0] = 0;
		l[1] = 0;
		u_lu[0] = -1;
		u_lu[1] = 0;
		u = 0;
        
		do {
			u++;
			if (d[u] == -1) {
				l[u + 1] = l[u];
				for (i = 0; i <= l[u]; i++) {
					elp[u + 1][i] = elp[u][i];
					elp[u][i] = index_of[elp[u][i]]; /* put elp into index form  */
				}
			} else {
                /*
                 * search for words with greatest u_lu[q] for
                 * which d[q]!=0 
                 */
				q = u - 1;
				while ((d[q] == -1) && (q > 0)) q--;
				/* have found first non-zero d[q]  */
				if (q > 0) {
                    j = q;
                    do {
                        j--;
                        if ((d[j] != -1) && (u_lu[q] < u_lu[j]))
                            q = j;
                    } while (j > 0);
				}
                
				/*
				 * have now found q such that d[u]!=0 and
				 * u_lu[q] is maximum 
				 */
				/* store degree of new elp polynomial */
				if (l[u] > l[q] + u - q) l[u + 1] = l[u];
				else l[u + 1] = l[q] + u - q;
                
				/* form new elp(x) */
				for (i = 0; i < t2; i++) elp[u + 1][i] = 0;
				for (i = 0; i <= l[q]; i++) {
					if (elp[q][i] != -1) elp[u + 1][i + u - q] = alpha_to[(d[u] + n - d[q] + elp[q][i]) % n];
                }
				for (i = 0; i <= l[u]; i++) {
					elp[u + 1][i] ^= elp[u][i];
					elp[u][i] = index_of[elp[u][i]]; /* put elp into index form  */
				}
			}
			u_lu[u + 1] = u - l[u + 1];
            
			/* form (u+1)th discrepancy */
			if (u < t2) {	
                /* no discrepancy computed on last iteration */
                if (s[u + 1] != -1) d[u + 1] = alpha_to[s[u + 1]];
                else d[u + 1] = 0;
			    for (i = 1; i <= l[u + 1]; i++) {
                    if ((s[u + 1 - i] != -1) && (elp[u + 1][i] != 0)) d[u + 1] ^= alpha_to[(s[u + 1 - i] + index_of[elp[u + 1][i]]) % n];
                }
                d[u + 1] = index_of[d[u + 1]]; /* put d[u+1] into index form */
			}
		} while ((u < t2) && (l[u + 1] <= t));
        
		u++;
		if (l[u] <= t) { /* Can correct errors */
			for (i = 0; i <= l[u]; i++) elp[u][i] = index_of[elp[u][i]]; /* put elp into index form */
            
			/* Chien search: find roots of the error location polynomial */
			for (i = 1; i <= l[u]; i++) reg[i] = elp[u][i];
			count = 0;
			for (i = 1; i <= n; i++) {
				q = 1;
				for (j = 1; j <= l[u]; j++) {
 					if (reg[j] != -1) {
						reg[j] = (reg[j] + j) % n;
						q ^= alpha_to[reg[j]];
					}
                }
				if (!q) {	/* store root and error
                             * location number indices */
					loc[count] = n - i; /* root[count] = i; */
					count++;
				}
			}

			if (count == l[u]){
                /* no. roots = degree of elp hence <= t errors */
				for (i = 0; i < l[u]; i++) recd[loc[i]] ^= 1;
            } else	{
                /* elp has degree >t hence cannot solve */
#ifdef DEBUG_BCH
                ARLOGe("count != l[u].\n");
#endif
                return (-1);
            }
		} else {
#ifdef DEBUG_BCH
            ARLOGe("l[u] > t.\n");
#endif
            return (-1);
        }
	} // End syn_error.
    
    // Pack the result into *out_p. Data bits begin with LSB at recd[length - k] through to MSB at recd[length - 1];
    *out_p = 0LL;
    out_bit = 1LL;
    for (i = length - k; i < length; i++) {
        *out_p += (uint64_t)recd[i] * out_bit;
        out_bit <<= 1;
    }
    
    if (syn_error) return (l[u]);
    else return (0);
}

//const signed char hamming63EncoderTable[8] = {0, 7, 25, 30, 42, 45, 51, 52};
const signed char hamming63DecoderTable[64] = {
    0, 0, 0, 1, 0, 1, 1, 1, 0, 2, 4, -1, -1, 5, 3, 1,
    0, 2, -1, 6, 7, -1, 3, 1, 2, 2, 3, 2, 3, 2, 3, 3,
    0, -1, 4, 6, 7, 5, -1, 1, 4, 5, 4, 4, 5, 5, 4, 5,
    7, 6, 6, 6, 7, 7, 7, 6, -1, 2, 4, 6, 7, 5, 3, -1};
const bool hamming63DecoderTableErrorCorrected[64] = {
    0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1,
    1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1,
    1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1,
    1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0};
//const signed char parity65EncoderTable[32] = {0, 33, 34, 3, 36, 5, 6, 39, 40, 9, 10, 43, 12, 45, 46, 15, 48, 17, 18, 51, 20, 53, 54, 23, 24, 57, 58, 27, 60, 29, 30, 63};
const signed char parity65DecoderTable[64] = {
    0, -1, -1, 3, -1, 5, 6, -1, -1, 9, 10, -1, 12, -1, -1, 15,
    -1, 17, 18, -1, 20, -1, -1, 23, 24, -1, -1, 27, -1, 29, 30, -1,
    -1, 1, 2, -1, 4, -1, -1, 7, 8, -1, -1, 11, -1, 13, 14, -1,
    16, -1, -1, 19, -1, 21, 22, -1, -1, 25, 26, -1, 28, -1, -1, 31};

/*-------------------------
   dir=0   1--
           ---
           1-0

   dir=1   --0
           ---
           1-1

   dir=2   0-1
           ---
           --1

   dir=3   1-1
           ---
           0--
---------------------------*/
static int get_matrix_code( ARUint8 *data, int size, int *code_out_p, int *dir, ARdouble *cf, const AR_MATRIX_CODE_TYPE matrixCodeType, int *errorCorrected )
{
    ARUint8  max, min, thresh;
    ARUint8  dirCode[4];
    int      corner[4];
    int      contrast, contrastMin;
    int      i, j, ret;
    uint64_t code, codeRaw;

    if( size < 3 || size > 8 ) {
        *code_out_p = -1;
        *dir  = 0;
        *cf   = -_1_0;
        return -1;
    }

	// Look at corners of unwarped marker pattern space to work out threshhold.
    corner[0] = 0;
    corner[1] = (size - 1)*size;
    corner[2] = size*size - 1;
    corner[3] = size - 1;
    max = 0;
    min = 255;
    for( i = 0; i < 4; i++ ) {
        if( data[corner[i]] > max ) max = data[corner[i]];
        if( data[corner[i]] < min ) min = data[corner[i]];
    }
    if( max - min < AR_PATT_CONTRAST_THRESH2 ) {
        *code_out_p = -1;
        *dir  = 0;
        *cf   = -_1_0;
        return -2; // Insufficient contrast.
    }
    thresh = (max + min)/2;
#if DEBUG_PATT_GETID
    ARLOGd("max=%d, min=%d, thresh=%d  ",max, min, thresh);
#endif

	// Look at corners to work out which direction pattern is facing.
	// An unrotated pattern has 1 in top-left corner, 1 in bottom-left corner,
	// and 0 in bottom-right corner, where 1 is a pixel less than the threshhold
	// (i.e. black).
    for( i = 0; i < 4; i++ ) dirCode[i] = (data[corner[i]] < thresh) ? 1 : 0;
    for( i = 0; i < 4; i++ ) {
        if( dirCode[i] == 1 && dirCode[(i+1)%4] == 1 && dirCode[(i+2)%4] == 0 ) {
            *dir = i;
            break;
        }
    }
    if( i == 4 ) { // Barcode locator pattern not found.
        *code_out_p = -1;
        *dir  = 0;
        *cf   = -_1_0;
        return -3; // Bad barcode.
    }

	// Binarize the unwarped marker pattern space.
	// Record the minimum observed contrast for use as a confidence measure.
    contrastMin = 255;
    for( i = 0; i < size*size; i++ ) {
#if DEBUG_PATT_GETID
    ARLOGd("%3d ", data[i]);
#endif
        contrast = data[i] - thresh;
        if( contrast < 0 ) contrast = -contrast;
        if( contrast < contrastMin ) contrastMin = contrast;
        data[i] = (data[i] < thresh)? 1: 0;
    }
#if DEBUG_PATT_GETID
    ARLOGd("\n");
#endif

	// Calculate the matrix code.
	// The three pixels forming the corners (used to determine which direction
    // the marker is facing) are ignored.
    codeRaw = 0LL;
    if( *dir == 0 ) {
#if DEBUG_PATT_GETID
        ARLOGd("DIR:%d  ", *dir);
#endif
        for( j = 0; j < size; j++ ) {
            for( i = 0; i < size; i++ ) {
                if( i == 0      && j == 0      ) continue;
                if( i == 0      && j == size-1 ) continue;
                if( i == size-1 && j == size-1 ) continue;
                codeRaw <<= 1;
                if( data[j*size + i] ) codeRaw++;
#if DEBUG_PATT_GETID
                ARLOGd("%2d ", data[j*size+i]);
#endif
            }
        }
#if DEBUG_PATT_GETID
        ARLOGd("\n");
#endif
    } else if( *dir == 1 ) {
#if DEBUG_PATT_GETID
        ARLOGd("DIR:%d  ", *dir);
#endif
        for( i = 0; i < size; i++ ) {
            for( j = size-1; j >= 0; j-- ) {
                if( i == 0      && j == size-1 ) continue;
                if( i == size-1 && j == size-1 ) continue;
                if( i == size-1 && j == 0      ) continue;
                codeRaw <<= 1;
                if( data[j*size+i] ) codeRaw++;
#if DEBUG_PATT_GETID
                ARLOGd("%2d ", data[j*size+i]);
#endif
            }
        }
#if DEBUG_PATT_GETID
        ARLOGd("\n");
#endif
    } else if( *dir == 2 ) {
#if DEBUG_PATT_GETID
        ARLOGd("DIR:%d  ", *dir);
#endif
        for( j = size-1; j >= 0; j-- ) {
            for( i = size-1; i >= 0; i-- ) {
                if( i == size-1 && j == size-1 ) continue;
                if( i == size-1 && j == 0      ) continue;
                if( i == 0      && j == 0      ) continue;
                codeRaw <<= 1;
                if( data[j*size+i] ) codeRaw++;
#if DEBUG_PATT_GETID
                ARLOGd("%2d ", data[j*size+i]);
#endif
            }
        }
#if DEBUG_PATT_GETID
        ARLOGd("\n");
#endif
    } else if( *dir == 3 ) {
#if DEBUG_PATT_GETID
        ARLOGd("DIR:%d  ", *dir);
#endif
        for( i = size-1; i >= 0; i-- ) {
            for( j = 0; j < size; j++ ) {
                if( i == size-1 && j == 0      ) continue;
                if( i == 0      && j == 0      ) continue;
                if( i == 0      && j == size-1 ) continue;
                codeRaw <<= 1;
                if( data[j*size+i] ) codeRaw++;
#if DEBUG_PATT_GETID
                ARLOGd("%2d ", data[j*size+i]);
#endif
            }
        }
#if DEBUG_PATT_GETID
        ARLOGd("\n");
#endif
    }
    
#if DEBUG_PATT_GETID
    ARLOGd("Contrast = %d\n", contrastMin);
#endif
    *cf = (contrastMin > 30)? _1_0: (ARdouble)contrastMin/_30_0;

    if (matrixCodeType == AR_MATRIX_CODE_3x3_PARITY65) {
        code = parity65DecoderTable[codeRaw];
        if (parity65DecoderTable[codeRaw] < 0) {
            *code_out_p = -1;
            *cf = -_1_0;
            return (-4); // EDC fail.
        } 
    } else if (matrixCodeType == AR_MATRIX_CODE_3x3_HAMMING63) {
        code = hamming63DecoderTable[codeRaw];
        if (errorCorrected) *errorCorrected = hamming63DecoderTableErrorCorrected[codeRaw];
        if (hamming63DecoderTable[codeRaw] < 0) {
            *code_out_p = -1;
            *cf = -_1_0;
            return (-4); // EDC fail.
        }
    } else if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_9_3 || matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_5_5
               || matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_12_5 || matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_7_7) {
        ret = decode_bch(matrixCodeType, codeRaw, NULL, &code);
        if (ret < 0) {
            *code_out_p = -1;
            *cf = -_1_0;
            return (-4); // EDC fail.
        } else if (ret > 0) {
            if (errorCorrected) *errorCorrected = ret;
        }
    } else {
        code = codeRaw;
    }
    
    *code_out_p = (int)code;
    return 0;
}

static int get_global_id_code( ARUint8 *data, uint64_t *code_out_p, int *dir_p, ARdouble *cf, int *errorCorrected )
{
    ARUint8  max, min, thresh;
    ARUint8  dirCode[4];
    int      dir;
    int      corner[4];
    int      contrast, contrastMin;
    int      i, j, ret, bit;
    uint64_t code;
    uint8_t  recd127[127];
    
	// Look at corners of unwarped marker pattern space to work out threshhold.
    corner[0] = 0;
    corner[1] = (AR_GLOBAL_ID_OUTER_SIZE - 1)*AR_GLOBAL_ID_OUTER_SIZE;
    corner[2] = AR_GLOBAL_ID_OUTER_SIZE*AR_GLOBAL_ID_OUTER_SIZE - 1;
    corner[3] = AR_GLOBAL_ID_OUTER_SIZE - 1;
    max = 0;
    min = 255;
    for( i = 0; i < 4; i++ ) {
        if( data[corner[i]] > max ) max = data[corner[i]];
        if( data[corner[i]] < min ) min = data[corner[i]];
    }
    if( max - min < AR_PATT_CONTRAST_THRESH2 ) {
        *dir_p  = 0;
        *cf   = -_1_0;
        return -2; // Insufficient contrast.
    }
    thresh = (max + min)/2;
#if DEBUG_PATT_GETID
    ARLOGd("max=%d, min=%d, thresh=%d  ", max, min, thresh);
#endif
    
	// Look at corners to work out which direction pattern is facing.
	// An unrotated pattern has 1 in top-left corner, 1 in bottom-left corner,
	// and 0 in bottom-right corner, where 1 is a pixel less than the threshhold
	// (i.e. black).
    for( i = 0; i < 4; i++ ) dirCode[i] = (data[corner[i]] < thresh) ? 1 : 0;
    for( i = 0; i < 4; i++ ) {
        if( dirCode[i] == 1 && dirCode[(i+1)%4] == 1 && dirCode[(i+2)%4] == 0 ) {
            dir = i;
            break;
        }
    }
    if( i == 4 ) { // Barcode locator pattern not found.
        *dir_p  = 0;
        *cf   = -_1_0;
        return -3; // Bad barcode.
    }
    
	// Calculate the matrix code.
	// The 12 pixels forming the corners (used to determine which direction
    // the marker is facing) are ignored.
	// Binarize the unwarped marker pattern space.
	// Record the minimum observed contrast for use as a confidence measure.
    contrastMin = 255;
    bit = 119; // Bits are read MSB to LSB. In our case, bit 119 is MSB, bit 0 is LSB.
    if( dir == 0 ) {
        for( j = 0; j < AR_GLOBAL_ID_OUTER_SIZE; j++ ) {
            for( i = 0; i < AR_GLOBAL_ID_OUTER_SIZE; i++ ) {
                if (i > (AR_GLOBAL_ID_INNER_SIZE - 1) && i < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE) && j > (AR_GLOBAL_ID_INNER_SIZE - 1) && j < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE)) continue; // Skip interior.
                if ((i&~1) == 0      && (j&~1) == 0     ) continue;
                if ((i&~1) == 0      && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                contrast = data[j*AR_GLOBAL_ID_OUTER_SIZE + i] - thresh;
                recd127[bit--] = (contrast < 0 ? 1 : 0);
                contrast = abs(contrast);
                if (contrast < contrastMin) contrastMin = contrast;
            }
        }
    } else if( dir == 1 ) {
        for( i = 0; i < AR_GLOBAL_ID_OUTER_SIZE; i++ ) {
            for( j = AR_GLOBAL_ID_OUTER_SIZE-1; j >= 0; j-- ) {
                if (i > (AR_GLOBAL_ID_INNER_SIZE - 1) && i < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE) && j > (AR_GLOBAL_ID_INNER_SIZE - 1) && j < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE)) continue; // Skip interior.
                if ((i&~1) == 0      && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == 0     ) continue;
                contrast = data[j*AR_GLOBAL_ID_OUTER_SIZE + i] - thresh;
                recd127[bit--] = (contrast < 0 ? 1 : 0);
                contrast = abs(contrast);
                if (contrast < contrastMin) contrastMin = contrast;
            }
        }
    } else if( dir == 2 ) {
        for( j = AR_GLOBAL_ID_OUTER_SIZE-1; j >= 0; j-- ) {
            for( i = AR_GLOBAL_ID_OUTER_SIZE-1; i >= 0; i-- ) {
                if (i > (AR_GLOBAL_ID_INNER_SIZE - 1) && i < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE) && j > (AR_GLOBAL_ID_INNER_SIZE - 1) && j < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE)) continue; // Skip interior.
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == 0     ) continue;
                if ((i&~1) == 0      && (j&~1) == 0     ) continue;
                contrast = data[j*AR_GLOBAL_ID_OUTER_SIZE + i] - thresh;
                recd127[bit--] = (contrast < 0 ? 1 : 0);
                contrast = abs(contrast);
                if (contrast < contrastMin) contrastMin = contrast;
            }
        }
    } else if( dir == 3 ) {
        for( i = AR_GLOBAL_ID_OUTER_SIZE-1; i >= 0; i-- ) {
            for( j = 0; j < AR_GLOBAL_ID_OUTER_SIZE; j++ ) {
                if (i > (AR_GLOBAL_ID_INNER_SIZE - 1) && i < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE) && j > (AR_GLOBAL_ID_INNER_SIZE - 1) && j < (AR_GLOBAL_ID_OUTER_SIZE - AR_GLOBAL_ID_INNER_SIZE)) continue; // Skip interior.
                if ((i&~1) == AR_GLOBAL_ID_OUTER_SIZE-2 && (j&~1) == 0     ) continue;
                if ((i&~1) == 0      && (j&~1) == 0     ) continue;
                if ((i&~1) == 0      && (j&~1) == AR_GLOBAL_ID_OUTER_SIZE-2) continue;
                contrast = data[j*AR_GLOBAL_ID_OUTER_SIZE + i] - thresh;
                recd127[bit--] = (contrast < 0 ? 1 : 0);
                contrast = abs(contrast);
                if (contrast < contrastMin) contrastMin = contrast;
            }
        }
    }
    
#if DEBUG_PATT_GETID
    ARLOGd("Contrast = %d\n", contrastMin);
#endif
    *dir_p = dir;
    *cf = (contrastMin > 30)? _1_0: (ARdouble)contrastMin/_30_0;
    ret = decode_bch(AR_MATRIX_CODE_GLOBAL_ID, 0, recd127, &code);
    if (ret < 0) {
        return (-4); // EDC fail.
    }
    if (errorCorrected) *errorCorrected = ret;
    *code_out_p = code;
    return 0;
}

