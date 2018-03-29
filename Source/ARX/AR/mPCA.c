/*
 *  mPCA.c
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
 *  Author(s): Shinsaku Hiura, Hirokazu Kato
 *
 */

#include <stdio.h>
#include <math.h>
#include <ARX/AR/ar.h>

#define     VZERO           1e-16
#define     EPS             1e-6
#define     MAX_ITER        100

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#else
#  define SQRT sqrt
#endif

static int EX(ARMat *input, ARVec *mean);
static int CENTER(ARMat *inout, ARVec *mean);
static int PCA(ARMat *input, ARMat *output, ARVec *ev);
static int x_by_xt(ARMat *input, ARMat *output);
static int xt_by_x(ARMat *input, ARMat *output);
static int EV_create(ARMat *input, ARMat *u, ARMat *output, ARVec *ev);
static int QRM(ARMat *u, ARVec *ev);


/* === matrix definition ===

Input:
  <---- clm (Data dimention)--->
  [ 10  20  30 ] ^
  [ 20  10  15 ] |
  [ 12  23  13 ] row
  [ 20  10  15 ] |(Sample number)
  [ 13  14  15 ] v

Evec:
  <---- clm (Eigen vector)--->
  [ 10  20  30 ] ^
  [ 20  10  15 ] |
  [ 12  23  13 ] row
  [ 20  10  15 ] |(Number of egen vec)
  [ 13  14  15 ] v

Ev:
  <---- clm (Number of eigen vector)--->
  [ 10  20  30 ] eigen value

Mean:
  <---- clm (Data dimention)--->
  [ 10  20  30 ] mean value

=========================== */


int arMatrixPCA(ARMat *input, ARMat *evec, ARVec *ev, ARVec *mean)
{
    ARMat     *work;
    ARdouble  srow, sum;
    int     row, clm;
    int     check, rval;
    int     i;

    row = input->row;
    clm = input->clm;
    check = (row < clm)? row: clm;
    if (row < 2 || clm < 2) return (-1);
    if (evec->clm != input->clm || evec->row != check) return (-1);
    if (ev->clm   != check)      return (-1);
    if (mean->clm != input->clm) return (-1);

    work = arMatrixAllocDup(input);
    if (work == NULL) return -1;

    srow = sqrt((ARdouble)row);
    if (EX(work, mean) < 0) {
        arMatrixFree(work);
        return (-1);
    }
    if (CENTER(work, mean) < 0) {
        arMatrixFree(work);
        return (-1);
    }
    for (i=0; i<row*clm; i++) work->m[i] /= srow;

    rval = PCA(work, evec, ev);
    arMatrixFree(work);

    sum = 0.0;
    for (i = 0; i < ev->clm; i++) sum += ev->v[i];
    for (i = 0; i < ev->clm; i++) ev->v[i] /= sum;

    return (rval);
}

int arMatrixPCA2(ARMat *input, ARMat *evec, ARVec *ev)
{
    ARMat   *work;
/*    ARdouble  srow;   */
    ARdouble  sum;
    int     row, clm;
    int     check, rval;
    int     i;

    row = input->row;
    clm = input->clm;
    check = (row < clm)? row: clm;
    if (row < 2 || clm < 2) return (-1);
    if (evec->clm != input->clm || evec->row != check) return (-1);
    if (ev->clm   != check)      return (-1);

    work = arMatrixAllocDup(input);
    if (work == NULL) return -1;

/*
    srow = sqrt((ARdouble)row);
    for(i=0; i<row*clm; i++) work->m[i] /= srow;
*/

    rval = PCA(work, evec, ev);
    arMatrixFree(work);

    sum = 0.0;
    for (i = 0; i < ev->clm; i++) sum += ev->v[i];
    for (i = 0; i < ev->clm; i++) ev->v[i] /= sum;

    return (rval);
}

static int PCA(ARMat *input, ARMat *output, ARVec *ev)
{
    //COVHI10384
    ARMat    *u = NULL;
    ARdouble *m1, *m2;
    int      row, clm, min;
    int      i, j;

    row = input->row;
    clm = input->clm;
    min = (clm < row)? clm: row;
    if (row < 2 || clm < 2)        return (-1);
    if (output->clm != input->clm) return (-1);
    if (output->row != min)        return (-1);
    if (ev->clm != min)            return (-1);

    u = arMatrixAlloc(min, min);
    if (u->row != min || u->clm != min) {
        arMatrixFree(u);
        return (-1);
    }
    if (row < clm) {
        if (x_by_xt(input, u) < 0) {
            arMatrixFree(u);
            return (-1);
        }
    } else {
        if (xt_by_x( input, u) < 0) {
            arMatrixFree(u);
            return (-1);
        }
    }

    if (QRM(u, ev) < 0) {
        arMatrixFree(u);
        return (-1);
    }

    if (row < clm) {
        if (EV_create( input, u, output, ev) < 0) {
            arMatrixFree(u);
            return (-1);
        }
    } else {
        m1 = u->m;
        m2 = output->m;
        for (i = 0; i < min; i++) {
            if (ev->v[i] < VZERO) break;
            for (j = 0; j < min; j++) *(m2++) = *(m1++);
        }
        for (; i < min; i++) {
            ev->v[i] = 0.0;
            for (j = 0; j < min; j++) *(m2++) = 0.0;
        }
    }

    arMatrixFree(u);
    return (0);
}

static int EX(ARMat *input, ARVec *mean)
{
    ARdouble  *m, *v;
    int     row, clm;
    int     i, j;

    row = input->row;
    clm = input->clm;
    if (row <= 0 || clm <= 0)  return (-1);
    if (mean->clm != clm)      return (-1);

    for (i = 0; i < clm; i++) mean->v[i] = 0.0;

    m = input->m;
    for (i = 0; i < row; i++) {
        v = mean->v;
        for (j = 0; j < clm; j++) *(v++) += *(m++);
    }

    for (i = 0; i < clm; i++) mean->v[i] /= row;

    return (0);
}

static int CENTER(ARMat *inout, ARVec *mean)
{
    ARdouble  *m, *v;
    int     row, clm;
    int     i, j;

    row = inout->row;
    clm = inout->clm;
    if (mean->clm != clm) return (-1);

    m = inout->m;
    for (i = 0; i < row; i++) {
        v = mean->v;
        for (j = 0; j < clm; j++) *(m++) -= *(v++);
    }

    return (0);
}

static int x_by_xt(ARMat *input, ARMat *output)
{
    ARdouble  *in1, *in2, *out;
    int     row, clm;
    int     i, j, k;

    row = input->row;
    clm = input->clm;
    if (output->row != row || output->clm != row) return (-1);

    out = output->m;
    for (i = 0; i < row; i++) {
        for (j = 0; j < row; j++) {
            if (j < i) {
                *out = output->m[j*row+i];
            }
            else {
                in1 = &(input->m[clm*i]);
                in2 = &(input->m[clm*j]);
                *out = 0.0;
                for (k = 0; k < clm; k++) {
                    *out += *(in1++) * *(in2++);
                }
            }
            out++;
        }
    }

    return (0);
}

static int xt_by_x(ARMat *input, ARMat *output)
{
    ARdouble  *in1, *in2, *out;
    int     row, clm;
    int     i, j, k;

    row = input->row;
    clm = input->clm;
    if (output->row != clm || output->clm != clm) return (-1);

    out = output->m;
    for (i = 0; i < clm; i++) {
        for (j = 0; j < clm; j++) {
            if (j < i) {
                *out = output->m[j*clm+i];
            }
            else {
                in1 = &(input->m[i]);
                in2 = &(input->m[j]);
                *out = 0.0;
                for (k = 0; k < row; k++) {
                    *out += *in1 * *in2;
                    in1 += clm;
                    in2 += clm;
                }
            }
            out++;
        }
    }

    return (0);
}

static int EV_create(ARMat *input, ARMat *u, ARMat *output, ARVec *ev)
{
    ARdouble  *m, *m1, *m2;
    ARdouble  sum, work;
    int     row, clm;
    int     i, j, k;

    row = input->row;
    clm = input->clm;
    if (row <= 0 || clm <= 0) return (-1);
    if (u->row != row || u->clm != row) return (-1);
    if (output->row != row || output->clm != clm) return (-1);
    if (ev->clm != row) return (-1);

    m = output->m;
    for (i = 0; i < row; i++) {
        if (ev->v[i] < VZERO) break;
        work = 1 / sqrt(fabs(ev->v[i]));
        for (j = 0; j < clm; j++) {
            sum = 0.0;
            m1 = &(u->m[i*row]);
            m2 = &(input->m[j]);
            for (k = 0; k < row; k++) {
                sum += *m1 * *m2;
                m1++;
                m2 += clm;
            }
            *(m++) = sum * work;
        }
    }
    for (; i < row; i++) {
        ev->v[i] = 0.0;
        for (j = 0; j < clm; j++) *(m++) = 0.0;
    }

    return (0);
}

static int QRM(ARMat *a, ARVec *dv)
{
    ARVec     *ev, ev1;
    ARdouble  w, t, s, x, y, c;
    ARdouble  *v1, *v2;
    int     dim, iter;
    int     i, j, k, h;

    dim = a->row;
    if (dim != a->clm || dim < 2) return (-1);
    if (dv->clm != dim) return (-1);

    ev = arVecAlloc(dim);
    if (ev == NULL) return (-1);

    ev1.clm = dim-1;
    ev1.v = &(ev->v[1]);
    if (arVecTridiagonalize(a, dv, &ev1) < 0) {
        arVecFree(ev);
        return (-1);
    }

    ev->v[0] = 0.0;
    for (h = dim-1; h > 0; h--) {
        j = h;
        while (j>0 && fabs(ev->v[j]) > EPS*(fabs(dv->v[j-1])+fabs(dv->v[j]))) j--;
        if (j == h) continue;

        iter = 0;
        do {
            iter++;
            if (iter > MAX_ITER) break;
            
            w = (dv->v[h-1] - dv->v[h]) / 2;
            t = ev->v[h] * ev->v[h];
            s = sqrt(w*w+t);
            if (w < 0) s = -s;
            x = dv->v[j] - dv->v[h] + t/(w+s);
            y = ev->v[j+1];
            for (k = j; k < h; k++) {
                if (fabs(x) >= fabs(y)) {
                    if (fabs(x) > VZERO) {
                        t = -y / x;
                        c = 1 / sqrt(t*t+1);
                        s = t * c;
                    } else{
                        c = 1.0;
                        s = 0.0;
                    }
                } else{
                    t = -x / y;
                    s = 1.0 / sqrt(t*t+1);
                    c = t * s;
                }
                w = dv->v[k] - dv->v[k+1];
                t = (w * s + 2 * c * ev->v[k+1]) * s;
                dv->v[k]   -= t;
                dv->v[k+1] += t;
                if( k > j) ev->v[k] = c * ev->v[k] - s * y;
                ev->v[k+1] += s * (c * w - 2 * s * ev->v[k+1]);
                
                for (i = 0; i < dim; i++) {
                    x = a->m[k*dim+i];
                    y = a->m[(k+1)*dim+i];
                    a->m[k*dim+i]     = c * x - s * y;
                    a->m[(k+1)*dim+i] = s * x + c * y;
                }
                if (k < h-1) {
                    x = ev->v[k+1];
                    y = -s * ev->v[k+2];
                    ev->v[k+2] *= c;
                }
            }
        } while (fabs(ev->v[h]) > EPS*(fabs(dv->v[h-1])+fabs(dv->v[h])));
    }

    for (k = 0; k < dim-1; k++) {
        h = k;
        t = dv->v[h];
        for (i = k+1; i < dim; i++) {
            if (dv->v[i] > t) {
                h = i;
                t = dv->v[h];
            }
        }
        dv->v[h] = dv->v[k];
        dv->v[k] = t;
        v1 = &(a->m[h*dim]);
        v2 = &(a->m[k*dim]);
        for (i = 0; i < dim; i++) {
            w = *v1;
            *v1 = *v2;
            *v2 = w;
            v1++;
            v2++;
        }
    }

    arVecFree(ev);
    return (0);
}
