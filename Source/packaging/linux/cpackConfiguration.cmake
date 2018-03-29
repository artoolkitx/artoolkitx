set(BUILD_ARTEFACTS_PREFIX lib)

###General configuration

##Fill the package control file with the needed information
#Package control file is required and has some important fields that need to be filled.
SET(CPACK_PACKAGE_VENDOR "artoolkitx.org")
SET(CPACK_PACKAGE_CONTACT "info@artoolkitx.org")
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})


##Set the artoolkitX SDK version into the controll file
#Fetch the artoolkitX SDK version from the config.h file and set it to the corresponding variables
execute_process(COMMAND sed -En -e "s/.*AR_HEADER_VERSION_MAJOR[[:space:]]+([0-9]*).*/\\1/p" ${ARTK_HOME}/SDK/include/ARX/AR/config.h OUTPUT_VARIABLE MAJOR_VERSION)
execute_process(COMMAND sed -En -e "s/.*AR_HEADER_VERSION_MINOR[[:space:]]+([0-9]*).*/\\1/p" ${ARTK_HOME}/SDK/include/ARX/AR/config.h OUTPUT_VARIABLE MINOR_VERSION)
execute_process(COMMAND sed -En -e "s/.*AR_HEADER_VERSION_TINY[[:space:]]+([0-9]*).*/\\1/p" ${ARTK_HOME}/SDK/include/ARX/AR/config.h OUTPUT_VARIABLE PATCH_VERSION)
execute_process(COMMAND sed -En -e "s/.*AR_HEADER_VERSION_DEV[[:space:]]+([0-9]*).*/\\1/p" ${ARTK_HOME}/SDK/include/ARX/AR/config.h OUTPUT_VARIABLE DEV_VERSION)

string(STRIP "${MAJOR_VERSION}" MAJOR_VERSION)
string(STRIP "${MINOR_VERSION}" MINOR_VERSION)
string(STRIP "${PATCH_VERSION}" PATCH_VERSION)
string(STRIP "${DEV_VERSION}" DEV_VERSION)

set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
set(CPACK_PACKAGE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")

if(${DEV_VERSION})
  set(CPACK_PACKAGE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}.${DEV_VERSION}")
endif()

message(STATUS "Version: " "${MAJOR_VERSION}" "." "${MINOR_VERSION}" "." "${PATCH_VERSION}")
##End Set artoolkitx SDK version in controll file


###End configuration

#Copy ChangeLog.txt to ${PROJECT_BINARY_DIR} because we need to gzip it later and gzip command does not work with relative paths
file(COPY ${ARTK_HOME}/ChangeLog.txt DESTINATION ${PROJECT_BINARY_DIR})
#Architecture and package file name
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CPACK_DEBIAN_ARCHITECTURE "amd64")
  set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
  if(${CMAKE_PROJECT_NAME} STREQUAL "artoolkitx")
    SET(CPACK_RPM_PACKAGE_ARCHITECTURE "noarch")
  endif()
  set(BUILD_ARTEFACTS_DIR ${ARTK_HOME}/SDK/lib)
  set(LIB_POSTFIX "64")
else()
    message(FATAL_ERROR "Can only package x86_64 architectures at the moment." )
endif()

if(${CPACK_GENERATOR} STREQUAL "DEB")
  set(ARTKSDK_PACKAGE_ARCH_SUFFIX ${CPACK_DEBIAN_ARCHITECTURE})
  #Set DEB package file name. Needs to be ProjectName_MajorVersion.MinorVersion.PatchVersion_CpuArchitecture
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${ARTKSDK_PACKAGE_ARCH_SUFFIX}")
elseif(${CPACK_GENERATOR} STREQUAL "RPM")
  set(ARTKSDK_PACKAGE_ARCH_SUFFIX ${CPACK_RPM_PACKAGE_ARCHITECTURE})
  #The '-1' is the package release version number. As we number artoolkitx on its own I don't see a reason for making this number configurable.
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-1.${CPACK_RPM_PACKAGE_ARCHITECTURE}")
else()
  set(ARTKSDK_PACKAGE_ARCH_SUFFIX ${CMAKE_SYSTEM_PROCESSOR})
endif()

message(STATUS ${ARTKSDK_PACKAGE_ARCH_SUFFIX})
