/*
 *  AR2/featureSet.h
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

#ifndef AR2_FEATURE_SET_H
#define AR2_FEATURE_SET_H
#include <ARX/AR2/config.h>
#include <ARX/AR2/imageSet.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float   *map;
    int     xsize;
    int     ysize;
} AR2FeatureMapT;

typedef struct {
    int               x;
    int               y;
    float             mx;
    float             my;
    float             maxSim;
} AR2FeatureCoordT;

// One AR2FeaturePointsT holds the feature coordinates for one scalefactor of one image.
typedef struct {
    AR2FeatureCoordT  *coord;
    int               num;
    int               scale;
    float             maxdpi;
    float             mindpi;
} AR2FeaturePointsT;

// Structure to hold a set of one or more AR2FeaturePointsT structures for one image.
typedef struct {
    AR2FeaturePointsT *list;
    int               num;
} AR2FeatureSetT;


AR2_EXTERN AR2FeatureMapT *ar2GenFeatureMap( AR2ImageT *image,
                                  int ts1, int ts2,
                                  int search_size1, int search_size2,
                                  float  max_sim_thresh, float  sd_thresh );

AR2_EXTERN AR2FeatureMapT *ar2ReadFeatureMap( char *filename, char *ext );

AR2_EXTERN int ar2SaveFeatureMap( char *filename, char *ext, AR2FeatureMapT *featureMap );

AR2_EXTERN int ar2FreeFeatureMap( AR2FeatureMapT *featureMap );


AR2_EXTERN int ar2PrintFeatureInfo( AR2ImageT *image, AR2FeatureMapT *featureMap, int ts1, int ts2, int search_size2, int cx, int cy );

AR2_EXTERN AR2FeatureCoordT *ar2SelectFeature( AR2ImageT *image, AR2FeatureMapT *featureMap,
                                    int ts1, int ts2, int search_size2, int occ_size,
                                    float  max_sim_thresh, float  min_sim_thresh, float  sd_thresh, int *num );

AR2_EXTERN AR2FeatureCoordT *ar2SelectFeature2( AR2ImageT *image, AR2FeatureMapT *featureMap,
                                     int ts1, int ts2, int search_size2, int occ_size,
                                     float  max_sim_thresh, float  min_sim_thresh, float  sd_thresh, int *num );

AR2_EXTERN AR2FeatureSetT *ar2ReadFeatureSet( const char *filename, const char *ext );
AR2_EXTERN int             ar2SaveFeatureSet( const char *filename, const char *ext, AR2FeatureSetT *featureSet );
AR2_EXTERN int             ar2FreeFeatureSet( AR2FeatureSetT **featureSet );

#ifdef __cplusplus
}
#endif
#endif
