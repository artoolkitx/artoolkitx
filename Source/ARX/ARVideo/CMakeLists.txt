#
# CMake config for libARVideo allows for sub-modules to set config.
# Submodules may append to the following variables:
#     INCLUDE_DIRS
#     PUBLIC_HEADERS
#     SOURCE
#     LIBS
#     DEFINES
# Submodules should use PARENT_SCOPE in the set command to ensure that
# their changes propagate to this main build file.
#

set(SOURCE
    cparamSearch.h
    cparamSearch.c
    nxjson.h
    nxjson.c
    video.c
    video2.c
    videoAspectRatio.c
    videoLuma.c
    videoRGBA.c
    videoSaveImage.c
)

set(PUBLIC_HEADERS
    include/ARX/ARVideo/video.h
    include/ARX/ARVideo/videoConfig.h
    include/ARX/ARVideo/videoLuma.h
    include/ARX/ARVideo/videoRGBA.h
)

set(INCLUDE_DIRS
    ${JPEG_INCLUDE_DIR}
)

set(LIBS
    ${JPEG_LIBRARIES}
)

if (USE_CPARAM_SEARCH)
    find_package(CURL REQUIRED)
    if (NOT ARX_TARGET_PLATFORM_LINUX)
        set(LIBS ${LIBS}
            ${CURL_LIBRARIES}
            z
        )
    else()
        set(LIBS ${LIBS}
            ${CURL_LIBRARIES}
            z
            crypto
            ssl
        )
    endif()
    message(${CURL_LIBRARIES})
    # On all platforms except Android, we link to sqlite's library. On Android, we compile include sqlite as source.
    if (NOT ARX_TARGET_PLATFORM_ANDROID)
        set(LIBS ${LIBS}
            sqlite3
        )
    endif()
    if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
        set(LIBS ${LIBS}
            "-framework Security"
        )
    endif()
    if (ARX_TARGET_PLATFORM_MACOS)
        set(LIBS ${LIBS}
            "-framework CoreServices"
        )
    endif()
endif()

# Video modules for all platforms.
add_subdirectory("Dummy")
add_subdirectory("Image")

# Video modules for Android.
if(ARX_TARGET_PLATFORM_ANDROID)
    add_subdirectory("Android")
    set(DEFINES ${DEFINES} "ARVIDEO_INPUT_DEFAULT_ANDROID")
endif()

# Video modules for iOS.
if(ARX_TARGET_PLATFORM_IOS)
    add_subdirectory("AVFoundation")
    set(DEFINES ${DEFINES} "ARVIDEO_INPUT_DEFAULT_AVFOUNDATION")
endif()

# Video modules for macOS.
if(ARX_TARGET_PLATFORM_MACOS)
    add_subdirectory("AVFoundation")
    set(DEFINES ${DEFINES} "ARVIDEO_INPUT_DEFAULT_AVFOUNDATION")
    add_subdirectory("libdc1394")
endif()

# Video modules for Linux.
if(ARX_TARGET_PLATFORM_LINUX)
    add_subdirectory("Video4Linux2")
    set(DEFINES ${DEFINES} "ARVIDEO_INPUT_DEFAULT_V4L2")
    add_subdirectory("libdc1394")
    add_subdirectory("GStreamer")
endif()

# Video modules for Windows.
if(ARX_TARGET_PLATFORM_WINDOWS)
    add_subdirectory("WindowsMediaFoundation")
    set(DEFINES ${DEFINES} "ARVIDEO_INPUT_DEFAULT_WINDOWS_MEDIA_FOUNDATION")
endif()

add_library(ARVideo STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

add_dependencies(ARVideo
    AR
    ARUtil
)

target_include_directories(ARVideo
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../AR/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
)
# According to cmake documentation, the dependencies on AR and ARUtil should make the
# explicit private inclusion of their 'include' directories above unnecessary. But it
# doesn't work without them.

target_link_libraries(ARVideo
    INTERFACE ${LIBS}
)

if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
    set_source_files_properties(cparamSearch.c PROPERTIES
        COMPILE_FLAGS "-x objective-c"
        XCODE_EXPLICIT_FILE_TYPE "sourcecode.c.objc"
    )
endif()

foreach(d ${DEFINES})
    add_definitions("-D${d}")
endforeach()

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "ARVideo/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)
