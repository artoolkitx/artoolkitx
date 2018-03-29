/*
 *  mSelfInv.c
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
#include <ARX/AR/ar.h>
#ifdef ARDOUBLE_IS_FLOAT
#  define ZERO 0.0f
#  define ONE 1.0f
#  define FABS fabsf
#  define EPS 1.0e-10f
#else
#  define ZERO 0.0
#  define ONE 1.0
#  define FABS fabs
#  define EPS 1.0e-10
#endif

static ARdouble *minv( ARdouble *ap, int dimen, int rowa );

int arMatrixSelfInv(ARMat *m)
{
	if(minv(m->m, m->row, m->row) == NULL) return -1;

	return 0;
}

	
/********************************/
/*                              */
/*    MATRIX inverse function   */
/*                              */
/********************************/
static ARdouble *minv( ARdouble *ap, int dimen, int rowa )
{
        ARdouble *wap, *wcp, *wbp;/* work pointer                 */
        int i, j, n, ip, nwork;
        int nos[500];
        ARdouble p, pbuf, work;

        if ( dimen > 500 ) return NULL;

        switch (dimen) {
                case (0): return(NULL);                 /* check size */
                case (1): *ap = ONE / (*ap);
                          return(ap);                   /* 1 dimension */
        }

        for (n = 0; n < dimen ; n++)
            nos[n] = n;

        for (n = 0; n < dimen ; n++) {
            wcp = ap + n * rowa;
            for (i = n, wap = wcp, p = ZERO, ip = -1; i < dimen; i++, wap += rowa) {
                if ( p < ( pbuf = FABS(*wap)) ) {
                    p = pbuf;
                    ip = i;
                }
            }
            if (p <= EPS || -1 == ip) /* EPS == Threshold value */
                return(NULL);

            nwork = nos[ip];
            nos[ip] = nos[n];
            nos[n] = nwork;

            for (j = 0, wap = ap + ip * rowa, wbp = wcp; j < dimen ; j++) {
                work = *wap;
                *wap++ = *wbp;
                *wbp++ = work;
            }

            for (j = 1, wap = wcp, work = *wcp; j < dimen; j++, wap++) {
                *wap = *(wap + 1) / work;
            }
            *wap = ONE / work;

            for (i = 0; i < dimen ; i++) {
                if (i != n) {
                        wap = ap + i * rowa;
                    for(j = 1, wbp = wcp, work = *wap; j < dimen; j++, wap++, wbp++) {
                                *wap = *(wap + 1) - work * (*wbp);
                    }
                    *wap = -work * (*wbp);
                }
            }
        } //end: for (n = 0; n < dimen ; n++)

        for(n = 0; n < dimen ; n++) {
                for(j = n; j < dimen ; j++)
                        if( nos[j] == n) break;
                nos[j] = nos[n];
                for (i = 0, wap = ap + j, wbp = ap + n; i < dimen; i++, wap += rowa, wbp += rowa) {
                        work = *wap;
                        *wap = *wbp;
                        *wbp = work;
                }
        }
        return(ap);
}

#ifndef ARDOUBLE_IS_FLOAT
static float *minvf( float *ap, int dimen, int rowa );

int arMatrixSelfInvf(ARMatf *m)
{
	if(minvf(m->m, m->row, m->row) == NULL) return -1;
    
	return 0;
}

static float *minvf( float *ap, int dimen, int rowa )
{
    const float EPSL = 1.0e-10f;
    float *wap, *wcp, *wbp;/* work pointer */
    int i, j, n, ip, nwork;
    int nos[500];
    float p,pbuf,work;
    
    if ( dimen > 500 ) return NULL;

    switch (dimen) {
        case (0): return(NULL);           /* check size */
        case (1): *ap = 1.0f / (*ap);
            return(ap);                   /* 1 dimension */
    }
    
    for (n = 0; n < dimen; n++) {
        nos[n] = n;
    }
    
    for (n = 0; n < dimen; n++) {
        wcp = ap + n * rowa;
        
        for (i = n, wap = wcp, p = 0.0, ip = -1; i < dimen; i++, wap += rowa)
            if( p < ( pbuf = fabsf(*wap)) ) {
                p = pbuf;
                ip = i;
            }
        if (p <= EPSL || -1 == ip) /* EPSL Threshold value */
            return(NULL);
        
        nwork = nos[ip];
        nos[ip] = nos[n];
        nos[n] = nwork;
        
        for (j = 0, wap = ap + ip * rowa, wbp = wcp; j < dimen; j++) {
            work = *wap;
            *wap++ = *wbp;
            *wbp++ = work;
        }
        
        for (j = 1, wap = wcp, work = *wcp; j < dimen ; j++, wap++) {
            *wap = *(wap + 1) / work;
        }
        *wap = 1.0f / work;
        
        for (i = 0; i < dimen; i++) {
            if (i != n) {
                wap = ap + i * rowa;
                for(j = 1, wbp = wcp, work = *wap;
                    j < dimen ; j++, wap++, wbp++)
                    *wap = *(wap + 1) - work * (*wbp);
                *wap = -work * (*wbp);
            }
        }
    } //end: for (n = 0; n < dimen; n++)
    
    for(n = 0; n < dimen ; n++) {
        for(j = n; j < dimen ; j++)
            if( nos[j] == n) break;
        nos[j] = nos[n];
        for(i = 0, wap = ap + j, wbp = ap + n; i < dimen ;
            i++, wap += rowa, wbp += rowa) {
            work = *wap;
            *wap = *wbp;
            *wbp = work;
        }
    }
    return(ap);
}
#endif
