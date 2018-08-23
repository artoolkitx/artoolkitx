/*
 *  paramClear.c
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
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 2.1
 * Date: 99/07/16
 *
 *******************************************************/

#include <stdio.h>
#include <math.h>
#include <ARX/AR/ar.h>

int arParamClear( ARParam *param, int xsize, int ysize, int dist_function_version )
{
    if (!param) return (-1);
    
    param->xsize = xsize;
    param->ysize = ysize;
	param->dist_function_version = dist_function_version;

    param->mat[0][0] =   1.0;
    param->mat[0][1] =   0.0;
    param->mat[0][2] = xsize/2.0;
    param->mat[0][3] =   0.0;
    param->mat[1][0] =   0.0;
    param->mat[1][1] =   1.0;
    param->mat[1][2] = ysize/2.0;
    param->mat[1][3] =   0.0;
    param->mat[2][0] =   0.0;
    param->mat[2][1] =   0.0;
    param->mat[2][2] =   1.0;
    param->mat[2][3] =   0.0;

    return arParamDistFactorClear( param->dist_factor, xsize, ysize, param->dist_function_version );
}

int arParamDistFactorClear( ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], int xsize, int ysize, int dist_function_version )
{
    if (!dist_factor) return (-1);

    if (dist_function_version == 5) {
        dist_factor[0] = 0.0;           /*  k1  */
        dist_factor[1] = 0.0;           /*  k2  */
        dist_factor[2] = 0.0;           /*  p1  */
        dist_factor[3] = 0.0;           /*  p2  */
        dist_factor[4] = 0.0;           /*  k3  */
        dist_factor[5] = 0.0;           /*  k4  */
        dist_factor[6] = 0.0;           /*  k5  */
        dist_factor[7] = 0.0;           /*  k6  */
        dist_factor[8] = 0.0;           /*  s1  */
        dist_factor[9] = 0.0;           /*  s2  */
        dist_factor[10] = 0.0;          /*  s3  */
        dist_factor[11] = 0.0;          /*  s4  */
        dist_factor[12] = 1.0;          /*  fx  */
        dist_factor[13] = 1.0;          /*  fy  */
        dist_factor[14] = xsize / 2.0;  /*  cx  */
        dist_factor[15] = ysize / 2.0;  /*  cy  */
        dist_factor[16] = 1.0;          /*  Size adjust */
        return 0;
    } else if (dist_function_version == 4) {
		dist_factor[0] = 0.0;           /*  k1  */
		dist_factor[1] = 0.0;           /*  k2  */
		dist_factor[2] = 0.0;           /*  p1  */
		dist_factor[3] = 0.0;           /*  p2  */
		dist_factor[4] = 1.0;           /*  fx  */
		dist_factor[5] = 1.0;           /*  fy  */
		dist_factor[6] = xsize / 2.0;   /*  cx  */
		dist_factor[7] = ysize / 2.0;   /*  cy  */
		dist_factor[8] = 1.0;           /*  Size adjust */
		return 0;
	} else if (dist_function_version == 3) {
		dist_factor[0] = xsize / 2.0;
		dist_factor[1] = ysize / 2.0;
		dist_factor[2] = 1.0;
		dist_factor[3] = 1.0;
		dist_factor[4] = 0.0;
		dist_factor[5] = 0.0;
		return 0;
	} else if (dist_function_version == 2) {
		dist_factor[0] = xsize / 2.0;
		dist_factor[1] = ysize / 2.0;
		dist_factor[2] = 1.0;
		dist_factor[3] = 0.0;
		dist_factor[4] = 0.0;
		return 0;		
	} else if (dist_function_version == 1) {
		dist_factor[0] = xsize / 2.0;
		dist_factor[1] = ysize / 2.0;
		dist_factor[2] = 1.0;
		dist_factor[3] = 0.0;
		return 0;		
	} else {
		return -1;
	}
}

int arParamClearWithFOVy(ARParam *param, int xsize, int ysize, ARdouble FOVy)
{
    if (!param) return (-1);
    
    param->xsize = xsize;
    param->ysize = ysize;
    param->dist_function_version = 5;
    
#ifdef ARDOUBLE_IS_FLOAT
    ARdouble f = ysize/2.0f / tanf(FOVy / 2.0f);
#else
    ARdouble f = ysize/2.0 / tan(FOVy / 2.0);
#endif
    
    param->mat[0][0] =   f;
    param->mat[0][1] =   0.0;
    param->mat[0][2] = xsize/2.0;
    param->mat[0][3] =   0.0;
    param->mat[1][0] =   0.0;
    param->mat[1][1] =   f;
    param->mat[1][2] = ysize/2.0;
    param->mat[1][3] =   0.0;
    param->mat[2][0] =   0.0;
    param->mat[2][1] =   0.0;
    param->mat[2][2] =   1.0;
    param->mat[2][3] =   0.0;
    
    param->dist_factor[0] = 0.0;           /*  k1  */
    param->dist_factor[1] = 0.0;           /*  k2  */
    param->dist_factor[2] = 0.0;           /*  p1  */
    param->dist_factor[3] = 0.0;           /*  p2  */
    param->dist_factor[4] = 0.0;           /*  k3  */
    param->dist_factor[5] = 0.0;           /*  k4  */
    param->dist_factor[6] = 0.0;           /*  k5  */
    param->dist_factor[7] = 0.0;           /*  k6  */
    param->dist_factor[8] = 0.0;           /*  s1  */
    param->dist_factor[9] = 0.0;           /*  s2  */
    param->dist_factor[10] = 0.0;          /*  s3  */
    param->dist_factor[11] = 0.0;          /*  s4  */
    param->dist_factor[12] = f;            /*  fx  */
    param->dist_factor[13] = f;            /*  fy  */
    param->dist_factor[14] = xsize / 2.0;  /*  cx  */
    param->dist_factor[15] = ysize / 2.0;  /*  cy  */
    param->dist_factor[16] = 1.0;          /*  Size adjust */
    
    return 0;
}

