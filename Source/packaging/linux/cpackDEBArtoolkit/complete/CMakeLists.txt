cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-completeConfiguration.cmake)
include(../cpackDebConfiguration.cmake)

#Name needed dependencies
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "artoolkitx-lib, artoolkitx-dev, artoolkitx-examples, artoolkitx-utils")

INCLUDE(CPack)
