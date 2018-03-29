/*
 *  kpm.h
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
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

/*!
	@header kpm
	@brief libKPM NFT image recognition and tracking initialisation routines.
	@details
        This header declares types and API for an NFT tracker,
        in particular those routines involved in recognising a texture page and
        initialising the tracking for use by the texture tracker.
    @copyright 2006-2015 ARToolworks, Inc.
 */

#ifndef KPM_H
#define KPM_H

#define     BINARY_FEATURE    1

#include <ARX/AR/ar.h>
#include <ARX/KPM/kpmType.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#ifdef _WIN32
#  ifdef KPM_STATIC
#    define KPM_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define KPM_EXTERN __declspec(dllexport)
#    else
#      define KPM_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define KPM_EXTERN
#endif

#define   KpmPose6DOF            1
#define   KpmPoseHomography      2

typedef enum {
    KpmProcFullSize        = 1,
    KpmProcHalfSize        = 2,
    KpmProcQuatSize        = 3,
    KpmProcOneThirdSize    = 4,
    KpmProcTwoThirdSize    = 5
} KPM_PROC_MODE;
#define   KpmDefaultProcMode     KpmProcFullSize

#define   KpmCompNull            0
#define   KpmCompX               1
#define   KpmCompY               2
#define   KpmDefaultComp         KpmCompNull

#define   KpmChangePageNoAllPages (-1)

typedef struct {
    float             x;
    float             y;
} KpmCoord2D;

typedef struct {
    int               width;
    int               height;
    int               imageNo;
} KpmImageInfo;

typedef struct {
    KpmImageInfo     *imageInfo;
    int               imageNum;
    int               pageNo;
} KpmPageInfo;

typedef struct _KpmRefData {
    KpmCoord2D        coord2D;
    KpmCoord2D        coord3D;      // millimetres.
#if BINARY_FEATURE
    FreakFeature      featureVec;
#else
    SurfFeature       featureVec;
#endif
    int               pageNo;
    int               refImageNo;
} KpmRefData;

/*!
    @typedef    KpmRefDataSet
    @brief   A loaded dataset for KPM tracking.
    @details
        Key point matching takes as input a reference data set of points. This structure holds a set of points in memory prior to loading into the tracker.
	@field		refPoint Tracking reference points.
	@field		num Number of refPoints in the dataset.
	@field		pageInfo Array of info about each page in the dataset. One entry per page.
	@field		pageNum Number of pages in the dataset (i.e. a count, not an index).
 */
typedef struct {
    KpmRefData       *refPoint;
    int               num;
    KpmPageInfo      *pageInfo;
    int               pageNum;
} KpmRefDataSet;

/*!
    @typedef    KpmInputDataSet
    @brief   Data describing the number and location of keypoints in an input image to be matched against a loaded data set.
    @details
        Key point matching occurs between a loaded data set and a set of keypoints extracted from an input image. This structure
        holds the number and pixel location of keypoints in the input image. The keypoints themselves are an array of 'num'
        KpmRefData structures.
	@field		coord Array of pixel locations of the keypoints in an input image.
	@field		num Number of coords in the array.
  */
typedef struct {
    KpmCoord2D       *coord;
    int               num;
} KpmInputDataSet;

#if !BINARY_FEATURE
typedef struct {
    int               refIndex;
    int               inIndex;
} KpmMatchData;

typedef struct {
    KpmMatchData     *match;
    int               num;
} KpmMatchResult;
#endif

typedef struct {
    float                     camPose[3][4];
    int                       pageNo;
    float                     error;
    int                       inlierNum;
    int                       camPoseF;
    int                       skipF;
} KpmResult;

typedef struct _KpmHandle KpmHandle;


/*!
    @brief Allocate and initialise essential structures for KPM tracking, using full six degree-of-freedom tracking.
    @param cparamLT Pointer to an ARParamLT structure holding camera parameters in lookup-table form.
        The pointer only is copied, and the ARParamLT structure itself is NOT copied, and must remain
        valid for the lifetime of the KpmHandle.
        This structure also specifies the size of video frames which will be later supplied to the
        kpmMatching() function as cparamLT->param.xsize and cparamLT->param.ysize.
    @result Pointer to a newly-allocated KpmHandle structure. This structure must be deallocated
        via a call to kpmDeleteHandle() when no longer needed.
    @see kpmCreateHandleHomography kpmCreateHandleHomography
    @see kpmDeleteHandle kpmDeleteHandle
 */
KPM_EXTERN KpmHandle  *kpmCreateHandle (ARParamLT *cparamLT);
#define     kpmCreatHandle kpmCreateHandle

KPM_EXTERN KpmHandle  *kpmCreateHandle2(int xsize, int ysize);
#define     kpmCreatHandle2 kpmCreateHandle2

/*!
    @brief Allocate and initialise essential structures for KPM tracking, using homography-only tracking.
    @details
        Homography tracking assumes that the camera has zero lens-distortion, and this does not depend on
        camera parameters. It is therefore unable to provide correctly calibrated position measurements,
        but the resulting pose is suitable for visual overlay purposes.
    @param xsize Width of video frames which will be later supplied to the kpmMatching() function.
    @param ysize Height of video frames which will be later supplied to the kpmMatching() function.
    @result Pointer to a newly-allocated KpmHandle structure. This structure must be deallocated
        via a call to kpmDeleteHandle() when no longer needed.
    @see kpmCreateHandle kpmCreateHandle
    @see kpmDeleteHandle kpmDeleteHandle
 */
KPM_EXTERN KpmHandle  *kpmCreateHandleHomography(int xsize, int ysize);
#define     kpmCreatHandleHomography kpmCreateHandleHomography

/*!
    @brief Finalise and dispose of structures for KPM tracking.
    @details
        Once KPM processing has completed, this routine should be called to
        dispose of memory allocated.
    @param kpmHandle Pointer to a location which holds a pointer to a KpmHandle structure.
        On return, the location pointed to will be set to NULL.
    @result 0 if successful, or value &lt;0 in case of error.
    @see kpmCreateHandle kpmCreateHandle
    @see kpmCreateHandleHomography kpmCreateHandleHomography
 */
KPM_EXTERN int         kpmDeleteHandle( KpmHandle **kpmHandle );

KPM_EXTERN int         kpmHandleGetXSize(const KpmHandle *kpmHandle);
KPM_EXTERN int         kpmHandleGetYSize(const KpmHandle *kpmHandle);
    
KPM_EXTERN int         kpmSetProcMode( KpmHandle *kpmHandle, KPM_PROC_MODE  procMode );
KPM_EXTERN int         kpmGetProcMode( KpmHandle *kpmHandle, KPM_PROC_MODE *procMode );
KPM_EXTERN int         kpmSetDetectedFeatureMax( KpmHandle *kpmHandle, int  detectedMaxFeature );
KPM_EXTERN int         kpmGetDetectedFeatureMax( KpmHandle *kpmHandle, int *detectedMaxFeature );
KPM_EXTERN int         kpmSetSurfThreadNum( KpmHandle *kpmHandle, int surfThreadNum );

/*!
    @brief Load a reference data set into the key point matcher for tracking.
    @details
        This function takes a reference data set already in memory and makes it the current
        dataset for key point matching.
    @param kpmHandle Handle to the current KPM tracker instance, as generated by kpmCreateHandle or kpmCreateHandleHomography.
    @param refDataSet The reference data set to load into the KPM handle. The operation takes
        a copy of the data required from this dataset, thus unless the need for a further load
        at a later time is required, the dataset can be disposed of by calling kpmDeleteRefDataSet
        after this operation succeeds.
    @result 0 if successful, or value &lt;0 in case of error.
    @see kpmCreateHandle kpmCreateHandle
    @see kpmCreateHandleHomography kpmCreateHandleHomography
    @see kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
KPM_EXTERN int         kpmSetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet *refDataSet );

/*!
    @brief
        Loads a reference data set from a file into the KPM tracker.
    @details
        This is a convenience method which performs a sequence of kpmLoadRefDataSet, followed
        by kpmSetRefDataSet and finally kpmDeleteRefDataSet. When tracking from a single
        reference dataset file, this is the simplest means to start.
    @param kpmHandle Handle to the current KPM tracker instance, as generated by kpmCreateHandle or kpmCreateHandleHomography.
    @param filename Path to the dataset. Either full path, or a relative path if supported by
        the operating system.
    @param ext If non-NULL, a '.' charater and this string will be appended to 'filename'.
        Often, this parameter is a pointer to the string "fset3".
    @result Returns 0 if successful, or value &lt;0 in case of error.
    @see kpmLoadRefDataSet kpmLoadRefDataSet
    @see kpmSetRefDataSet kpmSetRefDataSet
    @see kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
KPM_EXTERN int         kpmSetRefDataSetFile( KpmHandle *kpmHandle, const char *filename, const char *ext );

KPM_EXTERN int         kpmSetRefDataSetFileOld( KpmHandle *kpmHandle, const char *filename, const char *ext );

/*!
    @brief Perform key-point matching on an image.
    @param kpmHandle
    @param inImageLuma Source image containing the pixels which will be searched for features.
        Typically, this is one frame from a video stream. The dimensions of this image must
        match the values specified at the time of creation of the KPM handle. Luma only.
    @result 0 if successful, or value &lt;0 in case of error.
    @see kpmCreateHandle kpmCreateHandle
    @see kpmCreateHandleHomography kpmCreateHandleHomography
 */
KPM_EXTERN int         kpmMatching(KpmHandle *kpmHandle, ARUint8 *inImageLuma);

KPM_EXTERN int         kpmSetMatchingSkipPage( KpmHandle *kpmHandle, int *skipPages, int num );
#if !BINARY_FEATURE
KPM_EXTERN int         kpmSetMatchingSkipRegion( KpmHandle *kpmHandle, SurfSubRect *skipRegion, int regionNum);
#endif

KPM_EXTERN int         kpmGetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet **refDataSet );
KPM_EXTERN int         kpmGetInDataSet( KpmHandle *kpmHandle, KpmInputDataSet **inDataSet );
#if !BINARY_FEATURE
KPM_EXTERN int         kpmGetMatchingResult( KpmHandle *kpmHandle, KpmMatchResult **preRANSAC, KpmMatchResult **aftRANSAC );
#endif
KPM_EXTERN int         kpmGetPose( KpmHandle *kpmHandle, float  pose[3][4], int *pageNo, float  *error );
KPM_EXTERN int         kpmGetResult( KpmHandle *kpmHandle, KpmResult **result, int *resultNum );


KPM_EXTERN int         kpmGenRefDataSet ( ARUint8 *refImage, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                               int pageNo, int imageNo, KpmRefDataSet **refDataSet );
KPM_EXTERN int         kpmAddRefDataSet ( ARUint8 *refImage, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                               int pageNo, int imageNo, KpmRefDataSet **refDataSet );

/*!
    @brief Merge a second KPM dataset into the first, and dispose of second.
    @details
        This function merges two KPM datasets by adding the reference points in
        the second into the first (allocating a new set if the location pointed to
        by refDataSetPtr1 is NULL) and then deleting the second set.
    @param refDataSetPtr1 Pointer to a location which points to the first data set, or pointer
        to NULL if a new dataset is to be created. This will hold the results of the merge.
    @param refDataSetPtr2 Pointer to a location which points to the second data set. After the
        merge, the dataset pointed to will be deleted and the location pointed to set to NULL.
    @result 0 if the merge succeeded, or a value &lt; 0 in case of error.
 */
KPM_EXTERN int         kpmMergeRefDataSet  ( KpmRefDataSet **refDataSetPtr1, KpmRefDataSet **refDataSetPtr2 );
#define     kpmMargeRefDataSet kpmMergeRefDataSet
    
/*!
    @brief Dispose of a reference data set and its allocated memory.
    @details
        Once a data set has been loaded into a KPM handle, or is otherwise no longer required
        to be held in memory, it should be deleted (i.e. disposed) from memory by calling
        this function.
    @param refDataSetPtr Pointer to memory location which points to the dataset. On success,
        this location will be set to NULL.
    @result 0 if the delete succeeded, or a value &lt; 0 in case of error.
    @see kpmLoadRefDataSet kpmLoadRefDataSet
 */
KPM_EXTERN int         kpmDeleteRefDataSet ( KpmRefDataSet **refDataSetPtr );

/*!
    @brief 
    @param filename
    @param ext
    @param refDataSet
    @result 
 */
KPM_EXTERN int         kpmSaveRefDataSet   ( const char *filename, const char *ext, KpmRefDataSet  *refDataSet );

/*!
    @brief Load a reference data set from the filesystem into memory.
    @details
        This does not set the reference data as the current tracking set. To do that, call
        kpmSetRefDataSet after this load completes. Alternately, the loaded set can be
        merged with another loaded set by calling kpmMergeRefDataSet. To dispose of the
        loaded dataset, call kpmDeleteRefDataSet.
    @param filename Path to the dataset. Either full path, or a relative path if supported by
        the operating system.
    @param ext If non-NULL, a '.' charater and this string will be appended to 'filename'.
        Often, this parameter is a pointer to the string "fset3".
    @result Returns 0 if successful, or value &lt;0 in case of error.
    @param refDataSetPtr Pointer to a location which after loading will point to the loaded
        reference data set.
    @result 0 if the load succeeded, or a value &lt; 0 in case of error.
    @see kpmSetRefDataSet kpmSetRefDataSet
    @see kpmMergeRefDataSet kpmMergeRefDataSet
    @see kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
KPM_EXTERN int         kpmLoadRefDataSet   ( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr );

KPM_EXTERN int         kpmLoadRefDataSetOld( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr );

/*!
    @brief 
    @param refDataSet
    @param oldPageNo
    @param newPageNo
    @result 
 */
KPM_EXTERN int         kpmChangePageNoOfRefDataSet ( KpmRefDataSet *refDataSet, int oldPageNo, int newPageNo );


/*!
    @brief 
    @param imageLuma Source luminance image, as an unpadded pixel buffer beginning with the leftmost pixel of the top row.
    @param xsize Width of pixel data in 'imageLuma'.
    @param ysize height of pixel data in 'imageLuma'.
    @param procMode
    @result 
 */
KPM_EXTERN ARUint8    *kpmUtilResizeImage( ARUint8 *imageLuma, int xsize, int ysize, int procMode, int *newXsize, int *newYsize );

#if !BINARY_FEATURE
KPM_EXTERN int         kpmUtilGetPose ( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *err );
    
KPM_EXTERN int         kpmUtilGetPose2( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, int *redDataIndex, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error );
KPM_EXTERN int         kpmUtilGetPoseHomography( KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *err );
#endif
KPM_EXTERN int         kpmUtilGetCorner( ARUint8 *inImagePtr, int xsize, int ysize, int procMode, int maxPointNum, CornerPoints *cornerPoints );


double wallclock(void);
    
KPM_EXTERN int kpmLoadImageDb(const char *filename);
    
#ifdef __cplusplus
}
#endif
#endif
