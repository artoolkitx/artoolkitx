set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/videoWindowsMediaFoundation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/videoWindowsMediaFoundation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BufferLock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogMediaType.cpp
    PARENT_SCOPE
)

set(DEFINES
    ${DEFINES}
    ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
    PARENT_SCOPE
)
