cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-lib)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-libConfiguration.cmake)
include(../cpackRpmConfiguration.cmake)

set(DEPENDENCIES "libjpeg-turbo-devel, libv4l-devel, libdc1394-devel,libsqlite3x-devel,libcurl-devel, glib2-devel, gstreamer-devel, OpenSceneGraph-devel, opencv-devel")

#Include all the libs into the package
install(FILES ${BUILD_ARTEFACTS_DIR}/libARX.so.${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION} ${BUILD_ARTEFACTS_DIR}/libARX.so.${MAJOR_VERSION} DESTINATION lib${LIB_POSTFIX})

set(CPACK_RPM_PACKAGE_REQUIRES ${DEPENDENCIES})


include(CPack)
