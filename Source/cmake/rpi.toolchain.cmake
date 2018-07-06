# Raspbian cross-compiling toolchain.
# See https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(triple arm-linux-gnueabihf)

set(CMAKE_SYSROOT "${CMAKE_CURRENT_SOURCE_DIR}/build-linux-raspbian/rpi-sysroot")
#set(CMAKE_STAGING_PREFIX /home/devel/stage)

# If using GNU compiler:
set(CMAKE_C_COMPILER ${triple}-gcc)
set(CMAKE_CXX_COMPILER ${triple}-g++)

# If using Clang:
#set(CMAKE_C_COMPILER clang)
#set(CMAKE_C_COMPILER_TARGET ${triple})
#set(CMAKE_CXX_COMPILER clang++)
#set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Additional search requirements for pkgconfig when using sysroot:
set(PKG_CONFIG_EXECUTABLE ${triple}-pkg-config)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/lib/${triple}/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")

