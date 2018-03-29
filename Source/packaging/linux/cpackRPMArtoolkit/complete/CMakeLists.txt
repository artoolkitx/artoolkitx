cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-completeConfiguration.cmake)
include(../cpackRpmConfiguration.cmake)

#Name needed dependencies
SET(CPACK_RPM_PACKAGE_REQUIRES "artoolkitx-lib, artoolkitx-dev, artoolkitx-examples, artoolkitx-utils")

INCLUDE(CPack)
