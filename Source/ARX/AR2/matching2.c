/*
 *  AR2/matching2.c
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


#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static int ar2GetBestMatchingSubFine   ( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                         AR2Template2T *mtemp, int sx, int sy, int *val);
static int ar2GetBestMatchingSubFine2  ( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                         AR2Template2T *mtemp, int sx, int sy, int *val, int *blurLevel);
static void updateCandidate            ( int x, int y, int wval,
                                         int *keep_num, int cx[KEEP_NUM], int cy[KEEP_NUM], int cval[KEEP_NUM] );
#endif


#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2GetBestMatching2( ARUint8 *img, ARUint8 *mfImage, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                         AR2Template2T *mtemp, int rx, int ry,
                         int search[3][2], int *bx, int *by, float *val, int *blurLevel)
{
    int              search_flag[] = {USE_SEARCH1, USE_SEARCH2, USE_SEARCH3};
    int              px, py, sx, sy, ex, ey;
    int              yts1, yts2;
    int              keep_num;
    int              cx[KEEP_NUM], cy[KEEP_NUM];
    int              cval[KEEP_NUM];
    int              wval, wval2;
    int              wlevel;
    int              i, j, l;
    int              ii;
    int              ret;
    ARUint8         *pmf;

    keep_num = 0;

    yts1 = mtemp->yts1;
    yts2 = mtemp->yts2;

    for( ii = 0; ii < 3; ii++ ) {
        if( search_flag[ii] == 0 ) continue;
        if( search[ii][0] < 0 ) break;

        px = (search[ii][0]/(SKIP_INTERVAL+1))*(SKIP_INTERVAL+1) + (SKIP_INTERVAL+1)/2;
        py = (search[ii][1]/(SKIP_INTERVAL+1))*(SKIP_INTERVAL+1) + (SKIP_INTERVAL+1)/2;

        sx = px - rx;
        if( sx < 0 ) sx = 0;
        ex = px + rx;
        if( ex >= xsize ) ex = xsize-1;

        sy = py - ry;
        if( sy < 0 ) sy = 0;
        ey = py + ry;
        if( ey >= ysize ) ey = ysize-1;

        for( j = sy; j <= ey; j++ ) {
            pmf = &mfImage[j*xsize+sx];
            for( i = sx; i <= ex; i++ ) {
                *(pmf++) = 0;
            }
        }
    }

    ret = 1;
    for( ii = 0; ii < 3; ii++ ) {      
        if( search_flag[ii] == 0 ) continue;
        if( search[ii][0] < 0 ) {
            if( ret ) return -1;
            else    break;
        }

        px = (search[ii][0]/(SKIP_INTERVAL+1))*(SKIP_INTERVAL+1) + (SKIP_INTERVAL+1)/2;
        py = (search[ii][1]/(SKIP_INTERVAL+1))*(SKIP_INTERVAL+1) + (SKIP_INTERVAL+1)/2;

        for( j = py - ry; j <= py + ry; j += SKIP_INTERVAL+1 ) {
            if( j - yts1*AR2_TEMP_SCALE <  0     ) continue;
            if( j + yts2*AR2_TEMP_SCALE >= ysize ) break;
            for( i = px - rx; i <= px + rx; i += SKIP_INTERVAL+1 ) {
                if( i - mtemp->xts1*AR2_TEMP_SCALE <  0     ) continue;
                if( i + mtemp->xts2*AR2_TEMP_SCALE >= xsize ) break;
                if( mfImage[j*xsize+i] ) continue;
                mfImage[j*xsize+i] = 1;
                if( ar2GetBestMatchingSubFine(img,xsize,ysize,pixFormat,mtemp,i,j,&wval) < 0 ) {
                    continue;
                }
                ret = 0;
                updateCandidate(i, j, wval, &keep_num, cx, cy, cval);
            }
        }
    }

    wval2 = 0;
    ret = -1;
    for(l = 0; l < keep_num; l++) {
        for( j = cy[l]-SKIP_INTERVAL; j <= cy[l]+SKIP_INTERVAL; j++ ) {
            if( j-mtemp->yts1*AR2_TEMP_SCALE <  0     ) continue;
            if( j+mtemp->yts2*AR2_TEMP_SCALE >= ysize ) break;
            for( i = cx[l]-SKIP_INTERVAL; i <= cx[l]+SKIP_INTERVAL; i++ ) {
                if( i-mtemp->xts1*AR2_TEMP_SCALE <  0     ) continue;
                if( i+mtemp->xts2*AR2_TEMP_SCALE >= xsize ) break;
                if( ar2GetBestMatchingSubFine2(img,xsize,ysize,pixFormat,mtemp,i,j,&wval, &wlevel) < 0 ) {
                    continue;
                }
                if( wval > wval2 ) {
                    *bx   =  i;
                    *by   =  j;
                     wval2 = wval;
                    *val  =  (float)wval / 10000;
                    *blurLevel = wlevel;
                    ret = 0;
                }
            }
        }
    }
   
    return ret;
}
#else
int ar2GetBestMatching2(void)
{
    return 0;
}
#endif
 
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
static int ar2GetBestMatchingSubFine( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                      AR2Template2T *mtemp, int sx, int sy, int *val)
{
    ARUint16            *p1;
    ARUint8             *p2;
    int                  sum1, sum2, sum3;
    int                  w, vlen;
    int                  i, j;

    p1 = mtemp->img1[1];
    sum1 = sum2 = sum3 = 0;
    if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    sum1 += (*p2);
                    sum2 += (*p2) * (*p2);
                    sum3 += (*p2) * (*p1);
                }
                p2+=AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*3];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+0) + *(p2+1) + *(p2+2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2+=3*AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+0) + *(p2+1) + *(p2+2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2+=4*AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    else if( pixFormat == AR_PIXEL_FORMAT_ARGB || pixFormat == AR_PIXEL_FORMAT_ABGR ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+1) + *(p2+2) + *(p2+3))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2+=4*AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *(p2+1);
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2+=2*AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p1 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *p2;
                    sum1 += w;
                    sum2 += w*w;
                    sum3 += w * (*p1);
                }
                p2+=2*AR2_TEMP_SCALE;
                p1++;
            }
        }
        if( k == 0 ) return -1;
    }

    sum3 -= sum1 * mtemp->sum / mtemp->validNum;
    vlen = sum2 - sum1*sum1/mtemp->validNum;
    if( vlen == 0 ) *val = 0;
    else            *val = sum3 * 100 / mtemp->vlen[1] * 100 / (int)sqrtf((float)vlen);

    return 0;
}

static int ar2GetBestMatchingSubFine2( ARUint8 *img, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                                       AR2Template2T *mtemp, int sx, int sy, int *val, int *blurLevel)
{
    ARUint16            *p11, *p12, *p13;
    ARUint8             *p2;
    int                  sum1, sum2, sum31, sum32, sum33;
    int                  w, vlen;
    int                  val1, val2, val3;
    int                  i, j, k;

    p11 = mtemp->img1[0];
    p12 = mtemp->img1[1];
    p13 = mtemp->img1[2];
    k = sum1 = sum2 = sum31 = sum32 = sum33 = 0;
    if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    sum1  += (*p2);
                    sum2  += (*p2) * (*p2);
                    sum31 += (*p2) * (*p11);
                    sum32 += (*p2) * (*p12);
                    sum33 += (*p2) * (*p13);
                    k++;
                }
                p2+=AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*3];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+0) + *(p2+1) + *(p2+2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum31 += w * (*p11);
                    sum32 += w * (*p12);
                    sum33 += w * (*p13);
                    k++;
                }
                p2+=3*AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+0) + *(p2+1) + *(p2+2))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum31 += w * (*p11);
                    sum32 += w * (*p12);
                    sum33 += w * (*p13);
                    k++;
                }
                p2+=4*AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    else if( pixFormat == AR_PIXEL_FORMAT_ARGB || pixFormat == AR_PIXEL_FORMAT_ABGR ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*4];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = (*(p2+1) + *(p2+2) + *(p2+3))/3;
                    sum1 += w;
                    sum2 += w*w;
                    sum31 += w * (*p11);
                    sum32 += w * (*p12);
                    sum33 += w * (*p13);
                    k++;
                }
                p2+=4*AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *(p2+1);
                    sum1 += w;
                    sum2 += w*w;
                    sum31 += w * (*p11);
                    sum32 += w * (*p12);
                    sum33 += w * (*p13);
                    k++;
                }
                p2+=2*AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        for( j = -(mtemp->yts1); j <= mtemp->yts2; j++ ) {
            p2 = &img[((sy+j*AR2_TEMP_SCALE)*xsize + sx - mtemp->xts1*AR2_TEMP_SCALE)*2];
            for( i = -(mtemp->xts1); i <= mtemp->xts2; i++ ) {
                if( *p11 != AR2_TEMPLATE_NULL_PIXEL ) {
                    w = *p2;
                    sum1 += w;
                    sum2 += w*w;
                    sum31 += w * (*p11);
                    sum32 += w * (*p12);
                    sum33 += w * (*p13);
                    k++;
                }
                p2+=2*AR2_TEMP_SCALE;
                p11++;
                p12++;
                p13++;
            }
        }
        if( k == 0 ) return -1;
    }

    vlen = sum2 - sum1*sum1/k;
    if( vlen == 0 ) {
        *val = 0;
        return 0;
    }

    sum31 -= sum1 * mtemp->sum[0] / k;
    sum32 -= sum1 * mtemp->sum[1] / k;
    sum33 -= sum1 * mtemp->sum[2] / k;

    vlen = (int)sqrtf((float)vlen);
    val1 = sum31 * 100 / mtemp->vlen[0] * 100 / vlen;
    val2 = sum32 * 100 / mtemp->vlen[1] * 100 / vlen;
    val3 = sum33 * 100 / mtemp->vlen[2] * 100 / vlen;
    if( val1 > val2 ) {
        if( val1 > val3 ) { *val = val1; *blurLevel = 0;}
        else              { *val = val3; *blurLevel = 2;}
    }
    else {
        if( val2 > val3 ) { *val = val2; *blurLevel = 1;}
        else              { *val = val3; *blurLevel = 2;}
    }

    return 0;
}

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
        cx[n]   =  cx[n-1];
        cy[n]   =  cy[n-1];
        cval[n] =  cval[n-1]; 
    }
    cx[n]   = x;
    cy[n]   = y;
    cval[n] = wval;

    return;
}
#endif
