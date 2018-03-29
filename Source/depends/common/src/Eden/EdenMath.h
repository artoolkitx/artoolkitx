/*
 *  EdenMath.h
 *  The Eden Library
 *
 *	Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2001-07-28	PRL		Initial version.
 *
 */

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

/*!
	@header EdenMath
	@brief Math utilities.
	@details
		EdenMath forms one part of the Eden library.
	@copyright 2001-2013 Philip Lamb
 */

#ifndef __EdenMath_h__
#define __EdenMath_h__
	
// ============================================================================
//	Public includes
// ============================================================================
#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#if defined(_WIN32) && !defined(_USE_MATH_DEFINES)
#  define _USE_MATH_DEFINES	// Make Win32 math.h define M_... constants.
#endif
#include <math.h>

// Fixes for missing floating point function declarations in Win32 ISO C.
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__cplusplus) && _MSC_VER < 1600
#  define sinf(x) sin(x)
#  define cosf(x) cos(x)
#  define tanf(x) tan(x)
#  define asinf(x) asin(x)
#  define acosf(x) acos(x)
#  define atanf(x) atan(x)
#  define atan2f(y,x) atan2(y,x)
#  define sinhf(x) sinh(x)
#  define coshf(x) cosh(x)
#  define tanhf(x) tanh(x)
#  define expf(x) exp(x)
#  define logf(x) log(x)
#  define log10f(x) log10(x)
#  define modff(x,ip) modf(x,ip)
#  define powf(x,y) pow(x,y)
#  define sqrtf(x) sqrtf(x)
#  define ceilf(x) ceil(x)
#  define fabsf(x) fabs(x)
#  define floorf(x) floor(x)
#  define fmodf(x,y) fmod(x,y)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//	Defines
// ============================================================================

#ifndef M_PI
/*!
    @defined M_PI
    @brief Value of PI.
    @details
		Defined only if not already defined in math.h.
*/
#define M_PI 3.141592653589793238462643
#endif

/*!
    @defined PI
    @brief Value of PI.
*/
#define PI				M_PI

/*!
    @defined TWOPI
    @brief   (description)
    @details (description)
*/
#define TWOPI			(2*M_PI)

/*!
    @defined HALFPI
    @brief   (description)
    @details (description)
*/
#define HALFPI			(0.5*M_PI)

/*!
    @defined DTOR
    @brief Convert degrees to radians.
    @details
		Multiply an angle in degrees by this constant to
		get the value of the angle in radians.
*/
#define DTOR            0.0174532925

/*!
    @defined RTOD
    @brief Convert radians to degrees.
    @details
		Multiply an angle in radians by this constant to
		get the value of the angle in degrees.
*/
#define RTOD            57.2957795

/*!
    @defined MIN
	@brief Determine minimum of two values.
*/
#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif
	
/*!
	@defined MAX
	@brief Determine maximum of two values.
*/
#ifndef MAX
#define MAX(x,y) (x > y ? x : y)
#endif

/*!
    @defined CROSS
    @brief Vector cross-product in R3.
    @details
		6 multiplies + 3 subtracts.
		Vector cross product calculates a vector with direction
		orthogonal to plane formed by the other two vectors, and length
		equal to the area of the parallelogram formed by the other two
		vectors.
		Right hand rule for vector cross products: Point thumb of right
		hand in direction of v1, fingers together in direction of v2,
		then palm faces in the direction of dest.
 */
#define CROSS(dest,v1,v2) {dest[0] = v1[1]*v2[2] - v1[2]*v2[1]; dest[1] = v1[2]*v2[0] - v1[0]*v2[2]; dest[2] = v1[0]*v2[1] - v1[1]*v2[0];}

/*!
    @defined LENGTH
    @brief   (description)
    @details (description)
*/
#define LENGTH(v) (sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]))

/*!
    @defined DOT
    @brief Vector dot-product in R3.
    @details
		3 multiplies + 2 adds.
*/
#define DOT(v1,v2) ((v1[0]*v2[0]) + (v1[1]*v2[1]) + (v1[2]*v2[2]))

/*!
    @defined ADD
    @brief   (description)
    @details (description)
*/
#define ADD(dest,v1,v2) {dest[0] = v1[0] + v2[0]; dest[1] = v1[1] + v2[1]; dest[2] = v1[2] + v2[2];}

/*!
    @defined SUB
    @brief   (description)
    @details (description)
*/
#define SUB(dest,v1,v2) {dest[0] = v1[0] - v2[0]; dest[1] = v1[1] - v2[1]; dest[2] = v1[2] - v2[2];}

/*!
    @defined AVERAGE
	@brief   (description)
	@details (description)
 */
#define AVERAGE(dest,v1,v2) {dest[0] = (v1[0] + v2[0])*0.5; dest[1] = (v1[1] + v2[1])*0.5; dest[2] = (v1[2] + v2[2])*0.5;}
	
/*!
    @defined COPY
	@brief   (description)
	@details (description)
 */
#define COPY(dest, v) {dest[0] = v[0]; dest[1] = v[1]; dest[2] = v[2];}

// ============================================================================
//	Public functions
// ============================================================================

/*!
    @brief Normalise a vector.
    @details
    @param	v Vector to normalise.
*/
float EdenMathNormalise(float v[3]);

/*!
    @brief Normalise a vector.
	@details
	@param	v Vector to normalise.
*/
double EdenMathNormalised(double v[3]);
	
/*!
    @brief Calculates the Hessian normal (unit normal vector) to a plane.
    @details
		The plane is specified by three vectors which are points on the plane.
 
		The normal calculated is of unit length (i.e. it is the Hessian normal).
 
		See http://mathworld.wolfram.com/HessianNormalForm.html
	@param      n Vector representing Hessian normal (unit normal vector) to plane.
	@param      p Value of distance of plane from origin.
		The sign of *p determines the side of the plane on which the origin is located.
		If &gt; 0, it is in the half-space determined by the direction of n, and if &lt; 0,
		it is in the other half-space.
    @param      p1 Vector describing point on the plane.
	@param      p2 Vector describing point on the plane.
	@param      p3 Vector describing point on the plane.
*/
void EdenMathPointsToPlaneHessianNormal(float n[3], float *p, const float p1[3], const float p2[3], const float p3[3]);

/*!
    @function 
    @brief   Calculate the general form of a plane, given three points lying on the plane.
    @details
		The general form of a plane is the equation Ax + By + Cz + D = 0.
		The plane is specified by three vectors which are points on the plane.
		The vector ABC is normal to the plane (not unit normal though).

		See http://mathworld.wolfram.com/Plane.html
	@param      abc Vector describing the plane.
	@param      d Value describing the plane.
	@param      p1 Vector describing point on the plane.
	@param      p2 Vector describing point on the plane.
	@param      p3 Vector describing point on the plane.
*/
void EdenMathPointsToPlane(float abc[3], float *d, const float p1[3], const float p2[3], const float p3[3]);

/*!
    @function 
    @brief   Calculate the point of intersection (if any) between a line and a plane.
    @details
		See http://mathworld.wolfram.com/Plane.html
    @param      intersection Pointer to an array of 3 floats which will be
		filled out with the coordinates of the point of intersection.
	@param		u_out If non-null, the float pointed to will be filled out with
		the value 'u', representing the point of intersection, relative to the
		line segement between p1 and p2. I.e., when u is 0, the intersection is
		at p1, and when u is 1 the intersection is at p2. Values of u &lt; 0 or &gt; 1
		indicate the intersection point is outside the line segment (but there IS
		still an intersection point). NULL may be passed if you are not interested
		in this value.
	@param		p1 A point lying on the line.
	@param		p2 Another point lying on the line.
	@param		abc The vector of values A, B, and C from the general definition of a plane
		Ax + By + Cz + D = 0 (see EdenMathPointsToPlane()).
	@param		abc The value D from the general definition of a plane
		Ax + By + Cz + D = 0 (see EdenMathPointsToPlane()).
    @result     FALSE if the normal to the plane is perpendicular to the line, or
		if a required value was not specified, TRUE otherwise.
*/
EDEN_BOOL EdenMathIntersectionLinePlane(float intersection[3], float *u_out, const float p1[3], const float p2[3], const float abc[3], const float d);

/*!
    @function 
	@brief   Caclulate signed distance from point to a plane.
	@details
		See http://mathworld.wolfram.com/Point-PlaneDistance.html
	@param      x0 Position of the point.
	@param		abc The vector of values A, B, and C from the general definition of a plane
		Ax + By + Cz + D = 0 (see EdenMathPointsToPlane()).
	@param		abc The value D from the general definition of a plane
		Ax + By + Cz + D = 0 (see EdenMathPointsToPlane()).
	@result     The distance from the point to the plane, which is positive if the
		point lies on the same side of the plane as the normal to the plane, or
		negative if it lies on the opposite side, or 0 if the parameters are invalid.
 */
float EdenMathCalcDistanceToPlane(const float x0[3], const float abc[3], const float d);

/*!
    @brief   Creates a 3x3 identity matrix.
	@details
	@param      mtx9 A 3x3 matrix that will receive the output.
 */
void EdenMathIdentityMatrix3by3(float mtx9[9]);

/*!
    @brief Multiplies 3x3 matrix B into 3x3 matrix A, placing result in C.
    @details
		A, B and C are in column-major form, which is standard for OpenGL
		(wheras the usual mathematical matrix notation is row-major.)
*/
void EdenMathMultMatrix3by3(float C[9], const float B[9], const float A[9]);

/*!
    @brief Invert a 3x3 matrix A, placing result in Ainv.
    @details
		A and Ainv are in column-major form, which is standard for OpenGL
		(wheras the usual mathematical matrix notation is row-major.)
*/
EDEN_BOOL EdenMathInvertMatrix3by3(float A[9], float Ainv[9]);

/*!
    @brief   Creates a 4x4 identity matrix.
    @details
    @param      mtx16 A 4x4 matrix that will receive the output.
*/
void EdenMathIdentityMatrix(float mtx16[16]);

/*!
    @brief Multiplies 4x4 matrix B into 4x4 matrix A, placing result in C.
    @details
		A, B and C are in column-major form, which is standard for OpenGL
		(wheras the usual mathematical matrix notation is row-major.)
*/
void EdenMathMultMatrix(float C[16], const float B[16], const float A[16]);

/*!
    	@brief Multiplies 4x4 matrix B into 4x4 matrix A, placing result in C.
	@details
		A, B and C are in column-major form, which is standard for OpenGL
		(wheras the usual mathematical matrix notation is row-major.)
 */
void EdenMathMultMatrixd(double C[16], const double B[16], const double A[16]);

/*!
    @brief   Compute inverse of 4x4 transformation matrix
    @details Code contributed by Jacques Leroy jle\@star.be
    @param      m Input, 4x4 column-major matrix.
    @param      out Output, 4x4 column-major matrix.
    @result     Return TRUE for success, FALSE for failure (singular matrix)
*/
EDEN_BOOL EdenMathInvertMatrix(float out[16], const float m[16]);

/*!
    @brief   Compute inverse of 4x4 transformation matrix
    @details Code contributed by Jacques Leroy jle\@star.be
    @param      m Input, 4x4 column-major matrix.
    @param      out Output, 4x4 column-major matrix.
    @result     Return TRUE for success, FALSE for failure (singular matrix)
*/
EDEN_BOOL EdenMathInvertMatrixd(double out[16], const double m[16]);

/*!
    @brief   Multiples 4x4 matrix A into column vector p, placing result in q.
    @details 
         A is in column-major form, which is standard for OpenGL
         (wheras the usual mathematical matrix notation is row-major.)
*/
void EdenMathMultMatrixByVector(float q[4], const float A[16], const float p[4]);
    
/*!
    @brief   Multiples 4x4 matrix A into column vector p, placing result in q.
    @details 
        A is in column-major form, which is standard for OpenGL
        (wheras the usual mathematical matrix notation is row-major.)
*/
void EdenMathMultMatrixByVectord(double q[4], const double A[16], const double p[4]);

/*!
    @function 
    @brief   Creates a matrix which represents translation by a vector.
    @details
	@param      mtx16 A 4x4 matrix that will receive the output in column major form.
	@param		x X component of the translation vector.
	@param		y Y component of the translation vector.
	@param		z Z component of the translation vector.
*/
void EdenMathTranslationMatrix(float mtx16[16], const float x, const float y, const float z);

/*!
    @brief   Translate a matrix by a vector.
    @details
	@param      B A 4x4 matrix that will receive the output in column major form.
	@param      A A 4x4 matrix that will supply the input in column major form.
	@param		x X component of the translation vector.
	@param		y Y component of the translation vector.
	@param		z Z component of the translation vector.
*/
void EdenMathTranslateMatrix(float B[16], const float A[16], const float x, const float y, const float z);

/*!
    @brief Creates a matrix which represents the general case of a rotation about an arbitrary axis.
	@details
	@param      mtx16 A 4x4 matrix that will receive the output in column major form.
	@param		q The angle of rotation measured in a right-hand sense, in radians.
	@param		x X component of the normalised non-zero vector representing the axis of rotation.
	@param		y Y component of the normalised non-zero vector representing the axis of rotation.
	@param		z Z component of the normalised non-zero vector representing the axis of rotation.
*/
void EdenMathRotationMatrix(float mtx16[16], const float q, const float x, const float y, const float z);

/*!
    @brief Rotate a matrix about an arbitrary axis.
    @details (description)
	@param      B A 4x4 matrix that will receive the output in column major form.
	@param      A A 4x4 matrix that will supply the input in column major form.
	@param		q The angle of rotation measured in a right-hand sense, in radians.
	@param		x X component of the normalised non-zero vector representing the axis of rotation.
	@param		y Y component of the normalised non-zero vector representing the axis of rotation.
	@param		z Z component of the normalised non-zero vector representing the axis of rotation.
 */
void EdenMathRotateMatrix(float B[16], const float A[16], const float q, const float x, const float y, const float z);

void EdenMathScalingMatrix(float mtx16[16], const float x, const float y, const float z);

void EdenMathScaleMatrix(float B[16], const float A[16], const float x, const float y, const float z);

/*!
    @brief Creates a rotation matrix that rotates a vector called
		"from" into another vector called "to".
    @details
		Author: Tomas Moller, 1999
	@param      from Normalised non-zero vector.
	@param      to Normalised non-zero vector.
	@param      mtx9 A 3x3 matrix in column-major form which receives the result.
*/
void EdenMathRotationMatrixFromTo(const float from[3], const float to[3], float mtx9[9]);

/*!
    @brief Rotate a point about an arbitrary axis.
	@details
	@param      p2 Rotated point.
	@param      p1 Point to rotate.
	@param		q The angle of rotation measured in a right-hand sense, in radians.
	@param		a Normalised non-zero vector representing the axis of rotation.
 */
void EdenMathRotatePointAboutAxis(float p2[3], const float p1[3], const float q, const float a[3]);

/*!
    @brief Rotate a point about an arbitrary axis.
	@details
	@param      p2 Rotated point.
	@param      p1 Point to rotate.
	@param		q The angle of rotation measured in a right-hand sense, in radians.
	@param		a Normalised non-zero vector representing the axis of rotation.
*/
void EdenMathRotatePointAboutAxisd(double p2[3], const double p1[3], const double q, const double a[3]);

#ifdef __ppc__
/*!
    @brief Fast calculation of square root.
    @details
		Uses a ppc-only instruction to get an estimate of the
		square root, then performs 4 iterations of
		Newton-Rhapson refinement to give better precision.
		Note: these functions are not IEEE-754 compliant and do not calculate 
		the last bit correctly. For correct accuary, refer to libm on MacOS X. 
		Written by A. Sazegari, started on February 2002. 
		Copyright 2002 Apple Computer, Inc. All rights reserved.
	@param arg Pointer to double-precision floating point number.
		Will be replaced with the root of the number.
		(*arg = (*arg)^0.5).
	@availability Available on PowerPC CPUs only.
*/
void fsqrt(double *arg);

/*!
    @brief Fast calculation of three square roots.
	@details
		Uses a ppc-only instruction to get an estimate of the
		square root, then performs 4 iterations of
		Newton-Rhapson refinement to give better precision.
		Note: these functions are not IEEE-754 compliant and do not calculate 
		the last bit correctly. For correct accuary, refer to libm on MacOS X. 
		Written by A. Sazegari, started on February 2002. 
		Copyright 2002 Apple Computer, Inc. All rights reserved.
	@param arg1 Pointer to double-precision floating point number.
		Will be replaced with the root of the number.
	@param arg2 Pointer to double-precision floating point number.
		Will be replaced with the root of the number.
	@param arg3 Pointer to double-precision floating point number.
		Will be replaced with the root of the number.
	@availability Available on PowerPC CPUs only.
 */
void fsqrt3(double *arg1, double *arg2, double *arg3);

/*!
    @brief Fast calculation of reciprocal square root.
	@details
		Uses a ppc-only instruction to get an estimate of the
		reciprocal square root, then performs 4 iterations of
		Newton-Rhapson refinement to give better precision.
		Note: these functions are not IEEE-754 compliant and do not calculate 
		the last bit correctly. For correct accuary, refer to libm on MacOS X. 
		Written by A. Sazegari, started on February 2002. 
		Copyright 2002 Apple Computer, Inc. All rights reserved.
	@param arg Pointer to double-precision floating point number.
		Will be replaced with the reciprocal of the root of the number.
		(*arg = (*arg)^-0.5).
	@availability Available on PowerPC CPUs only.
*/
void frsqrt(double *arg);

/*!
    @brief Fast calculation of three reciprocal square roots.
	@details
		Uses a ppc-only instruction to get an estimate of the
		reciprocal square root, then performs 4 iterations of
		Newton-Rhapson refinement to give better precision.
		Note: these functions are not IEEE-754 compliant and do not calculate 
		the last bit correctly. For correct accuary, refer to libm on MacOS X. 
		Written by A. Sazegari, started on February 2002. 
		Copyright 2002 Apple Computer, Inc. All rights reserved.
	@param arg1 Pointer to double-precision floating point number.
		Will be replaced with the reciprocal of the root of the number.
	@param arg2 Pointer to double-precision floating point number.
		Will be replaced with the reciprocal of the root of the number.
	@param arg3 Pointer to double-precision floating point number.
		Will be replaced with the reciprocal of the root of the number.
	@availability Available on PowerPC CPUs only.
*/
void frsqrt3(double *arg1, double *arg2, double *arg3);
#endif // __ppc__

#ifdef __cplusplus
}
#endif

#endif                  /* !__EdenMath_h__ */
