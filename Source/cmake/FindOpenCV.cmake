# This module defines
# OpenCV_LIBS, the library/libraries to link against
# OpenCV_INCLUDE_DIR, where to find opencv2/opencv.hpp
# OpenCV_FOUND, if false, do not try to link to OpenCV
#
# $OPENCVDIR is an environment variable that would
# correspond to the ./configure --prefix=$OPENCV
# used in building OpenCV.
#
# On macOS/iOS, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# OpenCV_LIBS to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#

SET(OpenCV_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	${OpenCV_PATH}
)

FIND_PATH(OpenCV_INCLUDE_DIR opencv2/opencv.hpp
	HINTS
	$ENV{OPENCV2DIR}
	PATH_SUFFIXES include/opencv4 include
	PATHS ${OpenCV_SEARCH_PATHS}
)

IF(${OpenCV_INCLUDE_DIR} MATCHES ".framework")
    FIND_LIBRARY(OpenCV_LIBS opencv2 HINTS $ENV{OPENCV2DIR} PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCL_LIBS OpenCL)
    IF (NOT ${OpenCL_LIBS} STREQUAL "OpenCL_LIBS-NOTFOUND")
        SET(OpenCV_LIBS ${OpenCV_LIBS} ${OpenCL_LIBS})
    ENDIF()
ELSE()
    FIND_LIBRARY(OpenCV_LIB_CALIB32 NAMES opencv_calib3d HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCV_LIB_CORE NAMES opencv_core HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCV_LIB_FEATURES2D NAMES opencv_features2d HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCV_LIB_FLANN NAMES opencv_flann HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCV_LIB_IMGPROC NAMES opencv_imgproc HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OpenCV_LIB_VIDEO NAMES opencv_video HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    SET(OpenCV_LIBS ${OpenCV_LIB_CALIB32} ${OpenCV_LIB_FEATURES2D} ${OpenCV_LIB_FLANN} ${OpenCV_LIB_VIDEO} ${OpenCV_LIB_IMGPROC} ${OpenCV_LIB_CORE})
ENDIF()

IF(ANDROID)
    FIND_LIBRARY(OPENCV_LIB_TBB NAMES tbb HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    SET(OpenCV_LIBS ${OpenCV_LIBS} ${OPENCV_LIB_TBB})
    FIND_LIBRARY(OPENCV_LIB_ITT NAMES ittnotify HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OPENCV_LIB_TEG NAMES tegra_hal HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    FIND_LIBRARY(OPENCV_LIB_CPUF NAMES cpufeatures HINTS $ENV{OPENCV2DIR} PATH_SUFFIXES lib64 lib lib/x64 lib/x86 PATHS ${OpenCV_SEARCH_PATHS})
    IF (NOT ${OPENCV_LIB_ITT} STREQUAL "OPENCV_LIB_ITT-NOTFOUND")
        SET(OpenCV_LIBS ${OpenCV_LIBS} ${OPENCV_LIB_ITT})
    ENDIF()
    IF (NOT ${OPENCV_LIB_TEG} STREQUAL "OPENCV_LIB_TEG-NOTFOUND")
        SET(OpenCV_LIBS ${OpenCV_LIBS} ${OPENCV_LIB_TEG})
    ENDIF()
    IF (NOT ${OPENCV_LIB_CPUF} STREQUAL "OPENCV_LIB_CPUF-NOTFOUND")
        SET(OpenCV_LIBS ${OpenCV_LIBS} ${OPENCV_LIB_CPUF})
    ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set OpenCV_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenCV REQUIRED_VARS OpenCV_LIBS OpenCV_INCLUDE_DIR)
