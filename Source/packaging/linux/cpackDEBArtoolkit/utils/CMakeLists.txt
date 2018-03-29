cmake_minimum_required(VERSION 3.4)
 
PROJECT(artoolkitx-utils)

include(../../cpackConfiguration.cmake)
include(../../cpackArtoolkit-utilsConfiguration.cmake)
include(../cpackDebConfiguration.cmake)


#TODO: Somehow the CMake build produces util files that contain debug information. We do not want that. That is why we strip it here.
foreach(util-file ${AR_utils})
    execute_process(COMMAND strip -s -o ${util-file} ${util-file})
endforeach(util-file)

#SETUP directory structure with correct permissions
INSTALL(DIRECTORY DESTINATION /usr/share/${CPACK_DEBIAN_PACKAGE_NAME} DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

INSTALL(DIRECTORY DESTINATION /usr/share/${CPACK_DEBIAN_PACKAGE_NAME}/Data DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

INSTALL(DIRECTORY DESTINATION /usr/bin DIRECTORY_PERMISSIONS
OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

#Name needed dependencies
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "artoolkitx-lib, libc6, libsdl2-2.0-0")

INCLUDE(CPack)
