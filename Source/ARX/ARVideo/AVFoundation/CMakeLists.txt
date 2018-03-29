set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraVideo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraVideo.m
    ${CMAKE_CURRENT_SOURCE_DIR}/MovieVideo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MovieVideo.m
    ${CMAKE_CURRENT_SOURCE_DIR}/cparams.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cparams.c
    ${CMAKE_CURRENT_SOURCE_DIR}/videoAVFoundation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/videoAVFoundation.m
    ${CMAKE_CURRENT_SOURCE_DIR}/videoAVFoundationCameraVideoTookPictureDelegate.h
    PARENT_SCOPE
)

if(ARX_TARGET_PLATFORM_IOS)
	set(LIBS ${LIBS}
		"-framework UIKit"
	)
endif()
set(LIBS
    ${LIBS}
    "-framework Accelerate"
    "-framework AVFoundation"
    "-framework CoreMedia"
    "-framework CoreVideo"
	"-framework Foundation"
	PARENT_SCOPE
)

set(DEFINES
    ${DEFINES}
    ARVIDEO_INPUT_AVFOUNDATION
    PARENT_SCOPE
)
