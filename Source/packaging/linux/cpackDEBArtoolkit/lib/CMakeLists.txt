cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-lib)

include(../../cpackConfiguration.cmake)
include(../cpackDebConfiguration.cmake)
include(../../cpackArtoolkit-libConfiguration.cmake)

set(DEPENDENCIES "libjpeg-dev,libgstreamer1.0-dev,libv4l-dev,libdc1394-22-dev,libgl1-mesa-dev,freeglut3-dev,libopenscenegraph-dev,libopencv-dev,libc6,libcurl3,libsqlite3-0,zlib1g,libudev-dev")

#Include the cmake configuration into the package
INSTALL(DIRECTORY ${BUILD_ARTEFACTS_DIR}/ARX DESTINATION /usr/lib DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE 
)

#Create the directory usr/lib/ with the right permissions
INSTALL(DIRECTORY DESTINATION /usr/lib DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE 
)
#Install the library and the soft link
INSTALL(FILES ${BUILD_ARTEFACTS_DIR}/libARX.so.${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION} ${BUILD_ARTEFACTS_DIR}/libARX.so.${MAJOR_VERSION} DESTINATION /usr/lib PERMISSIONS
OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

##According to the packaging guidelines we need a post installation script to call `ldconfig` after installation. This is the case because we install a dynamic library (ARWrapper.so)
#change the postinst file permissions and include them into the package. Permission needs to be 755
file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/postinst DESTINATION ${PROJECT_BINARY_DIR} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${PROJECT_BINARY_DIR}/postinst;)

#Name needed dependencies
SET(CPACK_DEBIAN_PACKAGE_DEPENDS ${DEPENDENCIES})

SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

INCLUDE(CPack)
