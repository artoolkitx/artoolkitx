/*
 *  kpmType.h
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

#pragma once

#if !BINARY_FEATURE
#include <ARX/KPM/surfSub.h>
#endif

//-------------------------------------------------------------------------
// Corner points
//-------------------------------------------------------------------------

#define   MAX_CORNER_POINTS   2000

typedef struct _Point2f {
	float x;
	float y;
} Point2f;

typedef struct _Point2i {
	int x;
	int y;
} Point2i;

typedef struct _CornerPoints {
        int      num;
        Point2i  pt[MAX_CORNER_POINTS];
} CornerPoints;

//-------------------------------------------------------------------------
// Correspondence
//-------------------------------------------------------------------------

typedef struct _MatchPoint {
	float       x1, y1; // Coordinates of corner points in an input image
	float       x2, y2; // Coordinates of corner points in a reference image
} MatchPoint;

typedef struct _CorspMap {
	int          num; // Same as the number of corner points in the input image
	MatchPoint  *mp;
} CorspMap;

#if BINARY_FEATURE

#define    FREAK_SUB_DIMENSION             96

//FREAK feature vector
typedef struct _FreakFeature {
    unsigned char    v[FREAK_SUB_DIMENSION];
    float              angle;
    float              scale;
    int               maxima;
} FreakFeature;
#else
//-------------------------------------------------------------------------
// Surf feature vector
//-------------------------------------------------------------------------

typedef struct _SurfFeature {
	float            v[SURF_SUB_DIMENSION];
    int              l;
} SurfFeature;
#endif

typedef struct _FeatureVector {
	int              num;
#if BINARY_FEATURE
    FreakFeature *sf;
#else
    SurfFeature     *sf;
#endif
    
} FeatureVector;

