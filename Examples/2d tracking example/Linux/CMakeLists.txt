#
# artoolkitX Examples CMake configuration file.
# 2d tracking example
#
# To invoke CMake on this file, from the chosen build directory, invoke:
#     cmake <path to directory containing this file>.
#
# Author(s): Philip Lamb <phil@artoolkitx.org>
#

cmake_minimum_required( VERSION 3.2 )

# Any directives that must go before the project command.

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Specifies what build types (configurations) will be available." FORCE)
if(DEFINED CMAKE_BUILD_TYPE)
  set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )
endif()

project("artoolkitx_2d_tracking_example"
        VERSION 1.0.0
        LANGUAGES CXX C
)

set(ARTOOLKITX_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_definitions("-DARX_TARGET_PLATFORM_LINUX=1")

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions("-DDEBUG=1")
endif()

# Search prefix.
list(APPEND CMAKE_MODULE_PATH "${ARTOOLKITX_HOME}/Source/cmake")
if(ARX_TARGET_PLATFORM_VARIANT)
    list(APPEND CMAKE_FIND_ROOT_PATH "${ARTOOLKITX_HOME}/Source/depends/linux-${ARX_TARGET_PLATFORM_VARIANT}")
else()
    list(APPEND CMAKE_FIND_ROOT_PATH "${ARTOOLKITX_HOME}/Source/depends/linux")
endif()
if ("${ARX_TARGET_PLATFORM_VARIANT}" STREQUAL "raspbian")
    list(APPEND CMAKE_FIND_ROOT_PATH "/opt/vc/")
    set(GL_PREFER_EMBEDDED 1)
endif()


# Installation settings.
# If the user didn't define CMAKE_INSTALL_PREFIX, set a local install destination.
# When building packages, this will typically be overridden with DESTDIR (e.g. "make install DESTDIR=/tmp").
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${ARTOOLKITX_HOME}/SDK" CACHE PATH "default install path" FORCE )
endif()

# Enable C++11 for GCC/Clang compilers.
set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
string(STRIP "${SDL2_LIBRARY}" SDL2_LIBRARY)
include_directories(${SDL2_INCLUDE_DIR})
if ("${ARX_TARGET_PLATFORM_VARIANT}" STREQUAL "raspbian")
    list(APPEND SDL2_LIBRARY -Wl,--no-undefined -lm -L/opt/vc/lib -lbcm_host -ldl -ldl -lpthread -lrt)
endif()

find_package(OpenCV REQUIRED)

find_package(OpenGL)
find_package(OpenGLES2)
find_package(OpenGL3)
if ((OPENGL_FOUND OR OpenGL3_FOUND) AND OPENGLES2_FOUND)
    if(GL_PREFER_EMBEDDED)
        set(OPENGL_FOUND 0)
        set(OpenGL3_FOUND 0)
        message(STATUS "Opting for embedded OpenGL")
    else()
        set(OPENGLES2_FOUND 0)
        message(STATUS "Opting for desktop OpenGL. Set GL_PREFER_EMBEDDED to change.")
    endif()
endif()


include(${ARTOOLKITX_HOME}/SDK/lib/ARX/ARX.cmake)

if ("${ARX_TARGET_PLATFORM_VARIANT}" STREQUAL "raspbian")
    if(ARX_TARGET_DEVICE_RASPBERRY_PI_2_MODEL_B)
        set(DEVICE_FLAGS "-march=armv7-a+neon-vfpv4 -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ftree-vectorize")
        set(HAVE_ARM_NEON 1)
    elseif(ARX_TARGET_DEVICE_RASPBERRY_PI_3)
        set(DEVICE_FLAGS "-march=armv8-a+crc -mtune=cortex-a53 -mfpu=crypto-neon-fp-armv8 -mfloat-abi=hard -ftree-vectorize")
        set(HAVE_ARM64_NEON 1)
    else()
        set(DEVICE_FLAGS "-march=armv6z -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard")
    endif()
else()
    set(DEVICE_FLAGS "-march=core2")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEVICE_FLAGS} -DGL_GLEXT_PROTOTYPES=1 -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEVICE_FLAGS} -DGL_GLEXT_PROTOTYPES=1 -fPIC")

set(RESOURCES
    ../hiro.patt
    ../kanji.patt
)

set(SOURCE
    ../main.cpp
    ../draw.h
    ../draw.cpp
    ${RESOURCES}
    ${FRAMEWORKS}
)

add_executable(${PROJECT_NAME} ${SOURCE})

add_dependencies(${PROJECT_NAME}
    ARX
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "\$ORIGIN/../lib"
)

target_link_libraries(${PROJECT_NAME}
    ARX
    ${SDL2_LIBRARY}
    ${OpenCV_LIBS}
)

if(OPENGL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})
endif()
if(OPENGLES2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGLES2_INCLUDE_DIR})
endif()
if(OpenGL3_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenGL3_INCLUDE_PATH})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES ${RESOURCES} DESTINATION share/${PROJECT_NAME})

