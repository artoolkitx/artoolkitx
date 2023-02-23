# Build system for a utility tool to be included in artoolkitX.

set(TARGET "artoolkitx_genTexData")
set(TARGET_PACKAGE "org.artoolkitx.utility.genTexData")

if(ARX_TARGET_PLATFORM_IOS)
    set(LIBS
        jpeg
    )
    link_directories(${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_FILESYSTEM}/lib)
endif()

#set(RESOURCES
#    some_file.jpg
#)

set(SOURCE
	genTexData.c
    ${RESOURCES}
)

add_executable(${TARGET} ${SOURCE})

add_dependencies(${TARGET}
    AR
    AR2
    ARUtil
    KPM
)

target_include_directories(${TARGET}
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/AR/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/AR2/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/ARUtil/include
    PRIVATE ${CMAKE_SOURCE_DIR}/ARX/KPM/include
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
    AR2
    ARX
    ${LIBS}
)    

install(TARGETS ${TARGET}
    RUNTIME DESTINATION bin
)
