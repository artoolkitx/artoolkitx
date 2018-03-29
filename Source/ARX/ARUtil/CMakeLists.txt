if(NOT ARX_TARGET_PLATFORM_WINDOWS)
    find_package(ZLIB REQUIRED)
	find_path(
		STB_INCLUDE_DIR
		stb_image 
		NO_SYSTEM_ENVIRONMENT_PATH
	)
else()
    FIND_PATH( ZLIB_INCLUDE_DIRS
            NAMES zlib.h zconf.h
            PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include
            DOC "The directory where zlib.h resides"
            )

    FIND_PATH( PTHREAD_INCLUDE_DIRS
            NAMES pthread.h sched.h semaphore.h
            PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include
            DOC "The directory where pthread headers resides"
            )

    FIND_LIBRARY( ZLIB_LIBRARIES
                NAMES zlibstatic
                PATHS ${PROJECT_SOURCE_DIR}/depends/windows/lib/x64
                DOC "The directory where zlibstatic.lib static library resides"
                )
	FIND_PATH(
		STB_INCLUDE_DIR
		NAMES stb_image.h stb_image_write.h
		PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include/stb_image
	)
endif()

set(PUBLIC_HEADERS
    include/ARX/ARUtil/types.h
    include/ARX/ARUtil/log.h
    include/ARX/ARUtil/profile.h
    include/ARX/ARUtil/thread_sub.h
    include/ARX/ARUtil/system.h
    include/ARX/ARUtil/android.h
    include/ARX/ARUtil/time.h
    include/ARX/ARUtil/file_utils.h
    include/ARX/ARUtil/image_utils.h
)

set(SOURCE
    log.c
    profile.c
    thread_sub_winrt.h
    thread_sub_winrt.cpp
    thread_sub.c
    system.c
    android_system_property_get.c
    time.c
    file_utils.c
    image_utils.cpp
    crypt.h
    crypt.c
    ioapi.h
    ioapi.c
    unzip.h
    unzip.c
    zip.h
    zip.c
)

if (ARX_TARGET_PLATFORM_ANDROID)
    set(SOURCE ${SOURCE}
        ftw.h
        nftw.c
    )
endif()

add_library(ARUtil STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
    set_source_files_properties(system.c PROPERTIES
        COMPILE_FLAGS "-x objective-c"
        XCODE_EXPLICIT_FILE_TYPE "sourcecode.c.objc"
    )
endif()

target_include_directories(ARUtil
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${STB_INCLUDE_DIR}
    PRIVATE ${ZLIB_INCLUDE_DIRS} 
    PRIVATE ${PTHREAD_INCLUDE_DIRS}
)

target_link_libraries(ARUtil ${ZLIB_LIBRARIES})

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "ARUtil/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)
