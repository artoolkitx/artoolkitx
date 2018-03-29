/*
 *  arFilterTransMat.h
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

/*!
    @file arFilterTransMat.h
    @brief   Provides filtering of transformation matrices (pose estimates).
    @details
        High frequency noise ("jitter") can be an undesirable property in
        optical tracking systems like artoolkitX. These functions implement a
        first-order low-pass filter for both the position and orientation
        components of the pose estimate transformation matrices.
 
        Usage:<br>
        For each pose estimate which is to be filtered, an ARFilterTransMatInfo
        structure should be allocated by calling arFilterTransMatInit(). Samples
        are then added to the filter and the filtered value returned by calling
        arFilterTransMat(). At the end of operations, the filter should be
        disposed of by calling arFilterTransMatFinal().
*/

#ifndef AR_FILTERTRANSMAT_H
#define AR_FILTERTRANSMAT_H

#include <ARX/AR/ar.h>

#ifdef __cplusplus
extern "C" {
#endif

    
typedef struct _ARFilterTransMatInfo ARFilterTransMatInfo;

/*!
    @defined 
    @brief   Default sample rate (in Hertz).
    @details This value provides a default value for the
        sampleRate parameter in arFilterTransMatInit().
*/
#define AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT 30.0
    
/*!
    @defined 
    @brief   Default cutoff frequency (in Hertz).
    @details This value provides a default value for the
        cutoffFreq parameter in arFilterTransMatInit().
 */
#define AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT 15.0

/*!
    @brief   Initialise a filter for a single transformation matrix (pose estimate).
    @details
        In order to filter a pose estimate, this function should be called to setup
        filter data structures.
 
        Each ARFilterTransMatInfo structure should be used only with a single transformation
        matrix. For example, if you have 5 pose estimates to filter, you would call this
        function 5 times, once for each pose, and keep the pose data and filter data paired.
 
        The structure produced should be passed to other filter functions dealing with
        the same transformation matrix.
    @param      sampleRate This value partially determines the filter properties.
        It should be as close as possible to the rate at which pose estimates arrive,
        which is usually the camera frame rate. If you are unsure what value to use,
        use the constant AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT. This value may not be 0.0.
    @param      cutoffFreq This value partially determines the filter properties.
        It should be as close as possible to the rate above which you do not wish
        changes to the incoming transformation matrix pose estimate to be observed.
        This would usually be the maximum rate at which you imagine to-and-fro rotiational
        or positional motion of a marker to occur. If you are unsure what value to use,
        use the constant AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT. This value may not be 0.0.
    @result     Pointer to an ARFilterTransMatInfo structure, which should be passed
        to other filter functions dealing with the same transformation matrix, or NULL
        in case of error.
*/
ARFilterTransMatInfo *arFilterTransMatInit(const ARdouble sampleRate, const ARdouble cutoffFreq);

/*!
    @brief   Set the filter parameters.
    @details
        This convenience function allows adjustment of the initial filter parameters.
    @param      ftmi Filter settings of which the parameters should be set.
    @param      sampleRate This value partially determines the filter properties.
        It should be as close as possible to the rate at which pose estimates arrive,
        which is usually the camera frame rate. If you are unsure what value to use,
        use the constant AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT. This value may not be 0.0.
    @param      cutoffFreq This value partially determines the filter properties.
        It should be as close as possible to the rate above which you do not wish
        changes to the incoming transformation matrix pose estimate to be observed.
        This would usually be the maximum rate at which you imagine to-and-fro rotiational
        or positional motion of a marker to occur. If you are unsure what value to use,
        use the constant AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT. This value may not be 0.0.
    @result
        0 The filter parameters were set without error.<br>
        -1   Invalid parameter.<br>
        -2   One of the parameters was 0.0.
*/
int arFilterTransMatSetParams(ARFilterTransMatInfo *ftmi, const ARdouble sampleRate, const ARdouble cutoffFreq);

/*!
    @brief   Filters the supplied pose estimate transformation matrix in-place.
    @details
        This performs the filter function for a single transformation matrix.
    @param      ftmi Filter settings to be used with this transformation matrix.
    @param      m Transformation matrix representing the current pose estimate.
    @param      reset
        If a discontinuity in transformation matrix pose estimates has occured
        (e.g. when a marker is first acquired, or is reacquired after a period of not being
        visible) the filter initial state or "memory" should be set to the
        same value as the current sample by setting this parameter to 1. If
        transformation matrix pose estimates have occured continuously, set this
        parameter to 0.
    @result
        0   No error.<br>
        -1   Invalid parameter.<br>
        -2   Invalid transformation matrix.
*/
int arFilterTransMat(ARFilterTransMatInfo *ftmi, ARdouble m[3][4], const int reset);

/*!
    @brief   Finalise a filter.
    @details
        When all filter use has completed, this function should be called to dispose
        of the filter structure.
    @param      ftmi Filter settings to dispose of.
*/
void arFilterTransMatFinal(ARFilterTransMatInfo *ftmi);
    
    
#ifdef __cplusplus
}
#endif

#endif // !AR_FILTERTRANSMAT_H