/*
 *  AR2/template.c
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
#include <ARX/AR2/config.h>
#include <ARX/AR2/coord.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/template.h>
#include <ARX/AR2/tracking.h>


#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
AR2TemplateT *ar2GenTemplate( int ts1, int ts2 )
#else
AR2TemplateT *ar2GenTemplate( int ts1, int ts2 )
#endif
{
    AR2TemplateT  *templ;
    int           xsize, ysize;

    arMalloc( templ, AR2TemplateT, 1 );
    templ->xts1 = templ->yts1 = ts1;
    templ->xts2 = templ->yts2 = ts2;

    templ->xsize = xsize = templ->xts1 + templ->xts2 + 1;
    templ->ysize = ysize = templ->yts1 + templ->yts2 + 1;
    arMalloc( templ->img1,  ARUint16,  xsize*ysize );

    return templ;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
AR2Template2T *ar2GenTemplate2( int ts1, int ts2 )
{
    AR2Template2T  *templ2;
    int             xsize, ysize;

    arMalloc( templ2, AR2Template2T, 1 );
    templ2->xts1 = templ2->yts1 = ts1;
    templ2->xts2 = templ2->yts2 = ts2;

    templ2->xsize = xsize = templ2->xts1 + templ2->xts2 + 1;
    templ2->ysize = ysize = templ2->yts1 + templ2->yts2 + 1;
    arMalloc( templ2->img1[0], ARUint16,  xsize*ysize );
    arMalloc( templ2->img1[1], ARUint16,  xsize*ysize );
    arMalloc( templ2->img1[2], ARUint16,  xsize*ysize );

    return templ2;
}
#endif

int ar2FreeTemplate( AR2TemplateT *templ )
{
    free( templ->img1 );
    free( templ );

    return 0;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2FreeTemplate2( AR2Template2T *templ2 )
{
    free( templ2->img1[0] );
    free( templ2->img1[1] );
    free( templ2->img1[2] );
    free( templ2 );

    return 0;
}
#endif

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2SetTemplateSub( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                       AR2FeaturePointsT *featurePoints, int num, int blurLevel,
                       AR2TemplateT *templ )
#else
int ar2SetTemplateSub( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                       AR2FeaturePointsT *featurePoints, int num,
                       AR2TemplateT *templ )
#endif
{
    float    mx, my;
    float    sx, sy;
    float    wtrans[3][4];
    ARUint16 *img1;
    int      sum, sum2;
    int      vlen;
    ARUint8  pixel;
    int      ix, iy;
    int      ix2, iy2;
    int      ret;
    int      i, j, k;

    if( cparamLT != NULL ) {
#ifdef ARDOUBLE_IS_FLOAT
        arUtilMatMul( cparamLT->param.mat, trans, wtrans );
#else
        arUtilMatMuldff( cparamLT->param.mat, trans, wtrans );
#endif
        
        mx = featurePoints->coord[num].mx;
        my = featurePoints->coord[num].my;
        if( ar2MarkerCoord2ScreenCoord( NULL, (const float (*)[4])wtrans, mx, my, &mx, &my ) < 0 ) return -1;
        if( arParamIdeal2ObservLTf( &cparamLT->paramLTf, mx, my, &sx, &sy ) < 0 ) return -1;
        ix = (int)(sx + 0.5F);
        iy = (int)(sy + 0.5F);

        img1 = templ->img1;
        sum = sum2 = 0;
        k = 0;
        iy2 = iy - (templ->yts1)*AR2_TEMP_SCALE;
        for( j = -(templ->yts1); j <= templ->yts2; j++, iy2+=AR2_TEMP_SCALE ) {
            ix2 = ix - (templ->xts1)*AR2_TEMP_SCALE;
            for( i = -(templ->xts1); i <= templ->xts2; i++, ix2+=AR2_TEMP_SCALE ) {

                if( arParamObserv2IdealLTf( &cparamLT->paramLTf, (float)ix2, (float)iy2, &sx, &sy) < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                    continue;
                }

                ret = ar2GetImageValue( NULL, (const float (*)[4])wtrans, imageSet->scale[featurePoints->scale],
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                                       sx, sy, blurLevel, &pixel );
#else
                                        sx, sy, &pixel );
#endif
                if( ret < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                }
                else {
                    *(img1++) = pixel;
                    sum  += pixel;
                    sum2 += pixel*pixel;
                    k++;
                }
            }
        }
    }
    else {
        mx = featurePoints->coord[num].mx;
        my = featurePoints->coord[num].my;
        if( ar2MarkerCoord2ScreenCoord( NULL, trans, mx, my, &sx, &sy ) < 0 ) return -1;
        ix = (int)(sx + 0.5F);
        iy = (int)(sy + 0.5F);
        
        img1 = templ->img1;
        sum = sum2 = 0;
        k = 0;
        iy2 = iy - (templ->yts1)*AR2_TEMP_SCALE;
        for( j = -(templ->yts1); j <= templ->yts2; j++, iy2+=AR2_TEMP_SCALE ) {
            ix2 = ix - (templ->xts1)*AR2_TEMP_SCALE;
            for( i = -(templ->xts1); i <= templ->xts2; i++, ix2+=AR2_TEMP_SCALE ) {
                
                ret = ar2GetImageValue( NULL, trans, imageSet->scale[featurePoints->scale],
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                                       (float)ix2, (float)iy2, blurLevel, &pixel );
#else
                                       (float)ix2, (float)iy2, &pixel );
#endif
                if( ret < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                }
                else {
                    *(img1++) = pixel;
                    sum  += pixel;
                    sum2 += pixel*pixel;
                    k++;
                }
            }
        }
    }
    if( k == 0 ) return -1;

    vlen = sum2 - sum*sum/k;
    templ->vlen = (int)sqrtf((float)vlen);
    templ->sum = sum;
    templ->validNum = k;

    return 0;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2SetTemplate2Sub( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                        AR2FeaturePointsT *featurePoints, int num, int blurLevel,
                        AR2Template2T *templ2 )
{
    float    mx, my;
    float    sx, sy;
    float    wtrans[3][4];
    ARUint16 *img1, *img2, *img3;
    int      sum11, sum21, sum31;
    int      sum12, sum22, sum32;
    int      vlen1, vlen2, vlen3;
    ARUint8  pixel1, pixel2, pixel3;
    int      ix, iy;
    int      ix2, iy2;
    int      ret;
    int      i, j, k;

    if( cparamLT != NULL ) {
        arUtilMatMul( cparamLT->param.mat, trans, wtrans );

        mx = featurePoints->coord[num].mx;
        my = featurePoints->coord[num].my;
        if( ar2MarkerCoord2ScreenCoord( NULL, wtrans, mx, my, &mx, &my ) < 0 ) return -1;
        if( arParamIdeal2ObservLTf( &cparamLT->paramLTf, mx, my, &sx, &sy ) < 0 ) return -1;
        ix = (int)(sx + 0.5F);
        iy = (int)(sy + 0.5F);

        img1 = templ2->img1[0];
        img2 = templ2->img1[1];
        img3 = templ2->img1[2];
        sum11 = sum21 = sum31 = 0;
        sum12 = sum22 = sum32 = 0;
        k = 0;
        iy2 = iy - (templ2->yts1)*AR2_TEMP_SCALE;
        for( j = -(templ2->yts1); j <= templ2->yts2; j++, iy2+=AR2_TEMP_SCALE ) {
            ix2 = ix - (templ2->xts1)*AR2_TEMP_SCALE;
            for( i = -(templ2->xts1); i <= templ2->xts2; i++, ix2+=AR2_TEMP_SCALE ) {

                if( arParamObserv2IdealLTf( &cparamLT->paramLTf, ix2, iy2, &sx, &sy) < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img2++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img3++) = AR2_TEMPLATE_NULL_PIXEL;
                    continue;
                }

                ret = ar2GetImageValue2( NULL, wtrans, imageSet->scale[featurePoints->scale],
                                        sx, sy, blurLevel, &pixel1, &pixel2, &pixel3 );
                if( ret < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img2++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img3++) = AR2_TEMPLATE_NULL_PIXEL;
                }
                else {
                    *(img1++) = pixel1;
                    sum11 += pixel1;
                    sum12 += pixel1*pixel1;
                    *(img2++) = pixel2;
                    sum21 += pixel2;
                    sum22 += pixel2*pixel2;
                    *(img3++) = pixel3;
                    sum31 += pixel3;
                    sum32 += pixel3*pixel3;
                    k++;
                }
            }
        }
    }
    else {
        mx = featurePoints->coord[num].mx;
        my = featurePoints->coord[num].my;
        if( ar2MarkerCoord2ScreenCoord( NULL, trans, mx, my, &sx, &sy ) < 0 ) return -1;
        ix = (int)(sx + 0.5F);
        iy = (int)(sy + 0.5F);
        
        img1 = templ2->img1[0];
        img2 = templ2->img1[1];
        img3 = templ2->img1[2];
        sum11 = sum21 = sum31 = 0;
        sum12 = sum22 = sum32 = 0;
        k = 0;
        iy2 = iy - (templ2->yts1)*AR2_TEMP_SCALE;
        for( j = -(templ2->yts1); j <= templ2->yts2; j++, iy2+=AR2_TEMP_SCALE ) {
            ix2 = ix - (templ2->xts1)*AR2_TEMP_SCALE;
            for( i = -(templ2->xts1); i <= templ2->xts2; i++, ix2+=AR2_TEMP_SCALE ) {
                
                ret = ar2GetImageValue2( NULL, trans, imageSet->scale[featurePoints->scale],
                                        ix2, iy2, blurLevel, &pixel1, &pixel2, &pixel3 );
                if( ret < 0 ) {
                    *(img1++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img2++) = AR2_TEMPLATE_NULL_PIXEL;
                    *(img3++) = AR2_TEMPLATE_NULL_PIXEL;
                }
                else {
                    *(img1++) = pixel1;
                    sum11 += pixel1;
                    sum12 += pixel1*pixel1;
                    *(img2++) = pixel2;
                    sum21 += pixel2;
                    sum22 += pixel2*pixel2;
                    *(img3++) = pixel3;
                    sum31 += pixel3;
                    sum32 += pixel3*pixel3;
                    k++;
                }
            }
        }
    }
    if( k == 0 ) return -1;

    vlen1 = sum12 - sum11*sum11/k;
    vlen2 = sum22 - sum21*sum21/k;
    vlen3 = sum32 - sum31*sum31/k;
    templ2->vlen[0] = (int)sqrtf((float)vlen1);
    templ2->vlen[1] = (int)sqrtf((float)vlen2); 
    templ2->vlen[2] = (int)sqrtf((float)vlen3);
    templ2->sum[0] = sum11;
    templ2->sum[0] = sum21;
    templ2->sum[0] = sum31;
    templ2->validNum = k;

    return 0;
}
#endif
