/*
 *  mtx.h
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

/*
 *
 *  Matrix routines that emulate the identical OpenGL 1.0-3.0 / OpenGL ES 1.0-1.1 routines,
 *  but with the current matrix passed as the first parameter, rather than being controlled
 *  by glMatrixMode calls. For documentation, read the relevant gl* manual pages (e.g.
 *  'man 3 glLoadIdentity')
 *
 *  mtx is available as part of libARG.
 *
 */

#ifndef __mtx_h__
#define __mtx_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef ARG_STATIC
#    define MTX_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define MTX_EXTERN __declspec(dllexport)
#    else
#      define MTX_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define MTX_EXTERN
#endif

MTX_EXTERN void mtxLoadIdentityf(float M[16]); // Replace matrix M with the identity matrix.
MTX_EXTERN void mtxLoadMatrixf(float M[16], const float T[16]); // Replace matrix M with the specified matrix T.
MTX_EXTERN void mtxMultMatrixf(float M[16], const float T[16]); // Multiply matrix M with matrix T.
MTX_EXTERN void mtxTranslatef(float M[16], const float x, const float y, const float z); // Multiply M by a translation matrix.
MTX_EXTERN void mtxRotatef(float M[16], const float q, const float x, const float y, const float z); // Multiply matrix M by a rotation matrix. q in degrees.
MTX_EXTERN void mtxScalef(float M[16], const float x, const float y, const float z); // Multiply matrix M by a general scaling matrix.
MTX_EXTERN void mtxOrthof(float M[16], float left, float right, float bottom, float top, float zNear, float zFar); // Multiply matrix M with an orthographic matrix.
MTX_EXTERN void mtxFrustumf(float M[16], float left, float right, float bottom, float top, float zNear, float zFar); // Multiply matrix M by a perspective matrix.
MTX_EXTERN void mtxPerspectivef(float M[16], float fovy, float aspect, float zNear, float zFar); // Multiply matrix M by a perspective matrix. fovy in degrees.
MTX_EXTERN void mtxLookAtf(float M[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ); // Multiply matrix M by a viewing matrix derived from an eye point, a reference point indicating the center of the scene, and an UP vector.

MTX_EXTERN void mtxLoadIdentityd(double M[16]); // Replace matrix M with the identity matrix.
MTX_EXTERN void mtxLoadMatrixd(double M[16], const double T[16]); // Replace matrix M with the specified matrix T.
MTX_EXTERN void mtxMultMatrixd(double M[16], const double T[16]); // Multiply matrix M with matrix T.
MTX_EXTERN void mtxTranslated(double M[16], const double x, const double y, const double z); // Multiply M by a translation matrix.
MTX_EXTERN void mtxRotated(double M[16], const double q, const double x, const double y, const double z); // Multiply matrix M by a rotation matrix. q in degrees.
MTX_EXTERN void mtxScaled(double M[16], const double x, const double y, const double z); // Multiply matrix M by a general scaling matrix.
MTX_EXTERN void mtxOrthod(double M[16], double left, double right, double bottom, double top, double zNear, double zFar); // Multiply matrix M with an orthographic matrix.
MTX_EXTERN void mtxFrustumd(double M[16], double left, double right, double bottom, double top, double zNear, double zFar); // Multiply matrix M by a perspective matrix.
MTX_EXTERN void mtxPerspectived(double M[16], double fovy, double aspect, double zNear, double zFar); // Multiply matrix M by a perspective matrix. fovy in degrees.
MTX_EXTERN void mtxLookAtd(double M[16], double eyeX, double eyeY, double eyeZ, double centerX, double centerY, double centerZ, double upX, double upY, double upZ); // Multiply matrix M by a viewing matrix derived from an eye point, a reference point indicating the center of the scene, and an UP vector.

#ifdef __cplusplus
}
#endif
#endif // !__mtx_h__