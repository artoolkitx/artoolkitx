/*
 *  paramGL.c
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

// ============================================================================
//	Private includes.
// ============================================================================

#include <ARX/AR/paramGL.h>


// ============================================================================
//	Public functions.
// ============================================================================

//
// Convert a camera parameter structure into an OpenGL projection matrix.
//
void arglCameraFrustumRH(const ARParam *cparam, const ARdouble focalmin, const ARdouble focalmax, ARdouble m_projection[16])
{
	ARdouble    icpara[3][4];
    ARdouble    trans[3][4];
    ARdouble    p[3][3], q[4][4];
	int         width, height;
    int         i, j;
	
    width  = cparam->xsize;
    height = cparam->ysize;
	
    if (arParamDecompMat(cparam->mat, icpara, trans) < 0) {
        ARLOGe("arglCameraFrustumRH(): arParamDecompMat() indicated parameter error.\n");
        return;
    }
	for (i = 0; i < 4; i++) {
        icpara[1][i] = (height - 1)*(icpara[2][i]) - icpara[1][i];
    }
	
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0 * p[0][0] / (width - 1));
    q[0][1] = (2.0 * p[0][1] / (width - 1));
    q[0][2] = -((2.0 * p[0][2] / (width - 1))  - 1.0);
    q[0][3] = 0.0;
	
    q[1][0] = 0.0;
    q[1][1] = -(2.0 * p[1][1] / (height - 1));
    q[1][2] = -((2.0 * p[1][2] / (height - 1)) - 1.0);
    q[1][3] = 0.0;
	
    q[2][0] = 0.0;
    q[2][1] = 0.0;
    q[2][2] = (focalmax + focalmin)/(focalmin - focalmax);
    q[2][3] = 2.0 * focalmax * focalmin / (focalmin - focalmax);
	
    q[3][0] = 0.0;
    q[3][1] = 0.0;
    q[3][2] = -1.0;
    q[3][3] = 0.0;
	
    for (i = 0; i < 4; i++) { // Row.
		// First 3 columns of the current row.
        for (j = 0; j < 3; j++) { // Column.
            m_projection[i + j*4] = q[i][0] * trans[0][j] +
									q[i][1] * trans[1][j] +
									q[i][2] * trans[2][j];
        }
		// Fourth column of the current row.
        m_projection[i + 3*4] = q[i][0] * trans[0][3] +
								q[i][1] * trans[1][3] +
								q[i][2] * trans[2][3] +
								q[i][3];
    }	
}

// para's type is also equivalent to (double(*)[4]).
void arglCameraViewRH(const ARdouble para[3][4], ARdouble m_modelview[16], const ARdouble scale)
{
	m_modelview[0 + 0*4] = para[0][0]; // R1C1
	m_modelview[0 + 1*4] = para[0][1]; // R1C2
	m_modelview[0 + 2*4] = para[0][2];
	m_modelview[0 + 3*4] = para[0][3];
	m_modelview[1 + 0*4] = -para[1][0]; // R2
	m_modelview[1 + 1*4] = -para[1][1];
	m_modelview[1 + 2*4] = -para[1][2];
	m_modelview[1 + 3*4] = -para[1][3];
	m_modelview[2 + 0*4] = -para[2][0]; // R3
	m_modelview[2 + 1*4] = -para[2][1];
	m_modelview[2 + 2*4] = -para[2][2];
	m_modelview[2 + 3*4] = -para[2][3];
	m_modelview[3 + 0*4] = 0.0;
	m_modelview[3 + 1*4] = 0.0;
	m_modelview[3 + 2*4] = 0.0;
	m_modelview[3 + 3*4] = 1.0;
	if (scale != 0.0) {
		m_modelview[12] *= scale;
		m_modelview[13] *= scale;
		m_modelview[14] *= scale;
	}
}

#ifndef ARDOUBLE_IS_FLOAT
void arglCameraFrustumRHf(const ARParam *cparam, const float focalmin, const float focalmax, float m_projection[16])
{
    float   icpara[3][4];
    float   trans[3][4];
    float   p[3][3], q[4][4];
    float   widthm1, heightm1;
    int     i, j;
    
    widthm1  = (float)(cparam->xsize - 1);
    heightm1 = (float)(cparam->ysize - 1);
    
    if (arParamDecompMatf(cparam->mat, icpara, trans) < 0) {
        printf("arglCameraFrustumRHf(): arParamDecompMat() indicated parameter error.\n"); // Windows bug: when running multi-threaded, can't write to stderr!
        return;
    }
    for (i = 0; i < 4; i++) {
        icpara[1][i] = heightm1*(icpara[2][i]) - icpara[1][i];
    }
    
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0f * p[0][0] / widthm1);
    q[0][1] = (2.0f * p[0][1] / widthm1);
    q[0][2] = -((2.0f * p[0][2] / widthm1)  - 1.0f);
    q[0][3] = 0.0f;
    
    q[1][0] = 0.0f;
    q[1][1] = -(2.0f * p[1][1] / heightm1);
    q[1][2] = -((2.0f * p[1][2] / heightm1) - 1.0f);
    q[1][3] = 0.0f;
    
    q[2][0] = 0.0f;
    q[2][1] = 0.0f;
    q[2][2] = (focalmax + focalmin)/(focalmin - focalmax);
    q[2][3] = 2.0f * focalmax * focalmin / (focalmin - focalmax);
    
    q[3][0] = 0.0f;
    q[3][1] = 0.0f;
    q[3][2] = -1.0f;
    q[3][3] = 0.0f;
    
    for (i = 0; i < 4; i++) { // Row.
        // First 3 columns of the current row.
        for (j = 0; j < 3; j++) { // Column.
            m_projection[i + j*4] = q[i][0] * trans[0][j] +
                                    q[i][1] * trans[1][j] +
                                    q[i][2] * trans[2][j];
        }
        // Fourth column of the current row.
        m_projection[i + 3*4] = q[i][0] * trans[0][3] +
                                q[i][1] * trans[1][3] +
                                q[i][2] * trans[2][3] +
                                q[i][3];
    }    
}

// para's type is also equivalent to (float(*)[4]).
void arglCameraViewRHf(float para[3][4], float m_modelview[16], const float scale)
{
    m_modelview[0 + 0*4] = para[0][0]; // R1C1
    m_modelview[0 + 1*4] = para[0][1]; // R1C2
    m_modelview[0 + 2*4] = para[0][2];
    m_modelview[0 + 3*4] = para[0][3];
    m_modelview[1 + 0*4] = -para[1][0]; // R2
    m_modelview[1 + 1*4] = -para[1][1];
    m_modelview[1 + 2*4] = -para[1][2];
    m_modelview[1 + 3*4] = -para[1][3];
    m_modelview[2 + 0*4] = -para[2][0]; // R3
    m_modelview[2 + 1*4] = -para[2][1];
    m_modelview[2 + 2*4] = -para[2][2];
    m_modelview[2 + 3*4] = -para[2][3];
    m_modelview[3 + 0*4] = 0.0f;
    m_modelview[3 + 1*4] = 0.0f;
    m_modelview[3 + 2*4] = 0.0f;
    m_modelview[3 + 3*4] = 1.0f;
    if (scale != 0.0f) {
        m_modelview[12] *= scale;
        m_modelview[13] *= scale;
        m_modelview[14] *= scale;
    }
}
#endif
