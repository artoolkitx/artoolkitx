/*
 *  paramGL.h
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

/*!
	@file paramGL.h
	@brief Routines for converting ARParam and matrix types to OpenGL
	@details
		paramGL provides utility functions for setting the
		OpenGL viewing frustum and camera position based on artoolkitX-
		camera parameters and marker positions.
	@copyright 2015-2016 Daqri, LLC.
    @version 5.4
 */

#ifndef __AR_paramGL_h__
#define __AR_paramGL_h__

// ============================================================================
//	Public includes.
// ============================================================================

#include <ARX/AR/ar.h>
#include <ARX/AR/param.h>

#ifdef __cplusplus
extern "C" {
#endif
    
// ============================================================================
//	Public functions.
// ============================================================================
	
/*!
    @brief Create an OpenGL perspective projection matrix.
    @details
		Use this function to create a matrix suitable for passing to OpenGL
		to set the viewing projection.
    @param cparam Pointer to a set of artoolkitX camera parameters for the
		current video source.
    @param focalmax The maximum distance at which geometry will be rendered.
        Any geometry further away from the camera than this distance will be clipped
        and will not be appear in a rendered frame. Thus, this value should be
        set high enough to avoid clipping of any geometry you care about. However,
        the precision of the depth buffer is correlated with the ratio of focalmin
        to focalmax, thus you should not set focalmax any higher than it needs to be.
        This value should be specified in the same units as your OpenGL drawing.
    @param focalmin The minimum distance at which geometry will be rendered.
        Any geometry closer to the camera than this distance will be clipped
        and will not be appear in a rendered frame. Thus, this value should be
        set low enough to avoid clipping of any geometry you care about. However,
        the precision of the depth buffer is correlated with the ratio of focalmin
        to focalmax, thus you should not set focalmin any lower than it needs to be.
        Additionally, geometry viewed in a stereo projections that is too close to
        camera is difficult and tiring to view, so if you are rendering stereo
        perspectives you should set this value no lower than the near-point of
        he eyes. The near point in humans varies, but usually lies between 0.1 m
        0.3 m. This value should be specified in the same units as your OpenGL drawing.
    @param      m_projection (description)
*/
AR_EXTERN void arglCameraFrustumRH(const ARParam *cparam, const ARdouble focalmin, const ARdouble focalmax, ARdouble m_projection[16]);

/*!
    @brief Create an OpenGL viewing transformation matrix.
	@details
		Use this function to create a matrix suitable for passing to OpenGL
		to set the viewing transformation of the virtual camera.

        The matrix is formed so that the world coordinate system is placed
        at the centre of the marker, with the marker lying in the x-y plane.
        The positive x axis points to the right of the marker, the positive y
        axis to the top of the marker, and the positive z axis out of the marker.
        This is a right-hand coordinate system with the usual sense for OpenGL
        graphics.
	@param para Pointer to 3x4 matrix array of doubles which specify the
		position of an artoolkitX marker, as returned by arGetTransMat().
	@param m_modelview Pointer to a array of 16 GLdoubles, which will be filled
		out with a modelview matrix suitable for passing to OpenGL. The matrix
		is specified in column major order.
	@param scale Specifies a scaling between artoolkitX's
		units (usually millimeters) and OpenGL's coordinate system units.
		What you pass for the scalefactor parameter depends on what units you
		want to do your OpenGL drawing in. If you use a scalefactor of 1.0, then
		1.0 OpenGL unit will equal 1.0 millimetre (artoolkitX's default units).
		To use different OpenGL units, e.g. metres, then you would pass 0.001.
*/
AR_EXTERN void arglCameraViewRH(const ARdouble para[3][4], ARdouble m_modelview[16], const ARdouble scale);

#ifndef ARDOUBLE_IS_FLOAT
AR_EXTERN void arglCameraFrustumRHf(const ARParam *cparam, const float focalmin, const float focalmax, float m_projection[16]);
AR_EXTERN void arglCameraViewRHf(float para[3][4], float m_modelview[16], const float scale);
#else
#define arglCameraFrustumRHf arglCameraFrustumRH
#define arglCameraViewRHf arglCameraViewRH
#endif


#ifdef __cplusplus
}
#endif
#endif /* !__AR_paramGL_h__ */
