cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-devel)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-devConfiguration.cmake)
include(../cpackRpmConfiguration.cmake)

#Name needed dependencies
SET(CPACK_RPM_PACKAGE_REQUIRES "artoolkitx-lib")

INCLUDE(CPack)
