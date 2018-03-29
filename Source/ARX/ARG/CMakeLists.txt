if (ARX_TARGET_PLATFORM_WINDOWS)
    FIND_PATH( GLEXT_INCLUDE_DIRS
               NAMES GL/glext.h GL/wglext.h
		       PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include
		       DOC "The directory where gl extension headers resides"
             )
endif()

set(PUBLIC_HEADERS
    include/ARX/ARG/arg.h
    include/ARX/ARG/mtx.h
    include/ARX/ARG/glStateCache2.h
    include/ARX/ARG/shader_gl.h
)

set(SOURCE
    arg_private.h
    arg.c
    arg_gl.h
    arg_gl.c
    arg_gles2.h
    arg_gles2.c
    arg_gl3.h
    arg_gl3.c
    mtx.c
    glStateCache2.c
    shader_gl.c
)

add_library(ARG STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

add_dependencies(ARG
    AR
    ARUtil
)
if (ARX_TARGET_PLATFORM_WINDOWS)
    target_include_directories(ARG
                               PRIVATE ${GLEXT_INCLUDE_DIRS}
    )

    add_definitions(-DLIBARVIDEO_STATIC)
endif()

target_include_directories(ARG
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../AR/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
)
if(HAVE_GL)
    target_include_directories(ARG PUBLIC ${OPENGL_INCLUDE_DIR})
endif()
if(HAVE_GLES2)
    target_include_directories(ARG PUBLIC ${OPENGLES2_INCLUDE_DIR})
endif()
if(HAVE_GL3)
    target_include_directories(ARG PUBLIC ${OpenGL3_INCLUDE_PATH})
endif()

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "ARG/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)
