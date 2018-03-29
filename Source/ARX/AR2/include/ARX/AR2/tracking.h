/*
 *  AR2/tracking.h
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*!
	@header tracking
	@brief artoolkitX NFT core routines.
	@details This header declares essential types and API for the NFT portion of the
        artoolkitX SDK.
 
        For compile-time per-machine and NFT configuration, see &lt;AR2/config.h&gt;.
    @copyright 2015 Daqri, LLC.
 */

#ifndef AR2_TRACKING_H
#define AR2_TRACKING_H
#include <ARX/ARUtil/thread_sub.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>
#include <ARX/AR2/config.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/template.h>
#include <ARX/AR2/marker.h>

#define    AR2_TRACKING_6DOF                   1
#define    AR2_TRACKING_HOMOGRAPHY             2

#define    AR2_TRACKING_DEFAULT_THREAD_NUM    -1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    AR2ImageSetT         *imageSet;
    AR2FeatureSetT       *featureSet;
    AR2MarkerSetT        *markerSet;
    float                 trans[3][4];
    float                 itrans[3][4];
    char                 *jpegName;
} AR2SurfaceT;

typedef struct {
    AR2SurfaceT          *surface;
    int                   num;
    float                 trans1[3][4];
    float                 trans2[3][4];
    float                 trans3[3][4];
    int                   contNum;
    AR2TemplateCandidateT     prevFeature[AR2_SEARCH_FEATURE_MAX+1];
} AR2SurfaceSetT;

typedef struct {
    float             sim;
    float             pos2d[2];
    float             pos3d[3];
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    int               blurLevel;
#endif
} AR2Tracking2DResultT;


typedef struct _AR2HandleT           AR2HandleT;
typedef struct _AR2Tracking2DParamT  AR2Tracking2DParamT;

// Structure to pass parameters to threads spawned to run ar2Tracking2d().
struct _AR2Tracking2DParamT {
    struct _AR2HandleT      *ar2Handle;  // Reference to parent AR2HandleT.
    AR2SurfaceSetT          *surfaceSet;
    AR2TemplateCandidateT   *candidate;
    ARUint8                 *dataPtr;    // Input image.
    ARUint8                 *mfImage;    // (Internally allocated buffer same size as input image).
    AR2TemplateT            *templ;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    AR2Template2T           *templ2;
#endif
    AR2Tracking2DResultT     result;
    int                      ret;
};

struct _AR2HandleT {
    int               trackingMode;
    int               xsize;
    int               ysize;
    ARParamLT        *cparamLT;
    ICPHandleT       *icpHandle;
    AR_PIXEL_FORMAT   pixFormat;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    int               blurMethod;
    int               blurLevel;
#endif
    int               searchSize;
    int               templateSize1;
    int               templateSize2;
    int               searchFeatureNum;
    float             simThresh;
    float             trackingThresh;
    /*--------------------------------*/
    float                     wtrans1[AR2_TRACKING_SURFACE_MAX][3][4];
    float                     wtrans2[AR2_TRACKING_SURFACE_MAX][3][4];
    float                     wtrans3[AR2_TRACKING_SURFACE_MAX][3][4];
    float                     pos[AR2_SEARCH_FEATURE_MAX+AR2_THREAD_MAX][2];
    float                     pos2d[AR2_SEARCH_FEATURE_MAX][2];
    float                     pos3d[AR2_SEARCH_FEATURE_MAX][3];
    AR2TemplateCandidateT     candidate[AR2_TRACKING_CANDIDATE_MAX+1];
    AR2TemplateCandidateT     candidate2[AR2_TRACKING_CANDIDATE_MAX+1];
    AR2TemplateCandidateT     usedFeature[AR2_SEARCH_FEATURE_MAX];
    int                       threadNum;
    struct _AR2Tracking2DParamT       arg[AR2_THREAD_MAX];
    THREAD_HANDLE_T          *threadHandle[AR2_THREAD_MAX];
};


/*!
    Perform NFT texture tracking on an image frame.
        Before the first call to this function, ar2SetInitTrans() must be called to set
        the initial tracking transform. The initial transform may be obtained from NFT KPM
        tracking, or via a fiducial marker embedded in the NFT image. The initial transform
        must also be set after each loss of tracking (i.e. after each instance when this
        function does not return 0.
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param surfaceSet Tracking surface set, as returned via ar2ReadSurfaceSet.
    @param dataPtr Pointer to image data on which tracking will be performed.
    @param trans Pointer to a float[3][4] array which will be filled out with the pose.
    @param err On successful return, will be filled out with pose error value.
    @result 0 in case of successful tracking, or &lt; 0 in case of error.
        Error codes:<br>
        -1: Bad parameter<br>
        -2: Tracking not initialised<br>
        -3: Insufficient texture features<br>
        -4: Pose error exceeds value set with ar2SetTrackingThresh()
    @see ar2SetInitTrans ar2SetInitTrans
    @see ar2SetTrackingThresh ar2SetTrackingThresh
    @see ar2CreateHandle ar2CreateHandle
    @see ar2ReadSurfaceSet ar2ReadSurfaceSet
 */
int             ar2Tracking              ( AR2HandleT *ar2Handle, AR2SurfaceSetT *surfaceSet,
                                           ARUint8 *dataPtr, float  trans[3][4], float  *err );
void           *ar2Tracking2d            ( THREAD_HANDLE_T *threadHandle );
/*
int             ar2Tracking2d            ( AR2HandleT *ar2Handle, AR2SurfaceSetT *surfaceSet,
                                           AR2TemplateCandidateT *candidate,
                                           ARUint8 *dataPtr, AR2Tracking2DResultT *result    );
*/

/*!
    Read an NFT texture tracking surface set from file.
        Allocates, initialises and reads the contents of a surface set from storage.
        The surface set is usually generated by the genTexData utility, or equivalent.
        
        Once the surface set is no longer required, it should be disposed of by calling ar2FreeSurfaceSet().
    @param filename Pathname of the surface set to be loaded, less any filename extension.
    @param ext Filename extension of the surface set to be loaded. Ususally this will be "fset".
    @param pattHandle If the surface set includes, template markers, a valid ARPattHandle is required
        to be passed in this parameter. Otherwise, NULL may be passed.
    @result A pointer to the loaded AR2SurfaceSetT, or NULL in case of error.
    @see ar2FreeSurfaceSet ar2FreeSurfaceSet
 */
AR2SurfaceSetT *ar2ReadSurfaceSet        ( const char *filename, const char *ext, ARPattHandle *pattHandle          );

/*!
    Finalise and dispose of an NFT texture tracking surface set.
        Once a surface set (read by ar2ReadSurfaceSet()) is no longer required, it should be disposed
        of by calling ar2FreeSurfaceSet().
    @param surfaceSet Pointer to a location pointing to an AR2SurfaceSetT. On return,
        this pointer will be set to NULL.
    @result 0 if successful, -1 otherwise.
    @see ar2ReadSurfaceSet ar2ReadSurfaceSet
 */
int             ar2FreeSurfaceSet        ( AR2SurfaceSetT **surfaceSet                       );

/*!
    Sets initial transform for subsequent NFT texture tracking.
        Before the first call to ar2Tracking(), this function must be called to set
        the initial tracking transform. The initial transform may be obtained from NFT KPM
        tracking, or via a fiducial marker embedded in the NFT image. The initial transform
        must also be set after each loss of tracking (i.e. after each instance when
        ar2Tracking() does not return 0.
    @param surfaceSet
    @param trans Pointer to a float[3][4] array from which the initial transform will be copied.
    @result 0 if successful, or -1 in case of error.
    @see ar2Tracking ar2Tracking
 */
int             ar2SetInitTrans          ( AR2SurfaceSetT *surfaceSet, float  trans[3][4]    );

/*!
    Allocate and initialise essential structures for NFT texture tracking, using full six degree-of-freedom tracking.
        Full 6 degree-of-freedom tracking requires a calibrated camera lens model, and provides measurement
        of surface position in all axes, as well as orientation.
    @param cparamLT Pointer to an ARParamLT structure holding camera parameters in lookup-table form.
        The pointer only is copied, and the ARParamLT structure itself is NOT copied, and must remain
        valid for the lifetime of the AR2HandleT.
        This structure also specifies the size of video frames which will be later supplied to the
        ar2Tracking() function as cparamLT->param.xsize and cparamLT->param.ysize.
    @param pixFormat Pixel format of video frames which will be later supplied to the ar2Tracking() function.
    @param threadNum Number of threads to spawn for the NFT texture tracking task.
        Use AR2_TRACKING_DEFAULT_THREAD_NUM to have artoolkitX calculate a sensible default.
    @result Pointer to a newly allocated AR2HandleT structure, which holds the current state of the NFT
        texture tracker, or NULL if an error occurred.
        This structure must be deallocated via a call to ar2DeleteHandle() when no longer needed.
    @see ar2CreateHandleHomography ar2CreateHandleHomography
    @see ar2DeleteHandle ar2DeleteHandle
 */
AR2HandleT     *ar2CreateHandle          ( ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat, int threadNum );

/*!
    Allocate and initialise essential structures for NFT texture tracking, using homography-only tracking.
        Homography tracking assumes that the camera has zero lens-distortion, and this does not depend on
        camera parameters. It is therefore unable to provide correctly calibrated position measurements,
        but the resulting pose is suitable for visual overlay purposes.
    @param xsize Width of video frames which will be later supplied to the ar2Tracking() function.
    @param ysize Height of video frames which will be later supplied to the ar2Tracking() function.
    @param pixFormat Pixel format of video frames which will be later supplied to the ar2Tracking() function.
    @param threadNum Number of threads to spawn for the NFT texture tracking task.
        Use AR2_TRACKING_DEFAULT_THREAD_NUM to have artoolkitX calculate a sensible default.
    @result Pointer to a newly allocated AR2HandleT structure, which holds the current state of the NFT
        texture tracker, or NULL if an error occurred.
        This structure must be deallocated via a call to ar2DeleteHandle() when no longer needed.
    @see ar2CreateHandle ar2CreateHandle
    @see ar2DeleteHandle ar2DeleteHandle
 */
AR2HandleT     *ar2CreateHandleHomography      ( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat, int threadNum );

/*!
    Finalise and dispose of structures for NFT texture tracking.
        Once texture tracking processing has completed, this routine should be called to
        dispose of memory allocated.
    @param ar2Handle Pointer to a location which holds a pointer to a AR2HandleT structure.
        On return, the location pointed to will be set to NULL.
    @result -1 in case of error, or 0 otherwise.
    @see ar2CreateHandle ar2CreateHandle
    @see ar2CreateHandleHomography ar2CreateHandleHomography
 */
int             ar2DeleteHandle          ( AR2HandleT **ar2Handle );

#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
int             ar2SetBlurMethod         ( AR2HandleT *ar2Handle, int  blurMethod        );
int             ar2GetBlurMethod         ( AR2HandleT *ar2Handle, int *blurMethod        );
int             ar2SetBlurLevel          ( AR2HandleT *ar2Handle, int  blurLevel         );
int             ar2GetBlurLevel          ( AR2HandleT *ar2Handle, int *blurLevel         );
#endif

/*!
    Set feature point search window size.
        Sets the size of the window around a feature location from previous frame in which
        the feature will be searched for in the next frame. Value is radius of the search
        window, in pixels. I.e. searchSize pixels either size of the current feature position
        will be searched for the new location of the feature.
 
        A larger search window allows for greater movement of a feature between frames
        (e.g. faster optical motion, or same degree of optical motion but at a higher frame
        resolution), at the cost of greater search effort. Search effort increases with
        the square of the search radius.
 
        Default value is AR2_DEFAULT_SEARCH_SIZE, as defined in &lt;AR2/config.h&gt;
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param searchSize The new search size to use.
    @result -1 in case of error, or 0 otherwise.
    @see ar2GetSearchSize ar2GetSearchSize
 */
int             ar2SetSearchSize         ( AR2HandleT *ar2Handle, int  searchSize        );

/*!
    Get feature point search window size.
        See the discussion under ar2SetSearchSize.
 
        Default value is AR2_DEFAULT_SEARCH_SIZE, as defined in &lt;AR2/config.h&gt;
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param searchSize Pointer to an int, which on return will be filled with the current search size in use.
    @result -1 in case of error, or 0 otherwise.
    @see ar2SetSearchSize ar2SetSearchSize
 */
int             ar2GetSearchSize         ( AR2HandleT *ar2Handle, int *searchSize        );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param templateSize1
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2SetTemplateSize1      ( AR2HandleT *ar2Handle, int  templateSize1     );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param templateSize1
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2GetTemplateSize1      ( AR2HandleT *ar2Handle, int *templateSize1     );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param templateSize2
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2SetTemplateSize2      ( AR2HandleT *ar2Handle, int  templateSize2     );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param templateSize2
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2GetTemplateSize2      ( AR2HandleT *ar2Handle, int *templateSize2     );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param searchTemplateMax
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2SetSearchFeatureNum   ( AR2HandleT *ar2Handle, int  searchTemplateMax );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param searchTemplateMax
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2GetSearchFeatureNum   ( AR2HandleT *ar2Handle, int *searchTemplateMax );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param simThresh
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2SetSimThresh          ( AR2HandleT *ar2Handle, float   simThresh      );

/*!
    @brief
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param simThresh
    @result -1 in case of error, or 0 otherwise.
 */
int             ar2GetSimThresh          ( AR2HandleT *ar2Handle, float  *simThresh      );

/*!
    Set threshold value for acceptable pose estimate error.
        During the final phase of a single tracking pass, the pose estimate is calculated,
        along with an estimate of the error in this value (an uncertainty). A higher value
        indicates less goodness-of-fit of the pose estimate to the data. If only high-quality
        pose estimates are desired, this function can be used to lower the acceptable maximum
        error value.
 
        The actual error value itself is reported in parameter 'err' of function ar2Tracking.
 
        Default value is AR2_DEFAULT_TRACKING_THRESH, as defined in &lt;AR2/config.h&gt;
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param trackingThresh floating point value to use as the new tracking threshold.
    @result -1 in case of error, or 0 otherwise.
    @see ar2GetTrackingThresh ar2GetTrackingThresh
    @see ar2Tracking ar2Tracking
 */
int             ar2SetTrackingThresh     ( AR2HandleT *ar2Handle, float   trackingThresh );

/*!
    Get threshold value for acceptable pose estimate error.
        See the discussion under ar2SetTrackingThresh.
 
        Default value is AR2_DEFAULT_TRACKING_THRESH, as defined in &lt;AR2/config.h&gt;
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param trackingThresh Pointer to a float, which on return will be filled with the threshold value.
    @result -1 in case of error, or 0 otherwise.
    @see ar2SetTrackingThresh ar2SetTrackingThresh
*/
int             ar2GetTrackingThresh     ( AR2HandleT *ar2Handle, float  *trackingThresh );

/*!
    Choose whether full 6 degree-of-freedom tracking is performed, or homography extraction only.
        Note that while it is possible to switch an AR2HandleT created in 6DOF mode (via ar2CreateHandle) to
        HOMOGRAPHY mode and back to 6DOF ,an AR2HandleT created in HOMOGRAPHY mode (via ar2CreateHandleHomography)
        cannot be switched to 6DOF mode.
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param trackingMode Either AR2_TRACKING_6DOF or AR2_TRACKING_HOMOGRAPHY.
    @result -1 in case of error, or 0 otherwise.
    @see ar2CreateHandle ar2CreateHandle
    @see ar2CreateHandleHomography ar2CreateHandleHomography
    @see ar2GetTrackingMode ar2GetTrackingMode
 */
int             ar2SetTrackingMode       ( AR2HandleT *ar2Handle, int  trackingMode      );

/*!
    Report whether an AR2HandleT is providing full 6 degree-of-freedom tracking or homography extraction only.
    @param ar2Handle Tracking settings structure, as returned via ar2CreateHandle.
    @param trackingMode
    @result -1 in case of error, or 0 otherwise.
    @see ar2CreateHandle ar2CreateHandle
    @see ar2CreateHandleHomography ar2CreateHandleHomography
    @see ar2SetTrackingMode ar2SetTrackingMode
 */
int             ar2GetTrackingMode       ( AR2HandleT *ar2Handle, int *trackingMode      );

#ifdef __cplusplus
}
#endif
#endif
