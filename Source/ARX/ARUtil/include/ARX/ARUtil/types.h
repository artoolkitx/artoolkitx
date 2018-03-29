/*
 *  types.h
 *  artoolkitX
 *
 *  Types common to various utility functions.
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
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef __ARUtil_types_h__
#define __ARUtil_types_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  ifdef ARUTIL_STATIC
#    define ARUTIL_EXTERN
#  else
#    ifdef ARX_EXPORTS
#      define ARUTIL_EXTERN __declspec(dllexport)
#    else
#      define ARUTIL_EXTERN __declspec(dllimport)
#    endif
#  endif
#  define ARUTIL_CALLBACK __stdcall
#else
#  define ARUTIL_EXTERN
#  define ARUTIL_CALLBACK
#endif

#ifdef __cplusplus
}
#endif
#endif // !__ARUtil_types_h__
