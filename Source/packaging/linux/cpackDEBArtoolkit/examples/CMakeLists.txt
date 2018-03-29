cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-examples)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-examplesConfiguration.cmake)
include(../cpackDebConfiguration.cmake)

INSTALL(DIRECTORY DESTINATION /usr/bin DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

#Name needed dependencies
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "artoolkitx-lib, libc6")

INCLUDE(CPack)
