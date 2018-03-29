/*
 *  paramDecomp.c
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
 *  Author(s): Takeshi Mita, Shinsaku Hiura, Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Takeshi Mita, Shinsaku Hiura, Hirokazu Kato
 *
 *         tmita@inolab.sys.es.osaka-u.ac.jp
 *         shinsaku@sys.es.osaka-u.ac.jp
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.1
 * Date: 01/12/07
 *
 *******************************************************/

#include <stdio.h>
#include <math.h>
#include <ARX/AR/ar.h>

static ARdouble norm( const ARdouble a, const ARdouble b, const ARdouble c );
static ARdouble dot( const ARdouble a1, const ARdouble a2, const ARdouble a3,
                   const ARdouble b1, const ARdouble b2, const ARdouble b3 );
#ifndef ARDOUBLE_IS_FLOAT
static float normf( const float a, const float b, const float c );
static float dotf( const float a1, const float a2, const float a3,
                   const float b1, const float b2, const float b3 );
#endif

int  arParamDecomp( const ARParam *source, ARParam *icpara, ARdouble trans[3][4] )
{
	int i;
	
	if (source->dist_function_version < 1 || source->dist_function_version > AR_DIST_FUNCTION_VERSION_MAX) return (-1);
	
	icpara->dist_function_version = source->dist_function_version;
    icpara->xsize          = source->xsize;
    icpara->ysize          = source->ysize;
	for (i = 0; i < arParamVersionInfo[source->dist_function_version - 1].dist_factor_num; i++) icpara->dist_factor[i] = source->dist_factor[i];
    return arParamDecompMat( source->mat, icpara->mat, trans );
}

int  arParamDecompMat( const ARdouble source[3][4], ARdouble cpara[3][4], ARdouble trans[3][4] )
{
    int       r, c;
    ARdouble    Cpara[3][4];
    ARdouble    rem1, rem2, rem3;

    if( source[2][3] >= 0 ) {
        for( r = 0; r < 3; r++ ){
            for( c = 0; c < 4; c++ ){
                Cpara[r][c] = source[r][c];
            }
        }
    }
    else {
        for( r = 0; r < 3; r++ ){
            for( c = 0; c < 4; c++ ){
                    Cpara[r][c] = -(source[r][c]);
                }
        }
    }

    for( r = 0; r < 3; r++ ){
        for( c = 0; c < 4; c++ ){
                cpara[r][c] = 0.0;
        }
    }
    cpara[2][2] = norm( Cpara[2][0], Cpara[2][1], Cpara[2][2] );
    trans[2][0] = Cpara[2][0] / cpara[2][2];
    trans[2][1] = Cpara[2][1] / cpara[2][2];
    trans[2][2] = Cpara[2][2] / cpara[2][2];
    trans[2][3] = Cpara[2][3] / cpara[2][2];
	
    cpara[1][2] = dot( trans[2][0], trans[2][1], trans[2][2],
                       Cpara[1][0], Cpara[1][1], Cpara[1][2] );
    rem1 = Cpara[1][0] - cpara[1][2] * trans[2][0];
    rem2 = Cpara[1][1] - cpara[1][2] * trans[2][1];
    rem3 = Cpara[1][2] - cpara[1][2] * trans[2][2];
    cpara[1][1] = norm( rem1, rem2, rem3 );
    trans[1][0] = rem1 / cpara[1][1];
    trans[1][1] = rem2 / cpara[1][1];
    trans[1][2] = rem3 / cpara[1][1];

    cpara[0][2] = dot( trans[2][0], trans[2][1], trans[2][2],
                       Cpara[0][0], Cpara[0][1], Cpara[0][2] );
    cpara[0][1] = dot( trans[1][0], trans[1][1], trans[1][2],
                       Cpara[0][0], Cpara[0][1], Cpara[0][2] );
    rem1 = Cpara[0][0] - cpara[0][1]*trans[1][0] - cpara[0][2]*trans[2][0];
    rem2 = Cpara[0][1] - cpara[0][1]*trans[1][1] - cpara[0][2]*trans[2][1];
    rem3 = Cpara[0][2] - cpara[0][1]*trans[1][2] - cpara[0][2]*trans[2][2];
    cpara[0][0] = norm( rem1, rem2, rem3 );
    trans[0][0] = rem1 / cpara[0][0];
    trans[0][1] = rem2 / cpara[0][0];
    trans[0][2] = rem3 / cpara[0][0];

    trans[1][3] = (Cpara[1][3] - cpara[1][2]*trans[2][3]) / cpara[1][1];
    trans[0][3] = (Cpara[0][3] - cpara[0][1]*trans[1][3]
                               - cpara[0][2]*trans[2][3]) / cpara[0][0];

    for( r = 0; r < 3; r++ ){
        for( c = 0; c < 3; c++ ){
                cpara[r][c] /= cpara[2][2];
        }
    }

    return 0;
}

#ifndef ARDOUBLE_IS_FLOAT
int  arParamDecompMatf( const ARdouble source[3][4], float cpara[3][4], float trans[3][4] )
{
    int       r, c;
    float     Cpara[3][4];
    float     rem1, rem2, rem3;
    
    if( source[2][3] >= 0.0 ) {
        for( r = 0; r < 3; r++ ){
            for( c = 0; c < 4; c++ ){
                Cpara[r][c] = (float)source[r][c];
            }
        }
    }
    else {
        for( r = 0; r < 3; r++ ){
            for( c = 0; c < 4; c++ ){
                Cpara[r][c] = -(float)(source[r][c]);
            }
        }
    }
    
    for( r = 0; r < 3; r++ ){
        for( c = 0; c < 4; c++ ){
            cpara[r][c] = 0.0f;
        }
    }
    cpara[2][2] = normf( Cpara[2][0], Cpara[2][1], Cpara[2][2] );
    trans[2][0] = Cpara[2][0] / cpara[2][2];
    trans[2][1] = Cpara[2][1] / cpara[2][2];
    trans[2][2] = Cpara[2][2] / cpara[2][2];
    trans[2][3] = Cpara[2][3] / cpara[2][2];
	
    cpara[1][2] = dotf( trans[2][0], trans[2][1], trans[2][2],
                      Cpara[1][0], Cpara[1][1], Cpara[1][2] );
    rem1 = Cpara[1][0] - cpara[1][2] * trans[2][0];
    rem2 = Cpara[1][1] - cpara[1][2] * trans[2][1];
    rem3 = Cpara[1][2] - cpara[1][2] * trans[2][2];
    cpara[1][1] = normf( rem1, rem2, rem3 );
    trans[1][0] = rem1 / cpara[1][1];
    trans[1][1] = rem2 / cpara[1][1];
    trans[1][2] = rem3 / cpara[1][1];
    
    cpara[0][2] = dotf( trans[2][0], trans[2][1], trans[2][2],
                      Cpara[0][0], Cpara[0][1], Cpara[0][2] );
    cpara[0][1] = dotf( trans[1][0], trans[1][1], trans[1][2],
                      Cpara[0][0], Cpara[0][1], Cpara[0][2] );
    rem1 = Cpara[0][0] - cpara[0][1]*trans[1][0] - cpara[0][2]*trans[2][0];
    rem2 = Cpara[0][1] - cpara[0][1]*trans[1][1] - cpara[0][2]*trans[2][1];
    rem3 = Cpara[0][2] - cpara[0][1]*trans[1][2] - cpara[0][2]*trans[2][2];
    cpara[0][0] = normf( rem1, rem2, rem3 );
    trans[0][0] = rem1 / cpara[0][0];
    trans[0][1] = rem2 / cpara[0][0];
    trans[0][2] = rem3 / cpara[0][0];
    
    trans[1][3] = (Cpara[1][3] - cpara[1][2]*trans[2][3]) / cpara[1][1];
    trans[0][3] = (Cpara[0][3] - cpara[0][1]*trans[1][3]
                   - cpara[0][2]*trans[2][3]) / cpara[0][0];
    
    for( r = 0; r < 3; r++ ){
        for( c = 0; c < 3; c++ ){
            cpara[r][c] /= cpara[2][2];
        }
    }
    
    return 0;
}
#endif

static ARdouble norm( const ARdouble a, const ARdouble b, const ARdouble c )
{
    return( sqrt( a*a + b*b + c*c ) );
}

static ARdouble dot( const ARdouble a1, const ARdouble a2, const ARdouble a3,
		   const ARdouble b1, const ARdouble b2, const ARdouble b3 )
{
    return( a1 * b1 + a2 * b2 + a3 * b3 );
}

#ifndef ARDOUBLE_IS_FLOAT
static float normf( const float a, const float b, const float c )
{
    return( sqrtf( a*a + b*b + c*c ) );
}

static float dotf( const float a1, const float a2, const float a3,
                  const float b1, const float b2, const float b3 )
{
    return( a1 * b1 + a2 * b2 + a3 * b3 );
}
#endif