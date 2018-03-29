/*
 *  kpmPrivate.h
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
 *  Author(s): Philip Lamb
 *
 */

#ifndef __kpmPrivate_h__
#define __kpmPrivate_h__

#if BINARY_FEATURE
#include <facade/visual_database_facade.h>
#else
#include <ARX/KPM/surfSub.h>
#endif
#define DB_IMAGE_MAX 1024
#if !BINARY_FEATURE
typedef struct {
    SurfSubSkipRegion    *region;
    int                   regionNum;
    int                   regionMax;
} KpmSkipRegionSet;

typedef struct {
    void                     *ann;
    int                      *annCoordIndex;
    int                       pageID;
    int                       imageID;
} KpmAnnInfo;
#endif

struct _KpmHandle {
#if !BINARY_FEATURE
    SurfSubHandleT           *surfHandle;
    void                     *ann2;
#else
    vision::VisualDatabaseFacade *freakMatcher;
#endif
    
    ARParamLT                *cparamLT;
    int                       poseMode;
    int                       xsize, ysize;
    KPM_PROC_MODE             procMode;
    int                       detectedMaxFeature;
#if !BINARY_FEATURE
    int                       surfThreadNum;
#endif
    
    KpmRefDataSet             refDataSet;
    KpmInputDataSet           inDataSet;
#if !BINARY_FEATURE
    KpmMatchResult            preRANSAC;
    KpmMatchResult            aftRANSAC;
#endif
    
#if !BINARY_FEATURE
    KpmSkipRegionSet          skipRegion;
#endif
    
    KpmResult                *result;
    int                       resultNum;
    int                       pageIDs[DB_IMAGE_MAX];
};

#endif // !__kpmPrivate_h__
