/*
 *  paramDistortion.c
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

#define  PD_LOOP   3
#define  PD_LOOP2  4

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#else
#  define SQRT sqrt
#endif

int arParamObserv2Ideal( const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ox, const ARdouble oy,
                         ARdouble *ix, ARdouble *iy, const int dist_function_version )
{
	// ----------------------------------------
	if (dist_function_version == 4) {

    // OpenCV distortion model, with addition of a scale factor so that
    // entire image fits onscreen.
    ARdouble k1, k2, p1, p2, fx, fy, x0, y0, s;
    ARdouble px, py, x02, y02;
    int    i;

    k1 = dist_factor[0];
    k2 = dist_factor[1];
    p1 = dist_factor[2];
    p2 = dist_factor[3];
    fx = dist_factor[4];
    fy = dist_factor[5];
    x0 = dist_factor[6];
    y0 = dist_factor[7];
    s  = dist_factor[8];

    px = (ox - x0)/fx;
    py = (oy - y0)/fy;

    x02 = px*px;
    y02 = py*py;
  
    for( i = 1; ; i++ ) {
        if( x02 != 0.0 || y02 != 0.0 ) {
            px = px - ((1.0 + k1*(x02+y02) + k2*(x02+y02)*(x02+y02))*px + 2.0*p1*px*py + p2*(x02 + y02 + 2.0*x02)-((ox - x0)/fx))/(1.0+k1*(3.0*x02+y02)+k2*(5.0*x02*x02+3.0*x02*y02+y02*y02)+2.0*p1*py+6.0*p2*px);
            //px = px - ((1.0 + k1*(x02+y02) + k2*(x02+y02)*(x02+y02))*px + 2.0*p1*px*py + p2*(x02 + y02 + 2.0*x02)-((ox - x0)/fx))/(1.0+k1*(3.0*x02+y02)+k2*(5.0*x02*x02+6.0*x02*y02+y02*y02)+2.0*p1*py+6.0*p2*px);

            py = py - ((1.0 + k1*(x02+y02) + k2*(x02+y02)*(x02+y02))*py + p1*(x02 + y02 + 2.0*y02) + 2.0*p2*px*py-((oy - y0)/fy))/(1.0+k1*(x02+3.0*y02)+k2*(x02*x02+3.0*x02*y02+5.0*y02*y02)+6.0*p1*py+2.0*p2*px);
            //py = py - ((1.0 + k1*(x02+y02) + k2*(x02+y02)*(x02+y02))*py + p1*(x02 + y02 + 2.0*y02) + 2.0*p2*px*py-((oy - y0)/fy))/(1.0+k1*(x02+3.0*y02)+k2*(x02*x02+6.0*x02*y02+5.0*y02*y02)+6.0*p1*py+2.0*p2*px);
        }
        else {
          px = 0.0;
          py = 0.0;
          break;
        }
        if( i == PD_LOOP2 ) break;
    
        x02 = px*px;
        y02 = py*py;
    }
  
    *ix = px*fx/s + x0;
    *iy = py*fy/s + y0;

    return 0;

	// ----------------------------------------
	} else if (dist_function_version == 3) {

    ARdouble  z02, z0, p1, p2, q, z, px, py, ar;
    int     i;

    ar = dist_factor[3];
    px = (ox - dist_factor[0]) / ar;
    py =  oy - dist_factor[1];
    p1 = dist_factor[4]/100000000.0;
    p2 = dist_factor[5]/100000000.0/100000.0;
    z02 = px*px+ py*py;
    q = z0 = SQRT(px*px+ py*py);

    for( i = 1; ; i++ ) {
        if( z0 != 0.0 ) {
            z = z0 - ((1.0 - p1*z02 - p2*z02*z02)*z0 - q) / (1.0 - 3.0*p1*z02 - 5.0*p2*z02*z02);
            px = px * z / z0;
            py = py * z / z0;
        }
        else {
            px = 0.0;
            py = 0.0;
            break;
        }
        if( i == PD_LOOP ) break;

        z02 = px*px+ py*py;
        z0 = SQRT(px*px+ py*py);
    }

    *ix = px / dist_factor[2] + dist_factor[0];
    *iy = py / dist_factor[2] + dist_factor[1];

    return 0;

	// ----------------------------------------
	} else if (dist_function_version == 2) {

    ARdouble  z02, z0, p1, p2, q, z, px, py;
    int     i;

    px = ox - dist_factor[0];
    py = oy - dist_factor[1];
    p1 = dist_factor[3]/100000000.0;
    p2 = dist_factor[4]/100000000.0/100000.0;
    z02 = px*px+ py*py;
    q = z0 = SQRT(px*px+ py*py);

    for( i = 1; ; i++ ) {
        if( z0 != 0.0 ) {
            z = z0 - ((1.0 - p1*z02 - p2*z02*z02)*z0 - q) / (1.0 - 3.0*p1*z02 - 5.0*p2*z02*z02);
            px = px * z / z0;
            py = py * z / z0;
        }
        else {
            px = 0.0;
            py = 0.0;
            break;
        }
        if( i == PD_LOOP ) break;

        z02 = px*px+ py*py;
        z0 = SQRT(px*px+ py*py);
    }

    *ix = px / dist_factor[2] + dist_factor[0];
    *iy = py / dist_factor[2] + dist_factor[1];

    return 0;

	// ----------------------------------------
	} else if (dist_function_version == 1) {
		
    ARdouble  z02, z0, p, q, z, px, py;
    int     i;

    px = ox - dist_factor[0];
    py = oy - dist_factor[1];
    p = dist_factor[3]/100000000.0;
    z02 = px*px+ py*py;
    q = z0 = SQRT(px*px+ py*py);

    for( i = 1; ; i++ ) {
        if( z0 != 0.0 ) {
            z = z0 - ((1.0 - p*z02)*z0 - q) / (1.0 - 3.0*p*z02);
            px = px * z / z0;
            py = py * z / z0;
        }
        else {
            px = 0.0;
            py = 0.0;
            break;
        }
        if( i == PD_LOOP ) break;

        z02 = px*px+ py*py;
        z0 = SQRT(px*px+ py*py);
    }

    *ix = px / dist_factor[2] + dist_factor[0];
    *iy = py / dist_factor[2] + dist_factor[1];

    return 0;

	// ----------------------------------------
	} else {
		
    return -1;
	
	}
}

int arParamIdeal2Observ( const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ix, const ARdouble iy,
                         ARdouble *ox, ARdouble *oy, const int dist_function_version )
{
	// ----------------------------------------
	if (dist_function_version == 4) {

    ARdouble k1, k2, p1, p2, fx, fy, x0, y0, s;
    ARdouble l, x, y;

    k1 = dist_factor[0];
    k2 = dist_factor[1];
    p1 = dist_factor[2];
    p2 = dist_factor[3];
    fx = dist_factor[4];
    fy = dist_factor[5];
    x0 = dist_factor[6];
    y0 = dist_factor[7];
    s  = dist_factor[8];
  
    x = (ix - x0)*s/fx;
    y = (iy - y0)*s/fy;
    l = x*x + y*y;
    *ox = (x*(1.0+k1*l+k2*l*l)+2.0*p1*x*y+p2*(l+2.0*x*x))*fx+x0;
    *oy = (y*(1.0+k1*l+k2*l*l)+p1*(l+2.0*y*y)+2.0*p2*x*y)*fy+y0;

    return 0;

	// ----------------------------------------
	} else if (dist_function_version == 3) {

	ARdouble    x, y, l, d, ar;

    ar = dist_factor[3];
    x = (ix - dist_factor[0]) * dist_factor[2];
    y = (iy - dist_factor[1]) * dist_factor[2];
    if( x == 0.0 && y == 0.0 ) {
        *ox = dist_factor[0];
        *oy = dist_factor[1];
    }
    else {
        l = x*x + y*y;
        d = 1.0 - dist_factor[4]/100000000.0 * l - dist_factor[5]/100000000.0/100000.0 * l * l;
        *ox = x * d * ar + dist_factor[0];
        *oy = y * d      + dist_factor[1];
    }

    return 0;
	
	// ----------------------------------------
	} else if (dist_function_version == 2) {
	
    ARdouble    x, y, l, d;

    x = (ix - dist_factor[0]) * dist_factor[2];
    y = (iy - dist_factor[1]) * dist_factor[2];
    if( x == 0.0 && y == 0.0 ) {
        *ox = dist_factor[0];
        *oy = dist_factor[1];
    }
    else {
        l = x*x + y*y;
        d = 1.0 - dist_factor[3]/100000000.0 * l - dist_factor[4]/100000000.0/100000.0 * l * l;
        *ox = x * d + dist_factor[0];
        *oy = y * d + dist_factor[1];
    }

    return 0;
	
	// ----------------------------------------
	} else if (dist_function_version == 1) {
		
	ARdouble    x, y, d;

    x = (ix - dist_factor[0]) * dist_factor[2];
    y = (iy - dist_factor[1]) * dist_factor[2];
    if( x == 0.0 && y == 0.0 ) {
        *ox = dist_factor[0];
        *oy = dist_factor[1];
    }
    else {
        d = 1.0 - dist_factor[3]/100000000.0 * (x*x+y*y);
        *ox = x * d + dist_factor[0];
        *oy = y * d + dist_factor[1];
    }

    return 0;
	// ----------------------------------------
	} else {
	
	return -1;
	
	}
}
