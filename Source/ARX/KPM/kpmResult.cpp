/*
 *  kpmResult.cpp
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
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <stdio.h>
#include <ARX/AR/ar.h>
#include <ARX/KPM/kpm.h>
#include "kpmPrivate.h"

int kpmGetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet **refDataSet )
{
    if( kpmHandle == NULL ) return -1;
    if( refDataSet == NULL ) return -1;

    *refDataSet = &(kpmHandle->refDataSet);

    return 0;
}

int kpmGetInDataSet( KpmHandle *kpmHandle, KpmInputDataSet **inDataSet )
{
    if( kpmHandle == NULL ) return -1;
    if( inDataSet == NULL ) return -1;

    *inDataSet = &(kpmHandle->inDataSet);
    return 0;
}

#if !BINARY_FEATURE
int kpmGetMatchingResult( KpmHandle *kpmHandle, KpmMatchResult **preRANSAC, KpmMatchResult **aftRANSAC )
{
    if( kpmHandle == NULL ) return -1;
    if( preRANSAC != NULL ) {
        *preRANSAC = &(kpmHandle->preRANSAC);
    }
    if( aftRANSAC != NULL ) {
        *aftRANSAC = &(kpmHandle->aftRANSAC);
    }

    return 0;
}
#endif

int kpmGetPose( KpmHandle *kpmHandle, float pose[3][4], int *pageNo, float *error )
{
    int     i, j;

    if( kpmHandle == NULL ) return -1;
    if( kpmHandle->refDataSet.pageNum == 0 ) return -1;

    for(int pageLoop= 0; pageLoop < kpmHandle->resultNum; pageLoop++) {
        if( kpmHandle->result[pageLoop].camPoseF == 0 ) {
            for(j=0;j<3;j++) for(i=0;i<4;i++) pose[j][i] = kpmHandle->result[pageLoop].camPose[j][i];
            *pageNo = kpmHandle->result[pageLoop].pageNo;
            *error = kpmHandle->result[pageLoop].error;
            return 0;
        }
    }

    return -1;
}

int kpmGetResult( KpmHandle *kpmHandle, KpmResult **result, int *resultNum )
{
    if( kpmHandle == NULL ) return -1;

    *result = kpmHandle->result;
    *resultNum = kpmHandle->resultNum;

    return 0;
}
