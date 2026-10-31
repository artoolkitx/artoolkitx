/*
 *  AR2/selectTemplate.c
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
#include <time.h>
#include <math.h>
#include <ARX/AR2/template.h>

static int    ar2GetVectorAngle( float  p1[2], float  p2[2], float  *psinf, float  *pcosf );
static float  ar2GetTriangleArea( float  p1[2], float  p2[2], float  p3[2] );
static float  ar2GetRegionArea( float  pos[4][2], int q1, int r1, int r2 );


int ar2GetResolution( const ARParamLT *cparamLT, const float  trans[3][4], const float  pos[2], float  dpi[2] )
{
    if( cparamLT != NULL ) {
        return ar2GetResolution2( &(cparamLT->param), trans, pos, dpi );
    }
    else {
        return ar2GetResolution2( NULL, trans, pos, dpi );
    }
}

int ar2GetResolution2( const ARParam *cparam, const float  trans[3][4], const float  pos[2], float  dpi[2] )
{
    float   mat[3][4];
    float   mx, my, hx, hy, h;
    float   x0, y0, x1, y1, x2, y2, d1, d2;

    if( cparam != NULL ) {
        arUtilMatMuldff( cparam->mat, trans, mat );

        mx = pos[0];
        my = pos[1];
        hx = mat[0][0] * mx + mat[0][1] * my + mat[0][3];
        hy = mat[1][0] * mx + mat[1][1] * my + mat[1][3];
        h  = mat[2][0] * mx + mat[2][1] * my + mat[2][3];
        x0 = hx / h;
        y0 = hy / h;

        mx = pos[0] + 10.0F;
        my = pos[1];
        hx = mat[0][0] * mx + mat[0][1] * my + mat[0][3];
        hy = mat[1][0] * mx + mat[1][1] * my + mat[1][3];
        h  = mat[2][0] * mx + mat[2][1] * my + mat[2][3];
        x1 = hx / h;
        y1 = hy / h;

        mx = pos[0];
        my = pos[1] + 10.0F;
        hx = mat[0][0] * mx + mat[0][1] * my + mat[0][3];
        hy = mat[1][0] * mx + mat[1][1] * my + mat[1][3];
        h  = mat[2][0] * mx + mat[2][1] * my + mat[2][3];
        x2 = hx / h;
        y2 = hy / h;
    }
    else {
        mx = pos[0];
        my = pos[1];
        hx = trans[0][0] * mx + trans[0][1] * my + trans[0][3];
        hy = trans[1][0] * mx + trans[1][1] * my + trans[1][3];
        h  = trans[2][0] * mx + trans[2][1] * my + trans[2][3];
        x0 = hx / h;
        y0 = hy / h;

        mx = pos[0] + 10.0F;
        my = pos[1];
        hx = trans[0][0] * mx + trans[0][1] * my + trans[0][3];
        hy = trans[1][0] * mx + trans[1][1] * my + trans[1][3];
        h  = trans[2][0] * mx + trans[2][1] * my + trans[2][3];
        x1 = hx / h;
        y1 = hy / h;

        mx = pos[0];
        my = pos[1] + 10.0F;
        hx = trans[0][0] * mx + trans[0][1] * my + trans[0][3];
        hy = trans[1][0] * mx + trans[1][1] * my + trans[1][3];
        h  = trans[2][0] * mx + trans[2][1] * my + trans[2][3];
        x2 = hx / h;
        y2 = hy / h;
    }

    d1 = (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0);
    d2 = (x2-x0)*(x2-x0) + (y2-y0)*(y2-y0);
    if( d1 < d2 ) {
        dpi[0] = sqrtf(d2) * 2.54F;
        dpi[1] = sqrtf(d1) * 2.54F;
    }
    else {
        dpi[0] = sqrtf(d1) * 2.54F;
        dpi[1] = sqrtf(d2) * 2.54F;
    }

    return 0;
}

int ar2SelectTemplate( AR2TemplateCandidateT *candidate, AR2TemplateCandidateT *prevFeature, int num,
                       float  pos[4][2], int xsize, int ysize )
{
    if( num < 0 ) return -1;

    if( num == 0 ) {
        float   d, dmax;
        int     i, j;

        dmax = 0.0f; j = -1;
        for( i = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag != 0 ) continue;
            if( candidate[i].sx < xsize/8 || candidate[i].sx > xsize*7/8
             || candidate[i].sy < ysize/8 || candidate[i].sy > ysize*7/8 ) continue;

            d = (candidate[i].sx - xsize/2)*(candidate[i].sx - xsize/2)
              + (candidate[i].sy - ysize/2)*(candidate[i].sy - ysize/2);
            if( d > dmax ) { dmax = d; j = i; }
        }

        if( j != -1 ) candidate[j].flag = 1;
        return j;
    }

    else if( num == 1 ) {
        float   d, dmax;
        int     i, j;

        dmax = 0.0f; j = -1;
        for( i = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag != 0 ) continue;
            if( candidate[i].sx < xsize/8 || candidate[i].sx > xsize*7/8
             || candidate[i].sy < ysize/8 || candidate[i].sy > ysize*7/8 ) continue;

            d = (candidate[i].sx - pos[0][0])*(candidate[i].sx - pos[0][0])
              + (candidate[i].sy - pos[0][1])*(candidate[i].sy - pos[0][1]);
            if( d > dmax ) { dmax = d; j = i; }
        }

        if( j != -1 ) candidate[j].flag = 1;
        return j;
    }

    else if( num == 2 ) {
        float   d, dmax;
        int     i, j;

        dmax = 0.0f; j = -1;
        for( i = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag != 0 ) continue;
            if( candidate[i].sx < xsize/8 || candidate[i].sx > xsize*7/8
             || candidate[i].sy < ysize/8 || candidate[i].sy > ysize*7/8 ) continue;

            d = ((candidate[i].sx - pos[0][0])*(pos[1][1] - pos[0][1])
               - (candidate[i].sy - pos[0][1])*(pos[1][0] - pos[0][0]));
            d = d * d;
            if( d > dmax ) { dmax = d; j = i; }
        }

        if( j != -1 ) candidate[j].flag = 1;
        return j;
    }

    else if( num == 3 ) {
        float  p2sinf, p2cosf, p3sinf, p3cosf, p4sinf, p4cosf;
        float  smax, s;
        int    q1, r1, r2;
        int    i, j;

        ar2GetVectorAngle(pos[0], pos[1], &p2sinf, &p2cosf);
        ar2GetVectorAngle(pos[0], pos[2], &p3sinf, &p3cosf);

        j = -1;
        smax = 0.0f;
        for( i = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag != 0 ) continue;
            if( candidate[i].sx < xsize/8 || candidate[i].sx > xsize*7/8
             || candidate[i].sy < ysize/8 || candidate[i].sy > ysize*7/8 ) continue;

            pos[3][0] = candidate[i].sx;
            pos[3][1] = candidate[i].sy;
            ar2GetVectorAngle(pos[0], pos[3], &p4sinf, &p4cosf);

            if( ((p3sinf*p2cosf - p3cosf*p2sinf) >= 0.0F) && ((p4sinf*p2cosf - p4cosf*p2sinf) >= 0.0F) ) {
                if( p4sinf*p3cosf - p4cosf*p3sinf >= 0.0F ) {
                    q1 = 1; r1 = 2; r2 = 3;
                }
                else {
                    q1 = 1; r1 = 3; r2 = 2;
                }
            }
            else if( ((p4sinf*p3cosf - p4cosf*p3sinf) >= 0.0F) && ((p2sinf*p3cosf - p2cosf*p3sinf) >= 0.0F) ) {
                if( p4sinf*p2cosf - p4cosf*p2sinf >= 0.0F ) {
                    q1 = 2; r1 = 1; r2 = 3;
                }
                else {
                    q1 = 2; r1 = 3; r2 = 1;
                }
            }
            else if( ((p2sinf*p4cosf - p2cosf*p4sinf) >= 0.0F) && ((p3sinf*p4cosf - p3cosf*p4sinf) >= 0.0F) ) {
                if( p3sinf*p2cosf - p3cosf*p2sinf >= 0.0F ) {
                    q1 = 3; r1 = 1; r2 = 2;
                }
                else {
                    q1 = 3; r1 = 2; r2 = 1;
                }
            }
            else continue;

            s = ar2GetRegionArea( pos, q1, r1, r2 );
            if( s > smax ) { smax = s; j = i; }
       }

        if( j != -1 ) candidate[j].flag = 1;
        return j;
    }

    else {
        int     i, j, k;
        static  int s = 0;

        for( i = 0; prevFeature[i].flag != -1; i++ ) {
            if( prevFeature[i].flag != 0 ) continue;
            prevFeature[i].flag = 1;
            for( j = 0; candidate[j].flag != -1; j++ ) {
                if( candidate[j].flag == 0
                 && prevFeature[i].snum == candidate[j].snum
                 && prevFeature[i].level == candidate[j].level
                 && prevFeature[i].num == candidate[j].num ) break;
            }
            if( candidate[j].flag != -1 ) {
                candidate[j].flag = 1;
                return j;
            }
        }
        prevFeature[0].flag = -1;

        if( s == 0 ) srand((unsigned int)time(NULL));
        s++;
        if( s == 128 ) s = 0;

        for( i = j = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag == 0 ) j++;
        }
        if( j == 0 ) return -1;

        k = (int)((float )j * (float)rand() / ((float)RAND_MAX + 1.0f));
        for( i = j = 0; candidate[i].flag != -1; i++ ) {
            if( candidate[i].flag != 0 ) continue;
            if( j == k ) {
                candidate[i].flag = 1;
                return i;
            }
            j++;
        }
        return -1;
    }
}

static int ar2GetVectorAngle( float  p1[2], float  p2[2], float  *psinf, float  *pcosf )
{
    float    l;

    l = sqrtf( (p2[0]-p1[0])*(p2[0]-p1[0]) + (p2[1]-p1[1])*(p2[1]-p1[1]) );
    if( l == 0.0F ) return -1;

    *psinf = (p2[1] - p1[1]) / l;
    *pcosf = (p2[0] - p1[0]) / l;
    return 0;
}

static float  ar2GetRegionArea( float  pos[4][2], int q1, int r1, int r2 )
{
    float     s;

    s  = ar2GetTriangleArea( pos[0], pos[q1], pos[r1] );
    s += ar2GetTriangleArea( pos[0], pos[r1], pos[r2] );

    return s;
}

static float  ar2GetTriangleArea( float  p1[2], float  p2[2], float  p3[2] )
{
    float   x1, y1, x2, y2;
    float   s;

    x1 = p2[0] - p1[0];
    y1 = p2[1] - p1[1];
    x2 = p3[0] - p1[0];
    y2 = p3[1] - p1[1];

    s = (x1 * y2 - x2 * y1) / 2.0f;
    if( s < 0.0f ) s = -s;

    return s;
}
