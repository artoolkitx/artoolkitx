if(${CMAKE_VERSION} VERSION_LESS "3.14.0") 
    message("Please consider to switch to CMake 3.14.0")
    set(CMAKE_SYSTEM_NAME Darwin)

    # Set the CMAKE_OSX_SYSROOT to the latest SDK found
    execute_process(COMMAND /usr/bin/xcrun -sdk iphoneos --show-sdk-path
                OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -integrated-as")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -integrated-as")

    # Cache set values.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "Flags used by the compiler during all build types.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags used by the compiler during all build types.")
    set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "The product will be built against the headers and libraries located inside the indicated SDK.")
else()
    set(CMAKE_SYSTEM_NAME iOS)
endif()

# Target architectures must be set before Project() command.
#set(CMAKE_OSX_ARCHITECTURES "armv7;armv7s;arm64" CACHE STRING "A semicolon-separated list of the architectures for which the product will be built.")
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Need to set these values for to avoid errors (see: https://cmake.org/Bug/view.php?id=15329)
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")

