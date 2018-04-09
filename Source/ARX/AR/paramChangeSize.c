/*
 *  paramChangeSize.c
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


int arParamChangeSize( ARParam *source, int xsize, int ysize, ARParam *newparam )
{
    ARdouble  x_scale, y_scale;
    int     i;

    x_scale = (ARdouble)xsize / (ARdouble)(source->xsize);
    y_scale = (ARdouble)ysize / (ARdouble)(source->ysize);

    newparam->xsize = xsize;
    newparam->ysize = ysize;
    for( i = 0; i < 4; i++ ) {
        newparam->mat[0][i] = source->mat[0][i] * x_scale;
        newparam->mat[1][i] = source->mat[1][i] * y_scale;
        newparam->mat[2][i] = source->mat[2][i];
    }

    if (source->dist_function_version == 5) {
        newparam->dist_factor[0] = source->dist_factor[0];             /*  k1  */
        newparam->dist_factor[1] = source->dist_factor[1];             /*  k2  */
        newparam->dist_factor[2] = source->dist_factor[2];             /*  p1  */
        newparam->dist_factor[3] = source->dist_factor[3];             /*  p2  */
        newparam->dist_factor[4] = source->dist_factor[4];             /*  k3  */
        newparam->dist_factor[5] = source->dist_factor[5];             /*  k4  */
        newparam->dist_factor[6] = source->dist_factor[6];             /*  k5  */
        newparam->dist_factor[7] = source->dist_factor[7];             /*  k6  */
        newparam->dist_factor[8] = source->dist_factor[8];             /*  s1  */
        newparam->dist_factor[9] = source->dist_factor[9];             /*  s2  */
        newparam->dist_factor[10] = source->dist_factor[10];           /*  s3  */
        newparam->dist_factor[11] = source->dist_factor[11];           /*  s4  */
        newparam->dist_factor[12] = source->dist_factor[12] * x_scale; /*  fx  */
        newparam->dist_factor[13] = source->dist_factor[13] * y_scale; /*  fy  */
        newparam->dist_factor[14] = source->dist_factor[14] * x_scale; /*  cx  */
        newparam->dist_factor[15] = source->dist_factor[15] * y_scale; /*  cy  */
        newparam->dist_factor[16] = source->dist_factor[16];           /*  Size adjust */
    } else if (source->dist_function_version == 4) {
		newparam->dist_factor[0] = source->dist_factor[0];             /*  k1  */
		newparam->dist_factor[1] = source->dist_factor[1];             /*  k2  */
		newparam->dist_factor[2] = source->dist_factor[2];             /*  p1  */
		newparam->dist_factor[3] = source->dist_factor[3];             /*  p2  */
		newparam->dist_factor[4] = source->dist_factor[4] * x_scale;   /*  fx  */
		newparam->dist_factor[5] = source->dist_factor[5] * y_scale;   /*  fy  */
		newparam->dist_factor[6] = source->dist_factor[6] * x_scale;   /*  cx  */
		newparam->dist_factor[7] = source->dist_factor[7] * y_scale;   /*  cy  */
		newparam->dist_factor[8] = source->dist_factor[8];             /*  Size adjust */
	} else if (source->dist_function_version == 3) {
		newparam->dist_factor[0] = source->dist_factor[0] * x_scale;
		newparam->dist_factor[1] = source->dist_factor[1] * y_scale;
		newparam->dist_factor[2] = source->dist_factor[2];
		newparam->dist_factor[3] = source->dist_factor[3];
		newparam->dist_factor[4] = source->dist_factor[4] / (x_scale*y_scale);
	    newparam->dist_factor[5] = source->dist_factor[5] / (x_scale*x_scale*y_scale*y_scale);
	} else if (source->dist_function_version == 2) {
		newparam->dist_factor[0] = source->dist_factor[0] * x_scale;
		newparam->dist_factor[1] = source->dist_factor[1] * y_scale;
		newparam->dist_factor[2] = source->dist_factor[2];
		newparam->dist_factor[3] = source->dist_factor[3] / (x_scale*y_scale);
		newparam->dist_factor[4] = source->dist_factor[4] / (x_scale*x_scale*y_scale*y_scale);
	} else if (source->dist_function_version == 1) {
		newparam->dist_factor[0] = source->dist_factor[0] * x_scale;
		newparam->dist_factor[1] = source->dist_factor[1] * y_scale;
		newparam->dist_factor[2] = source->dist_factor[2];
		newparam->dist_factor[3] = source->dist_factor[3] / (x_scale*y_scale);
	} else {
		// Unknown distortion function version.
		return -1;
	}
	newparam->dist_function_version = source->dist_function_version;
    return 0;
}
