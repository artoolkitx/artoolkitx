# Set the CMAKE_OSX_SYSROOT to the latest SDK found
execute_process(COMMAND /usr/bin/xcrun -sdk macosx --show-sdk-path
                OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Target architectures must be set before Project() command.
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "A semicolon-separated list of the architectures for which the product will be built.")
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")

# Cache set values.
set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "The product will be built against the headers and libraries located inside the indicated SDK.")
