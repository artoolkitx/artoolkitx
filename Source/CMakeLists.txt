#
# artoolkitX master CMake configuration file.
#
# To invoke CMake on this file, from the chosen build directory, invoke:
#     cmake <path to directory containing this file>.
#
# Copyright 2018, artoolkitX Contributors.
# Author(s): Philip Lamb <phil@artoolkitx.org>
#

cmake_minimum_required( VERSION 3.4 )

# Any directives that must go before the project command.

#if(DEFINED CMAKE_BUILD_TYPE)
#  set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )
#endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Options
option(BUILD_UTILITIES "Build the utilities" ON)

set(ARX_VERSION_MAJOR 1)
set(ARX_VERSION_MINOR 0)
set(ARX_VERSION_TINY  6)
set(ARX_VERSION_DEV   0)
set(ARX_VERSION_STRING "${ARX_VERSION_MAJOR}.${ARX_VERSION_MINOR}.${ARX_VERSION_TINY}")
math(EXPR ARX_VERSION_NUMBER "${ARX_VERSION_MAJOR}*1000000+${ARX_VERSION_MINOR}*10000+${ARX_VERSION_TINY}*100+${ARX_VERSION_DEV}")

# Define project.
project(artoolkitX
        VERSION ${ARX_VERSION_STRING}
        LANGUAGES CXX C
)

if(CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Using multi-configuration CMake generator.")
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Specifies what build types (configurations) will be available." FORCE)
else()
  message(STATUS "Using single-configuration CMake generator.")
endif()

# Identify platform
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(ARX_PLATFORM_NAME "iOS")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(${CMAKE_TOOLCHAIN_FILE} MATCHES "ios.toolchain.cmake")
        set(ARX_PLATFORM_NAME "iOS")
    else()
        set(ARX_PLATFORM_NAME "macOS")
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(ARX_PLATFORM_NAME "Linux")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(ARX_PLATFORM_NAME "Android")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(ARX_PLATFORM_NAME "Windows")
else()
    message(FATAL_ERROR "Unsupported platform \"${CMAKE_SYSTEM_NAME}\".")
endif()
string(TOUPPER ${ARX_PLATFORM_NAME} ARX_PLATFORM_NAME_UC)
string(TOLOWER ${ARX_PLATFORM_NAME} ARX_PLATFORM_NAME_LC)
set(ARX_TARGET_PLATFORM_${ARX_PLATFORM_NAME_UC} ON)
if(ARX_TARGET_PLATFORM_VARIANT)
    set(ARX_PLATFORM_NAME_FILESYSTEM "${ARX_PLATFORM_NAME_LC}-${ARX_TARGET_PLATFORM_VARIANT}")
else()
    set(ARX_PLATFORM_NAME_FILESYSTEM "${ARX_PLATFORM_NAME_LC}")
endif()

# Set supported platform and SDK.
if (ARX_TARGET_PLATFORM_IOS)
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "8.0" CACHE STRING "Code will load on this and later versions of iOS.")
elseif(ARX_TARGET_PLATFORM_MACOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Code will load on this and later versions of macOS." FORCE)
    execute_process(COMMAND /usr/bin/xcrun -sdk macosx --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE STRING "The product will be built against the headers and libraries located inside the indicated SDK." FORCE)
endif()

# Add project-wide definitions.
add_definitions("-DARX_TARGET_PLATFORM_${ARX_PLATFORM_NAME_UC}=1")
if(CMAKE_CONFIGURATION_TYPES)
  # Definitions for multi-configuration generator, e.g. Xcode, Visual Studio.
  set_property(DIRECTORY APPEND PROPERTY 
    COMPILE_DEFINITIONS $<$<CONFIG:Debug>:DEBUG=1>
  )
else()
  # Definitions for single-configuration generator, e.g. Makefiles.
  if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
      add_definitions("-DDEBUG=1")
  endif()
endif()

# Search prefix.
list(APPEND CMAKE_FIND_ROOT_PATH "${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_FILESYSTEM}")
if (ARX_TARGET_PLATFORM_ANDROID)
    set(CMAKE_LIBRARY_ARCHITECTURE "${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/obj/local/${ANDROID_ABI}")
endif()
if (ARX_TARGET_PLATFORM_LINUX AND ("${ARX_TARGET_PLATFORM_VARIANT}" STREQUAL "raspbian"))
    list(APPEND CMAKE_FIND_ROOT_PATH "/opt/vc/")
endif()

# Installation settings.
# If the user didn't define CMAKE_INSTALL_PREFIX, set a local install destination.
# When building packages, this will typically be overridden with DESTDIR (e.g. "make install DESTDIR=/tmp").
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/../SDK" CACHE PATH "default install path" FORCE )
endif()

# Enable C++11 for GCC/Clang compilers.
if (NOT ARX_TARGET_PLATFORM_ANDROID)
    set(CMAKE_CXX_STANDARD "11")
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
if (ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
    if (${CMAKE_GENERATOR} STREQUAL "Xcode")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
    if (ARX_TARGET_PLATFORM_MACOS)
        # enable @rpath in the install name for any shared library being built
        set(CMAKE_MACOSX_RPATH 1)
        # use, i.e. don't skip the full RPATH for the build tree
        SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
    endif()
endif()

# For gcc < 5, ensure C99 compliance. (As of gcc v5, gnu11 is the default.)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
endif()

# Configuration defaults.
set(HAVE_NFT 1)
set(HAVE_2D 0)
set(HAVE_OPENCV 0)
set(USE_CPARAM_SEARCH 0)
set(ARX_INSTALL_LIBRARY_DIR "lib")

# Major dependencies that affect the whole SDK.
# OpenCV.
if(ARX_TARGET_PLATFORM_IOS OR ARX_TARGET_PLATFORM_MACOS)
	set(OpenCV_PATH "${CMAKE_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/Frameworks")
endif()
if(ARX_TARGET_PLATFORM_WINDOWS)
	set(OpenCV_PATH "${CMAKE_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}")
endif()
if(ARX_TARGET_PLATFORM_ANDROID)
    set(OpenCV_PATH "${CMAKE_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_LC}/lib/${ANDROID_ABI}")
endif()
find_package(OpenCV)
if(OpenCV_FOUND)
    set(HAVE_OPENCV 1)
    set(HAVE_2D 1)
else()
    message(WARNING "OpenCV not found, will build without 2D tracker" )
endif()

# Cached and project-specific platform variables.
if (ARX_TARGET_PLATFORM_IOS)

    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
    set(USE_CPARAM_SEARCH 1)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER CACHE STRING "org.artoolkitx.\${PRODUCT_NAME:identifier}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${ARX_VERSION_STRING}")
    set(MACOSX_BUNDLE_VERSION "${ARX_VERSION_NUMBER}")

elseif(ARX_TARGET_PLATFORM_MACOS)

    set(USE_CPARAM_SEARCH 1)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER CACHE STRING "org.artoolkitx.\${PRODUCT_NAME:identifier}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${ARX_VERSION_STRING}")
    set(MACOSX_BUNDLE_VERSION "${ARX_VERSION_NUMBER}")
    set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

elseif(ARX_TARGET_PLATFORM_ANDROID)

    set(USE_CPARAM_SEARCH 1)
    set(ARX_INSTALL_LIBRARY_DIR "lib/${ANDROID_ABI}")

elseif(ARX_TARGET_PLATFORM_LINUX)

    set(USE_CPARAM_SEARCH 1)
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
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEVICE_FLAGS} -fPIC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEVICE_FLAGS} -fPIC")
        set(ARX_GL_PREFER_EMBEDDED 1)
        set(HAVE_INTEL_SIMD 0)
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=core2 -fPIC -Wl,-rpath-link,${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_FILESYSTEM}/lib")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=core2 -fPIC -Wl,-rpath-link,${PROJECT_SOURCE_DIR}/depends/${ARX_PLATFORM_NAME_FILESYSTEM}/lib")
        set(HAVE_INTEL_SIMD 1)
    endif()
    
    set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

elseif(ARX_TARGET_PLATFORM_WINDOWS)

    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
    # Disable some VS warnings we don't care about.
    # C4068: Unknown pragma.
    # C4244: Scalar type conversion.
    # C4996: Deprecated POSIX function name.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4068 /wd4244 /wd4996")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4068 /wd4244 /wd4996")

endif()

get_directory_property(ARX_DEFINES DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach(d ${ARX_DEFINES})
    message(STATUS "Defined: " ${d})
endforeach()

add_subdirectory(ARX)
add_subdirectory(depends)
add_subdirectory(Utilities)

