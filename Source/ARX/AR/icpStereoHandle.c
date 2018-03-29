/*
 *  icpStereoHandle.c
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


ICPStereoHandleT *icpStereoCreateHandle( const ARdouble matXcl2Ul[3][4], const ARdouble matXcr2Ur[3][4], const ARdouble matC2L[3][4], const ARdouble matC2R[3][4] )
{
    ICPStereoHandleT *handle;
    int               i, j;

    handle = (ICPStereoHandleT *)malloc(sizeof(ICPStereoHandleT));
    if( handle == NULL ) return NULL;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matXcl2Ul[j][i] = matXcl2Ul[j][i];
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matXcr2Ur[j][i] = matXcr2Ur[j][i];
    }
    if( matC2L != ICP_TRANS_MAT_IDENTITY ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) handle->matC2L[j][i] = matC2L[j][i];
        }
    }
    else {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) handle->matC2L[j][i] = (i == j)? 1.0: 0.0;
        }
    }
    if( matC2R != ICP_TRANS_MAT_IDENTITY ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) handle->matC2R[j][i] = matC2R[j][i];
        }
    }
    else {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) handle->matC2R[j][i] = (i == j)? 1.0: 0.0;
        }
    }
    handle->maxLoop                   = ICP_MAX_LOOP;
    handle->breakLoopErrorThresh      = ICP_BREAK_LOOP_ERROR_THRESH;
    handle->breakLoopErrorRatioThresh = ICP_BREAK_LOOP_ERROR_RATIO_THRESH;
    handle->breakLoopErrorThresh2     = ICP_BREAK_LOOP_ERROR_THRESH2;
    handle->inlierProb                = ICP_INLIER_PROBABILITY;

    return handle;
}

int icpStereoDeleteHandle( ICPStereoHandleT **handle )
{
    if( *handle == NULL ) return -1;

    free( *handle );
    *handle = NULL;

    return 0;
}


int icpStereoSetMatXcl2Ul( ICPStereoHandleT *handle, ARdouble matXcl2Ul[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matXcl2Ul[j][i] = matXcl2Ul[j][i];
    }
    return 0;
}

int icpStereoSetMatXcr2Ur( ICPStereoHandleT *handle, ARdouble matXcr2Ur[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matXcr2Ur[j][i] = matXcr2Ur[j][i];
    }
    return 0;
}

int icpStereoSetMatC2L( ICPStereoHandleT *handle, ARdouble matC2L[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matC2L[j][i] = matC2L[j][i];
    }
    return 0;
}

int icpStereoSetMatC2R( ICPStereoHandleT *handle, ARdouble matC2R[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) handle->matC2R[j][i] = matC2R[j][i];
    }
    return 0;
}

int icpStereoSetMaxLoop( ICPStereoHandleT *handle, int maxLoop )
{
    if( handle == NULL ) return -1;

    handle->maxLoop = maxLoop;
    return 0;
}

int icpStereoSetBreakLoopErrorThresh( ICPStereoHandleT *handle, ARdouble breakLoopErrorThresh )
{
    if( handle == NULL ) return -1;

    handle->breakLoopErrorThresh = breakLoopErrorThresh;
    return 0;
}

int icpStereoSetBreakLoopErrorRatioThresh( ICPStereoHandleT *handle, ARdouble breakLoopErrorRatioThresh )
{
    if( handle == NULL ) return -1;

    handle->breakLoopErrorRatioThresh = breakLoopErrorRatioThresh;
    return 0;
}

int icpStereoGetMatXcl2Ul( ICPStereoHandleT *handle, ARdouble matXcl2Ul[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matXcl2Ul[j][i] = handle->matXcl2Ul[j][i];
    }
    return 0;
}

int icpStereoGetMatXcr2Ur( ICPStereoHandleT *handle, ARdouble matXcr2Ur[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matXcr2Ur[j][i] = handle->matXcr2Ur[j][i];
    }
    return 0;
}

int icpStereoGetMatC2L( ICPStereoHandleT *handle, ARdouble matC2L[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matC2L[j][i] = handle->matC2L[j][i];
    }
    return 0;
}

int icpStereoGetMatC2R( ICPStereoHandleT *handle, ARdouble matC2R[3][4] )
{
    int     i, j;

    if( handle == NULL ) return -1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) matC2R[j][i] = handle->matC2R[j][i];
    }
    return 0;
}

int icpStereoGetMaxLoop( ICPStereoHandleT *handle, int *maxLoop )
{
    if( handle == NULL ) return -1;

    *maxLoop = handle->maxLoop;
    return 0;
}

int icpStereoGetBreakLoopErrorThresh( ICPStereoHandleT *handle, ARdouble *breakLoopErrorThresh )
{
    if( handle == NULL ) return -1;

    *breakLoopErrorThresh = handle->breakLoopErrorThresh;
    return 0;
}

int icpStereoGetBreakLoopErrorRatioThresh( ICPStereoHandleT *handle, ARdouble *breakLoopErrorRatioThresh )
{
    if( handle == NULL ) return -1;

    *breakLoopErrorRatioThresh = handle->breakLoopErrorRatioThresh;
    return 0;
}

int icpStereoGetInlierProbability( ICPStereoHandleT *handle, ARdouble *inlierProb )
{
    if( handle == NULL ) return -1;

    *inlierProb = handle->inlierProb;
    return 0;
}

int icpStereoSetInlierProbability( ICPStereoHandleT *handle, ARdouble inlierProb )
{
    if( handle == NULL ) return -1;

    handle->inlierProb = inlierProb;
    return 0;
}
