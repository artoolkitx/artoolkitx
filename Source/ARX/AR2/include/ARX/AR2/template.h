/*
 *  AR2/template.h
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

#ifndef AR2_TEMPLATE_H
#define AR2_TEMPLATE_H
#include <ARX/AR/ar.h>
#include <ARX/AR2/config.h>
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/featureSet.h>

#ifdef __cplusplus
extern "C" {
#endif


#define  AR2_TEMPLATE_NULL_PIXEL     0x1000


typedef struct {
    int          xsize, ysize;      /* template size         */
    int          xts1, xts2;        /* template size         */
    int          yts1, yts2;        /* template size         */
    ARUint16    *img1;              /* template for mode 0   */
    int          vlen;              /* length of vector *img */
    int          sum;
    int          validNum;
} AR2TemplateT;

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
typedef struct {
    int          xsize, ysize;      /* template size         */
    int          xts1, xts2;        /* template size         */
    int          yts1, yts2;        /* template size         */
    ARUint16    *img1[3];           /* template for mode 0   */
    int          vlen[3];           /* length of vector *img */
    int          sum[3];
    int          validNum;
} AR2Template2T;
#endif


typedef struct {
    int     snum;
    int     level;
    int     num;
    int     flag;                   // Set to -1 to indicate that this candidate is the last candidate in the list/array (i.e. candidate count has reached AR2_TRACKING_CANDIDATE_MAX). 0 to indicate a valid candidate. Set to 1 to indicate that this template has been chosen as a candidate.
    float   sx, sy;
} AR2TemplateCandidateT;



#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
AR_EXTERN AR2TemplateT  *ar2GenTemplate  ( int ts1, int ts2 );
AR_EXTERN AR2Template2T *ar2GenTemplate2 ( int ts1, int ts2 );
AR_EXTERN int            ar2FreeTemplate ( AR2TemplateT  *templ  );
AR_EXTERN int            ar2FreeTemplate2( AR2Template2T *templ2 );

AR_EXTERN int ar2SetTemplateSub ( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                        AR2FeaturePointsT *featurePoints, int num, int blurLevel,
                        AR2TemplateT *templ );
AR_EXTERN int ar2SetTemplate2Sub( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                        AR2FeaturePointsT *featurePoints, int num, int blurLevel,
                        AR2Template2T *templ2 );
#else
AR_EXTERN AR2TemplateT  *ar2GenTemplate ( int ts1, int ts2 );
AR_EXTERN int            ar2FreeTemplate( AR2TemplateT  *templ  );

AR_EXTERN int ar2SetTemplateSub ( const ARParamLT *cparamLT, const float  trans[3][4], AR2ImageSetT *imageSet,
                        AR2FeaturePointsT *featurePoints, int num,
                        AR2TemplateT *templ );
#endif


AR_EXTERN int ar2GetBestMatching ( ARUint8 *img, ARUint8 *mfImage, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                         AR2TemplateT *mtemp, int rx, int ry,
                         int search[3][2], int *bx, int *by, float *val);

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
AR_EXTERN int ar2GetBestMatching2( ARUint8 *img, ARUint8 *mfImage, int xsize, int ysize, AR_PIXEL_FORMAT pixFormat,
                         AR2Template2T *mtemp, int rx, int ry,
                         int search[3][2], int *bx, int *by, float *val, int *blurLevel);
#else
AR_EXTERN int ar2GetBestMatching2(void);
#endif

AR_EXTERN int ar2GetResolution( const ARParamLT *cparamLT, const float  trans[3][4], const float  pos[2], float  dpi[2] );
AR_EXTERN int ar2GetResolution2( const ARParam *cparam, const float  trans[3][4], const float  pos[2], float  dpi[2] );

// Returns -1 if no template selected, otherwise returns the 0-based index of the selected template
// and sets 'flag' of the template to 1.
AR_EXTERN int ar2SelectTemplate( AR2TemplateCandidateT *candidate, AR2TemplateCandidateT *prevFeature, int num,
                       float  pos[4][2], int xsize, int ysize );



#ifdef __cplusplus
}
#endif
#endif
