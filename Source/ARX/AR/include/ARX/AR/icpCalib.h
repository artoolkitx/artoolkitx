/*
 *	icpCalib.h
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */

#ifndef ICP_CALIB_H
#define ICP_CALIB_H

#include <ARX/AR/icpCore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define      ICP_CALIB_STEREO_MAX_LOOP                        100
#define      ICP_CALIB_STEREO_BREAK_LOOP_ERROR_THRESH         0.001
#define      ICP_CALIB_STEREO_BREAK_LOOP_ERROR_RATIO_THRESH   0.99


typedef struct {
    ICP2DCoordT   *screenCoordL;
    ICP2DCoordT   *screenCoordR;
    ICP3DCoordT   *worldCoordL;
    ICP3DCoordT   *worldCoordR;
    int            numL;
    int            numR;
    ARdouble       initMatXw2Xcl[3][4];
} ICPCalibDataT;

int icpCalibStereo( ICPCalibDataT data[], int num,
                    ARdouble matXcl2Ul[3][4], ARdouble matXcr2Ur[3][4], ARdouble initTransL2R[3][4],
                    ARdouble matTransL2R[3][4],
                    ARdouble *err );

#ifdef __cplusplus
}
#endif
#endif
