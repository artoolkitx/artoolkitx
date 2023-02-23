# Try to find OpenGLES2. Once done this will define:
#     OPENGLES2_FOUND
#     OPENGLES2_INCLUDE_DIRS
#     OPENGLES2_LIBRARIES
#     OPENGLES2_DEFINITIONS

if(APPLE)
    find_path(OPENGLES2_INCLUDE_DIR
        OpenGLES/ES2/gl.h
    )

    find_library(OPENGLES2_FRAMEWORKS OpenGLES)
    if(OPENGLES2_FRAMEWORKS)
        set(OPENGLES2_LIBRARIES "-framework OpenGLES")
    endif()
elseif(ANDROID)
    find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
        "${ANDROID_STANDALONE_TOOLCHAIN}/usr/include"
    )

    find_library(OPENGLES2_LIBRARIES  NAMES GLESv2
        PATHS "${ANDROID_STANDALONE_TOOLCHAIN}/usr/lib"
    )
else()
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_OPENGLES2 glesv2)

    if (PC_OPENGLES2_FOUND)
        set(OPENGLES2_DEFINITIONS ${PC_OPENGLES2_CFLAGS_OTHER})
    endif ()

    find_path(OPENGLES2_INCLUDE_DIR NAMES GLES2/gl2.h
        HINTS ${PC_OPENGLES2_INCLUDEDIR} ${PC_OPENGLES2_INCLUDE_DIRS}
    )

    if(EMSCRIPTEN)
        # GLES2 is native, no linking required.
        set(OPENGLES2_LIBRARIES "")
    else()
        set(OPENGLES2_NAMES ${OPENGLES2_NAMES} glesv2 GLESv2 brcmGLESv2)
        find_library(OPENGLES2_LIBRARIES NAMES ${OPENGLES2_NAMES}
            HINTS ${PC_OPENGLES2_LIBDIR} ${PC_OPENGLES2_LIBRARY_DIRS}
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES2 REQUIRED_VARS OPENGLES2_INCLUDE_DIR
                                  FOUND_VAR OPENGLES2_FOUND)

mark_as_advanced(OPENGLES2_INCLUDE_DIR OPENGLES2_LIBRARIES)
