/*
 *  arFilterTransMat.c
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <ARX/AR/arFilterTransMat.h>

struct _ARFilterTransMatInfo {
    ARdouble alpha;
    ARdouble q[4];
    ARdouble p[3];
};

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
#define CLAMP(x,r1,r2) (MIN(MAX(x,r1),r2))

ARFilterTransMatInfo *arFilterTransMatInit(const ARdouble sampleRate, const ARdouble cutoffFreq)
{
    ARFilterTransMatInfo *ftmi = (ARFilterTransMatInfo *)malloc(sizeof(ARFilterTransMatInfo));
    if (ftmi) {
#ifdef ARDOUBLE_IS_FLOAT
        ftmi->q[0] = 0.0f;
        ftmi->q[1] = 0.0f;
        ftmi->q[2] = 0.0f;
        ftmi->q[3] = 1.0f;
        ftmi->p[0] = 0.0f;
        ftmi->p[1] = 0.0f;
        ftmi->p[2] = 0.0f;
#else
        ftmi->q[0] = 0.0;
        ftmi->q[1] = 0.0;
        ftmi->q[2] = 0.0;
        ftmi->q[3] = 1.0;
        ftmi->p[0] = 0.0;
        ftmi->p[1] = 0.0;
        ftmi->p[2] = 0.0;
#endif
        if (arFilterTransMatSetParams(ftmi, sampleRate, cutoffFreq) < 0) {
            free (ftmi);
            ftmi = NULL;
        }
    }
    return (ftmi);
}

int arFilterTransMatSetParams(ARFilterTransMatInfo *ftmi, const ARdouble sampleRate, const ARdouble cutoffFreq)
{
    ARdouble dt, RC;
 
    if (!ftmi) return (-1);
    if (!sampleRate || !cutoffFreq) return (-2);

#ifdef ARDOUBLE_IS_FLOAT
    dt = 1.0f / sampleRate;
    RC = 1.0f / cutoffFreq;
#else
    dt = 1.0 / sampleRate;
    RC = 1.0 / cutoffFreq;
#endif
    ftmi->alpha = dt / (dt + RC);
    
    return (0);
}

int arFilterTransMat(ARFilterTransMatInfo *ftmi, ARdouble m[3][4], const int reset)
{
    ARdouble q[4], p[3], alpha, oneminusalpha, omega, cosomega, sinomega, s0, s1;
    
    if (!ftmi) return (-1);
    
    if (arUtilMat2QuatPos((const ARdouble (*)[4])m, q, p) < 0) return (-2);
    arUtilQuatNorm(q);
    
    if (reset) {
        ftmi->q[0] = q[0];
        ftmi->q[1] = q[1];
        ftmi->q[2] = q[2];
        ftmi->q[3] = q[3];
        ftmi->p[0] = p[0];
        ftmi->p[1] = p[1];
        ftmi->p[2] = p[2];
    } else {
        alpha = ftmi->alpha;
#ifdef ARDOUBLE_IS_FLOAT
        oneminusalpha = 1.0f - alpha;
#else
        oneminusalpha = 1.0 - alpha;
#endif
        
        // SLERP for orientation.
        cosomega = q[0]*ftmi->q[0] + q[1]*ftmi->q[1] + q[2]*ftmi->q[2] + q[3]*ftmi->q[3]; // cos of angle between vectors.
#ifdef ARDOUBLE_IS_FLOAT
        if (cosomega < 0.0f) {
            cosomega = -cosomega;
            q[0] = -q[0];
            q[1] = -q[1];
            q[2] = -q[2];
            q[3] = -q[3];
        } 
        if (cosomega > 0.9995f) {
            s0 = oneminusalpha;
            s1 = alpha;
        } else {
            omega = acosf(cosomega);
            sinomega = sinf(omega);
            s0 = sinf(oneminusalpha * omega) / sinomega;
            s1 = sinf(alpha * omega) / sinomega;
        }
#else
        if (cosomega < 0.0) {
            cosomega = -cosomega;
            q[0] = -q[0];
            q[1] = -q[1];
            q[2] = -q[2];
            q[3] = -q[3];
        } 
        if (cosomega > 0.9995) {
            s0 = oneminusalpha;
            s1 = alpha;
        } else {
            omega = acos(cosomega);
            sinomega = sin(omega);
            s0 = sin(oneminusalpha * omega) / sinomega;
            s1 = sin(alpha * omega) / sinomega;
        }
#endif
        ftmi->q[0] = q[0]*s1 + ftmi->q[0]*s0;
        ftmi->q[1] = q[1]*s1 + ftmi->q[1]*s0;
        ftmi->q[2] = q[2]*s1 + ftmi->q[2]*s0;
        ftmi->q[3] = q[3]*s1 + ftmi->q[3]*s0;
        arUtilQuatNorm(ftmi->q);
        
        // Linear interpolation for position.
        ftmi->p[0] = p[0]*alpha + ftmi->p[0]*oneminusalpha;
        ftmi->p[1] = p[1]*alpha + ftmi->p[1]*oneminusalpha;
        ftmi->p[2] = p[2]*alpha + ftmi->p[2]*oneminusalpha;
    }
    
    if (arUtilQuatPos2Mat(ftmi->q, ftmi->p, m) < 0) return (-2);
    
    return (0);
}

void arFilterTransMatFinal(ARFilterTransMatInfo *ftmi)
{
    if (!ftmi) return;
    free (ftmi);
}
