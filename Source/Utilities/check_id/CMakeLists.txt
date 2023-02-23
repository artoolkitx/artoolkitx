# Build system for a utility tool to be included in artoolkitX.

set(TARGET "artoolkitx_check_id")
set(TARGET_PACKAGE "org.artoolkitx.utility.check_id")

if (ARX_TARGET_PLATFORM_MACOS OR ARX_TARGET_PLATFORM_LINUX OR ARX_TARGET_PLATFORM_WINDOWS)
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIR})
endif()

if(ARX_TARGET_PLATFORM_MACOS)
    set(SDL2_PATH "${CMAKE_SOURCE_DIR}/depends/macos/Frameworks")
elseif(ARX_TARGET_PLATFORM_WINDOWS)
    set(SDL2_PATH "${CMAKE_SOURCE_DIR}/depends/windows")
endif()
find_package(SDL2 REQUIRED)
string(STRIP "${SDL2_LIBRARY}" SDL2_LIBRARY)
include_directories(${SDL2_INCLUDE_DIR})

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

if(ARX_TARGET_PLATFORM_IOS)
    set(LIBS
        jpeg
        curl
        "-framework AudioToolbox"
        "-framework Accelerate"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework GLKit"
        "-framework CoreGraphics"
        sqlite3
        z
        stdc++
    )
    link_directories(${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_FILESYSTEM}/lib)
endif()

#set(RESOURCES
#    some_file.jpg
#)

set(SOURCE
	check_id.cpp
    ${RESOURCES}
)

add_executable(${TARGET} ${SOURCE})

add_dependencies(${TARGET}
    AR
    AR2
    ARUtil
    ARG
    ARVideo
    Eden
)

target_include_directories(${TARGET}
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/AR/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/AR2/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/ARUtil/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/ARG/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/ARVideo/include
    PRIVATE ${CMAKE_SOURCE_DIR}/depends/common/src
    PRIVATE ${PROJECT_BINARY_DIR}/ARX/AR/include
    PRIVATE ${JPEG_INCLUDE_DIR}
)

if (ARX_TARGET_PLATFORM_MACOS OR ARX_TARGET_PLATFORM_IOS)
	set_target_properties(${TARGET} PROPERTIES
		RESOURCE "${RESOURCES}"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks"
        MACOSX_BUNDLE_GUI_IDENTIFIER ${TARGET_PACKAGE}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TARGET_PACKAGE}"
	)
	if (ARX_TARGET_PLATFORM_MACOS)
	    set_target_properties(${TARGET} PROPERTIES
	        XCODE_ATTRIBUTE_CREATE_INFOPLIST_SECTION_IN_BINARY "YES"
		    XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/macOS/Info.plist"
		)
    endif()
    if (ARX_TARGET_PLATFORM_IOS)
        set_target_properties(${TARGET} PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[sdk=iphoneos*] "iPhone Developer"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "0123456789A"
        )
    endif()
else()
    set_target_properties(${TARGET} PROPERTIES
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

target_link_libraries(${TARGET}
    ARX
    ARVideo
    Eden
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARY}
    ${LIBS}
    ${JPEG_LIBRARIES}
)    

install(TARGETS ${TARGET}
    RUNTIME DESTINATION bin
)
