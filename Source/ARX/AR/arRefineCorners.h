/*
 *  arRefineCorners.h
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
 *  Copyright 2018 Dan Bell & Philip Lamb.
 *
 *  Author(s): Dan Bell, Philip Lamb.
 *
 */

#ifndef AR_REFINE_CORNERS_H
#define AR_REFINE_CORNERS_H

#include <ARX/AR/ar.h>

#ifdef __cplusplus
extern "C" {
#endif

// Given corner locations 'vertex' in observed coordinates, refine location.
// buff is a luma-only buffer of dimensions width x height.
void arRefineCorners(float vertex[4][2], const unsigned char *buff, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // AR_REFINE_CORNERS_H
