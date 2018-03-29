/*
 *	icpCore.h
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

#ifndef ICP_CORE_H
#define ICP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef AR_STATIC
#    define ICP_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define ICP_EXTERN __declspec(dllexport)
#    else
#      define ICP_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define ICP_EXTERN
#endif

#ifndef  M_PI
#define  M_PI    3.14159265358979323846F
#endif

#define      ICP_DEBUG                           0
#define      ICP_MAX_LOOP                        10
#define      ICP_BREAK_LOOP_ERROR_THRESH         0.1F
#define      ICP_BREAK_LOOP_ERROR_RATIO_THRESH   0.99F
#define      ICP_BREAK_LOOP_ERROR_THRESH2        4.0F
#define      ICP_INLIER_PROBABILITY              0.50F

typedef struct {
    ARdouble    x;
    ARdouble    y;
} ICP2DCoordT;

typedef struct {
    ARdouble    x;
    ARdouble    y;
    ARdouble    z;
} ICP3DCoordT;

typedef struct {
    ARdouble    a;
    ARdouble    b;
    ARdouble    c;
} ICP2DLineT;

typedef struct {
    ICP2DCoordT  p1;
    ICP2DCoordT  p2;
} ICP2DLineSegT;

typedef struct {
    ICP3DCoordT  p1;
    ICP3DCoordT  p2;
} ICP3DLineSegT;


int        icpGetXc_from_Xw_by_MatXw2Xc( ICP3DCoordT *Xc, ARdouble matXw2Xc[3][4], ICP3DCoordT *Xw );
int        icpGetU_from_X_by_MatX2U( ICP2DCoordT *u, ARdouble matX2U[3][4], ICP3DCoordT *coord3d );
int        icpGetJ_U_S( ARdouble J_U_S[2][6], ARdouble matXc2U[3][4], ARdouble matXw2Xc[3][4], ICP3DCoordT *worldCoord );
int        icpGetDeltaS( ARdouble S[6], ARdouble dU[], ARdouble J_U_S[][6], int n );
int        icpUpdateMat( ARdouble matXw2Xc[3][4], ARdouble dS[6] );

#if ICP_DEBUG
void       icpDispMat( char *title, ARdouble *mat, int row, int clm );
#endif

#ifdef __cplusplus
}
#endif
#endif
