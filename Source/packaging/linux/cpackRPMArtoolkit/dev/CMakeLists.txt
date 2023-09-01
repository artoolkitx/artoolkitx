cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-devel)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-devConfiguration.cmake)
include(../cpackRpmConfiguration.cmake)

set(DEPENDENCIES "gcc gcc-c++ cmake mesa-libGL-devel")

#Name needed dependencies
SET(CPACK_RPM_PACKAGE_REQUIRES "artoolkitx-lib")

INCLUDE(CPack)
