/*
 *  cparams.h
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
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */


#ifndef __cparams_h__
#define __cparams_h__

#include <ARX/AR/config.h>

#define cparam_size 176

#if TARGET_OS_IOS
const unsigned char camera_para_iPhone[cparam_size];
const unsigned char camera_para_iPod_touch_4G_front[cparam_size];
const unsigned char camera_para_iPod_touch_4G_rear_640x480[cparam_size];
const unsigned char camera_para_iPod_touch_4G_rear_1280x720[cparam_size];
const unsigned char camera_para_iPad_2_front[cparam_size];
const unsigned char camera_para_iPad_2_rear_640x480[cparam_size];
const unsigned char camera_para_iPad_2_rear_1280x720[cparam_size];
const unsigned char camera_para_iPhone_4_front[cparam_size];
const unsigned char camera_para_iPhone_4_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPhone_4_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPhone_4_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPhone_4_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPhone_4_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPhone_4_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPhone_4_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPhone_4_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPhone_4S_front[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPhone_4S_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPhone_5_front_640x480[cparam_size];
const unsigned char camera_para_iPhone_5_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPhone_5_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPhone_5_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPhone_5_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPhone_5_front_1280x720[cparam_size];
const unsigned char camera_para_iPhone_5_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPhone_5_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPhone_5_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPhone_5_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPhone_5s_front_640x480[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPhone_5s_front_1280x720[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPhone_5s_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPad_mini_3_front_640x480[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPad_mini_3_front_1280x720[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPad_mini_3_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_front_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6_Plus_front_640x480[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_1280x720_0_3m[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_1280x720_1_0m[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_1280x720_inf[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_1280x720_macro[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_640x480_0_3m[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_640x480_1_0m[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_640x480_inf[cparam_size];
const unsigned char camera_para_iPad_Air_2_rear_640x480_macro[cparam_size];
const unsigned char camera_para_iPad_Air_2_front_1280x720[cparam_size];
const unsigned char camera_para_iPad_Air_2_front_640x480[cparam_size];
const unsigned char camera_para_iPhone_6_rear_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6_front_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6s_Plus_rear_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6s_Plus_front_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6s_rear_1280x720[cparam_size];
const unsigned char camera_para_iPhone_6s_front_1280x720[cparam_size];
const unsigned char camera_para_iPad_mini_4_rear_1280x720[cparam_size];
const unsigned char camera_para_iPad_mini_4_front_1280x720[cparam_size];
#endif

#endif // !__cparams_h__
