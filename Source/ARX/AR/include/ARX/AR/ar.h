/*
 *  ar.h
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.01
 * Date: 2003/11/07
 *
 *******************************************************/

/*!
	@file ar.h
	@brief artoolkitX core routines.
	@details
        This header declares essential types and API for the entire
        artoolkitX SDK.

        For compile-time per-machine configuration, see &lt;AR/config.h&gt;.
        For compile-time artoolkitX configuration, see &lt;AR/arConfig.h&gt;.
	@copyright 2015-2016 Daqri, LLC.
 */

#ifndef AR_H
#define AR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ARX/AR/config.h>
#include <ARX/AR/arConfig.h>
#ifdef __ANDROID__
#  include <jni.h>
#  include <android/log.h>
#endif
#include <ARX/ARUtil/log.h>

#ifdef __cplusplus
extern "C" {
#endif


#define arMalloc(V,T,S)  \
{ if( ((V) = (T *)malloc( sizeof(T) * (S) )) == NULL ) \
{ARLOGe("Out of memory!!\n"); exit(1);} }

#define arMallocClear(V,T,S)  \
{ if( ((V) = (T *)calloc( (S), sizeof(T) )) == NULL ) \
{ARLOGe("Out of memory!!\n"); exit(1);} }

typedef char              ARInt8;
typedef short             ARInt16;
typedef int               ARInt32;
typedef unsigned char     ARUint8;
typedef unsigned short    ARUint16;
typedef unsigned int      ARUint32;
typedef float             ARfloat;
#ifdef ARDOUBLE_IS_FLOAT
typedef float             ARdouble;
#else
typedef double            ARdouble;
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#ifdef __cplusplus
}
#endif

#include <ARX/AR/matrix.h>
#include <ARX/AR/icp.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef AR_STATIC
#    define AR_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define AR_EXTERN __declspec(dllexport)
#    else
#      define AR_EXTERN __declspec(dllimport)
#    endif
#  endif
#else
#  define AR_EXTERN
#endif

/* --------------------------------------------------*/

/*!
    @brief A structure to hold a timestamp in seconds and microseconds, with arbitrary epoch.
 */
typedef struct {
    uint64_t sec;
    uint32_t usec;
} AR2VideoTimestampT;

/*!
    @brief A structure which carries information about a video frame retrieved by the video library.
    @see arVideoGetPixelFormat arVideoGetPixelFormat
 */
typedef struct {
    ARUint8            *buff;           ///< A pointer to the packed video data for this video frame. The video data pixel format is as specified by arVideoGetPixelFormat(). For multi-planar frames, this pointer is a copy of bufPlanes[0].
    ARUint8           **bufPlanes;      ///< For multi-planar video frames, this must be an array of length bufPlaneCount of (ARUint8*), into which will be copied pointers to the packed video data for each plane. For single-plane formats, this will be NULL.
    unsigned int        bufPlaneCount;  ///< For multi-planar video frames, this is the number of frame planes. For single-plane formats, this will be 0.
    ARUint8            *buffLuma;       ///< A pointer to a luminance-only version of the image. For luminance-only video formats this pointer is a copy of buff. For multi-planar formats which include a luminance-only plane, this pointer is a copy of one of the bufPlanes[] pointers. In all other cases, this pointer points to a buffer containing a copy of the video frame converted to luminance only.
    int                 fillFlag;       ///< Set non-zero when buff is valid.
    AR2VideoTimestampT  time;           ///< Time at which buff was filled.
} AR2VideoBufferT;

/*!
    @brief Values controlling the labeling thresholding mode.
 */
typedef enum {
    AR_LABELING_THRESH_MODE_MANUAL = 0,     ///< Manual threshold selection via arSetLabelingThresh.
    AR_LABELING_THRESH_MODE_AUTO_MEDIAN,    ///< Automatic threshold selection via full-image histogram median.
    AR_LABELING_THRESH_MODE_AUTO_OTSU,      ///< Automatic threshold selection via Otsu's method for foreground/background selection.
    AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE,  ///< Adaptive thresholding.
    AR_LABELING_THRESH_MODE_AUTO_BRACKETING ///< Automatic threshold selection via heuristic-based exposure bracketing.
} AR_LABELING_THRESH_MODE;

/*!
    @brief Captures detail of a trapezoidal region which is a candidate for marker detection.
 */
typedef struct {
    int             area;                   ///< Area in pixels.
    ARdouble        pos[2];                 ///< Center.
    int             coord_num;              ///< Number of coordinates in x_coord, y_coord.
    int             x_coord[AR_CHAIN_MAX];  ///< X values of coordinates.
    int             y_coord[AR_CHAIN_MAX];  ///< Y values of coordinates.
    int             vertex[5];              ///< Vertices.
} ARMarkerInfo2;

/*!
    @brief Result codes returned by arDetectMarker to report state of individual detected trapezoidal regions.

    When detecting markers, all trapezoidal regions in the incoming image are considered for
    marker matching. Various heuristics are used to reject regions judged to be non-markers.
    The code will, as far as possible, report rejection by placing one of these constants
    into the ARMarkerInfo.cutoffPhase field of regions rejected during the arDetectMarker() call.
    Note that the ARMarkerInfo.id of such rejected regions will be -1.
 */
typedef enum {
    AR_MARKER_INFO_CUTOFF_PHASE_NONE,                   ///< Marker OK.
    AR_MARKER_INFO_CUTOFF_PHASE_PATTERN_EXTRACTION,     ///< Failure during pattern extraction.
    AR_MARKER_INFO_CUTOFF_PHASE_MATCH_GENERIC,          ///< Generic error during matching phase.
    AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONTRAST,         ///< Insufficient contrast during matching.
    AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_NOT_FOUND,///< Barcode matching could not find correct barcode locator pattern.
    AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_EDC_FAIL, ///< Barcode matching error detection/correction found unrecoverable error.
    AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE,       ///< Matching confidence cutoff value not reached.
    AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR,             ///< Maximum allowable pose error exceeded.
    AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR_MULTI,       ///< Multi-marker pose error value exceeded.
    AR_MARKER_INFO_CUTOFF_PHASE_HEURISTIC_TROUBLESOME_MATRIX_CODES ///< Heuristic-based rejection of troublesome matrix code which is often generated in error.
} AR_MARKER_INFO_CUTOFF_PHASE;

#define AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT 10
AR_EXTERN extern const char *arMarkerInfoCutoffPhaseDescriptions[AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT];

/*!
    @brief   Describes a detected trapezoidal area (a candidate for a marker match).
    @details
        After marker detection, a number of trapezoidal areas in the camera image will have been identified. An
        ARMarkerInfo struct is returned for each area so matched. Trapezoidal areas which have been matched
        with marker images (in pattern mode) or barcodes (in matrix mode) will have valid values assigned to the
        appropriate id field.
 */
typedef struct {
    int             area;                   ///< Area in pixels of the largest connected region, comprising the marker border and regions connected to it. Note that this is not the same as the actual onscreen area inside the marker border.
    int             id;                     ///< If pattern detection mode is either pattern mode OR matrix but not both, will be marker ID (>= 0) if marker is valid, or -1 if invalid.
    int             idPatt;                 ///< If pattern detection mode includes a pattern mode, will be marker ID (>= 0) if marker is valid, or -1 if invalid.
    int             idMatrix;               ///< If pattern detection mode includes a matrix mode, will be marker ID (>= 0) if marker is valid, or -1 if invalid.
    int             dir;                    ///< If pattern detection mode is either pattern mode OR matrix but not both, and id != -1, will be marker direction (range 0 to 3, inclusive).
    int             dirPatt;                ///< If pattern detection mode includes a pattern mode, and id != -1, will be marker direction (range 0 to 3, inclusive).
    int             dirMatrix;              ///< If pattern detection mode includes a matrix mode, and id != -1, will be marker direction (range 0 to 3, inclusive).
    ARdouble        cf;                     ///< If pattern detection mode is either pattern mode OR matrix but not both, will be marker matching confidence (range 0.0 to 1.0 inclusive) if marker is valid, or -1.0 if marker is invalid.
    ARdouble        cfPatt;                 ///< If pattern detection mode includes a pattern mode, will be marker matching confidence (range 0.0 to 1.0 inclusive) if marker is valid, or -1.0 if marker is invalid.
    ARdouble        cfMatrix;               ///< If pattern detection mode includes a matrix mode, will be marker matching confidence (range 0.0 to 1.0 inclusive) if marker is valid, or -1.0 if marker is invalid.
    ARdouble        pos[2];                 ///< 2D position (in camera image coordinates, origin at top-left) of the centre of the marker.
    ARdouble        line[4][3];             ///< Line equations for the 4 sides of the marker.
    ARdouble        vertex[4][2];           ///< 2D positions (in camera image coordinates, origin at top-left) of the corners of the marker. vertex[(4 - dir)%4][] is the top-left corner of the marker. Other vertices proceed clockwise from this. These are idealised coordinates (i.e. the onscreen position aligns correctly with the undistorted camera image.)
    ARMarkerInfo2  *markerInfo2Ptr;         ///< Pointer to source region info for this marker.
    AR_MARKER_INFO_CUTOFF_PHASE cutoffPhase;///< If a trapezoidal region is detected, but is eliminated from the candidates for tracking, this field is filled out with the tracking phase at which the marker was cut off. An English-language description of the phase can be obtained by indexing into the C-string array arMarkerInfoCutoffPhaseDescriptions[].
    int             errorCorrected;         ///< For marker types including error detection and correction, the numbers of errors detected and corrected.
    uint64_t        globalID;               ///< If arPattDetectionMode is a matrix mode, matrixCodeType is AR_MATRIX_CODE_GLOBAL_ID, and idMatrix >= 0, will contain the globalID.
} ARMarkerInfo;

/*!
	@brief   (description)
	@details (description)
 */
typedef struct {
    ARMarkerInfo    marker;         ///< 
    int             count;          ///< 
} ARTrackingHistory;

/*!
	@brief   (description)
	@details (description)
 */
typedef struct {
    AR_LABELING_LABEL_TYPE *labelImage;
#if !AR_DISABLE_LABELING_DEBUG_MODE
    ARUint8        *bwImage;
#endif
    int             label_num;
    int             area[AR_LABELING_WORK_SIZE];
    int             clip[AR_LABELING_WORK_SIZE][4];
    ARdouble        pos[AR_LABELING_WORK_SIZE][2];
    int             work[AR_LABELING_WORK_SIZE];
    int             work2[AR_LABELING_WORK_SIZE*7]; ///< area, pos[2], clip[4].
} ARLabelInfo;

/* --------------------------------------------------*/

/*!
    @brief   A structure which holds descriptions of trained patterns for template matching.
    @details Template (picture)-based pattern matching requires details of the pattern
        to be supplied to the matching functions. This structure holds such details. It is
        generally setup by loading pattern files from disk.
*/
typedef struct {
    int             patt_num;       ///< Number of valid patterns in the structure.
    int             patt_num_max;   ///< Maximum number of patterns that may be loaded in this structure.
    int            *pattf;          ///< 0 = no pattern loaded at this position. 1 = pattern loaded and activated. 2 = pattern loaded but deactivated.
    int           **patt;           ///< Array of 4 different orientations of each pattern's colour values, in 1-byte per component BGR order.
    ARdouble       *pattpow;        ///< Root-mean-square of the pattern intensities.
    int           **pattBW;         ///< Array of 4 different orientations of each pattern's 1-byte luminosity values.
    ARdouble       *pattpowBW;      ///< Root-mean-square of the pattern intensities.
    //ARdouble        pattRatio;      ///< 
    int             pattSize;       ///< Number of rows/columns in the pattern.
} ARPattHandle;

/*!
    @brief Defines a pattern rectangle as a sub-portion of a marker image.
    @details A complete marker image has coordinates {0.0f, 0.0f, 1.0f, 1.0f}.
        A standard artoolkitX marker with a pattern ratio of 0.5 has coordinates
        {0.25f, 0.25f, 0.75f, 0.75f}.
 */
typedef struct {
    float   topLeftX;       ///< Horizontal coordinate of the top left corner of the pattern space, in range 0.0f-1.0f.
    float   topLeftY;       ///< Vertical coordinate of the top left corner of the pattern space, in range 0.0f-1.0f.
    float   bottomRightX;   ///< Horizontal coordinate of the bottom right corner of the pattern space, in range 0.0f-1.0f.
    float   bottomRightY;   ///< Vertical coordinate of the bottom right corner of the pattern space, in range 0.0f-1.0f.
} ARPattRectInfo;

/* --------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#include <ARX/AR/param.h>
#include <ARX/AR/arImageProc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AR_MATRIX_CODE_TYPE_SIZE_MASK 0x000000ff  ///< Mask value, bitwise-OR with matrix code type to find matrix code size.
#define AR_MATRIX_CODE_TYPE_ECC_NONE 0x00000000   ///< No error detection or correction.
#define AR_MATRIX_CODE_TYPE_ECC_PARITY 0x00000100 ///< Single-bit parity.
#define AR_MATRIX_CODE_TYPE_ECC_HAMMING 0x00000200 ///< Hamming code with Hamming distance of 3.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___3 0x00000300 ///< BCH code with Hamming distance of 3.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___5 0x00000400 ///< BCH code with Hamming distance of 5.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___7 0x00000500 ///< BCH code with Hamming distance of 7.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___9 0x00000600 ///< BCH code with Hamming distance of 9.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___11 0x00000700 ///< BCH code with Hamming distance of 11.
#define AR_MATRIX_CODE_TYPE_ECC_BCH___19 0x00000b00 ///< BCH code with Hamming distance of 19.

/*!
    @brief Values specifying the type of matrix code in use.
 */
typedef enum {
    AR_MATRIX_CODE_3x3 = 0x03,                                                  ///< Matrix code in range 0-63.
    AR_MATRIX_CODE_3x3_PARITY65 = 0x03 | AR_MATRIX_CODE_TYPE_ECC_PARITY,        ///< Matrix code in range 0-31.
    AR_MATRIX_CODE_3x3_HAMMING63 = 0x03 | AR_MATRIX_CODE_TYPE_ECC_HAMMING,      ///< Matrix code in range 0-7.
    AR_MATRIX_CODE_4x4 = 0x04,                                                  ///< Matrix code in range 0-8191.
    AR_MATRIX_CODE_4x4_BCH_13_9_3 = 0x04 | AR_MATRIX_CODE_TYPE_ECC_BCH___3,     ///< Matrix code in range 0-511.
    AR_MATRIX_CODE_4x4_BCH_13_5_5 = 0x04 | AR_MATRIX_CODE_TYPE_ECC_BCH___5,     ///< Matrix code in range 0-31.
    AR_MATRIX_CODE_5x5_BCH_22_12_5 = 0x05 | AR_MATRIX_CODE_TYPE_ECC_BCH___5,    ///< Matrix code in range 0-4095.
    AR_MATRIX_CODE_5x5_BCH_22_7_7 = 0x05 | AR_MATRIX_CODE_TYPE_ECC_BCH___7,     ///< Matrix code in range 0-127.
    AR_MATRIX_CODE_5x5 = 0x05,                                                  ///< Matrix code in range 0-4194303.
    AR_MATRIX_CODE_6x6 = 0x06,                                                  ///< Matrix code in range 0-8589934591.
    AR_MATRIX_CODE_GLOBAL_ID = 0x0e | AR_MATRIX_CODE_TYPE_ECC_BCH___19
} AR_MATRIX_CODE_TYPE;

/*!
    @brief   Structure holding state of an instance of the square marker tracker.
    @details
        This is the master object holding the current state of an intance of the square
        marker tracker, including tracker configuration, working variables, and results.
    @see        arCreateHandle
    @see        arDeteleHandle
 */
typedef struct {
    int                arDebug;
    AR_PIXEL_FORMAT    arPixelFormat;
    int                arPixelSize;
    int                arLabelingMode;
    int                arLabelingThresh;
    int                arImageProcMode;                     ///< To query this value, call arGetImageProcMode(). To set this value, call arSetImageProcMode().
    int                arPatternDetectionMode;
    int                arMarkerExtractionMode;
    ARParamLT         *arParamLT;
    int                xsize;
    int                ysize;
    int                marker_num;
    ARMarkerInfo       markerInfo[AR_SQUARE_MAX];
    int                marker2_num;
    ARMarkerInfo2      markerInfo2[AR_SQUARE_MAX];
    int                history_num;
    ARTrackingHistory  history[AR_SQUARE_MAX];
    ARLabelInfo        labelInfo;
    ARPattHandle      *pattHandle;
    AR_LABELING_THRESH_MODE arLabelingThreshMode;
    int                arLabelingThreshAutoInterval;
    int                arLabelingThreshAutoIntervalTTL;
    int                arLabelingThreshAutoBracketOver;
    int                arLabelingThreshAutoBracketUnder;
    int                arLabelingThreshAutoAdaptiveKernelSize;
    int                arLabelingThreshAutoAdaptiveBias;
    ARImageProcInfo   *arImageProcInfo;
    ARdouble           pattRatio;                           ///< A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    AR_MATRIX_CODE_TYPE matrixCodeType;                     ///< When matrix code pattern detection mode is active, indicates the type of matrix code to detect.
    int                arCornerRefinementMode;
    ARdouble           areaMax;
    ARdouble           areaMin;
    ARdouble           squareFitThresh;
} ARHandle;


/* --------------------------------------------------*/

/*!
    @brief   Structure holding state of an instance of the monocular pose estimator.
    @details (description)
*/
typedef struct {
    ICPHandleT          *icpHandle;
} AR3DHandle;

#define   AR_TRANS_MAT_IDENTITY            ICP_TRANS_MAT_IDENTITY

/*!
    @brief   Structure holding state of an instance of the stereo pose estimator.
    @details (description)
*/
typedef struct {
    ICPStereoHandleT    *icpStereoHandle;
} AR3DStereoHandle;


/***********************************/
/*                                 */
/*    For square detection         */
/*                                 */
/***********************************/

/*!
	@functiongroup "Square detection".
 */
/*!
    @brief   Create a handle to hold settings for an artoolkitX tracker instance.
    @details
        ARHandle is the primary structure holding the settings for a single artoolkitX
        square marker tracking instance. Settings include expected video stream image
        size and pixel format, tracking modes, loaded markers and more.

        Expected video stream image size is taken directly from the supplied ARParamLT
        structure's xsize and ysize fields. Video stream image pixel format must be set
        by a subsequent call to arSetPixelFormat() to set the correct format.

        After creation of the ARHandle, tracking settings should be set via appropriate
        calls to other arSet*() functions.

        The ARHandle should be disposed of via a call to arDeleteHandle when tracking
        with this instance is complete.
    @param      paramLT The created handle will hold a pointer to the calibrated
		camera parameters specified by this parameter. This parameter uses the new
        lookup-table based form of the camera parameters introduced in ARToolKit v5.
        An ARParamLT structure may be created from an ARParam structure via the
        call:
        <code>ARParamLT *paramLT = arParamLTCreate(&param, AR_PARAM_LT_DEFAULT_OFFSET);</code>
        Note that the pointer is only copied, and so the ARParamLT structure must remain
        valid until the ARHandle is disposed of by calling arDeleteHandle.
    @result     An ARHandle which should be passed to other functions which
		deal with the operations of the artoolkitX tracker.
    @see arSetPixelFormat
    @see arDeleteHandle
*/
AR_EXTERN ARHandle *arCreateHandle( ARParamLT *paramLT );

/*!
    @brief   Delete a handle which holds settings for an artoolkitX tracker instance.
	@details The calibrated camera parameters pointed to by the handle are
		NOT deleted by this operation.
    @param      handle The handle to delete, as created by arCreateHandle();
    @result     0 if no error occured.
    @see arCreateHandle
*/
AR_EXTERN int arDeleteHandle( ARHandle *handle );

/*!
    @brief   Enable or disable artoolkitX's debug mode.
    @details In debug mode, artoolkitX offers additional error reporting. Use
        this function to enable or disable debug mode at runtime.

        Additionally, in debug mode, artoolkitX creates a mono (8-bit grayscale)
        image of the thresholded video input, and makes this available
        through the field ARHandle->labelInfo.bwImage.
    @param      handle An ARHandle referring to the current AR tracker
		in which debug mode is to be set.
	@param      mode
		Options for this field are:
		AR_DEBUG_DISABLE
		AR_DEBUG_ENABLE
		The default mode is AR_DEBUG_DISABLE.
    @see arGetDebugMode
*/
AR_EXTERN void arSetDebugMode(ARHandle *handle, int mode);

/*!
    @brief   Find out whether artoolkitX's debug mode is enabled.
    @details See arSetDebugMode() for more info.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its mode.
    @result Value representing the mode.
    @see arSetDebugMode
*/
AR_EXTERN int arGetDebugMode(ARHandle *handle);

/*!
	@brief   Select between detection of black markers and white markers.
	@details
        artoolkitX's labelling algorithm can work with both black-bordered
        markers on a white background (AR_LABELING_BLACK_REGION) or
        white-bordered markers on a black background (AR_LABELING_WHITE_REGION).
        This function allows you to specify the type of markers to look for.
        Note that this does not affect the pattern-detection algorith
        which works on the interior of the marker.
	@param      handle An ARHandle referring to the current AR tracker
		to have its labeling mode set.
	@param      mode
		Options for this field are:
		AR_LABELING_WHITE_REGION
		AR_LABELING_BLACK_REGION
		The default mode is AR_LABELING_BLACK_REGION.
    @see arGetLabelingMode
 */
AR_EXTERN void arSetLabelingMode(ARHandle *handle, int mode);

/*!
    @brief   Enquire whether detection is looking for black markers or white markers.
    @details See discussion for arSetLabelingMode.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its labeling mode.
    @result     Value representing the mode.
    @see arSetLabelingMode
*/
AR_EXTERN int arGetLabelingMode(ARHandle *handle);

/*!
    @brief   Set the labeling threshhold.
    @details
        This function forces sets the threshold value.
        The default value is AR_DEFAULT_LABELING_THRESH which is 100,
        unless edited in arConfig.h.

        The current threshold mode is not affected by this call.
        Typically, this function is used when labeling threshold mode
        is AR_LABELING_THRESH_MODE_MANUAL.

        The threshold value is not relevant if threshold mode is
        AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE.

        Background: The labeling threshold is the value which
		the AR library uses to differentiate between black and white
		portions of an artoolkitX marker. Since the actual brightness,
		contrast, and gamma of incoming images can vary signficantly
		between different cameras and lighting conditions, this
		value typically needs to be adjusted dynamically to a
		suitable midpoint between the observed values for black
		and white portions of the markers in the image.
	@param      handle An ARHandle referring to the current AR tracker
		to have its labeling threshold value set.
	@param      thresh An integer in the range [0,255] (inclusive).
    @see arGetLabelingThresh
*/
AR_EXTERN void arSetLabelingThresh(ARHandle *handle, int thresh);

/*!
    @brief   Get the current labeling threshold.
    @details
        This function queries the current labeling threshold. For,
        AR_LABELING_THRESH_MODE_AUTO_MEDIAN, AR_LABELING_THRESH_MODE_AUTO_OTSU,
        and AR_LABELING_THRESH_MODE_AUTO_BRACKETING
        the threshold value is only valid until the next auto-update.

        The current threshold mode is not affected by this call.

        The threshold value is not relevant if threshold mode is
        AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its labeling threshold value.
    @result     Value of the labeling threshhold.
        An integer in the range [0,255] (inclusive)
    @see arSetLabelingThresh
*/
AR_EXTERN int arGetLabelingThresh(ARHandle *handle);

/*!
    @brief   Set the labeling threshold mode (auto/manual).
    @param      handle An ARHandle referring to the current AR tracker
        to be queried for its labeling threshold mode.
    @param		mode An integer specifying the mode. One of:
        AR_LABELING_THRESH_MODE_MANUAL,
        AR_LABELING_THRESH_MODE_AUTO_MEDIAN,
        AR_LABELING_THRESH_MODE_AUTO_OTSU,
        AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE,
        AR_LABELING_THRESH_MODE_AUTO_BRACKETING
    @result     0 if no error occured.
    @see arSetLabelingThresh
    @see arGetLabelingThreshMode
 */
AR_EXTERN void arSetLabelingThreshMode(ARHandle *handle, const AR_LABELING_THRESH_MODE mode);

/*!
    @brief   Get the labeling threshold mode (auto/manual).
    @param      handle An ARHandle referring to the current AR tracker
        to be queried for its labeling threshold value.
    @result Value of the labeling threshold mode, one of:
        AR_LABELING_THRESH_MODE_MANUAL,
        AR_LABELING_THRESH_MODE_AUTO_MEDIAN,
        AR_LABELING_THRESH_MODE_AUTO_OTSU,
        AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE,
        AR_LABELING_THRESH_MODE_AUTO_BRACKETING
    @see arSetLabelingThresh
    @see arSetLabelingThreshMode
 */
AR_EXTERN AR_LABELING_THRESH_MODE arGetLabelingThreshMode(const ARHandle *handle);

/*!
    @brief   Set the number of frames between auto-threshold calculations.
    @details
        This is the number of frames BETWEEN calculations, meaning that the
        calculation occurs every (interval + 1) frames.
    @param      handle An ARHandle referring to the current AR tracker
        for which the labeling threshold auto interval will be set.
    @param		interval The interval, specifying the number of frames between
        automatic updates to the threshold.
        An integer in the range [0,INT_MAX] (inclusive). Default
        value is AR_LABELING_THRESH_AUTO_INTERVAL_DEFAULT.
    @see arGetLabelingThreshModeAutoInterval
 */
AR_EXTERN void arSetLabelingThreshModeAutoInterval(ARHandle *handle, const int interval);

/*!
    @brief   Get the number of frames between auto-threshold calculations.
    @details
        This is the number of frames BETWEEN calculations, meaning that the
        calculation occurs every (interval + 1) frames.
    @param      handle An ARHandle referring to the current AR tracker
        to be queried for its labeling threshold auto interval value.
    @result Value of the labeling threshhold auto interval.
        An integer in the range [0,INT_MAX] (inclusive)
    @see arSetLabelingThreshModeAutoInterval
 */
AR_EXTERN int arGetLabelingThreshModeAutoInterval(const ARHandle *handle);

AR_EXTERN void arSetLabelingThreshAutoAdaptiveKernelSize(ARHandle *handle, const int labelingThreshAutoAdaptiveKernelSize);

AR_EXTERN int arGetLabelingThreshAutoAdaptiveKernelSize(ARHandle *handle);

AR_EXTERN void arSetLabelingThreshAutoAdaptiveBias(ARHandle *handle, const int labelingThreshAutoAdaptiveBias);

AR_EXTERN int arGetLabelingThreshAutoAdaptiveBias(ARHandle *handle);
    
/*!
    @brief   Set the image processing mode.
    @details
        When ARthe image processing mode is AR_IMAGE_PROC_FRAME_IMAGE,
        artoolkitX processes all pixels in each incoming image
        to locate markers. When the mode is AR_IMAGE_PROC_FIELD_IMAGE,
        artoolkitX processes pixels in only every second pixel row and
        column. This is useful both for handling images from interlaced
        video sources (where alternate lines are assembled from alternate
        fields and thus have one field time-difference, resulting in a
        "comb" effect) such as Digital Video cameras.
        The effective reduction by 75% in the pixels processed also
        has utility in accelerating tracking by effectively reducing
        the image size to one quarter size, at the cost of pose accuraccy.
	@param      handle An ARHandle referring to the current AR tracker
		to have its mode set.
    @param      mode
		Options for this field are:
		AR_IMAGE_PROC_FRAME_IMAGE
		AR_IMAGE_PROC_FIELD_IMAGE
		The default mode is AR_IMAGE_PROC_FRAME_IMAGE.
    @see arGetImageProcMode
 */
AR_EXTERN void arSetImageProcMode(ARHandle *handle, int mode);

/*!
    @brief   Get the image processing mode.
    @details
		See arSetImageProcMode() for a complete description.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its mode.
	@result Value representing the current image processing mode.
    @see arSetImageProcMode
 */
AR_EXTERN int arGetImageProcMode(ARHandle *handle);

/*!
    @brief   Set the pattern detection mode
    @details
        The pattern detection determines the method by which artoolkitX
        matches detected squares in the video image to marker templates
        and/or IDs. ARToolKit v4.x can match against pictorial "template" markers,
        whose pattern files are created with the mk_patt utility, in either colour
        or mono, and additionally can match against 2D-barcode-type "matrix"
        markers, which have an embedded marker ID. Two different two-pass modes
        are also available, in which a matrix-detection pass is made first,
        followed by a template-matching pass.
	@param      handle An ARHandle referring to the current AR tracker
		to have its mode set.
	@param      mode
		Options for this field are:
		AR_TEMPLATE_MATCHING_COLOR
		AR_TEMPLATE_MATCHING_MONO
		AR_MATRIX_CODE_DETECTION
		AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX
		AR_TEMPLATE_MATCHING_MONO_AND_MATRIX
		The default mode is AR_TEMPLATE_MATCHING_COLOR.
    @see arGetPatternDetectionMode
 */
AR_EXTERN void arSetPatternDetectionMode(ARHandle *handle, int mode);

/*!
    @brief   Get the pattern detection mode
    @details
        See arSetPatternDetectionMode() for a complete description.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its mode.
    @result     Value representing the mode.
    @see arGetPatternDetectionMode
 */
AR_EXTERN int arGetPatternDetectionMode(ARHandle *handle);

/*!
    @brief   Set the size and ECC algorithm to be used for matrix code (2D barcode) marker detection.
    @details
        When matrix-code (2D barcode) marker detection is enabled (see arSetPatternDetectionMode)
        then the size of the barcode pattern and the type of error checking and correction (ECC)
        with which the markers were produced can be set via this function.

        This setting is global to a given ARHandle; It is not possible to have two different matrix
        code types in use at once.
    @param      handle An ARHandle referring to the current AR tracker to have its mode set.
    @param      type The type of matrix code (2D barcode) in use. Options include:
        AR_MATRIX_CODE_3x3
        AR_MATRIX_CODE_3x3_HAMMING63
        AR_MATRIX_CODE_3x3_PARITY65
        AR_MATRIX_CODE_4x4
        AR_MATRIX_CODE_4x4_BCH_13_9_3
        AR_MATRIX_CODE_4x4_BCH_13_5_5
        AR_MATRIX_CODE_5x5_BCH_22_12_5
        AR_MATRIX_CODE_5x5_BCH_22_7_7
        The default mode is AR_MATRIX_CODE_3x3.
    @see arSetPatternDetectionMode
    @see arGetMatrixCodeType
 */
AR_EXTERN void arSetMatrixCodeType(ARHandle *handle, const AR_MATRIX_CODE_TYPE type);

/*!
    @brief   Get the size and ECC algorithm being used for matrix code (2D barcode) marker detection.
    @details See the description for arSetMatrixCodeType().
    @param      handle An ARHandle referring to the current AR tracker to be queried for its mode.
    @result		The value representing the mode.
    @see    arGetPatternDetectionMode
    @see    arSetMatrixCodeType
*/
AR_EXTERN AR_MATRIX_CODE_TYPE arGetMatrixCodeType(ARHandle *handle);

/*!
    @brief   Set the marker extraction mode
    @details (description)
	@param      handle An ARHandle referring to the current AR tracker to have its mode set.
	@param      mode
		Options for this field are:
		AR_USE_TRACKING_HISTORY
		AR_NOUSE_TRACKING_HISTORY
		AR_USE_TRACKING_HISTORY_V2
		The default mode is AR_USE_TRACKING_HISTORY_V2.
    @see arGetMarkerExtractionMode
 */
AR_EXTERN void arSetMarkerExtractionMode(ARHandle *handle, int mode);

/*!
    @brief   Get the marker extraction mode
    @details (description)
    @param      handle An ARHandle referring to the current AR tracker to be queried for its mode.
    @result     The value representing the mode.
    @see arSetMarkerExtractionMode
 */
AR_EXTERN int arGetMarkerExtractionMode(ARHandle *handle);

/*!
    @brief   Set the border size.
    @details N.B. Deprecated in favour of arSetPattRatio(), but retained for
        backwards compatibility.
    @param      handle An ARHandle referring to the current AR tracker
        to have its border size set.
    @param      borderSize The border size. To set the default, pass (1.0 - 2*AR_PATT_RATIO).
        If compatibility with ARToolKit verions 1.0 through 4.4 is required, this value
        must be 0.25.
    @see arGetBorderSize
 */
AR_EXTERN void arSetBorderSize(ARHandle *handle, const ARdouble borderSize);

/*!
    @brief   Get the border size.
    @details N.B. Deprecated in favour of arGetPattRatio(), but retained for
        backwards compatibility.
    @param      handle An ARHandle referring to the current AR tracker
        to be queried for its border size.
    @result		Value representing the border size. The default border size for newly-created
        ARHandle structures is AR_BORDER_SIZE_DEFAULT.
    @see arSetBorderSize
 */
AR_EXTERN ARdouble arGetBorderSize(ARHandle *handle);

/*!
    @brief   Set the width/height of the marker pattern space, as a proportion of marker width/height.
    @details N.B. Supercedes arSetBorderSize().
    @param      handle An ARHandle referring to the current AR tracker to be modified.
    @param		pattRatio The the width/height of the marker pattern space, as a proportion of marker
        width/height. To set the default, pass AR_PATT_RATIO.
        If compatibility with ARToolKit verions 1.0 through 4.4 is required, this value
        must be 0.5.
    @see arGetPattRatio
 */
AR_EXTERN void arSetPattRatio(ARHandle *handle, const ARdouble pattRatio);

/*!
    @brief   Get the width/height of the marker pattern space, as a proportion of marker width/height.
    @details N.B. Supercedes arGetBorderSize().
    @param      handle An ARHandle referring to the current AR tracker to be queried.
    @result     Value representing the width/height of the marker pattern space, as a proportion of marker
        width/height. The default border size for newly-created ARHandle structures is AR_PATT_RATIO.
    @see arSetPattRatio
 */
AR_EXTERN ARdouble arGetPattRatio(ARHandle *handle);

/*!
    @brief   Set the expected pixel format for video frames being passed to arDetectMarker
    @details
        This function must be used at least once after creation of an ARHandle, to set the pixel
        format with which images will be passed to arDetectMarker(). If the pixel format of
        incoming video images changes, this function must be called again to update the value.
    @param      handle Handle to settings structure in which to set the pixel format.
    @param      pixFormat Value representing the format of pixels to be
		processed by the artoolkitX detection routines. See AR_PIXEL_FORMAT
		reference for more information.
    @see arGetPixelFormat
    @see arCreateHandle
    @see arDetectMarker
 */
AR_EXTERN void arSetPixelFormat(ARHandle *handle, AR_PIXEL_FORMAT pixFormat);

/*!
    @brief   Get the expected pixel format for video frames being passed to arDetectMarker
    @details
        See discussion for arSetPixelFormat().
    @param      handle Handle to AR settings structure from which to retrieve the pixel format.
    @result     Value representing the format of pixels being
        processed by the artoolkitX detection routines. See AR_PIXEL_FORMAT
        reference for more information.
    @see arSetPixelFormat
    @see arCreateHandle
    @see arDetectMarker
 */
AR_EXTERN AR_PIXEL_FORMAT arGetPixelFormat(ARHandle *handle);

AR_EXTERN void arSetAreaMax(ARHandle *handle, const ARdouble areaMax);
    
AR_EXTERN ARdouble arGetAreaMax(ARHandle *handle);

AR_EXTERN void arSetAreaMin(ARHandle *handle, const ARdouble areaMin);

AR_EXTERN ARdouble arGetAreaMin(ARHandle *handle);

AR_EXTERN void arSetSquareFitThresh(ARHandle *handle, const ARdouble squareFitThresh);

AR_EXTERN ARdouble arGetSquareFitThresh(ARHandle *handle);

/*!
    @brief   Enable or disable square tracking subpixel corner refinement.
    @details If compiled with OpenCV available, the square tracker allows
        marker corner locations to be subpixel-refined.
    @param      handle Handle to settings structure in which to enable or disable subpixel corner refinement.
	@param      mode
		Options for this field are:
		AR_CORNER_REFINEMENT_DISABLE
		AR_CORNER_REFINEMENT_ENSABLE
		The default mode is AR_CORNER_REFINEMENT_DISABLE.
    @see arGetCornerRefinementMode
*/
AR_EXTERN void arSetCornerRefinementMode(ARHandle *handle, int mode);

/*!
    @brief   Find out whether square tracking subpixel corner refinement is enabled.
    @details See arSetCornerRefinementMode() for more info.
    @param      handle An ARHandle referring to the current AR tracker
		to be queried for its mode.
    @result Value representing the mode.
    @see arSetCornerRefinementMode
*/
AR_EXTERN int arGetCornerRefinementMode(ARHandle *handle);


/*!
    @brief   Detect markers in a video frame.
    @details
		This is the core artoolkitX marker detection function. It calls through to a set of
		internal functions to perform the key marker detection steps of binarization and
		labelling, contour extraction, and template matching and/or matrix code extraction.

        Typically, the resulting set of detected markers is retrieved by calling arGetMarkerNum
        to get the number of markers detected and arGetMarker to get an array of ARMarkerInfo
        structures with information on each detected marker, followed by a step in which
        detected markers are possibly examined for some measure of goodness of match (e.g. by
        examining the match confidence value) and pose extraction.

    @param      arHandle Handle to initialised settings, including camera parameters,
        incoming video image size and pixel format, markers, detection modes and other information.
	@param		frame Pointer to an AR2VideoBufferT structure which contains the pixel
		data for the image  frame which is to be processed for marker detection. The format of
		pixels in the frame is specified by arSetPixelFormat(). The width and height of
		the image are specified by the xsize and ysize parameters of the camera parameters
		held in arHandle.
    @result     0 if the function proceeded without error, or a value less than 0 in case of error.
		A result of 0 does not however, imply any markers were detected.
    @see arCreateHandle
    @see arGetMarkerNum
    @see arGetMarker
 */
AR_EXTERN int arDetectMarker(ARHandle *arHandle, AR2VideoBufferT *frame);

/*!
    @brief   Get the number of markers detected in a video frame.
    @result     The number of detected markers in the most recent image passed to arDetectMarker.
        Note that this is actually a count, not an index. A better name for this function would be
        arGetDetectedMarkerCount, but the current name lives on for historical reasons.
    @param      arHandle Handle upon which arDetectMarker has been called.
    @see arGetMarker
    @see ARMarkerInfo
    @see arDetectMarker
 */
AR_EXTERN int arGetMarkerNum( ARHandle *arHandle );

/*!
    @brief   Get information on the markers detected in a video frame.
    @result     An array (of length arGetMarkerNum(arHandle)) of ARMarkerInfo structs.
        A better name for this function would be arGetDetectedMarkerInfo, but the current name lives
        on for historical reasons.
    @param      arHandle Handle upon which arDetectMarker has been called.
    @see arGetMarkerNum
    @see ARMarkerInfo
    @see arDetectMarker
 */
AR_EXTERN ARMarkerInfo  *arGetMarker( ARHandle *arHandle );

/* ------------------------------ */

AR_EXTERN int            arLabeling( ARUint8 *imageLuma, int xsize, int ysize,
                           int debugMode, int labelingMode, int labelingThresh, int imageProcMode,
                           ARLabelInfo *labelInfo, ARUint8 *image_thresh );
AR_EXTERN int            arDetectMarker2( int xsize, int ysize, ARLabelInfo *labelInfo, int imageProcMode,
                                int areaMax, int areaMin, ARdouble squareFitThresh,
                                ARMarkerInfo2 *markerInfo2, int *marker2_num );
/*!
    @brief   Examine a set of detected squares for match with known markers.
    @details
        Performs the intermediate marker-detection stage of taking detected squares in a processed image, and
        matching the interior of these squares against known marker templates, or extracting matrix codes from
        the interior of the square.
    @param      image Image in which squares were detected.
    @param      xsize Horizontal dimension of image, in pixels.
    @param      ysize Vertical dimension of image, in pixels.
    @param      pixelFormat Format of pixels in image. See &lt;AR/config.h&gt; for values.
    @param      markerInfo2 Pointer to an array of ARMarkerInfo2 structures holding information on detected squares which are candidates for marker matching.
    @param      marker2_num Size of markerInfo2 array.
    @param      pattHandle Handle to loaded patterns for template matching against detected squares.
    @param      imageProcMode Indicates whether square detection was performed treating the image as a frame or a field.
    @param      pattDetectMode Whether to perform color/mono template matching, matrix code detection, or both.
    @param      arParamLTf Lookup table for the camera parameters for the optical source from which the image was acquired. See arParamLTCreate.
    @param      pattRatio A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    @param      markerInfo Output: Pointer to an array of ARMarkerInfo structures holding information on successful matches.
    @param      marker_num Output: Size of markerInfo array.
    @param      matrixCodeType When matrix code pattern detection mode is active, indicates the type of matrix code to detect.
    @result     0 in case of no error, or -1 otherwise.
    @see    arParamLTCreate
 */
AR_EXTERN int            arGetMarkerInfo( ARUint8 *image, int xsize, int ysize, int pixelFormat,
                                ARMarkerInfo2 *markerInfo2, int marker2_num,
                                ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode, ARParamLTf *arParamLTf, ARdouble pattRatio,
                                ARMarkerInfo *markerInfo, int *marker_num,
                                const AR_MATRIX_CODE_TYPE matrixCodeType );

AR_EXTERN int            arGetContour( AR_LABELING_LABEL_TYPE *lImage, int xsize, int ysize, int *label_ref, int label,
                             int clip[4], ARMarkerInfo2 *marker_info2 );
AR_EXTERN int            arGetLine( int x_coord[], int y_coord[], int coord_num, int vertex[], ARParamLTf *paramLTf,
                          ARdouble line[4][3], ARdouble v[4][2] );


/***********************************/
/*                                 */
/*    For pattern identification   */
/*                                 */
/***********************************/

/*!
	@functiongroup "Pattern identification".
 */
/*!
    @brief   Allocate a pattern handle.
    @details Allocates an empty pattern handle, into which patterns can
		be loaded by calling arPattLoad().
		When the pattern handle is no longer needed, it should be
		freed by calling arPattDeleteHandle().

        Note that a pattern handle is NOT required when using only matrix-
        code (2D barcode) markers.
    @see    arPattLoad
    @see    arPattDeleteHandle
    @result     The created pattern handle, or NULL in case of error.
*/
AR_EXTERN ARPattHandle *arPattCreateHandle(void);

/*!
    @brief   Allocate a pattern handle and set pattern template size and maximum number of patterns loadable.
    @details Allocates an empty pattern handle, into which patterns can
		be loaded by calling arPattLoad().
		When the pattern handle is no longer needed, it should be
		freed by calling arPattDeleteHandle().

        Note that a pattern handle is NOT required when using only matrix-
        code (2D barcode) markers.
    @param pattSize For any square template (pattern) markers, the number of rows and
        columns in the template. May not be less than 16 or more than AR_PATT_SIZE1_MAX.

        Pass AR_PATT_SIZE1 for the same behaviour as arPattCreateHandle().
    @param patternCountMax For any square template (pattern) markers, the maximum number of
        markers that may be loaded for a single matching pass. Must be > 0.

        Pass AR_PATT_NUM_MAX for the same behaviour as arPattCreateHandle().
    @see    arPattLoad
    @see    arPattDeleteHandle
    @result     The created pattern handle, or NULL in case of error.
*/

AR_EXTERN ARPattHandle *arPattCreateHandle2(const int pattSize, const int patternCountMax);

/*!
    @brief   Free all loaded patterns and pattern handle.
    @details Frees a pattern handle, freeing (unloading)
		any patterns loaded into the handle in the process.
    @param      pattHandle The handle to free.
    @result     0 on success, or -1 if trying to free a NULL handle.
*/
AR_EXTERN int arPattDeleteHandle(ARPattHandle *pattHandle);

/*!
    @brief   Load a pattern file into a pattern handle.
    @details
        This function loads a pattern template from a file on disk, and attaches
        it to the given ARPattHandle so making it available for future pattern-matching.
        Additional patterns can be loaded by calling again with the same
        ARPattHandle (however no more than AR_PATT_NUM_MAX patterns can be attached
        to a single ARPattHandle). Patterns are initially loaded
		in an active state.

        Note that matrix-code (2D barcode) markers do not have any associated
        pattern file and do not need to be loaded.
    @param      pattHandle Pattern handle, as generated by arPattCreateHandle(),
		into which the pattern file infomation will be loaded.
	@param      filename Pathname of pattern file to load. The pattern file
		is typically generated by the make_patt program. The pathname is
		relative to the current working directory, which is operating system-
		specific.
    @see arPattCreateHandle
    @see arPattActivate
    @see arPattDeactivate
    @see arPattFree
    @result     Returns the index number of the loaded pattern, in the range
		[0, AR_PATT_NUM_MAX - 1], or -1 if the pattern could not be loaded
		because the maximum number of patterns (AR_PATT_NUM_MAX) has already been
		loaded already into this handle.
*/
AR_EXTERN int arPattLoad( ARPattHandle *pattHandle, const char *filename );

AR_EXTERN int arPattLoadFromBuffer(ARPattHandle *pattHandle, const char *buffer);

/*!
    @brief   Save a pattern to a pattern file.
    @details This function is used by the make_patt utility. See the
		sourcecode to mk_patt for usage.
    @param      image (description)
	@param      xsize (description)
	@param      ysize (description)
	@param      pixelFormat (description)
	@param      paramLTf (description)
	@param      imageProcMode (description)
	@param      marker_info (description)
    @param      pattRatio A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    @param      pattSize The number of rows and columns to create in the pattern. Normally AR_PATT_SIZE1.
	@param      filename (description)
    @result     (description)
 */
AR_EXTERN int arPattSave( ARUint8 *image, int xsize, int ysize, int pixelFormat, ARParamLTf *paramLTf,
                int imageProcMode, ARMarkerInfo *marker_info, ARdouble pattRatio, int pattSize, const char *filename );

/*!
    @brief   Frees (unloads) a pattern file from memory.
    @details Unloads a pattern from a pattern handle, freeing that
		slot for another pattern to be loaded, if necessary.
    @param      pattHandle The pattern handle to unload from.
	@param		patno The index into the pattern handle's array of
		patterns to the pattern to be unloaded.
    @result     0 if the pattern was successfully unloaded, or -1
		if there was no pattern loaded.
    @see    arPattLoad
 */
AR_EXTERN int arPattFree( ARPattHandle *pattHandle, int patno );

/*!
    @brief   Activate a previously deactivated pattern.
    @details When a pattern is activated, is becomes available
		for recognition in a scene. This is the default state
		for a loaded pattern.
    @param      pattHandle The handle holding the loaded pattern
		which is to be reactivated.
	@param		patno The index into the pattern handle's array of
		patterns to the pattern to be reactivated.
	@result     0 on success, or -1 if the pattern was already
		activated or no pattern was loaded.
    @see    arPattDeactivate
*/
AR_EXTERN int arPattActivate( ARPattHandle *pattHandle, int patno );

/*!
	@brief   Deactivate a previously activated pattern.
	@details When a pattern is activated, is becomes unavailable
		for recognition in a scene. Deactivating unused patterns
		can speed up recognition time and accuracy when there are
		multiple patterns in a scene, and it is also useful for
		controlling interactivity in a scene.
	@param      pattHandle The handle holding the loaded pattern
		which is to be deactivated.
	@param		patno The index into the pattern handle's array of
		patterns to the pattern to be deactivated.
    @result     0 on success, or -1 if the pattern was already
		deactivated or no pattern was loaded.
    @see    arPattActivate
*/
AR_EXTERN int arPattDeactivate(ARPattHandle *pattHandle, int patno);

/*!
    @brief	Associate a set of patterns with an ARHandle.
    @details Associating a set of patterns with an ARHandle makes
		the patterns the set which will be searched when marker
		identification is performed on an image associated with the
		same ARHandle.
    @param      arHandle (description)
	@param      pattHandle (description)
    @see    arPattDetach
    @result     Returns 0 in the case of success, or -1 if the specified
        ARHandle already has an ARPattHandle attached, or if arHandle is NULL.
*/
AR_EXTERN int arPattAttach(ARHandle *arHandle, ARPattHandle *pattHandle);

/*!
    @brief   Reset an ARHandle to no pattern association.
    @details See arPattAttach() for more information.
    @param      arHandle (description)
    @see    arPattAttach
    @result     Returns 0 in the case of success, or -1 if the specified
        ARHandle has no ARPattHandle attached, or if arHandle is NULL.
*/
AR_EXTERN int arPattDetach(ARHandle *arHandle);

//int arPattGetPattRatio( ARPattHandle *pattHandle, float *ratio );
//int arPattSetPattRatio( ARPattHandle *pattHandle, float  ratio );

/* ------------------------------ */

#if !AR_DISABLE_NON_CORE_FNS
AR_EXTERN int            arPattGetID( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
                            ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat,
                            int *x_coord, int *y_coord, int *vertex, ARdouble pattRatio,
                            int *code, int *dir, ARdouble *cf, const AR_MATRIX_CODE_TYPE matrixCodeType );
AR_EXTERN int            arPattGetImage( int imageProcMode, int pattDetectMode, int patt_size, int sample_size,
                              ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat,
                              int *x_coord, int *y_coord, int *vertex, ARdouble pattRatio,
                              ARUint8 *ext_patt );

/*!
    @brief   Match the interior of a detected square against known patterns.
    @param      pattHandle Handle contained details of known patterns, i.e. loaded templates, or valid barcode IDs.
    @param      imageProcMode See discussion of arSetImageProcMode().
    @param      pattDetectMode See discussion of arSetPatternDetectionMode().
    @param      image Pointer to packed raw image data.
    @param      xsize Horizontal pixel dimension of raw image data.
    @param      ysize Vertical pixel dimension of raw image data.
    @param      pixelFormat Pixel format of raw image data.
    @param      arParamLTf Lookup table for the camera parameters for the optical source from which the image was acquired. See arParamLTCreate.
    @param      vertex 4x2 array of points which correspond to the x and y locations of the corners of the detected marker square.
    @param      pattRatio A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    @param      codePatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the ID of the pattern from pattHandle, or -1 if not identified.
    @param      dirPatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the direction (up, right, down, left) of the pattern from pattHandle.
    @param      cfPatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the confidence factor of the match (range [0.0 - 1.0]).
    @param      codeMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the ID of the pattern, or -1 if not identified.
    @param      dirMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the direction (up, right, down, left) of the pattern.
    @param      cfMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the confidence factor of the match (range [0.0 - 1.0]).
    @param      matrixCodeType When matrix code pattern detection mode is active, indicates the type of matrix code to detect.
    @result     0 if the function was able to correctly match, or -1 in case of error or no match.
    @see    arParamLTCreate
 */
AR_EXTERN int            arPattGetID2( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
                             ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *arParamLTf, ARdouble vertex[4][2], ARdouble pattRatio,
                             int *codePatt, int *dirPatt, ARdouble *cfPatt, int *codeMatrix, int *dirMatrix, ARdouble *cfMatrix,
                             const AR_MATRIX_CODE_TYPE matrixCodeType );
#endif // !AR_DISABLE_NON_CORE_FNS

/*!
    @brief   Match the interior of a detected square against known patterns with variable border width.
    @param      pattHandle Handle contained details of known patterns, i.e. loaded templates, or valid barcode IDs.
    @param      imageProcMode See discussion of arSetImageProcMode().
    @param      pattDetectMode See discussion of arSetPatternDetectionMode().
    @param      image Pointer to packed raw image data.
    @param      xsize Horizontal pixel dimension of raw image data.
    @param      ysize Vertical pixel dimension of raw image data.
    @param      pixelFormat Pixel format of raw image data.
    @param      arParamLTf Lookup table for the camera parameters for the optical source from which the image was acquired. See arParamLTCreate.
    @param      vertex 4x2 array of points which correspond to the x and y locations of the corners of the detected marker square.
    @param      pattRatio A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    @param      codePatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the ID of the pattern from pattHandle, or -1 if not identified.
    @param      dirPatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the direction (up, right, down, left) of the pattern from pattHandle.
    @param      cfPatt Where the pattern matching mode includes template (picture) matching, and a valid template is matched, the confidence factor of the match (range [0.0 - 1.0]).
    @param      codeMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the ID of the pattern, or -1 if not identified.
    @param      dirMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the direction (up, right, down, left) of the pattern.
    @param      cfMatrix Where the pattern matching mode includes matrix (barcode) matching, and a valid matrix is matched, the confidence factor of the match (range [0.0 - 1.0]).
    @param      matrixCodeType When matrix code pattern detection mode is active, indicates the type of matrix code to detect.
    @param      errorCorrected Pointer to an integer which will be filled out with the number of errors detected and corrected during marker identification, or NULL if this information is not required.
    @param      codeGlobalID_p Pointer to uint64_t which will be filled out with the global ID, or NULL if this value is not required.
    @result     0 if the function was able to correctly match, or -1 in case of error or no match.
    @see    arParamLTCreate
 */
AR_EXTERN int arPattGetIDGlobal( ARPattHandle *pattHandle, int imageProcMode, int pattDetectMode,
              ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *arParamLTf, ARdouble vertex[4][2], ARdouble pattRatio,
              int *codePatt, int *dirPatt, ARdouble *cfPatt, int *codeMatrix, int *dirMatrix, ARdouble *cfMatrix,
              const AR_MATRIX_CODE_TYPE matrixCodeType, int *errorCorrected, uint64_t *codeGlobalID_p );

/*!
    @brief   Extract the image (i.e. locate and unwarp) of the pattern-space portion of a detected square.
    @param      imageProcMode See discussion of arSetImageProcMode().
    @param      pattDetectMode See discussion of arSetPatternDetectionMode().
    @param      patt_size The number of horizontal and vertical units to subdivide the pattern-space into.
    @param      sample_size At present, must always be the square of patt_size.
    @param      image Pointer to packed raw image data.
    @param      xsize Horizontal pixel dimension of raw image data.
    @param      ysize Vertical pixel dimension of raw image data.
    @param      pixelFormat Pixel format of raw image data.
    @param      arParamLTf Lookup table for the camera parameters for the optical source from which the image was acquired. See arParamLTCreate.
    @param      vertex 4x2 array of points which correspond to the x and y locations of the corners of the detected marker square.
    @param      pattRatio A value between 0.0 and 1.0, representing the proportion of the marker width which constitutes the pattern. In earlier versions, this value was fixed at 0.5.
    @param      ext_patt Pointer to an array of appropriate size (i.e. patt_size*patt_size*3), which will be filled with the extracted image. Where a colour image is available, it will be supplied in BGR byte order.
    @result     0 if the function was able to correctly get the image, or -1 in case of error or no match.
    @see    arParamLTCreate
 */
AR_EXTERN int            arPattGetImage2( int imageProcMode, int pattDetectMode, int patt_size, int sample_size,
                                ARUint8 *image, int xsize, int ysize, AR_PIXEL_FORMAT pixelFormat, ARParamLTf *arParamLTf,
                                ARdouble vertex[4][2], ARdouble pattRatio, ARUint8 *ext_patt );

/*!
    @brief   Extract the image (i.e. locate and unwarp) of an arbitrary portion of a detected square.
    @details Use this function to obtain an image of the marker pattern space for display to the user.
    @param      arHandle The ARHandle structure associated with the current tracking data.
    @param      markerNo The marker number (in range 0 to arHandle->marker_num - 1, inclusive) from which to extract the pattern.
    @param      image The source video image.
    @param      rect Pointer to an ARPattRectInfo structure which defines the portion of the marker image to extract.
    @param      xsize Width of the output image, in pixels.
    @param      ysize Height of the output image, in pixels.
    @param      overSampleScale Number of samples to acquire per destination pixel, e.g. 2.
    @param      outImage Pointer to a buffer, at least xsize*ysize*arUtilGetPixelSize(arHandle->arPixelFormat) bytes in size, which will be filled out with the marker image.
    @result     0 if the function was able to correctly get the image, or -1 in case of error or no match.
    @see    ARPattRectInfo
 */
AR_EXTERN int            arPattGetImage3( ARHandle *arHandle, int markerNo, ARUint8 *image, ARPattRectInfo *rect, int xsize, int ysize,
                                int overSampleScale, ARUint8 *outImage );


/***********************************/
/*                                 */
/*    For 3D calculation           */
/*                                 */
/***********************************/

/*!
	@functiongroup "3D calculation".
 */

/*!
    @brief   Create handle used for 3D calculation from calibrated camera parameters.
    @details
        An AR3DHandle holds data structures used in calculating the 3D pose of a
        marker from the 2D location of its corners (i.e. pose estimation).
    @param      arParam (description)
    @result     The handle. When no more ar3D*() functions need be called, the handle should be deleted
        by calling ar3DDeleteHandle().
    @see    ar3DCreateHandle2
    @see    ar3DDeleteHandle
*/
AR_EXTERN AR3DHandle    *ar3DCreateHandle(const ARParam *arParam);

/*!
    @brief   Create handle used for 3D calculation from an intrinsic parameters matrix.
    @details
        An AR3DHandle holds data structures used in calculating the 3D pose of a
        marker from the 2D location of its corners (i.e. pose estimation).
    @param      cpara (description)
    @result     The handle. When no more ar3D*() functions need be called, the handle should be deleted
        by calling ar3DDeleteHandle().
    @see    ar3DCreateHandle
    @see    ar3DDeleteHandle
*/
AR_EXTERN AR3DHandle    *ar3DCreateHandle2(const ARdouble cpara[3][4]);

/*!
    @brief   Delete handle used for 3D calculation.
    @details When no more ar3D*() functions need be called, the handle should be deleted
        by calling ar3DDeleteHandle().
    @param      handle A pointer to the 3D handle. On success, the contents of this location will be set to NULL.
    @result     0 if the handle was successfully deleted, -1 otherwise.
    @see    ar3DDeleteHandle
*/
AR_EXTERN int            ar3DDeleteHandle( AR3DHandle **handle );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      cpara (description)
    @result     (description)
*/
AR_EXTERN int            ar3DChangeCpara( AR3DHandle *handle, const ARdouble cpara[3][4] );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      maxLoopCount (description)
    @result     (description)
*/
AR_EXTERN int            ar3DChangeMaxLoopCount( AR3DHandle *handle, int maxLoopCount );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      loopBreakThresh (description)
    @result     (description)
*/
AR_EXTERN int            ar3DChangeLoopBreakThresh( AR3DHandle *handle, ARdouble loopBreakThresh );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      loopBreakThreshRatio (description)
    @result     (description)
*/
AR_EXTERN int            ar3DChangeLoopBreakThreshRatio( AR3DHandle *handle, ARdouble loopBreakThreshRatio );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      marker_info (description)
    @param      width (description)
    @param      conv (description)
    @result     (description)
*/
AR_EXTERN ARdouble         arGetTransMatSquare( AR3DHandle *handle, ARMarkerInfo *marker_info,
                                    ARdouble width, ARdouble conv[3][4] );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      marker_info (description)
    @param      initConv (description)
    @param      width (description)
    @param      conv (description)
    @result     (description)
*/
AR_EXTERN ARdouble         arGetTransMatSquareCont( AR3DHandle *handle, ARMarkerInfo *marker_info,
                                        ARdouble initConv[3][4],
                                        ARdouble width, ARdouble conv[3][4] );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      initConv (description)
    @param      pos2d (description)
    @param      pos3d (description)
    @param      num (description)
    @param      conv (description)
    @result     (description)
*/
AR_EXTERN ARdouble         arGetTransMat( AR3DHandle *handle, ARdouble initConv[3][4],
                              ARdouble pos2d[][2], ARdouble pos3d[][3], int num,
                              ARdouble conv[3][4] );

/*!
    @brief   (description)
    @details (description)
    @param      handle (description)
    @param      initConv (description)
    @param      pos2d (description)
    @param      pos3d (description)
    @param      num (description)
    @param      conv (description)
    @result     (description)
*/
AR_EXTERN ARdouble         arGetTransMatRobust( AR3DHandle *handle, ARdouble initConv[3][4],
                                    ARdouble pos2d[][2], ARdouble pos3d[][3], int num,
                                    ARdouble conv[3][4] );


/***********************************/
/*                                 */
/*    For 3D calculation by Stereo */
/*                                 */
/***********************************/

/*!
	@functiongroup "3D calculation by Stereo".
 */

AR_EXTERN AR3DStereoHandle    *ar3DStereoCreateHandle(const ARParam *arParamL, const ARParam *arParamR, const ARdouble transL[3][4], const ARdouble transR[3][4]);
AR_EXTERN AR3DStereoHandle    *ar3DStereoCreateHandle2(const ARdouble cparaL[3][4], const ARdouble cparaR[3][4], const ARdouble transL[3][4], const ARdouble transR[3][4]);
AR_EXTERN int                  ar3DStereoDeleteHandle( AR3DStereoHandle **handle );
AR_EXTERN int                  ar3DStereoChangeMaxLoopCount( AR3DStereoHandle *handle, int maxLoopCount );
AR_EXTERN int                  ar3DStereoChangeLoopBreakThresh( AR3DStereoHandle *handle, ARdouble loopBreakThresh );
AR_EXTERN int                  ar3DStereoChangeLoopBreakThreshRatio( AR3DStereoHandle *handle, ARdouble loopBreakThreshRatio );
AR_EXTERN int                  ar3DStereoChangeCpara( AR3DStereoHandle *handle, ARdouble cparaL[3][4], ARdouble cparaR[3][4] );
AR_EXTERN int                  ar3DStereoChangeTransMat( AR3DStereoHandle *handle, ARdouble transL[3][4], ARdouble transR[3][4] );

AR_EXTERN ARdouble             arGetTransMatSquareStereo( AR3DStereoHandle *handle,
                                                ARMarkerInfo *marker_infoL, ARMarkerInfo *marker_infoR,
                                                ARdouble width, ARdouble conv[3][4] );
AR_EXTERN ARdouble             arGetTransMatSquareContStereo( AR3DStereoHandle *handle,
                                                    ARMarkerInfo *marker_infoL, ARMarkerInfo *marker_infoR,
                                                    ARdouble prev_conv[3][4],
                                                    ARdouble width, ARdouble conv[3][4] );
AR_EXTERN ARdouble             arGetTransMatStereo( AR3DStereoHandle *handle, ARdouble initConv[3][4],
                                          ARdouble pos2dL[][2], ARdouble pos3dL[][3], int numL,
                                          ARdouble pos2dR[][2], ARdouble pos3dR[][3], int numR,
                                          ARdouble conv[3][4] );
AR_EXTERN ARdouble             arGetTransMatStereoRobust( AR3DStereoHandle *handle, ARdouble initConv[3][4],
                                                ARdouble pos2dL[][2], ARdouble pos3dL[][3], int numL,
                                                ARdouble pos2dR[][2], ARdouble pos3dR[][3], int numR,
                                                ARdouble conv[3][4] );

AR_EXTERN ARdouble             arGetStereoMatchingErrorSquare( AR3DStereoHandle *handle,
                                                     ARMarkerInfo *marker_infoL,
                                                     ARMarkerInfo *marker_infoR );
AR_EXTERN ARdouble             arGetStereoMatchingError( AR3DStereoHandle *handle,
                                               ARdouble pos2dL[2], ARdouble pos2dR[2] );
AR_EXTERN int                  arGetStereoMatching( AR3DStereoHandle *handle,
                                          ARdouble pos2dL[2], ARdouble pos2dR[2], ARdouble pos3d[3] );



/***********************************/
/*                                 */
/*    Utility                      */
/*                                 */
/***********************************/

/*!
	@functiongroup "Utility".
 */

/*!
    @brief   Get the artoolkitX version information in numberic and string format.
    @details
		As of version 2.72, ARToolKit now allows querying of the version number
		of the toolkit available at runtime. It is highly recommended that
		any calling program that depends on features in a certain
		artoolkitX version, check at runtime that it is linked to a version
		of artoolkitX that can supply those features. It is NOT sufficient
		to check the artoolkitX SDK header versions, since with artoolkitX implemented
		in dynamically-loaded libraries, there is no guarantee that the
		version of artoolkitX installed on the machine at run-time will be as
		recent as the version of the artoolkitX SDK which the host
		program was compiled against.

		The version information is reported in binary-coded decimal format,
		and optionally in an ASCII string.

        A increase in the major version number indicates the removal of functionality
        previously provided in the API. An increase in the minor version number
        indicates that new functionality has been added. A change in the tiny version
        number indicates changes (e.g. bug fixes) which do not affect the API. See
        the comments in the config.h header for more discussion of the definition of
        major, minor, tiny and build version numbers.

	@param      versionStringRef
		If non-NULL, the location pointed to will be filled
		with a pointer to a string containing the version information.
		Fields in the version string are separated by spaces. As of version
		2.72.0, there is only one field implemented, and this field
		contains the major, minor and tiny version numbers
		in dotted-decimal format. The string is guaranteed to contain
		at least this field in all future versions of the toolkit.
		Later versions of the toolkit may add other fields to this string
		to report other types of version information. The storage for the
		string is malloc'ed inside the function. The caller is responsible
		for free'ing the string.
    @result
		Returns the full version number of the artoolkitX in
		binary coded decimal (BCD) format.
		BCD format allows simple tests of version number in the caller
		e.g. if ((arGetVersion(NULL) >> 16) > 0x0272) printf("This release is later than 2.72\n");
		The major version number is encoded in the most-significant byte
		(bits 31-24), the minor version number in the second-most-significant
		byte (bits 23-16), the tiny version number in the third-most-significant
		byte (bits 15-8), and the build version number in the least-significant
		byte (bits 7-0).
 */
AR_EXTERN ARUint32 arGetVersion(char **versionStringRef);

AR_EXTERN int            arUtilMatInv( const ARdouble s[3][4], ARdouble d[3][4] );
AR_EXTERN int            arUtilMatMul( const ARdouble s1[3][4], const ARdouble s2[3][4], ARdouble d[3][4] );

#ifdef ARDOUBLE_IS_FLOAT
#define arUtilMatInvf arUtilMatInv
#define arUtilMatMulf arUtilMatMul
#define arUtilMatMuldff arUtilMatMul
#else
AR_EXTERN int            arUtilMatInvf( const float s[3][4], float d[3][4] );
AR_EXTERN int            arUtilMatMulf( const float s1[3][4], const float s2[3][4], float d[3][4] );
AR_EXTERN int            arUtilMatMuldff( const ARdouble s1[3][4], const float s2[3][4], float d[3][4] );
#endif
AR_EXTERN int            arUtilMat2QuatPos( const ARdouble m[3][4], ARdouble q[4], ARdouble p[3] );
AR_EXTERN int            arUtilQuatPos2Mat( const ARdouble q[4], const ARdouble p[3], ARdouble m[3][4] );
AR_EXTERN int            arUtilQuatNorm(ARdouble q[4]);

AR_EXTERN int            arUtilReplaceExt( char *filename, int n, char *ext );
AR_EXTERN int            arUtilRemoveExt ( char *filename );
AR_EXTERN int            arUtilDivideExt ( const char *filename, char *s1, char *s2 );

AR_EXTERN int            arUtilGetSquareCenter( ARdouble vertex[4][2], ARdouble *x, ARdouble *y );

AR_EXTERN int            arUtilSortLabel( int mask[], int m, int n,
                                ARdouble pos[][2], int area[], int label_num,
                                int l1, int x1, int y1,
                                int l2, int x2, int y2,
                                int label[] );

/*!
    @brief   Get the size in bytes of a single pixel for a given pixel format.
    @details
        Different pixel formats have different sizes in bytes, and therefore
        different storage requirements per row of pixels. Use this function
        to calculate the number of bytes required to store a single pixel
        of the given type.
    @param      arPixelFormat The pixel type whose size is to be measured.
    @result     Number of bytes required to store 1 pixel of the given type.
*/
AR_EXTERN int            arUtilGetPixelSize( const AR_PIXEL_FORMAT arPixelFormat );

/*!
    @brief   Get a string holding a descriptive name for a given pixel format enumeration.
    @details
        On occasions it can be useful to display to the user the format of the pixels
        which artoolkitX is processing. This funtion converts a pixel-format number
        into a human-readable string description.
    @param      arPixelFormat Enumerated pixel format number for which to retrieve a name.
    @result     A constant c-string holding a descriptive name for the pixel format.
        The string returned matches the constants used in the definition of the type
        AR_PIXEL_FORMAT, e.g. "AR_PIXEL_FORMAT_RGB".
*/
AR_EXTERN const char *arUtilGetPixelFormatName(const AR_PIXEL_FORMAT arPixelFormat);

/*
    @brief Get the filename portion of a full pathname.
    @details
        Given a full or partial pathname passed in string path,
        returns a pointer to the first char of the filename
        portion of path.
 */
AR_EXTERN const char *arUtilGetFileNameFromPath(const char *path);


/*!
    @brief Get file base name from a path.
    @details
        Given a full or partial pathname passed in string path,
        returns a string with the base name portion of path,
        i.e. the text between the rightmost path separator and the
        the rightmost '.' character, if any.
        If the filename contains no '.', returns the filename.
	@param path Full or partial pathname.
    @param convertToLowercase If convertToLowercase is TRUE, uppercase
        ASCII characters in the basename will be converted to lowercase.
    @result A string with the basename portion of path.
        NB: The returned string must be freed by the caller.
 */
AR_EXTERN char *arUtilGetFileBasenameFromPath(const char *path, const int convertToLowercase);

/*!
    @brief Get file extension from a path.
    @details
        Given a full or partial pathname passed in string path,
        returns a string with the extension portion of path,
        i.e. the text after the rightmost '.' character, if any.
        If the filename contains no '.', NULL is returned.
	@param path Full or partial pathname.
    @param convertToLowercase If convertToLowercase is TRUE, uppercase
        ASCII characters in the extension will be converted to lowercase.
    @result A string with the extension portion of path.
        NB: The returned string must be freed by the caller.
 */
AR_EXTERN char *arUtilGetFileExtensionFromPath(const char *path, const int convertToLowercase);

    /*
     @brief Get the directory portion of a full pathname.
     @details
        Given a full or partial pathname passed in string path,
        returns a string with the directory name portion of path.
        The string is placed into the buffer of size n pointed to by dir.
        If the string and its terminating null byte cannot be accomodated by the
        buffer, NULL is returned, otherwise dir is returned.
        The string is terminated by the directory separator if addSeparator != 0.
 */
AR_EXTERN char *arUtilGetDirectoryNameFromPath(char *dir, const char *path, const size_t n, const int addSeparator);

/*!
    @brief Get a path as a file URI.
    @details
        Given a full or partial pathname passed in string path,
        returns a string with the file URI for that path.

        Partial pathnames are handled by concatening with the
        process's current working directory.
	@param path Full or partial pathname.

        On Windows, both partial pathnames, full pathnames including
        the drive letter, or UNC pathnames (beginning with "\\" are
        all OK.
    @result A string with the the file URI for that path, or NULL
        in the case of error.
        NB: The returned string must be freed by the caller (by
        calling free() once its use is complete).
 */
AR_EXTERN char *arUtilGetFileURI(const char *path);

/*!
    @brief Options for controlling the behavior of arUtilGetResourcesDirectoryPath and arUtilChangeToResourcesDirectory.
	@see arUtilGetResourcesDirectoryPath
	@see arUtilChangeToResourcesDirectory
 */
typedef enum {
    /*!
        Use a platform-dependent recommended-best option.
        Note the this behavior is subject to change in future versions of artoolkitX.
        At present, on macOS and iOS, this will change to the Apple-provided resources directory inside the application bundle.
        At present, on other platforms, this will change to the same directory as the executable.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST = 0,
    /*!
        Use the current working directory. For arUtilChangeToResourcesDirectory, this will leave the current working directory unchanged.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_CWD,
    /*!
        Change to the working directory specified.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH,
    /*!
        Change to the same directory as the executable.
        On OS X and iOS, this corresponds to the directory of the binary executable inside the app bundle, not the directory containing the app bundle.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_EXECUTABLE_DIR,
    /*!
        Change to the resources directory.
        On macOS and iOS, this is the Resources directory inside the application bundle.
        On Linux, this is a directory formed by taking the path to the directory containing
        the executable, appending "/../share" to it, and then pointing to a subdirectory under
        this path with the same name as the executable. Note that the existence of this path
        is not guaranteed. E.g. for an executable at path '/usr/bin/myapp' the returned path
        will be '/usr/bin/../share/myapp'.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR,
    /*!
        Change to the root of the implementation-dependent user-writable root.
        On iOS and sandboxed macOS, this is equivalent to the root of the application sandbox.
        On Linux and non-sandboxed macOS, this is equivalent to the "~" user home.
        On Android, this is the root of the "external" storage (e.g. an SD card).
        On Windows, this is the user home directory, typically "C:\Documents and Settings\USERNAME" or "C:\Users\USERNAME".
        On Windows UWP
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_USER_ROOT,
    /*!
        Change to a writable cache directory, i.e. a directory which is not normally shown to the user, in which files which may be subject to deletion by the system or the user.
        On Android, this is the applications's (internal) cache directory, and a valid instance of Android/Context must be passed in the instanceofAndroidContext parameter.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR,
    /*!
        Change to a writable data directory, i.e. a directory which is not normally shown to the user, but in which files are retained permanently.
        On Android, this is the applications's (internal) files directory, and a valid instance of Android/Context must be passed in the instanceofAndroidContext parameter.
     */
	AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_DATA_DIR,
    /*!
     Change to a writable temporary directory, i.e. a directory which is not normally shown to the user, and from which files may be deleted at the end of program execution.
     On Android, this is the applications's (internal) cache directory, and a valid instance of Android/Context must be passed in the instanceofAndroidContext parameter.
     */
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_TMP_DIR
} AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR;

/*!
    @brief   Get the path to the resources directory using the specified behavior.
    @details
    	artoolkitX uses relative paths to locate several types of resources, including
    	camera parameter files, pattern files, multimarker files and others.
    	This function provides the convenience of finding an appropriate value for your
        application.

        On Android only, the function has an optional parameter 'instanceOfAndroidContext'.
        If behavior is AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR, this
        parameter must be an instance of a class derived from android/content/Context.
        In all other cases, pass NULL for this parameter.
    @param behavior See AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR type for allowed values.
    @result NULL in the case of error, or the path otherwise. Must be free()d by the caller.
 */
#ifdef ANDROID
char *arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, jobject instanceOfAndroidContext);
#else
AR_EXTERN char *arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior);
#endif

/*!
    @brief   Get the path to the resources directory using the specified behavior, creating the path if it doesn't already exist.
    @details
    	artoolkitX uses relative paths to locate several types of resources, including
    	camera parameter files, pattern files, multimarker files and others.
    	This function provides the convenience of finding an appropriate value for your
        application.

        On Android only, the function has an optional parameter 'instanceOfAndroidContext'.
        If behavior is AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR, this
        parameter must be an instance of a class derived from android/content/Context.
        In all other cases, pass NULL for this parameter.
    @param behavior See AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR type for allowed values.
    @result NULL in the case of error, or the path otherwise. Must be free()d by the caller.
 */
#ifdef ANDROID
char *arUtilGetAndCreateResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, jobject instanceOfAndroidContext);
#else
AR_EXTERN char *arUtilGetAndCreateResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior);
#endif

#ifndef _WINRT
/*!
    @brief   Change to the resources directory using the specified behavior.
    @details
    	artoolkitX uses relative paths to locate several types of resources, including
    	camera parameter files, pattern files, multimarker files and others.
    	This function provides the convenience of setting the current process
    	working directory to the appropriate value for your application.

        On Android only, the function has an optional parameter 'instanceOfAndroidContext'.
        If behavior is AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR, this
        parameter must be an instance of a class derived from android/content/Context.
        In all other cases, pass NULL for this parameter.
    @param behavior See AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR type for allowed values.
    @param path When behavior is AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH,
    	the path to change to (absolute or relative to current working directory). In all
        other cases, if this parameter is non-NULL, it will be taken as a subdirectory
        of the desired path and to which the working directory should be changed.
    @result -1 in the case of error, or 0 otherwise.
	@since Not available on Windows Runtime (WinRT).
 */
#ifdef ANDROID
int arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, const char *path, jobject instanceOfAndroidContext);
#else
AR_EXTERN int arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, const char *path);
#endif
#endif // !_WINRT

#ifdef __ANDROID__
#  define ARPRINT(...)  __android_log_print(ANDROID_LOG_INFO, "artoolkitx", __VA_ARGS__)
#  define ARPRINTE(...) __android_log_print(ANDROID_LOG_ERROR, "artoolkitx", __VA_ARGS__)
#else
#  define ARPRINT(...)  printf(__VA_ARGS__)
#  define ARPRINTE(...) fprintf(stderr, __VA_ARGS__)
#endif

/*!
    @brief   Prints a transformation matrix via ARPRINT(...).
    @param trans The transformation matrix to print.
 */
AR_EXTERN void arUtilPrintTransMat(const ARdouble trans[3][4]);

/*!
    @brief   Prints a 4x4 row-major matrix via ARPRINT(...).
    @param mtx16 The matrix to print.
 */
AR_EXTERN void arUtilPrintMtx16(const ARdouble mtx16[16]);

#ifdef __cplusplus
}
#endif //#ifdef __cplusplus

#endif //#ifndef AR_H
