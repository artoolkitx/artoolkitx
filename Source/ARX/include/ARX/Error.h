/*
 *  Error.h
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
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef ARWRAPPER_ERROR_H
#define ARWRAPPER_ERROR_H

/**
 * \file Error.h
 * Defines error codes used in the ARX library.
 */

#ifdef __cplusplus
extern "C" {
#endif

    enum {
        ARX_ERROR_NONE                  =    0,
        ARX_ERROR_GENERIC               =   -1,
        ARX_ERROR_OUT_OF_MEMORY         =   -2,
        ARX_ERROR_OVERFLOW              =   -3,
        ARX_ERROR_NODATA				=   -4,
        ARX_ERROR_IOERROR               =   -5,
        ARX_ERROR_EOF                   =	-6,
        ARX_ERROR_TIMEOUT               =   -7,
        ARX_ERROR_INVALID_COMMAND       =   -8,
        ARX_ERROR_INVALID_ENUM          =   -9,
        ARX_ERROR_THREADS               =   -10,
        ARX_ERROR_FILE_NOT_FOUND		=   -11,
        ARX_ERROR_LENGTH_UNAVAILABLE	=	-12,
        ARX_ERROR_DEVICE_UNAVAILABLE    =   -13
    };

#ifdef __cplusplus
}
#endif
#endif // !ARWRAPPER_ERROR_H
