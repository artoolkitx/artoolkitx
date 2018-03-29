/*
 *  ar3DCreateHandle.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
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
 * Revision: 1.1
 * Date: 03/08/14
 *
 *******************************************************/

#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>

AR3DHandle *ar3DCreateHandle(const ARParam *arParam)
{
    return ar3DCreateHandle2( arParam->mat );
}

AR3DHandle *ar3DCreateHandle2(const ARdouble cpara[3][4])
{
    AR3DHandle   *handle;

    arMalloc( handle, AR3DHandle, 1 );

    handle->icpHandle = icpCreateHandle( cpara );
    if( handle->icpHandle == NULL ) {
        free( handle );
        return NULL;
    }

    return handle;
}

int ar3DDeleteHandle( AR3DHandle **handle )
{
    if( *handle == NULL ) return -1;

    icpDeleteHandle( &((*handle)->icpHandle) );
    free( *handle );
    *handle = NULL;

    return 0;
}

int ar3DChangeCpara( AR3DHandle *handle, const ARdouble cpara[3][4] )
{
    return icpSetMatXc2U( handle->icpHandle, cpara );
}


int ar3DChangeMaxLoopCount( AR3DHandle *handle, int maxLoopCount )
{
    return icpSetMaxLoop( handle->icpHandle, maxLoopCount );
}

int ar3DChangeLoopBreakThresh( AR3DHandle *handle, ARdouble loopBreakThresh )
{
    return icpSetBreakLoopErrorThresh( handle->icpHandle, loopBreakThresh );
}

int ar3DChangeLoopBreakThreshRatio( AR3DHandle *handle, ARdouble loopBreakThreshRatio )
{
    return icpSetBreakLoopErrorRatioThresh( handle->icpHandle, loopBreakThreshRatio );
}




AR3DStereoHandle *ar3DStereoCreateHandle(const ARParam *arParamL, const ARParam *arParamR,
                                         const ARdouble transL[3][4], const ARdouble transR[3][4])
{
    return ar3DStereoCreateHandle2( arParamL->mat, arParamR->mat, transL, transR );
}

AR3DStereoHandle *ar3DStereoCreateHandle2(const ARdouble cparaL[3][4], const ARdouble cparaR[3][4],
                                          const ARdouble transL[3][4], const ARdouble transR[3][4])
{
    AR3DStereoHandle   *handle;

    arMalloc( handle, AR3DStereoHandle, 1 );

    handle->icpStereoHandle = icpStereoCreateHandle(cparaL, cparaR, transL, transR);
    if( handle->icpStereoHandle == NULL ) {
        free( handle );
        return NULL;
    }

    return handle;
}

int ar3DStereoDeleteHandle( AR3DStereoHandle **handle )
{
    if( *handle == NULL ) return -1;

    icpStereoDeleteHandle( &((*handle)->icpStereoHandle) );
    free( *handle );
    *handle = NULL;

    return 0;
}

int ar3DStereoChangeMaxLoopCount( AR3DStereoHandle *handle, int maxLoopCount )
{
    return icpStereoSetMaxLoop( handle->icpStereoHandle, maxLoopCount );
}

int ar3DStereoChangeLoopBreakThresh( AR3DStereoHandle *handle, ARdouble loopBreakThresh )
{
    return icpStereoSetBreakLoopErrorThresh( handle->icpStereoHandle, loopBreakThresh );
}

int ar3DStereoChangeLoopBreakThreshRatio( AR3DStereoHandle *handle, ARdouble loopBreakThreshRatio )
{
    return icpStereoSetBreakLoopErrorRatioThresh( handle->icpStereoHandle, loopBreakThreshRatio );
}

int ar3DStereoChangeCpara( AR3DStereoHandle *handle, ARdouble cparaL[3][4], ARdouble cparaR[3][4] )
{
    if( icpStereoSetMatXcl2Ul(handle->icpStereoHandle, cparaL) < 0 ) return -1;
    if( icpStereoSetMatXcr2Ur(handle->icpStereoHandle, cparaR) < 0 ) return -1;
    return 0;
}

int ar3DStereoChangeTransMat( AR3DStereoHandle *handle, ARdouble transL[3][4], ARdouble transR[3][4] )
{
    if( icpStereoSetMatC2L(handle->icpStereoHandle, transL) < 0 ) return -1;
    if( icpStereoSetMatC2R(handle->icpStereoHandle, transR) < 0 ) return -1;
    return 0;
}
