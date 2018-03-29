if(ARX_TARGET_PLATFORM_MACOS)
    find_path(
        LIBDC1394_INCLUDE_DIRS
        dc1394 
        NO_SYSTEM_ENVIRONMENT_PATH
    )
    find_library(
        LIBDC1394_LDFLAGS
        NAMES dc1394 
        NO_SYSTEM_ENVIRONMENT_PATH
    )
    set(LIBS ${LIBS} "-framework IOKit")
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LIBDC1394 REQUIRED libdc1394-2)
endif()

set(INCLUDE_DIRS
    ${INCLUDE_DIRS}
    ${LIBDC1394_INCLUDE_DIRS}
	PARENT_SCOPE
)
set(LIBS
    ${LIBS}
    ${LIBDC1394_LDFLAGS}
	PARENT_SCOPE
)

set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/video1394.c
    ${CMAKE_CURRENT_SOURCE_DIR}/video1394.h
    ${CMAKE_CURRENT_SOURCE_DIR}/video1394Private.h
    ${CMAKE_CURRENT_SOURCE_DIR}/video1394Format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/video1394Setting.c
    PARENT_SCOPE
)

set(DEFINES
    ${DEFINES}
    ARVIDEO_INPUT_LIBDC1394
    PARENT_SCOPE
)
