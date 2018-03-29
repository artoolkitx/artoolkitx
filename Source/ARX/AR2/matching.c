/*
 *  AR2/matching.c
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ARX/AR2/tracking.h>
#include <ARX/AR2/config.h>
#include <ARX/AR2/template.h>

#define  USE_SEARCH1    1
#define  USE_SEARCH2    1
#define  USE_SEARCH3    1

#define  SKIP_INTERVAL  3
#define  KEEP_NUM       3


static int ar2GetBestMatchingSubFine   ( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                         AR2TemplateT *mtemp, int sx, int sy, int *val);
static void updateCandidate            ( int x, int y, int wval,
                                         int *keep_num, int cx[KEEP_NUM], int cy[KEEP_NUM], int cval[KEEP_NUM] );
#if 1
static int ar2GetBestMatchingSubFineOpt( ARUint8 *img, int xsize, int ysize, int sx1, int sy1, AR2TemplateT *mtemp,
                                         ARUint32 *subImage1, ARUint32 *subImage2, int sx2, int sy2, int *val);
#endif

/*!
    @brief Get best match for a candidate feature template.
    @param img Incoming image to match against.
    @param mfImage Buffer same size as img, to provide working memory for status of matched features.
    @param xsize Horizontal size of img and mfImage.
    @param ysize Vertical size of img and mfImage.
    @param pixFormat Pixel format of img.
    @param mtemp Template undergoing matching.
    @param rx search radius in x dimension.
    @param ry search radius in y dimension.
    @param search screen coordinates (second dimension is x and y) for up to three previous positions of this feature.
    @param bx On return, x position of best candidate.
    @param by On return, y position of best candidate.
    @param val On return, the quality of the match of the best candidate.
    @result -1 in case of error or no match, or 0 otherwise.
 */
 
int ar2GetBestMatching( ARUint8 *img, ARUint8 *mfImage, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                        AR2TemplateT *mtemp, int rx, int ry,
                         int search[3][2], int *bx, int *by, float *val)
{
    int              search_flag[] = {USE_SEARCH1, USE_SEARCH2, USE_SEARCH3};
    int              px, py, sx, sy, ex, ey;
    int              yts1, yts2;
    int              keep_num;
    int              cx[KEEP_NUM], cy[KEEP_NUM];
    int              cval[KEEP_NUM];
    int              wval, wval2;
    int              i, j, l;
    int              ii;
    int              ret;
    ARUint8         *pmf;
#if 0
#else
    ARUint32   *subImage1, *p11, *p12, w1;
    ARUint32   *subImage2, *p21, *p22, w2;
    ARUint32    subImage11[AR2_TEMP_SCALE];
    ARUint32    subImage21[AR2_TEMP_SCALE];
    ARUint8    *p3, *p4;
#endif

    // First pass: initialise.
    yts1 = mtemp->yts1;
    yts2 = mtemp->yts2;
    for( ii = 0; ii < 3; ii++ ) {
        if( search_flag[ii] == 0 ) continue;
        if( search[ii][0] < 0 ) break;

        // "Snap" position to centre of grid square.
        px = (search[ii][0]/(SKIP_INTERVAL + 1))*(SKIP_INTERVAL + 1) + (SKIP_INTERVAL + 1)/2;
        py = (search[ii][1]/(SKIP_INTERVAL + 1))*(SKIP_INTERVAL + 1) + (SKIP_INTERVAL + 1)/2;

        sx = px - rx; // Start position in x.
        if( sx < 0 ) sx = 0;
        ex = px + rx; // End position in x.
        if( ex >= xsize ) ex = xsize - 1;

        sy = py - ry; // Start position in y.
        if( sy < 0 ) sy = 0;
        ey = py + ry; // End position in y.
        if( ey >= ysize ) ey = ysize - 1;

        // Initialise mfImage by writing 0s into the potential search space.
        for( j = sy; j <= ey; j++ ) {
            pmf = &mfImage[j*xsize + sx];
            for( i = sx; i <= ex; i++ ) {
                *(pmf++) = 0;
            }
        }
    }

    // Second pass: get candidates.
    keep_num = 0;
    ret = 1;
    for( ii = 0; ii < 3; ii++ ) {      
        if( search_flag[ii] == 0 ) continue;
        if( search[ii][0] < 0 ) {
            if( ret ) return -1; // If we haven't got at least one starting point for a search, bail out.
            else    break;
        }

        px = (search[ii][0]/(SKIP_INTERVAL + 1))*(SKIP_INTERVAL + 1) + (SKIP_INTERVAL + 1)/2;
        py = (search[ii][1]/(SKIP_INTERVAL + 1))*(SKIP_INTERVAL + 1) + (SKIP_INTERVAL + 1)/2;

        for( j = py - ry; j <= py + ry; j += SKIP_INTERVAL + 1 ) {
            if( j - yts1*AR2_TEMP_SCALE <  0     ) continue;
            if( j + yts2*AR2_TEMP_SCALE >= ysize ) break;
            for( i = px - rx; i <= px + rx; i += SKIP_INTERVAL + 1 ) {
                if( i - mtemp->xts1*AR2_TEMP_SCALE <  0     ) continue;
                if( i + mtemp->xts2*AR2_TEMP_SCALE >= xsize ) break;
                if( mfImage[j*xsize + i] ) continue; // Skip pixels already matched.
                mfImage[j*xsize + i] = 1; // Mark this pixel as matched.
                if( ar2GetBestMatchingSubFine(img, xsize, ysize, pixFormat, mtemp, i, j, &wval) < 0 ) {
                    continue;
                }
                ret = 0;
                updateCandidate(i, j, wval, &keep_num, cx, cy, cval);
            }
        }
    }

    // Third pass. Determine best candidate.
    wval2 = 0;
    ret = -1;
#if 0
    for(l = 0; l < keep_num; l++) {
        for( j = cy[l] - SKIP_INTERVAL; j <= cy[l] + SKIP_INTERVAL; j++ ) {
            if( j - mtemp->yts1*AR2_TEMP_SCALE <  0     ) continue;
            if( j + mtemp->yts2*AR2_TEMP_SCALE >= ysize ) break;
            for( i = cx[l] - SKIP_INTERVAL; i <= cx[l] + SKIP_INTERVAL; i++ ) {
                if( i - mtemp->xts1*AR2_TEMP_SCALE <  0     ) continue;
                if( i + mtemp->xts2*AR2_TEMP_SCALE >= xsize ) break;
                if( ar2GetBestMatchingSubFine(img, xsize, ysize, pixFormat, mtemp, i, j, &wval) < 0 ) {
                    continue;
                }
                if( wval > wval2 ) {
                    *bx    =  i;
                    *by    =  j;
                     wval2 =  wval;
                    *val   = (float)wval / 10000.0f;
                    ret = 0;
                }
            }
        }
    }
#else
    arMalloc( subImage1, ARUint32, ( (mtemp->xsize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2)) * ((mtemp->ysize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2) ) );
    arMalloc( subImage2, ARUint32, ( (mtemp->xsize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2)) * ((mtemp->ysize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2) ) );

    for(l = 0; l < keep_num; l++) {
        if( mtemp->validNum != mtemp->xsize*mtemp->ysize
         || (pixFormat != AR_PIXEL_FORMAT_MONO && pixFormat != AR_PIXEL_FORMAT_420v && pixFormat != AR_PIXEL_FORMAT_420f && pixFormat != AR_PIXEL_FORMAT_NV21)
         || cy[l] - SKIP_INTERVAL - mtemp->yts1*AR2_TEMP_SCALE < 0
         || cy[l] + SKIP_INTERVAL + mtemp->yts2*AR2_TEMP_SCALE >= ysize
         || cx[l] - SKIP_INTERVAL - mtemp->xts1*AR2_TEMP_SCALE < 0
         || cx[l] + SKIP_INTERVAL + mtemp->xts2*AR2_TEMP_SCALE >= xsize ) {
            for( j = cy[l] - SKIP_INTERVAL; j <= cy[l] + SKIP_INTERVAL; j++ ) {
                if( j - mtemp->yts1*AR2_TEMP_SCALE <  0     ) continue;
                if( j + mtemp->yts2*AR2_TEMP_SCALE >= ysize ) break;
                for( i = cx[l] - SKIP_INTERVAL; i <= cx[l] + SKIP_INTERVAL; i++ ) {
                    if( i - mtemp->xts1*AR2_TEMP_SCALE <  0     ) continue;
                    if( i + mtemp->xts2*AR2_TEMP_SCALE >= xsize ) break;
                    if( ar2GetBestMatchingSubFine(img, xsize, ysize, pixFormat, mtemp, i, j, &wval) < 0 ) {
                        continue;
                    }
                    if( wval > wval2 ) {
                        *bx    =  i;
                        *by    =  j;
                        wval2 =  wval;
                        *val   = (float)wval / 10000.0f;
                        ret = 0;
                    }
                }
            }
        }
        else {
            // Optimised case for mono incoming image.
            int px1 = (mtemp->xsize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2);
            int py1 = mtemp->ysize*AR2_TEMP_SCALE + (SKIP_INTERVAL*2);
            int px2 = cx[l] - SKIP_INTERVAL - mtemp->xts1*AR2_TEMP_SCALE;
            int py2 = cy[l] - SKIP_INTERVAL - mtemp->yts1*AR2_TEMP_SCALE;
            int px3 = px1 - AR2_TEMP_SCALE;
            p11 = p12 = subImage1;
            p21 = p22 = subImage2;
            for( j = 0; j < AR2_TEMP_SCALE*px1; j++ ) {
                *(p11++) = 0;
                *(p21++) = 0;
            }
            p3 = p4 = &img[py2*xsize + px2];
            for( j = 0; j < py1; j++ ) {
                for( i = 0; i < AR2_TEMP_SCALE; i++ ) {
                    *(p11++) = 0;
                    *(p21++) = 0;
                    subImage11[i] = 0;
                    subImage21[i] = 0;
                }
                p12 += AR2_TEMP_SCALE;
                p22 += AR2_TEMP_SCALE;
                for( i = 0; i < px3; i++) {
                    w1 = subImage11[i%AR2_TEMP_SCALE] += (*p3);
                    w2 = subImage21[i%AR2_TEMP_SCALE] += (*p3)*(*p3);
                    p3++;
                    *(p11++) = w1 + *(p12++);
                    *(p21++) = w2 + *(p22++);
                }
                p3 = p4 += xsize;
            }
            for( j = 0; j < SKIP_INTERVAL*2 + 1; j++ ) {
                 for( i = 0; i < SKIP_INTERVAL*2 + 1; i++) {
                     if( ar2GetBestMatchingSubFineOpt(img, xsize, ysize, px2 + i, py2 + j,
                         mtemp, subImage1, subImage2, i + AR2_TEMP_SCALE, j + AR2_TEMP_SCALE, &wval) < 0 ) {
                         continue;
                     }
                     if( wval > wval2 ) {
                         *bx    =  cx[l] - SKIP_INTERVAL + i;
                         *by    =  cy[l] - SKIP_INTERVAL + j;
                         wval2 =  wval;
                         *val   = (float)wval / 10000;
                         ret = 0;
                     }
                 }
            }
        }
    }
    free(subImage1);
    free(subImage2);
#endif

    return ret;
}

static int ar2GetBestMatchingSubFine( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                      AR2TemplateT *mtemp, int sx, int sy, int *val)
{
    ARUint16            *p1;
    ARUint8             *p2;
    int                  w;
    int                  sum1, sum2, sum3;
    int                  vlen;
    int                  i, j;

    p1 = mtemp->img1;
    sum1 = sum2 = sum3 = 0;
    if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
#if 0
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    sum1 += (*p2);
                    sum2 += (*p2) * (*p2);
                    sum3 += (*p2) * (*p1);
                }
                p2 += AR2_TEMP_SCALE;
                p1++;
            }
        }
#else
        int        ssx, eex, ssy, eey;
        ARUint8   *p3;
        ssx = -(mtemp->xts1);
        eex =   mtemp->xts2;
        ssy = -(mtemp->yts1);
        eey =   mtemp->yts2;
        p2 = p3 = &img[((sy + ssy*AR2_TEMP_SCALE)*xsize + sx + ssx*AR2_TEMP_SCALE)];
        for( j = ssy; j <= eey; j++ ) {
            for( i = ssx; i <= eex; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    sum1 += (*p2);
                    sum2 += (*p2) * (*p2);
                    sum3 += (*p2) * (*p1);
                }
                p2 += AR2_TEMP_SCALE;
                p1++;
            }
            p2 = p3 += AR2_TEMP_SCALE*xsize; // i.e. p3 += AR2_TEMP_SCALE*xsize; p2 = p3;
        }
#endif
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*3];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2 + 0) + *(p2 + 1) + *(p2 + 2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2 += 3*AR2_TEMP_SCALE;
                p1++;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2 + 0) + *(p2 + 1) + *(p2 + 2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2 += 4*AR2_TEMP_SCALE;
                p1++;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ARGB || pixFormat == AR_PIXEL_FORMAT_ABGR ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2 + 1) + *(p2 + 2) + *(p2 + 3))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2 += 4*AR2_TEMP_SCALE;
                p1++;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *(p2 + 1);
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2 += 2*AR2_TEMP_SCALE;
                p1++;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy + j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *p2;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2 += 2*AR2_TEMP_SCALE;
                p1++;
            }
        }
    }
    
    sum3 -= sum1 * mtemp->sum / mtemp->validNum;
    vlen = sum2 - sum1*sum1/mtemp->validNum;
    if( vlen == 0 ) *val = 0;
    else            *val = sum3 * 100 / mtemp->vlen * 100 / (int)sqrtf((float)vlen);

    return 0;
}

#if 1
static int ar2GetBestMatchingSubFineOpt( ARUint8 *img, int xsize, int ysize, int sx1, int sy1, AR2TemplateT *mtemp,
                                         ARUint32 *subImage1, ARUint32 *subImage2, int sx2, int sy2, int *val)
{
    ARUint16            *p1;
    ARUint8             *p2, *p3;
    int                  sum1, sum2, sum3;
    int                  vlen;
    int                  subImageXsize, px1, px2, py1, py2;
    int                  i, j;
    
    p1 = mtemp->img1;
    sum3 = 0;
    p2 = p3 = &img[sy1*xsize + sx1];
    for( j = 0; j < mtemp->ysize; j++ ) {
        for( i = 0; i < mtemp->xsize; i++ ) {
            sum3 += (*p2) * *(p1++);
            p2 += AR2_TEMP_SCALE;
        }
        p2 = p3 += AR2_TEMP_SCALE*xsize;
    }

    subImageXsize = (mtemp->xsize + 1)*AR2_TEMP_SCALE + (SKIP_INTERVAL*2);
    px1 =  sx2 + (mtemp->xsize - 1)*AR2_TEMP_SCALE;
    px2 =  sx2                    - AR2_TEMP_SCALE;
    py1 = (sy2 + (mtemp->ysize - 1)*AR2_TEMP_SCALE)*subImageXsize;
    py2 = (sy2                    - AR2_TEMP_SCALE)*subImageXsize;
    sum1 = subImage1[py1 + px1]
         + subImage1[py2 + px2]
         - subImage1[py1 + px2]
         - subImage1[py2 + px1];
    
    sum2 = subImage2[py1 + px1]
         + subImage2[py2 + px2]
         - subImage2[py1 + px2]
         - subImage2[py2 + px1];

    sum3 -= sum1 * mtemp->sum / mtemp->validNum;
    vlen = sum2 - sum1*sum1/mtemp->validNum;
    if( vlen == 0 ) *val = 0;
    else            *val = sum3 * 100 / mtemp->vlen * 100 / (int)sqrtf((float)vlen);

    return 0;
}
#endif

static void updateCandidate( int x, int y, int wval,
                             int *keep_num, int cx[KEEP_NUM], int cy[KEEP_NUM], int cval[KEEP_NUM] )
{
    int    l, m, n;

    if( *keep_num == 0 ) {
        cx[0]   = x;
        cy[0]   = y;    
        cval[0] = wval;
        *keep_num = 1;
        return;
    }

    for(l = 0; l < *keep_num; l++) {
        if( cval[l] < wval ) break;
    }
    if( l == *keep_num ) {
        if( l < KEEP_NUM )  {
            cx[l]   = x;
            cy[l]   = y;
            cval[l] = wval;
            (*keep_num)++;
        }
        return;
    }

    if( *keep_num == KEEP_NUM ) {
        m = KEEP_NUM - 1;
    }
    else {
        m = *keep_num;
        (*keep_num)++;
    }

    for( n = m; n > l; n-- ) {                
        cx[n]   =  cx[n - 1];
        cy[n]   =  cy[n - 1];
        cval[n] =  cval[n - 1]; 
    }
    cx[n]   = x;
    cy[n]   = y;
    cval[n] = wval;

    return;
}
