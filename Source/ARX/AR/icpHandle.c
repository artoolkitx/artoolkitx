/*
 *  icpHandle.c
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>


ICPHandleT  *icpCreateHandle( const ARdouble matXc2U[3][4] )
{
    ICPHandleT *handle;
    int         i, j;

    handle = (ICPHandleT *)malloc(sizeof(ICPHandleT));
    if( handle == NULL ) return NULL;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            handle->matXc2U[j][i] = matXc2U[j][i];
        }
    }
    handle->maxLoop = ICP_MAX_LOOP;
    handle->breakLoopErrorThresh      = ICP_BREAK_LOOP_ERROR_THRESH;
    handle->breakLoopErrorRatioThresh = ICP_BREAK_LOOP_ERROR_RATIO_THRESH;
    handle->breakLoopErrorThresh2     = ICP_BREAK_LOOP_ERROR_THRESH2;
    handle->inlierProb                = ICP_INLIER_PROBABILITY;

    return handle;
}

int icpDeleteHandle( ICPHandleT **handle )
{
    if( *handle == NULL ) return -1;

    free( *handle );
    *handle = NULL;

    return 0;
}

int icpSetMatXc2U( ICPHandleT *handle, const ARdouble matXc2U[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            handle->matXc2U[j][i] = matXc2U[j][i];
        }
    }
    return 0;
}

int icpSetMaxLoop( ICPHandleT *handle, int maxLoop )
{
    if( handle == NULL ) return -1;

    handle->maxLoop = maxLoop;
    return 0;
}

int icpSetBreakLoopErrorThresh( ICPHandleT *handle, ARdouble breakLoopErrorThresh )
{
    if( handle == NULL ) return -1;

    handle->breakLoopErrorThresh = breakLoopErrorThresh;
    return 0;
}

int icpSetBreakLoopErrorRatioThresh( ICPHandleT *handle, ARdouble breakLoopErrorRatioThresh )
{
    if( handle == NULL ) return -1;

    handle->breakLoopErrorRatioThresh = breakLoopErrorRatioThresh;
    return 0;
}

int icpSetBreakLoopErrorThresh2( ICPHandleT *handle, ARdouble breakLoopErrorThresh2 )
{
    if( handle == NULL ) return -1;

    handle->breakLoopErrorThresh2 = breakLoopErrorThresh2;
    return 0;
}

int icpGetMatXc2U( ICPHandleT *handle, ARdouble matXc2U[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            matXc2U[j][i] = handle->matXc2U[j][i];
        }
    }
    return 0;
}

int icpGetMaxLoop( ICPHandleT *handle, int *maxLoop )
{
    if( handle == NULL ) return -1;

    *maxLoop = handle->maxLoop;
    return 0;
}

int icpGetBreakLoopErrorThresh( ICPHandleT *handle, ARdouble *breakLoopErrorThresh )
{
    if( handle == NULL ) return -1;

    *breakLoopErrorThresh = handle->breakLoopErrorThresh;
    return 0;
}

int icpGetBreakLoopErrorRatioThresh( ICPHandleT *handle, ARdouble *breakLoopErrorRatioThresh )
{
    if( handle == NULL ) return -1;

    *breakLoopErrorRatioThresh = handle->breakLoopErrorRatioThresh;
    return 0;
}

int icpGetBreakLoopErrorThresh2( ICPHandleT *handle, ARdouble *breakLoopErrorThresh2 )
{
    if( handle == NULL ) return -1;

    *breakLoopErrorThresh2 = handle->breakLoopErrorThresh2;
    return 0;
}

int icpSetInlierProbability( ICPHandleT *handle, ARdouble inlierProb )
{
    if( handle == NULL ) return -1;

    if (handle->inlierProb != inlierProb) {
        handle->inlierProb = inlierProb;
        ARLOGd("inlierProb=%f\n", inlierProb);
    };
    return 0;
}

int icpGetInlierProbability( ICPHandleT *handle, ARdouble *inlierProb )
{
    if( handle == NULL ) return -1;

    *inlierProb = handle->inlierProb;
    return 0;
}
