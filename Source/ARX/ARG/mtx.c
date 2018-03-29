/*
 *  mtx.c
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 */

#include <math.h>
#include <ARX/ARG/mtx.h>

#define DTORf 0.01745329251994f
#define DTORd 0.01745329251994

#define CROSS(dest,v1,v2) {dest[0] = v1[1]*v2[2] - v1[2]*v2[1]; dest[1] = v1[2]*v2[0] - v1[0]*v2[2]; dest[2] = v1[0]*v2[1] - v1[1]*v2[0];}

static float normalisef(float v[3])
{
    float l;
    
    l = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (l) {
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
	return (l);
}

static double normalised(double v[3])
{
    double l;
    
    l = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (l) {
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
	return (l);
}

void mtxLoadIdentityf(float M[16])
{
    M[ 0] = 1.0f; M[ 1] =       M[ 2] =       M[ 3] = 0.0f; // Column 0;
    M[ 4] = 0.0f; M[ 5] = 1.0f; M[ 6] =       M[ 7] = 0.0f; // Column 1;
    M[ 8] =       M[ 9] = 0.0f; M[10] = 1.0f; M[11] = 0.0f; // Column 2;
    M[12] =       M[13] =       M[14] = 0.0f; M[15] = 1.0f; // Column 3;
}

void mtxLoadMatrixf(float M[16], const float T[16])
{
    M[ 0] = T[ 0]; M[ 1] = T[ 1]; M[ 2] = T[ 2]; M[ 3] = T[ 3];
    M[ 4] = T[ 4]; M[ 5] = T[ 5]; M[ 6] = T[ 6]; M[ 7] = T[ 7];
    M[ 8] = T[ 8]; M[ 9] = T[ 9]; M[10] = T[10]; M[11] = T[11];
    M[12] = T[12]; M[13] = T[13]; M[14] = T[14]; M[15] = T[15];
}

void mtxMultMatrixf(float M[16], const float T[16])
{
    float M0[16];
    
    // Copy M to M0 so that result can be returned in M.
    M0[ 0] = M[ 0]; M0[ 1] = M[ 1]; M0[ 2] = M[ 2]; M0[ 3] = M[ 3];
    M0[ 4] = M[ 4]; M0[ 5] = M[ 5]; M0[ 6] = M[ 6]; M0[ 7] = M[ 7];
    M0[ 8] = M[ 8]; M0[ 9] = M[ 9]; M0[10] = M[10]; M0[11] = M[11];
    M0[12] = M[12]; M0[13] = M[13]; M0[14] = M[14]; M0[15] = M[15];
    
	M[ 0] = M0[ 0] * T[ 0] + M0[ 4] * T[ 1] + M0[ 8] * T[ 2] + M0[12] * T[ 3];
	M[ 1] = M0[ 1] * T[ 0] + M0[ 5] * T[ 1] + M0[ 9] * T[ 2] + M0[13] * T[ 3];
	M[ 2] = M0[ 2] * T[ 0] + M0[ 6] * T[ 1] + M0[10] * T[ 2] + M0[14] * T[ 3];
	M[ 3] = M0[ 3] * T[ 0] + M0[ 7] * T[ 1] + M0[11] * T[ 2] + M0[15] * T[ 3];
    
	M[ 4] = M0[ 0] * T[ 4] + M0[ 4] * T[ 5] + M0[ 8] * T[ 6] + M0[12] * T[ 7];
	M[ 5] = M0[ 1] * T[ 4] + M0[ 5] * T[ 5] + M0[ 9] * T[ 6] + M0[13] * T[ 7];
	M[ 6] = M0[ 2] * T[ 4] + M0[ 6] * T[ 5] + M0[10] * T[ 6] + M0[14] * T[ 7];
	M[ 7] = M0[ 3] * T[ 4] + M0[ 7] * T[ 5] + M0[11] * T[ 6] + M0[15] * T[ 7];
    
	M[ 8] = M0[ 0] * T[ 8] + M0[ 4] * T[ 9] + M0[ 8] * T[10] + M0[12] * T[11];
	M[ 9] = M0[ 1] * T[ 8] + M0[ 5] * T[ 9] + M0[ 9] * T[10] + M0[13] * T[11];
	M[10] = M0[ 2] * T[ 8] + M0[ 6] * T[ 9] + M0[10] * T[10] + M0[14] * T[11];
	M[11] = M0[ 3] * T[ 8] + M0[ 7] * T[ 9] + M0[11] * T[10] + M0[15] * T[11];
    
	M[12] = M0[ 0] * T[12] + M0[ 4] * T[13] + M0[ 8] * T[14] + M0[12] * T[15];
	M[13] = M0[ 1] * T[12] + M0[ 5] * T[13] + M0[ 9] * T[14] + M0[13] * T[15];
	M[14] = M0[ 2] * T[12] + M0[ 6] * T[13] + M0[10] * T[14] + M0[14] * T[15];
	M[15] = M0[ 3] * T[12] + M0[ 7] * T[13] + M0[11] * T[14] + M0[15] * T[15];
}

void mtxTranslatef(float M[16], const float x, const float y, const float z)
{
    float T[16];
    
    T[ 0] = 1.0f; T[ 1] =       T[ 2] =       T[ 3] = 0.0f; // Column 0;
    T[ 4] = 0.0f; T[ 5] = 1.0f; T[ 6] =       T[ 7] = 0.0f; // Column 1;
    T[ 8] =       T[ 9] = 0.0f; T[10] = 1.0f; T[11] = 0.0f; // Column 2;
    T[12] =    x; T[13] =    y; T[14] =    z; T[15] = 1.0f; // Column 3;
    
    mtxMultMatrixf(M, T);
}

void mtxRotatef(float M[16], const float q, const float x, const float y, const float z)
{
    float ll, l, x0, y0, z0;
    float T[16];
	float C, S, V;
	float xy, yz, xz;
	float Sx, Sy, Sz;
	float Vxy, Vyz, Vxz;
	
    if (q == 0.0f) return;
    
    ll = x*x + y*y + z*z;
    if (ll != 1.0f) {
        l = sqrtf(ll);
        if (!l) return;
        x0 = x / l;
        y0 = y / l;
        z0 = z / l;
    } else {
        x0 = x;
        y0 = y;
        z0 = z;
    }
    
	C = cosf(DTORf*q);
	S = sinf(DTORf*q);
	V = 1.0f - C;
	xy = x0*y0;
	yz = y0*z0;
	xz = x0*z0;
	Sx = S*x0;
	Sy = S*y0;
	Sz = S*z0;
	Vxy = V*xy;
	Vyz = V*yz;
	Vxz = V*xz;
	
    // Column 0;
	T[ 0] = V*x0*x0 + C;
	T[ 1] = Vxy + Sz;
	T[ 2] = Vxz - Sy;
    T[ 3] = 0.0f;
    
    // Column 1;
	T[ 4] = Vxy - Sz;
	T[ 5] = V*y0*y0 + C;
	T[ 6] = Vyz + Sx;
    T[ 7] = 0.0f;
    
    // Column 2;
	T[ 8] = Vxz + Sy;
	T[ 9] = Vyz - Sx;
	T[10] = V*z0*z0 + C;
    T[11] = 0.0f;
    
    // Column 3;
	T[12] = 0.0f;
	T[13] = 0.0f;
	T[14] = 0.0f;
    T[15] = 1.0f;

    mtxMultMatrixf(M, T);
}

void mtxScalef(float M[16], const float x, const float y, const float z)
{
    float T[16];
    
    T[ 0] =    x; T[ 1] =       T[ 2] =       T[ 3] = 0.0f; // Column 0;
    T[ 4] = 0.0f; T[ 5] =    y; T[ 6] =       T[ 7] = 0.0f; // Column 1;
    T[ 8] =       T[ 9] = 0.0f; T[10] =    z; T[11] = 0.0f; // Column 2;
    T[12] =       T[13] =       T[14] = 0.0f; T[15] = 1.0f; // Column 3;
    
    mtxMultMatrixf(M, T);
}

void mtxOrthof(float M[16], float left, float right, float bottom, float top, float zNear, float zFar)
{
    float T[16];
	float r_l = right - left;
	float t_b = top - bottom;
	float f_n = zFar - zNear;
    
    // Column 0;
	T[ 0] = 2.0f / r_l;
	T[ 1] = 0.0f;
	T[ 2] = 0.0f;
	T[ 3] = 0.0f;
	
    // Column 1;
	T[ 4] = 0.0f;
	T[ 5] = 2.0f / t_b;
	T[ 6] = 0.0f;
	T[ 7] = 0.0f;
	
    // Column 2;
	T[ 8] = 0.0f;
	T[ 9] = 0.0f;
	T[10] = -2.0f / f_n;
	T[11] = 0.0f;
	
    // Column 3;
	T[12] = - (right + left) / r_l;
	T[13] = - (top + bottom) / t_b;
	T[14] = - (zFar + zNear) / f_n;
	T[15] = 1.0f;
    
    mtxMultMatrixf(M, T);
}

void mtxFrustumf(float M[16], float left, float right, float bottom, float top, float zNear, float zFar)
{
    float T[16];
	float r_l = right - left;
	float t_b = top - bottom;
	float f_n = zFar - zNear;
    
    // Column 0;
	T[ 0] = 2.0f * zNear / r_l;
	T[ 1] = 0.0f;
	T[ 2] = 0.0f;
	T[ 3] = 0.0f;
	
    // Column 1;
	T[ 4] = 0.0f;
	T[ 5] = 2.0f * zNear / t_b;
	T[ 6] = 0.0f;
	T[ 7] = 0.0f;
	
    // Column 2;
	T[ 8] = (right + left) / r_l;
	T[ 9] = (top + bottom) / t_b;
	T[10] = - (zFar + zNear) / f_n;
	T[11] = -1.0f;
	
    // Column 3;
	T[12] = 0.0f;
	T[13] = 0.0f;
	T[14] = - (2.0f * zFar * zNear) / f_n;
	T[15] = 0.0f;
    
    mtxMultMatrixf(M, T);
}

void mtxPerspectivef(float M[16], float fovy, float aspect, float zNear, float zFar)
{
    float T[16];
    float f = 1.0f / tanf(DTORf*fovy/2.0f);
	float n_f = zNear - zFar;
    
    // Column 0;
	T[ 0] = f / aspect;
	T[ 1] = 0.0f;
	T[ 2] = 0.0f;
	T[ 3] = 0.0f;
	
    // Column 1;
	T[ 4] = 0.0f;
	T[ 5] = f;
	T[ 6] = 0.0f;
	T[ 7] = 0.0f;
	
    // Column 2;
	T[ 8] = 0.0f;
	T[ 9] = 0.0f;
	T[10] = (zFar + zNear) / n_f;
	T[11] = -1.0f;
	
    // Column 3;
	T[12] = 0.0f;
	T[13] = 0.0f;
	T[14] = 2.0f * zFar * zNear / n_f;
	T[15] = 0.0f;
    
    mtxMultMatrixf(M, T);
}

void mtxLookAtf(float M[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
    float T[16];
    float F[3], UP[3], S[3];
    
    F[0] = centerX - eyeX;
    F[1] = centerY - eyeY;
    F[2] = centerZ - eyeZ;
    normalisef(F);
    
    UP[0] = upX;
    UP[1] = upY;
    UP[2] = upZ;
    normalisef(UP);
    
    CROSS(S, F, UP);
    CROSS(UP, S, F);
    
    // Column 0;
	T[ 0] =  S[0];
	T[ 1] = UP[0];
	T[ 2] = -F[0];
	T[ 3] = 0.0f;
	
    // Column 1;
	T[ 4] =  S[1];
	T[ 5] = UP[1];
	T[ 6] = -F[1];
	T[ 7] = 0.0f;
	
    // Column 2;
	T[ 8] =  S[2];
	T[ 9] = UP[2];
	T[10] = -F[2];
	T[11] = 0.0f;
	
    // Column 3;
	T[12] = -eyeX *  S[0] - eyeY *  S[1] - eyeZ *  S[2];
	T[13] = -eyeX * UP[0] - eyeY * UP[1] - eyeZ * UP[2];
	T[14] =  eyeX *  F[0] + eyeY *  F[1] + eyeZ *  F[2];
	T[15] = 1.0f;
    
    mtxMultMatrixf(M, T);
}

void mtxLoadIdentityd(double M[16])
{
    M[ 0] = 1.0; M[ 1] =      M[ 2] =      M[ 3] = 0.0; // Column 0;
    M[ 4] = 0.0; M[ 5] = 1.0; M[ 6] =      M[ 7] = 0.0; // Column 1;
    M[ 8] =      M[ 9] = 0.0; M[10] = 1.0; M[11] = 0.0; // Column 2;
    M[12] =      M[13] =      M[14] = 0.0; M[15] = 1.0; // Column 3;
}

void mtxLoadMatrixd(double M[16], const double T[16])
{
    M[ 0] = T[ 0]; M[ 1] = T[ 1]; M[ 2] = T[ 2]; M[ 3] = T[ 3];
    M[ 4] = T[ 4]; M[ 5] = T[ 5]; M[ 6] = T[ 6]; M[ 7] = T[ 7];
    M[ 8] = T[ 8]; M[ 9] = T[ 9]; M[10] = T[10]; M[11] = T[11];
    M[12] = T[12]; M[13] = T[13]; M[14] = T[14]; M[15] = T[15];
}

void mtxMultMatrixd(double M[16], const double T[16])
{
    double M0[16];
    
    // Copy M to M0 so that result can be returned in M.
    M0[ 0] = M[ 0]; M0[ 1] = M[ 1]; M0[ 2] = M[ 2]; M0[ 3] = M[ 3];
    M0[ 4] = M[ 4]; M0[ 5] = M[ 5]; M0[ 6] = M[ 6]; M0[ 7] = M[ 7];
    M0[ 8] = M[ 8]; M0[ 9] = M[ 9]; M0[10] = M[10]; M0[11] = M[11];
    M0[12] = M[12]; M0[13] = M[13]; M0[14] = M[14]; M0[15] = M[15];
    
	M[ 0] = M0[ 0] * T[ 0] + M0[ 4] * T[ 1] + M0[ 8] * T[ 2] + M0[12] * T[ 3];
	M[ 1] = M0[ 1] * T[ 0] + M0[ 5] * T[ 1] + M0[ 9] * T[ 2] + M0[13] * T[ 3];
	M[ 2] = M0[ 2] * T[ 0] + M0[ 6] * T[ 1] + M0[10] * T[ 2] + M0[14] * T[ 3];
	M[ 3] = M0[ 3] * T[ 0] + M0[ 7] * T[ 1] + M0[11] * T[ 2] + M0[15] * T[ 3];
    
	M[ 4] = M0[ 0] * T[ 4] + M0[ 4] * T[ 5] + M0[ 8] * T[ 6] + M0[12] * T[ 7];
	M[ 5] = M0[ 1] * T[ 4] + M0[ 5] * T[ 5] + M0[ 9] * T[ 6] + M0[13] * T[ 7];
	M[ 6] = M0[ 2] * T[ 4] + M0[ 6] * T[ 5] + M0[10] * T[ 6] + M0[14] * T[ 7];
	M[ 7] = M0[ 3] * T[ 4] + M0[ 7] * T[ 5] + M0[11] * T[ 6] + M0[15] * T[ 7];
    
	M[ 8] = M0[ 0] * T[ 8] + M0[ 4] * T[ 9] + M0[ 8] * T[10] + M0[12] * T[11];
	M[ 9] = M0[ 1] * T[ 8] + M0[ 5] * T[ 9] + M0[ 9] * T[10] + M0[13] * T[11];
	M[10] = M0[ 2] * T[ 8] + M0[ 6] * T[ 9] + M0[10] * T[10] + M0[14] * T[11];
	M[11] = M0[ 3] * T[ 8] + M0[ 7] * T[ 9] + M0[11] * T[10] + M0[15] * T[11];
    
	M[12] = M0[ 0] * T[12] + M0[ 4] * T[13] + M0[ 8] * T[14] + M0[12] * T[15];
	M[13] = M0[ 1] * T[12] + M0[ 5] * T[13] + M0[ 9] * T[14] + M0[13] * T[15];
	M[14] = M0[ 2] * T[12] + M0[ 6] * T[13] + M0[10] * T[14] + M0[14] * T[15];
	M[15] = M0[ 3] * T[12] + M0[ 7] * T[13] + M0[11] * T[14] + M0[15] * T[15];
}

void mtxTranslated(double M[16], const double x, const double y, const double z)
{
    double T[16];
    
    T[ 0] = 1.0; T[ 1] =      T[ 2] =      T[ 3] = 0.0; // Column 0;
    T[ 4] = 0.0; T[ 5] = 1.0; T[ 6] =      T[ 7] = 0.0; // Column 1;
    T[ 8] =      T[ 9] = 0.0; T[10] = 1.0; T[11] = 0.0; // Column 2;
    T[12] =   x; T[13] =   y; T[14] =   z; T[15] = 1.0; // Column 3;
    
    mtxMultMatrixd(M, T);
}

void mtxRotated(double M[16], const double q, const double x, const double y, const double z)
{
    double ll, l, x0, y0, z0;
    double T[16];
	double C, S, V;
	double xy, yz, xz;
	double Sx, Sy, Sz;
	double Vxy, Vyz, Vxz;
	
    if (q == 0.0) return;
    
    ll = x*x + y*y + z*z;
    if (ll != 1.0) {
        l = sqrt(ll);
        if (!l) return;
        x0 = x / l;
        y0 = y / l;
        z0 = z / l;
    } else {
        x0 = x;
        y0 = y;
        z0 = z;
    }
    
	C = cos(DTORd*q);
	S = sin(DTORd*q);
	V = 1.0 - C;
	xy = x0*y0;
	yz = y0*z0;
	xz = x0*z0;
	Sx = S*x0;
	Sy = S*y0;
	Sz = S*z0;
	Vxy = V*xy;
	Vyz = V*yz;
	Vxz = V*xz;
	
    // Column 0;
	T[ 0] = V*x0*x0 + C;
	T[ 1] = Vxy + Sz;
	T[ 2] = Vxz - Sy;
    T[ 3] = 0.0;
    
    // Column 1;
	T[ 4] = Vxy - Sz;
	T[ 5] = V*y0*y0 + C;
	T[ 6] = Vyz + Sx;
    T[ 7] = 0.0;
    
    // Column 2;
	T[ 8] = Vxz + Sy;
	T[ 9] = Vyz - Sx;
	T[10] = V*z0*z0 + C;
    T[11] = 0.0;
    
    // Column 3;
	T[12] = 0.0;
	T[13] = 0.0;
	T[14] = 0.0;
    T[15] = 1.0;
    
    mtxMultMatrixd(M, T);
}

void mtxScaled(double M[16], const double x, const double y, const double z)
{
    double T[16];
    
    T[ 0] =   x; T[ 1] =      T[ 2] =      T[ 3] = 0.0; // Column 0;
    T[ 4] = 0.0; T[ 5] =   y; T[ 6] =      T[ 7] = 0.0; // Column 1;
    T[ 8] =      T[ 9] = 0.0; T[10] =   z; T[11] = 0.0; // Column 2;
    T[12] =      T[13] =      T[14] = 0.0; T[15] = 1.0; // Column 3;
    
    mtxMultMatrixd(M, T);
}

void mtxOrthod(double M[16], double left, double right, double bottom, double top, double zNear, double zFar)
{
    double T[16];
	double r_l = right - left;
	double t_b = top - bottom;
	double f_n = zFar - zNear;
    
    // Column 0;
	T[ 0] = 2.0 / r_l;
	T[ 1] = 0.0;
	T[ 2] = 0.0;
	T[ 3] = 0.0;
	
    // Column 1;
	T[ 4] = 0.0;
	T[ 5] = 2.0 / t_b;
	T[ 6] = 0.0;
	T[ 7] = 0.0;
	
    // Column 2;
	T[ 8] = 0.0;
	T[ 9] = 0.0;
	T[10] = -2.0 / f_n;
	T[11] = 0.0;
	
    // Column 3;
	T[12] = - (right + left) / r_l;
	T[13] = - (top + bottom) / t_b;
	T[14] = - (zFar + zNear) / f_n;
	T[15] = 1.0;
    
    mtxMultMatrixd(M, T);
}

void mtxFrustumd(double M[16], double left, double right, double bottom, double top, double zNear, double zFar)
{
    double T[16];
	double r_l = right - left;
	double t_b = top - bottom;
	double f_n = zFar - zNear;
    
    // Column 0;
	T[ 0] = 2.0 * zNear / r_l;
	T[ 1] = 0.0;
	T[ 2] = 0.0;
	T[ 3] = 0.0;
	
    // Column 1;
	T[ 4] = 0.0;
	T[ 5] = 2.0 * zNear / t_b;
	T[ 6] = 0.0;
	T[ 7] = 0.0;
	
    // Column 2;
	T[ 8] = (right + left) / r_l;
	T[ 9] = (top + bottom) / t_b;
	T[10] = - (zFar + zNear) / f_n;
	T[11] = -1.0;
	
    // Column 3;
	T[12] = 0.0;
	T[13] = 0.0;
	T[14] = - (2.0 * zFar * zNear) / f_n;
	T[15] = 0.0;
    
    mtxMultMatrixd(M, T);
}

void mtxPerspectived(double M[16], double fovy, double aspect, double zNear, double zFar)
{
    double T[16];
    double f = 1.0 / tan(DTORd*fovy/2.0);
	double n_f = zNear - zFar;
    
    // Column 0;
	T[ 0] = f / aspect;
	T[ 1] = 0.0;
	T[ 2] = 0.0;
	T[ 3] = 0.0;
	
    // Column 1;
	T[ 4] = 0.0;
	T[ 5] = f;
	T[ 6] = 0.0;
	T[ 7] = 0.0;
	
    // Column 2;
	T[ 8] = 0.0;
	T[ 9] = 0.0;
	T[10] = (zFar + zNear) / n_f;
	T[11] = -1.0;
	
    // Column 3;
	T[12] = 0.0;
	T[13] = 0.0;
	T[14] = 2.0 * zFar * zNear / n_f;
	T[15] = 0.0;
    
    mtxMultMatrixd(M, T);
}

void mtxLookAtd(double M[16], double eyeX, double eyeY, double eyeZ, double centerX, double centerY, double centerZ, double upX, double upY, double upZ)
{
    double T[16];
    double F[3], UP[3], S[3];
    
    F[0] = centerX - eyeX;
    F[1] = centerY - eyeY;
    F[2] = centerZ - eyeZ;
    normalised(F);
    
    UP[0] = upX;
    UP[1] = upY;
    UP[2] = upZ;
    normalised(UP);
    
    CROSS(S, F, UP);
    CROSS(UP, S, F);
    
    // Column 0;
	T[ 0] =  S[0];
	T[ 1] = UP[0];
	T[ 2] = -F[0];
	T[ 3] = 0.0;
	
    // Column 1;
	T[ 4] =  S[1];
	T[ 5] = UP[1];
	T[ 6] = -F[1];
	T[ 7] = 0.0;
	
    // Column 2;
	T[ 8] =  S[2];
	T[ 9] = UP[2];
	T[10] = -F[2];
	T[11] = 0.0;
	
    // Column 3;
	T[12] = -eyeX *  S[0] - eyeY *  S[1] - eyeZ *  S[2];
	T[13] = -eyeX * UP[0] - eyeY * UP[1] - eyeZ * UP[2];
	T[14] =  eyeX *  F[0] + eyeY *  F[1] + eyeZ *  F[2];
	T[15] = 1.0;
    
    mtxMultMatrixd(M, T);
}

