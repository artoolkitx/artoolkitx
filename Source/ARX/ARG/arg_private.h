/*
 *  arg_private.h
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <ARX/ARG/arg.h>

#ifndef __arg_private_h__
#define __arg_private_h__

#ifdef __cplusplus
extern "C" {
#endif

struct _ARGL_CONTEXT_SETTINGS {
    ARG_API api;
    void    *apiContextSettings;
    ARParam arParam;
    ARHandle *arhandle; // Not used except for debug mode.
    float   zoom;
    int     disableDistortionCompensation;
    int     rotate90;
    int     flipH;
    int     flipV;
};
typedef struct _ARGL_CONTEXT_SETTINGS ARGL_CONTEXT_SETTINGS;

int arglIsOpenGLES2OrLater();

// Gets binary-coded decimal gl version (ie. 1.4 is 0x0140).
uint16_t arglOpenGLVersion();

#ifdef __cplusplus
}
#endif

#endif // !__arg_private_h__
