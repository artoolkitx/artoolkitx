/*
 *  AR2/coord.c
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
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/coord.h>

int ar2MarkerCoord2ScreenCoord2( const ARParamLT *cparamLT, const float  trans[3][4], const float  mx, const float  my, float  *sx, float  *sy )
{
    float   wtrans[3][4];
    float   hx, hy, h;
    float   ix, iy;
    float   ix1, iy1;

    if( cparamLT != NULL ) {
        arUtilMatMuldff( cparamLT->param.mat, trans, wtrans );
    hx = wtrans[0][0] * mx
       + wtrans[0][1] * my
       + wtrans[0][3];
    hy = wtrans[1][0] * mx
       + wtrans[1][1] * my
       + wtrans[1][3];
    h  = wtrans[2][0] * mx
       + wtrans[2][1] * my
       + wtrans[2][3];
    ix = hx / h;
    iy = hy / h;
    }
    else {
        hx = trans[0][0] * mx
           + trans[0][1] * my
           + trans[0][3];
        hy = trans[1][0] * mx
           + trans[1][1] * my
           + trans[1][3];
        h  = trans[2][0] * mx
           + trans[2][1] * my
           + trans[2][3];        
        *sx = hx / h;
        *sy = hy / h;
        return 0;
    }

    if( arParamIdeal2ObservLTf( &cparamLT->paramLTf, ix, iy, sx, sy ) < 0 ) return -1;
    if( arParamObserv2IdealLTf( &cparamLT->paramLTf, *sx, *sy, &ix1, &iy1) < 0 ) return -1;
    if( (ix-ix1)*(ix-ix1) + (iy-iy1)*(iy-iy1) > 1.0F ) return -1;
    
    return 0;
}

int ar2MarkerCoord2ScreenCoord( const ARParamLT *cparamLT, const float  trans[3][4], const float  mx, const float  my, float  *sx, float  *sy )
{
    float   wtrans[3][4];
    float   hx, hy, h;
    float   ix, iy;

    if( cparamLT != NULL ) {
        arUtilMatMuldff( cparamLT->param.mat, trans, wtrans );
    hx = wtrans[0][0] * mx
       + wtrans[0][1] * my
       + wtrans[0][3];
    hy = wtrans[1][0] * mx
       + wtrans[1][1] * my
       + wtrans[1][3];
    h  = wtrans[2][0] * mx
       + wtrans[2][1] * my
       + wtrans[2][3];
    ix = hx / h;
    iy = hy / h;
    }
    else {
        hx = trans[0][0] * mx
           + trans[0][1] * my
           + trans[0][3];
        hy = trans[1][0] * mx
           + trans[1][1] * my
           + trans[1][3];
        h  = trans[2][0] * mx
           + trans[2][1] * my
           + trans[2][3];
        *sx = hx / h;
        *sy = hy / h;
        return 0;
    }

    if( arParamIdeal2ObservLTf( &cparamLT->paramLTf, ix, iy, sx, sy ) < 0 ) return -1;

    return 0;
}

int ar2ScreenCoord2MarkerCoord( const ARParamLT *cparamLT, const float  trans[3][4], const float  sx, const float  sy, float  *mx, float  *my )
{
    float   ix, iy;
    float   wtrans[3][4];
    float   c11, c12, c21, c22, b1, b2;
    float   m;

    if( cparamLT == NULL ) {
        c11 = trans[2][0] * sx - trans[0][0];
        c12 = trans[2][1] * sx - trans[0][1];
        c21 = trans[2][0] * sy - trans[1][0];
        c22 = trans[2][1] * sy - trans[1][1];
        b1  = trans[0][3] - trans[2][3] * sx;
        b2  = trans[1][3] - trans[2][3] * sy;
    }
    else {
        if( arParamObserv2IdealLTf( &cparamLT->paramLTf, sx, sy, &ix, &iy) < 0 ) return -1;
        arUtilMatMuldff( cparamLT->param.mat, trans, wtrans );
        c11 = wtrans[2][0] * ix - wtrans[0][0];
        c12 = wtrans[2][1] * ix - wtrans[0][1];
        c21 = wtrans[2][0] * iy - wtrans[1][0];
        c22 = wtrans[2][1] * iy - wtrans[1][1];
        b1  = wtrans[0][3] - wtrans[2][3] * ix;
        b2  = wtrans[1][3] - wtrans[2][3] * iy;
    }

    m = c11 * c22 - c12 * c21;
    if( m == 0.0F ) return -1;
    *mx = (c22 * b1 - c12 * b2) / m;
    *my = (c11 * b2 - c21 * b1) / m;

    return 0;
}

int ar2MarkerCoord2ImageCoord( const int xsize, const int ysize, const float dpi, const float  mx, const float  my, float  *ix, float  *iy )
{
    *ix = mx * dpi / 25.4f;
    *iy = ysize - my * dpi / 25.4f;

    return 0;
}

int ar2ImageCoord2MarkerCoord2( const int xsize, const int ysize, const float dpi, const float  ix, const float  iy, float  *mx, float  *my )
{
    *mx = ix * 25.4f / dpi;
    *my = (ysize - iy) * 25.4f / dpi;

    return 0;
}

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int ar2GetImageValue( const ARParamLT *cparamLT, const float  trans[3][4], const AR2ImageT *image,
                      const float  sx, const float  sy, const int blurLevel, ARUint8 *pBW )
{
    float   mx, my;
    float   iix, iiy;
    int     ix, iy;

    if( ar2ScreenCoord2MarkerCoord( cparamLT, trans, sx, sy, &mx, &my ) < 0 ) return -1;
    ar2MarkerCoord2ImageCoord( image->xsize, image->ysize, image->dpi, mx, my, &iix, &iiy );
    ix = (int)(iix + 0.5F);
    iy = (int)(iiy + 0.5F);

    if( ix < 0 || ix >= image->xsize
     || iy < 0 || iy >= image->ysize ) return -1;

    *pBW = image->imgBWBlur[blurLevel][iy*image->xsize+ix];

    return 0;
}

int ar2GetImageValue2( const ARParamLT *cparamLT, const float  trans[3][4], const AR2ImageT *image,
                       const float  sx, const float  sy, const int blurLevel, ARUint8 *pBW1, ARUint8 *pBW2, ARUint8 *pBW3 )
{
    float   mx, my;
    float   iix, iiy;
    int     ix, iy;

    if( ar2ScreenCoord2MarkerCoord( cparamLT, trans, sx, sy, &mx, &my ) < 0 ) return -1;
    ar2MarkerCoord2ImageCoord( image->xsize, image->ysize, image->dpi, mx, my, &iix, &iiy );
    ix = (int)(iix + 0.5);
    iy = (int)(iiy + 0.5);

    if( ix < 0 || ix >= image->xsize
     || iy < 0 || iy >= image->ysize ) return -1;

    if( blurLevel < 1 ) blurLevel = 1;
    if( blurLevel >= AR2_BLUR_IMAGE_MAX-1 ) blurLevel = AR2_BLUR_IMAGE_MAX-2;
    *pBW1 = image->imgBWBlur[blurLevel-1][iy*image->xsize+ix];
    *pBW2 = image->imgBWBlur[blurLevel  ][iy*image->xsize+ix];
    *pBW3 = image->imgBWBlur[blurLevel+1][iy*image->xsize+ix];

    return 0;
}
#else
int ar2GetImageValue( const ARParamLT *cparamLT, const float trans[3][4], const AR2ImageT *image,
                      const float sx, const float sy, ARUint8 *pBW )
{
    float   mx, my;
    int     ix, iy;

    if( ar2ScreenCoord2MarkerCoord( cparamLT, trans, sx, sy, &mx, &my ) < 0 ) return -1;
    
    ix = (int)(mx * image->dpi / 25.4F + 0.5F);
    if( ix < 0 || ix >= image->xsize ) return -1;

    iy = (int)(image->ysize - my * image->dpi / 25.4F + 0.5F);
    if( iy < 0 || iy >= image->ysize ) return -1;

    *pBW = image->imgBW[iy*image->xsize+ix];

    return 0;
}
#endif
