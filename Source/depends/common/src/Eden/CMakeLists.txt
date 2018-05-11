if(NOT ARX_TARGET_PLATFORM_WINDOWS)
	find_package(JPEG REQUIRED)
else()
	find_path(JPEG_INCLUDE_DIR
		NAMES jconfig.h jmorecfg.h jpeglib.h jversion.h
		PATHS ${PROJECT_SOURCE_DIR}/depends/windows/include
		DOC "The directory where jpeg headers resides"
	)

	find_library(JPEG_LIBRARIES
		NAMES libjpeg
		PATHS ${PROJECT_SOURCE_DIR}/depends/windows/lib/x64
		DOC "The directory where jpeg static library resides"
	)
endif()
find_package(OpenGL)
find_package(OpenGLES2)
if(ARX_TARGET_PLATFORM_WINDOWS)
    set(GL3_PATH "${PROJECT_SOURCE_DIR}/depends/windows/include")
endif()
find_package(OpenGL3)

set(SOURCE
    Eden.h
    EdenError.h
    EdenGLFont.c
    EdenGLFont.h
    EdenMath.h
    EdenMessage.c
    EdenMessage.h
    EdenSurfaces.c
    EdenSurfaces.h
    EdenTime.c
    EdenTime.h
    EdenUIInput.c
    EdenUIInput.h
    EdenUtil.c
    EdenUtil.h
    glStateCache.c
    glStateCache.h
    gluttext.h
    readtex.c
    readtex.h
    gluttext/glut_8x13.c
    gluttext/glut_9x15.c
    gluttext/glut_bitmap.c
    gluttext/glut_bwidth.c
    gluttext/glut_hel10.c
    gluttext/glut_hel12.c
    gluttext/glut_hel18.c
    gluttext/glut_mroman.c
    gluttext/glut_roman.c
    gluttext/glut_stroke.c
    gluttext/glut_swidth.c
    gluttext/glut_tr10.c
    gluttext/glut_tr24.c
    gluttext/glutbitmap.h
    gluttext/glutstroke.h
)

add_library(Eden STATIC
    ${SOURCE}
)

target_include_directories(Eden
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ARX/ARG/include
    PRIVATE ${JPEG_INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../../../ARX/AR/include
)

target_link_libraries(Eden
    PRIVATE ${JPEG_LIBRARIES}
)

if(OPENGL_FOUND)
    target_link_libraries(Eden PUBLIC ${OPENGL_LIBRARIES})
    target_include_directories(Eden PRIVATE ${OPENGL_INCLUDE_DIR})
endif()
if(OPENGLES2_FOUND)
    target_link_libraries(Eden PUBLIC ${OPENGLES2_LIBRARIES})
    target_include_directories(Eden PRIVATE ${OPENGLES2_INCLUDE_DIR})
endif()
if(OpenGL3_FOUND)
    target_link_libraries(Eden PUBLIC ${OpenGL3_LIBRARIES})
    target_include_directories(Eden PRIVATE ${OpenGL3_INCLUDE_PATH})
endif()

