/*
 *  AR2/searchPoint.c
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
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/coord.h>
#include <ARX/AR2/searchPoint.h>

void ar2GetSearchPoint( const ARParamLT *cparamLT,
                        const float  trans1[3][4], const float  trans2[3][4], const float  trans3[3][4],
                        AR2FeatureCoordT *feature,
                        int search[3][2] )
{
    float    mx, my;
    float    ox1, ox2, ox3;
    float    oy1, oy2, oy3;

    mx = feature->mx;
    my = feature->my;

    if( trans1 == NULL
     || ar2MarkerCoord2ScreenCoord( cparamLT, trans1, mx, my, &ox1, &oy1 ) < 0 ) {
        goto nosearch1;
    }
    search[0][0] = (int)ox1;
    search[0][1] = (int)oy1;

    if( trans2 == NULL
     || ar2MarkerCoord2ScreenCoord( cparamLT, trans2, mx, my, &ox2, &oy2 ) < 0 ) {
        goto nosearch2;
    }
    search[1][0] = (int)(2*ox1 - ox2);
    search[1][1] = (int)(2*oy1 - oy2);

    if( trans3 == NULL
     || ar2MarkerCoord2ScreenCoord( cparamLT, trans3, mx, my, &ox3, &oy3 ) < 0 ) {
        goto nosearch3;
    }
    search[2][0] = (int)(3*ox1 - 3*ox2 + ox3);
    search[2][1] = (int)(3*oy1 - 3*oy2 + oy3);

    return;

nosearch1:
    search[0][0] = -1;
    search[0][1] = -1;
nosearch2:
    search[1][0] = -1;
    search[1][1] = -1;
nosearch3:
    search[2][0] = -1;
    search[2][1] = -1;
    return;
}
