#
#   Copyright 2013 Pixar
#
#   Licensed under the Apache License, Version 2.0 (the "Apache License")
#   with the following modification; you may not use this file except in
#   compliance with the Apache License and the following modification to it:
#   Section 6. Trademarks. is deleted and replaced with:
#
#   6. Trademarks. This License does not grant permission to use the trade
#      names, trademarks, service marks, or product names of the Licensor
#      and its affiliates, except as required to comply with Section 4(c) of
#      the License and to reproduce the content of the NOTICE file.
#
#   You may obtain a copy of the Apache License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the Apache License with the above modification is
#   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#   KIND, either express or implied. See the Apache License for the specific
#   language governing permissions and limitations under the Apache License.
#

# - Try to find OpenGLES2
# Once done this will define
#  
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLES

if(ANDROID)
    FIND_PATH( OPENGLES2_INCLUDE_DIR
        GLES2/gl2.h
        "${ANDROID_STANDALONE_TOOLCHAIN}/usr/include"
    )

    FIND_LIBRARY( OPENGLES2_LIBRARIES
        NAMES
            GLESv2
        PATHS
            "${ANDROID_STANDALONE_TOOLCHAIN}/usr/lib"
    )

elseif(APPLE)
    FIND_PATH( OPENGLES2_INCLUDE_DIR
        OpenGLES/ES2/gl.h
    )

    FIND_LIBRARY( OPENGLES2_FRAMEWORKS OpenGLES )

    if(OPENGLES2_FRAMEWORKS)
        set( OPENGLES2_LIBRARIES "-framework OpenGLES" )
    endif()

else()
    FIND_PATH( OPENGLES2_INCLUDE_DIR
        GLES2/gl2.h
    )
    FIND_LIBRARY( OPENGLES2_LIBRARIES
        NAMES
            brcmGLESv2
            GLESv2
    )
endif()

SET( OPENGLES2_FOUND "NO" )
IF(OPENGLES2_LIBRARIES)
    SET( OPENGLES2_FOUND "YES" )
    message("-- Found OpenGLES2: ${OPENGLES2_INCLUDE_DIR}")
ELSE()
    message("-- Could NOT find OpenGLES2 (missing:  OPENGLES2_LIBRARIES OPENGLES2_INCLUDE_DIR)")
ENDIF(OPENGLES2_LIBRARIES)

MARK_AS_ADVANCED(
  OPENGLES2_INCLUDE_DIR
  OPENGLES2_LIBRARIES
)

