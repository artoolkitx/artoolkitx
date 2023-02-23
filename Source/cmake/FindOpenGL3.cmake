#
# Try to find OpenGL and include path.
# Once done this will define
#
# OpenGL3_FOUND
# OpenGL3_INCLUDE_PATH
# OpenGL3_LIBRARIES
# 

SET(OPENGL_SEARCH_PATHS
    $ENV{OpenGL_ROOT}
    ${DEPENDENCIES_ROOT}
    /usr 
    /usr/local
    /opt/local
    ${GL3_PATH}
)
#message(${OPENGL_SEARCH_PATHS})
IF(WIN32)
    FIND_PATH(OpenGL3_INCLUDE_PATH
        NAMES GL/glcorearb.h
        PATHS ${OPENGL_SEARCH_PATHS}
        PATH_SUFFIXES include
        DOC "The directory where GL/glcorearb.h resides"
    )
    SET(OpenGL3_LIBRARY OpenGL32)

ELSEIF(APPLE)
    FIND_PATH(OpenGL3_INCLUDE_PATH
        NAMES OpenGL/gl3.h
        PATHS ${OPENGL_SEARCH_PATHS}
        PATH_SUFFIXES include
        DOC "The directory where OpenGL/gl3.h resides"
    )
    SET(OpenGL3_LIBRARY "-framework Cocoa -framework OpenGL -framework IOKit" CACHE STRING "OpenGL lib for OSX")
 
ELSE()
    SET(OpenGL3_LIBRARY "GL" CACHE STRING "OpenGL lib for Linux")
    FIND_PATH(OpenGL3_INCLUDE_PATH
        NAMES GL/glcorearb.h
        PATHS ${OPENGL_SEARCH_PATHS} /usr/share/doc/NVIDIA_GLX-1.0 /usr/openwin/share /opt/graphics/OpenGL /usr/X11R6
        PATH_SUFFIXES include
        DOC "The directory where GL/glcorearb.h resides"
    )
ENDIF()

if(OpenGL3_INCLUDE_PATH)
    set(OpenGL3_LIBRARIES ${OpenGL3_LIBRARY})
else()
    unset(OpenGL3_LIBRARIES)
endif()

# handle the QUIETLY and REQUIRED arguments and set OPENGL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenGL3 REQUIRED_VARS OpenGL3_LIBRARIES OpenGL3_INCLUDE_PATH)

