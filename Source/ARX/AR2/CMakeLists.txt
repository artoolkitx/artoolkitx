set(PUBLIC_HEADERS
    include/ARX/AR2/config.h
    include/ARX/AR2/coord.h
    include/ARX/AR2/featureSet.h
    include/ARX/AR2/imageFormat.h
    include/ARX/AR2/imageSet.h
    include/ARX/AR2/marker.h
    include/ARX/AR2/searchPoint.h
    include/ARX/AR2/template.h
    include/ARX/AR2/tracking.h
    include/ARX/AR2/util.h
)

set(SOURCE
    coord.c
    featureMap.c
    featureSet.c
    handle.c
    imageSet.c
    jpeg.c
    marker.c
    matching.c
    matching2.c
    searchPoint.c
    selectTemplate.c
    surface.c
    template.c
    tracking.c
    tracking2d.c
    util.c
)

if(NOT ARX_TARGET_PLATFORM_WINDOWS)
    set(LIBS
    m
    )
endif()



if (ARX_TARGET_PLATFORM_ANDROID)
    set(LIBS ${LIBS}
        log
    )
endif()

add_library(AR2 STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

target_include_directories(AR2
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../AR/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
    PRIVATE ${JPEG_INCLUDE_DIR}
)

target_link_libraries(AR2
    INTERFACE ${LIBS}
    PRIVATE ${JPEG_LIBRARIES}
)

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "AR2/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)

