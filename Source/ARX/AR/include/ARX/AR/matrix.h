/*
 *	matrix.h
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR_MATRIX_H
#define AR_MATRIX_H

#include <math.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef AR_STATIC
#    define ARMATRIX_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define ARMATRIX_EXTERN __declspec(dllexport)
#    else
#      define ARMATRIX_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define ARMATRIX_EXTERN
#endif

/* === matrix definition ===

  <---- clm --->
  [ 10  20  30 ] ^
  [ 20  10  15 ] |
  [ 12  23  13 ] row
  [ 20  10  15 ] |
  [ 13  14  15 ] v

=========================== */

typedef struct {
	ARdouble *m;
	int      row;
	int      clm;
} ARMat;

#ifdef ARDOUBLE_IS_FLOAT
#  define ARMatf ARMat
#else
typedef struct {
	float   *m;
	int      row;
	int      clm;
} ARMatf;
#endif
    
typedef struct {
    ARdouble *v;
    int      clm;
} ARVec;


/* 0 origin */
#define ARELEM0(mat,r,c) ((mat)->m[(r)*((mat)->clm)+(c)])
/* 1 origin */
#define ARELEM1(mat,row,clm) ARELEM0(mat,row-1,clm-1)



ARMATRIX_EXTERN ARMat  *arMatrixAlloc(int row, int clm);
ARMATRIX_EXTERN int    arMatrixFree(ARMat *m);
#ifdef ARDOUBLE_IS_FLOAT
#  define arMatrixAllocf arMatrixAlloc
#  define arMatrixFreef arMatrixFree
#else
ARMATRIX_EXTERN ARMatf *arMatrixAllocf(int row, int clm);
int    arMatrixFreef(ARMatf *m);
#endif

ARMATRIX_EXTERN int    arMatrixDup(ARMat *dest, ARMat *source);
ARMATRIX_EXTERN ARMat  *arMatrixAllocDup(ARMat *source);

ARMATRIX_EXTERN int    arMatrixUnit(ARMat *unit);
ARMATRIX_EXTERN ARMat  *arMatrixAllocUnit(int dim);

ARMATRIX_EXTERN int    arMatrixMul(ARMat *dest, ARMat *a, ARMat *b);
ARMATRIX_EXTERN ARMat  *arMatrixAllocMul(ARMat *a, ARMat *b);

#ifdef ARDOUBLE_IS_FLOAT
#  define arMatrixMulf arMatrixMul
#  define arMatrixAllocMulf arMatrixAllocMul
#else
ARMATRIX_EXTERN int    arMatrixMulf(ARMatf *dest, ARMatf *a, ARMatf *b);
ARMATRIX_EXTERN ARMatf *arMatrixAllocMulf(ARMatf *a, ARMatf *b);
#endif

ARMATRIX_EXTERN int    arMatrixTrans(ARMat *dest, ARMat *source); // Transpose source, place result in dest.
ARMATRIX_EXTERN ARMat  *arMatrixAllocTrans(ARMat *source);        // Transpose source, place result in newly allocated matrix.

#ifdef ARDOUBLE_IS_FLOAT
#  define arMatrixTransf arMatrixTrans
#  define arMatrixAllocTransf arMatrixAllocTrans
#else
ARMATRIX_EXTERN int    arMatrixTransf(ARMatf *dest, ARMatf *source);
ARMATRIX_EXTERN ARMatf *arMatrixAllocTransf(ARMatf *source);
#endif

ARMATRIX_EXTERN int    arMatrixInv(ARMat *dest, ARMat *source);
ARMATRIX_EXTERN int    arMatrixSelfInv(ARMat *m);
ARMATRIX_EXTERN ARMat  *arMatrixAllocInv(ARMat *source);

#ifdef ARDOUBLE_IS_FLOAT
#  define arMatrixSelfInvf arMatrixSelfInv
#else
ARMATRIX_EXTERN int    arMatrixSelfInvf(ARMatf *m);
#endif

ARMATRIX_EXTERN ARdouble arMatrixDet(ARMat *m);

ARMATRIX_EXTERN int    arMatrixPCA( ARMat *input, ARMat *evec, ARVec *ev, ARVec *mean );
ARMATRIX_EXTERN int    arMatrixPCA2( ARMat *input, ARMat *evec, ARVec *ev );

ARMATRIX_EXTERN int    arMatrixDisp(ARMat *m);


ARVec  *arVecAlloc( int clm );
int    arVecFree( ARVec *v );
int    arVecDisp( ARVec *v );
ARdouble arVecHousehold( ARVec *x );
ARdouble arVecInnerproduct( ARVec *x, ARVec *y );
int    arVecTridiagonalize( ARMat *a, ARVec *d, ARVec *e );


#ifdef __cplusplus
}
#endif
#endif
