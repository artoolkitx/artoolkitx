set(PUBLIC_HEADERS
    include/ARX/OCVT/PlanarTracker.h
)

set(PRIVATE_HEADERS
    OCVConfig.h
    OCVUtils.h
    OCVFeatureDetector.h
    HomographyInfo.h
    HarrisDetector.h
    TrackableInfo.h
    TrackedPoint.h
    TrackingPointSelector.h
)

set(SOURCE
    OCVConfig.cpp 
    HarrisDetector.cpp
    OCVFeatureDetector.cpp
    PlanarTracker.cpp
    TrackedPoint.cpp
    TrackingPointSelector.cpp
    HomographyInfo.cpp
)

add_library(OCVT STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

target_include_directories(OCVT
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../AR/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/ARX/OCVT
    PRIVATE ${JPEG_INCLUDE_DIR}
    PRIVATE ${OpenCV_INCLUDE_DIR}
)

target_link_libraries(OCVT
    INTERFACE ${LIBS}
    PRIVATE ${JPEG_LIBRARIES}
)

if(NOT ARX_TARGET_PLATFORM_WINDOWS)
	target_link_libraries(OCVT
		PRIVATE ${OpenCV_LIBS}
	)
else()
	set (LINK_LIST "")
	foreach(x ${OpenCV_LIBS})
		string(REPLACE ".lib" "341d.lib" x_debug ${x})
		list (APPEND LINK_LIST debug ${x_debug} optimized ${x})
	endforeach()
	target_link_libraries(OCVT
	    PUBLIC ${LINK_LIST}
	)
endif()

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "OCVT/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)

