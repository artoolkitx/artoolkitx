if(ARX_TARGET_PLATFORM_WINDOWS)
    find_library(
        PTHREAD_LIBRARIES
        NAMES pthreadVC2static
        PATHS ${PROJECT_SOURCE_DIR}/depends/windows/lib/x64
        DOC "The directory of pthreadVC2static.lib"
    )

    add_definitions("-DPTW32_STATIC_LIB")

    FIND_PATH( PTHREAD_INCLUDE_DIRS
        NAMES pthread.h sched.h semaphore.h
        PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include
        DOC "The directory of pthread headers"
    )

    SET(ARX_LINKER_FLAGS "/INCREMENTAL:NO")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARX_LINKER_FLAGS}")
endif()

add_definitions("-DARX_EXPORTS=1")

set(HAVE_GL 0)
set(HAVE_GL3 0)
set(HAVE_GLES2 0)
set(USE_GL_STATE_CACHE 0)
set(HAVE_GTSAM 0)

# FindJPEG.cmake is so broken.
if(ARX_TARGET_PLATFORM_LINUX)
	find_package(JPEG REQUIRED)
else()
	find_path(JPEG_INCLUDE_DIR
		NAMES jconfig.h jmorecfg.h jpeglib.h jversion.h
		PATHS ${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/include
		DOC "The directory where jpeg headers resides"
		NO_CMAKE_SYSTEM_PATH
	)
    if(ARX_TARGET_PLATFORM_WINDOWS)
        find_library(JPEG_LIBRARIES
            NAMES libjpeg
            PATHS ${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/lib/x64
            DOC "The directory where jpeg static library resides"
        )
    else()
        find_library(JPEG_LIBRARIES
            NAMES jpeg
            PATHS ${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/lib
            DOC "The directory where jpeg static library resides"
        )
    endif()
endif()

find_package(OpenGL)
if (OPENGL_FOUND)
    set(HAVE_GL 1)
endif()
find_package(OpenGLES2)
if (OPENGLES2_FOUND)
    set(HAVE_GLES2 1)
    set(USE_GL_STATE_CACHE 1)
endif()
if(ARX_TARGET_PLATFORM_WINDOWS)
    set(GL3_PATH "${PROJECT_SOURCE_DIR}/depends/windows/include")
endif()
find_package(OpenGL3)
if (OpenGL3_FOUND)
   set(HAVE_GL3 1)
endif()
if ((HAVE_GL OR HAVE_GL3) AND HAVE_GLES2)
    if(ARX_GL_PREFER_EMBEDDED)
        set(HAVE_GL 0)
        set(HAVE_GL3 0)
        message(STATUS "Opting for embedded OpenGL")
    else()
        set(HAVE_GLES2 0)
        message(STATUS "Opting for desktop OpenGL. Set ARX_GL_PREFER_EMBEDDED to change.")
    endif()
endif()
find_package(GTSAM)
if (GTSAM_FOUND)
   set(HAVE_GTSAM 1)
endif()

# Create ARX/AR/config.h and allow subdirectories to find it.
configure_file(AR/include/ARX/AR/config.h.in AR/include/ARX/AR/config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/AR/include)

add_subdirectory(AR)
add_subdirectory(ARVideo)
add_subdirectory(ARUtil)
add_subdirectory(ARG)
if(HAVE_NFT)
    add_subdirectory(AR2)
    add_subdirectory(KPM)
endif()
if(HAVE_2D)
    add_subdirectory(OCVT)
endif()

if(ARX_TARGET_PLATFORM_IOS)
    set(ARX_LIBRARY_STYLE "STATIC")
    set(ARX_BUILD_PLUGIN OFF)
else()
    set(ARX_LIBRARY_STYLE "SHARED")
    if (ARX_TARGET_PLATFORM_MACOS)
        set(ARX_BUILD_PLUGIN ON)
        set(ARX_FRAMEWORK ON)
    else()
        set(ARX_BUILD_PLUGIN OFF)
    endif()
endif()

set(PUBLIC_HEADERS
    include/ARX/ARX_c.h
    include/ARX/ARController.h
    include/ARX/ARTrackable.h
    include/ARX/ARPattern.h
    include/ARX/ARTrackableMultiSquareAuto.h
    include/ARX/ARTrackableMultiSquare.h
    include/ARX/ARTrackableNFT.h
    include/ARX/ARTrackable2d.h
    include/ARX/ARTrackableSquare.h
    include/ARX/ARTracker.h
    include/ARX/ARTrackerVideo.h
    include/ARX/ARTrackerNFT.h
    include/ARX/ARTracker2d.h
    include/ARX/ARTrackerSquare.h
    include/ARX/Error.h
    include/ARX/Platform.h
    include/ARX/ARVideoSource.h
    include/ARX/ARVideoView.h
)

set(SOURCE
    ARX_c.cpp
    ARController.cpp
    ARTrackable.cpp
    ARPattern.cpp
    ARTrackableMultiSquareAuto.cpp
    ARTrackableMultiSquare.cpp
    ARTrackableNFT.cpp
    ARTrackable2d.cpp
    ARTrackableSquare.cpp
    ARTracker.cpp
    ARTrackerNFT.cpp
    ARTracker2d.cpp
    ARTrackerSquare.cpp
    ARVideoSource.cpp
    ARVideoView.cpp
    trackingSub.h
    trackingSub.c
    mapper.hpp
    mapper.cpp
)

add_library(ARX ${ARX_LIBRARY_STYLE}
    ${PUBLIC_HEADERS} ${SOURCE}
)

set_target_properties(ARX PROPERTIES
    SOVERSION ${ARX_VERSION_MAJOR}
    VERSION ${ARX_VERSION_STRING}
)

add_dependencies(ARX
    AR
    ARVideo
    ARUtil
    ARG
)

target_link_libraries(ARX
    PRIVATE AR
    PRIVATE ARVideo
    PRIVATE ARUtil
    PRIVATE ARG
)

target_include_directories(ARX
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PUBLIC $<INSTALL_INTERFACE:include>
    PRIVATE ${PTHREAD_INCLUDE_DIRS}
)

if(HAVE_GL)
    target_link_libraries(ARX PUBLIC ${OPENGL_LIBRARIES})
    target_include_directories(ARX PRIVATE ${OPENGL_INCLUDE_DIR})
endif()
if(HAVE_GLES2)
    target_link_libraries(ARX PUBLIC ${OPENGLES2_LIBRARIES})
    target_include_directories(ARX PRIVATE ${OPENGLES2_INCLUDE_DIR})
endif()
if(HAVE_GL3)
    target_link_libraries(ARX PUBLIC ${OpenGL3_LIBRARIES})
    target_include_directories(ARX PRIVATE ${OpenGL3_INCLUDE_PATH})
endif()
if(HAVE_NFT)
    add_dependencies(ARX AR2 KPM)
    target_link_libraries(ARX PRIVATE AR2 PRIVATE KPM)
endif()
if(HAVE_2D)
    add_dependencies(ARX OCVT)
    target_link_libraries(ARX PRIVATE OCVT)
endif()
if(HAVE_GTSAM)
    add_dependencies(ARX gtsam)
    target_link_libraries(ARX PUBLIC gtsam)
    target_include_directories(ARX PRIVATE ${GTSAM_INCLUDE_DIR})
endif()

if(ARX_TARGET_PLATFORM_WINDOWS) 
    target_link_libraries(ARX 
        PUBLIC ${PTHREAD_LIBRARIES}
        PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:AR>" 
        PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:ARUtil>" 
        PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:ARVideo>" 
        PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:ARG>" 
    )
    if(HAVE_NFT)
        target_link_libraries(ARX 
            PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:AR2>" 
            PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:KPM>" 
        )
    endif()
    if(HAVE_2D)
        target_link_libraries(ARX 
            PRIVATE "-WHOLEARCHIVE:$<TARGET_FILE:OCVT>"
        )
    endif()
endif()

if (${ARX_LIBRARY_STYLE} STREQUAL "STATIC")
    # Hide internal symbols in a static library composed of other static libraries. See http://stackoverflow.com/a/18949281/316487
    set_target_properties(ARX PROPERTIES
        XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES"
        XCODE_ATTRIBUTE_PRELINK_LIBS "$<TARGET_FILE:AR> $<TARGET_FILE:ARUtil> $<TARGET_FILE:ARVideo> $<TARGET_FILE:ARG> $<$<BOOL:${HAVE_NFT}>:$<TARGET_FILE:AR2>> $<$<BOOL:${HAVE_NFT}>:$<TARGET_FILE:KPM>> $<$<BOOL:${HAVE_2D}>:$<TARGET_FILE:OCVT>>"
        XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING "YES"
        XCODE_ATTRIBUTE_SEPARATE_STRIP "YES"
        XCODE_ATTRIBUTE_STRIP_STYLE "non-global"
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] "NO"
        XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT[variant=Debug] "NO"
    )
endif()

if (ARX_FRAMEWORK)
    set(hlist ${PUBLIC_HEADERS})
    set_target_properties(ARX PROPERTIES
        FRAMEWORK ON
        FRAMEWORK_VERSION A
        PUBLIC_HEADER "${hlist}"
        MACOSX_FRAMEWORK_IDENTIFIER org.artoolkitx.arx
    )
endif()

install(TARGETS ARX
    EXPORT ARX
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${ARX_INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${ARX_INSTALL_LIBRARY_DIR}
    FRAMEWORK DESTINATION Frameworks
    PUBLIC_HEADER DESTINATION Frameworks
)

# Install the headers.
if (NOT ARX_FRAMEWORK)
    set(ARX_HEADER_DIR "include/ARX")
else()
    get_target_property(fwver ARX FRAMEWORK_VERSION)
    set(ARX_HEADER_DIR "Frameworks/ARX.framework/Versions/${fwver}/Headers")
endif()
install(FILES ${PUBLIC_HEADERS} DESTINATION ${ARX_HEADER_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AR/include/ARX/AR/config.h DESTINATION ${ARX_HEADER_DIR}/AR)
# Turn header paths like "AR/include/ARX/AR/ar.h" into "AR/ar.h" for installation.
foreach (file ${FRAMEWORK_HEADERS})
    get_filename_component(dir ${file} DIRECTORY)
    string(REGEX REPLACE "^(AR|ARUtil|ARVideo|ARG|AR2|KPM|OCVT)/include/ARX/" "" rp ${dir})
    install(FILES ${file} DESTINATION ${ARX_HEADER_DIR}/${rp})
endforeach()

if (ARX_BUILD_PLUGIN)

    add_library(ARX_plugin MODULE
        ${HEADERS} ${SOURCE}
    )

    add_dependencies(ARX_plugin
        AR
        ARVideo
        ARUtil
        ARG
    )

    target_link_libraries(ARX_plugin
        PRIVATE AR
        PRIVATE ARVideo
        PRIVATE ARUtil
        PRIVATE ARG
    )

    target_include_directories(ARX_plugin
        PRIVATE include
    )
    
    if(HAVE_GL)
        target_link_libraries(ARX_plugin PUBLIC ${OPENGL_LIBRARIES})
        target_include_directories(ARX_plugin PRIVATE ${OPENGL_INCLUDE_DIR})
    endif()
    if(HAVE_GLES2)
        target_link_libraries(ARX_plugin PUBLIC ${OPENGLES2_LIBRARIES})
        target_include_directories(ARX_plugin PRIVATE ${OPENGLES2_INCLUDE_DIR})
    endif()
    if(HAVE_GL3)
        target_link_libraries(ARX_plugin PUBLIC ${OpenGL3_LIBRARIES})
        target_include_directories(ARX_plugin PRIVATE ${OpenGL3_INCLUDE_PATH})
    endif()
    if(HAVE_NFT)
        add_dependencies(ARX_plugin AR2 KPM)
        target_link_libraries(ARX_plugin PRIVATE AR2 PRIVATE KPM)
    endif()
    if(HAVE_2D)
        add_dependencies(ARX_plugin OCVT)
        target_link_libraries(ARX_plugin PRIVATE OCVT)
    endif()
    if(HAVE_GTSAM)
        add_dependencies(ARX_plugin gtsam)
        target_link_libraries(ARX_plugin PUBLIC gtsam)
        target_include_directories(ARX_plugin PRIVATE ${GTSAM_INCLUDE_DIR})
    endif()
    set_target_properties(ARX_plugin PROPERTIES OUTPUT_NAME ARX)
    if(ARX_TARGET_PLATFORM_MACOS)
        set_target_properties(ARX_plugin PROPERTIES
            BUNDLE ON
        )
    endif()

    install(TARGETS ARX_plugin
        LIBRARY DESTINATION Plugins
        ARCHIVE DESTINATION Plugins
        BUNDLE DESTINATION Plugins
    )
endif()

# Install the .cmake file for others to import.
if (NOT ${ARX_LIBRARY_STYLE} STREQUAL "STATIC")
    install(EXPORT ARX DESTINATION lib/ARX)
endif()
