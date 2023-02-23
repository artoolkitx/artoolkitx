# Build system for a utility tool to be included in artoolkitX.

set(TARGET "artoolkitx_utility_example")
set(TARGET_PACKAGE "org.artoolkitx.utility.example")

if (ARX_TARGET_PLATFORM_MACOS OR ARX_TARGET_PLATFORM_LINUX OR ARX_TARGET_PLATFORM_WINDOWS)
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIR})
endif()

if(ARX_TARGET_PLATFORM_MACOS)
	set(SDL2_PATH "${CMAKE_SOURCE_DIR}/depends/macos/Frameworks")
endif()
find_package(SDL2 REQUIRED)
string(STRIP "${SDL2_LIBRARY}" SDL2_LIBRARY)
include_directories(${SDL2_INCLUDE_DIR})

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

set(RESOURCES
    some_file.jpg
)

set(SOURCE
	main.cpp
    ${RESOURCES}
)

add_executable(${TARGET} ${SOURCE})

add_dependencies(${TARGET}
    ARX
    AR
    ARUtil
    ARG
    ARVideo
)

target_include_directories(${TARGET}
    PRIVATE ${PROJECT_BINARY_DIR}/ARX/AR/include
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
    AR
    ARUtil
    ARG
    ARVideo
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARY}
    ${LIBS}
)    

install(TARGETS ${TARGET}
    RUNTIME DESTINATION bin
)
