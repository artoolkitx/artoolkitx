/*
 *	icp.h
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */

#ifndef ICP_H
#define ICP_H

#include <stdio.h>
#include <ARX/AR/icpCore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define   ICP_TRANS_MAT_IDENTITY        NULL


/*
 *  Point Data
 */
typedef struct {
    ICP2DCoordT    *screenCoord;
    ICP3DCoordT    *worldCoord;
    int             num;
} ICPDataT;

typedef struct {
    ICP2DCoordT    *screenCoordL;
    ICP3DCoordT    *worldCoordL;
    int             numL;
    ICP2DCoordT    *screenCoordR;
    ICP3DCoordT    *worldCoordR;
    int             numR;
} ICPStereoDataT;



/*
 *  Handle
 */
typedef struct {
    ARdouble     matXc2U[3][4];
    int        maxLoop;
    ARdouble     breakLoopErrorThresh;
    ARdouble     breakLoopErrorRatioThresh;
    ARdouble     breakLoopErrorThresh2;
    ARdouble     inlierProb;
} ICPHandleT;

typedef struct {
    ARdouble     matXcl2Ul[3][4];
    ARdouble     matXcr2Ur[3][4];
    ARdouble     matC2L[3][4];
    ARdouble     matC2R[3][4];
    int        maxLoop;
    ARdouble     breakLoopErrorThresh;
    ARdouble     breakLoopErrorRatioThresh;
    ARdouble     breakLoopErrorThresh2;
    ARdouble     inlierProb;
} ICPStereoHandleT;




/*------------ icpUtil.c --------------*/
int icpGetInitXw2Xc_from_PlanarData( ARdouble matXc2U[3][4], ICP2DCoordT screenCoord[], ICP3DCoordT worldCoord[], int num, ARdouble initMatXw2Xc[3][4] );


/*------------ icpPoint.c --------------*/
ICPHandleT        *icpCreateHandle                 ( const ARdouble matXc2U[3][4] );
int                icpDeleteHandle                 ( ICPHandleT **handle );
int                icpSetMatXc2U                   ( ICPHandleT *handle, const ARdouble matXc2U[3][4] );
int                icpGetMatXc2U                   ( ICPHandleT *handle, ARdouble matXc2U[3][4] );
int                icpSetMaxLoop                   ( ICPHandleT *handle, int  maxLoop );
int                icpGetMaxLoop                   ( ICPHandleT *handle, int *maxLoop );
int                icpSetBreakLoopErrorThresh      ( ICPHandleT *handle, ARdouble  breakLoopErrorThresh );
int                icpGetBreakLoopErrorThresh      ( ICPHandleT *handle, ARdouble *breakLoopErrorThresh );
int                icpSetBreakLoopErrorRatioThresh ( ICPHandleT *handle, ARdouble  breakLoopErrorThresh );
int                icpGetBreakLoopErrorRatioThresh ( ICPHandleT *handle, ARdouble *breakLoopErrorThresh );
int                icpSetBreakLoopErrorThresh2     ( ICPHandleT *handle, ARdouble  breakLoopErrorThresh2 );
int                icpGetBreakLoopErrorThresh2     ( ICPHandleT *handle, ARdouble *breakLoopErrorThresh2 );
int                icpSetInlierProbability         ( ICPHandleT *handle, ARdouble  inlierProbability );
ICP_EXTERN int                icpGetInlierProbability         ( ICPHandleT *handle, ARdouble *inlierProbability );
int                icpPoint                        ( ICPHandleT *handle, ICPDataT *data, ARdouble initMatXw2Xc[3][4], ARdouble matXw2Xc[3][4], ARdouble *err );
int                icpPointRobust                  ( ICPHandleT *handle, ICPDataT *data, ARdouble initMatXw2Xc[3][4], ARdouble matXw2Xc[3][4], ARdouble *err );


/*------------ icpPointStereo.c --------------*/
ICPStereoHandleT  *icpStereoCreateHandle                 ( const ARdouble matXcl2Ul[3][4], const ARdouble matXcr2Ur[3][4], const ARdouble matC2L[3][4], const ARdouble matC2R[3][4] );
int                icpStereoDeleteHandle                 ( ICPStereoHandleT **handle );
int                icpStereoSetMatXcl2Ul                 ( ICPStereoHandleT *handle, ARdouble matXcl2Ul[3][4] );
int                icpStereoSetMatXcr2Ur                 ( ICPStereoHandleT *handle, ARdouble matXcr2Ur[3][4] );
int                icpStereoGetMatXcl2Ul                 ( ICPStereoHandleT *handle, ARdouble matXcl2Ul[3][4] );
int                icpStereoGetMatXcr2Ur                 ( ICPStereoHandleT *handle, ARdouble matXcr2Ur[3][4] );
int                icpStereoSetMatC2L                    ( ICPStereoHandleT *handle, ARdouble matC2L[3][4] );
int                icpStereoSetMatC2R                    ( ICPStereoHandleT *handle, ARdouble matC2R[3][4] );
int                icpStereoGetMatC2L                    ( ICPStereoHandleT *handle, ARdouble matC2L[3][4] );
int                icpStereoGetMatC2R                    ( ICPStereoHandleT *handle, ARdouble matC2R[3][4] );
int                icpStereoSetMaxLoop                   ( ICPStereoHandleT *handle, int  maxLoop );
int                icpStereoGetMaxLoop                   ( ICPStereoHandleT *handle, int *maxLoop );
int                icpStereoSetBreakLoopErrorThresh      ( ICPStereoHandleT *handle, ARdouble  breakLoopErrorThresh );
int                icpStereoGetBreakLoopErrorThresh      ( ICPStereoHandleT *handle, ARdouble *breakLoopErrorThresh );
int                icpStereoSetBreakLoopErrorRatioThresh ( ICPStereoHandleT *handle, ARdouble  breakLoopErrorThresh );
int                icpStereoGetBreakLoopErrorRatioThresh ( ICPStereoHandleT *handle, ARdouble *breakLoopErrorThresh );
int                icpStereoSetBreakLoopErrorThresh2     ( ICPStereoHandleT *handle, ARdouble  breakLoopErrorThresh2 );
int                icpStereoGetBreakLoopErrorThresh2     ( ICPStereoHandleT *handle, ARdouble *breakLoopErrorThresh2 );
int                icpStereoSetInlierProbability         ( ICPStereoHandleT *handle, ARdouble  inlierProbability );
int                icpStereoGetInlierProbability         ( ICPStereoHandleT *handle, ARdouble *inlierProbability );
int                icpStereoPoint                        ( ICPStereoHandleT *handle, ICPStereoDataT *data, ARdouble initMatXw2Xc[3][4], ARdouble matXw2Xc[3][4], ARdouble *err );
int                icpStereoPointRobust                  ( ICPStereoHandleT *handle, ICPStereoDataT *data, ARdouble initMatXw2Xc[3][4], ARdouble matXw2Xc[3][4], ARdouble *err );


#if 0
int                icpRobustGetXw2Xc( ICPHandleT   *handle,
                                      ICPDataT     *data,
                                      ARdouble        initMatXw2Xc[3][4],
                                      ARdouble        matXw2Xc[3][4],
                                      ARdouble       *err );

int                icpGetXw2XcFromLineAndPoint( ICPHandleT       *handle, 
                                                ICPLinesDataT    *linesData,
                                                ICPDataT         *pointData,
                                                ARdouble            initMatXw2Xc[3][4],
                                                ARdouble            matXw2Xc[3][4],
                                                ARdouble           *err );


int                icpStereoCheckErr( ARdouble S[6], ARdouble matXcl2Ul[3][4], ARdouble matXcl2Ur[3][4], ICPStereoDataT *data, ARdouble *err );
int                icpRobustCheckErr( ARdouble S[6], ARdouble matXc2U[3][4], ICPDataT *data, ARdouble *weight, ARdouble *err );
int                icpRobustUpdateWeight( ARdouble S[6], ARdouble matXc2U[3][4], ICPDataT *data, ARdouble sd2, ARdouble *weight );


int                icpGetJ_E_S( ARdouble J_E_S[6], ICP2DLineT *line, ICP2DLineSegT *lineSeg, ARdouble matXc2U[3][4], ARdouble q[7], ARdouble s[6],
                                ICP3DLineSegT *cameraCoord, ICP3DLineSegT *worldCoord, ICP2DCoordT *observedScreenCoord );
int                icpGetJ_E_L( ARdouble J_E_L[3], ICP2DLineT *line, ICP2DCoordT *screenCoord );
int                icpGetJ_L_X( ARdouble J_L_X[3][4], ICP2DLineSegT *lineSeg );
int                icpGetLine( ICP2DLineT *line, ICP2DLineSegT *lineSeg );
int                icpGetdE( ARdouble *dE, ICP2DLineT *line, ICP2DCoordT *screenCoord );
int                icpLineCheckErr( ARdouble S[6], ARdouble matXc2U[3][4], ICPLinesDataT *data,  ARdouble *err );
#endif



#ifdef __cplusplus
}
#endif
#endif
