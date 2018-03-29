/*
 *  mAlloc.c
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
 *  Author(s): Shinsaku Hiura, Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Shinsaku Hiura, Hirokazu Kato
 *
 *         shinsaku@sys.es.osaka-u.ac.jp
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 2.1
 * Date: 99/07/16
 *
 *******************************************************/

#include <stdio.h>
#include <math.h>
#ifndef __APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <ARX/AR/ar.h>

ARMat *arMatrixAlloc(int row, int clm)
{
	ARMat *m;

	m = (ARMat *)malloc(sizeof(ARMat));
	if( m == NULL ) return NULL;

	m->m = (ARdouble *)malloc(sizeof(ARdouble) * row * clm);
	if(m->m == NULL) {
		free(m);
		return NULL;
	}
	else {
		m->row = row;
		m->clm = clm;
	}

	return m;
}

#ifndef ARDOUBLE_IS_FLOAT
ARMatf *arMatrixAllocf(int row, int clm)
{
	ARMatf *m;
    
	m = (ARMatf *)malloc(sizeof(ARMatf));
	if( m == NULL ) return NULL;
    
	m->m = (float *)malloc(sizeof(float) * row * clm);
	if(m->m == NULL) {
		free(m);
		return NULL;
	}
	else {
		m->row = row;
		m->clm = clm;
	}
    
	return m;
}
#endif