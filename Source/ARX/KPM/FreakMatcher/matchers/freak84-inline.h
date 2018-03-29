//
//  freak84-inline.h
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#pragma once

namespace vision {

    /**
     * The total number of receptors from all the rings and
     * the center receptor.
     *
     * NUM_RINGS*NUM_RECEPTORS_PER_RING+1
     */
    static const int freak84_num_receptors = 37;
    
    /**
     * Total number of rings. This does not include the center receptor.
     */
    static const int freak84_num_rings = 6;
    
    /**
     * Total number of receptor per ring.
     */
    static const int freak84_num_receptors_per_ring = 6;
    
    /**
     * SIGMA value for the center receptor and the receptors 
     * in all the rings.
     */

    static const float freak84_sigma_center = 0.100000;
    static const float freak84_sigma_ring0 = 0.175000;
    static const float freak84_sigma_ring1 = 0.250000;
    static const float freak84_sigma_ring2 = 0.325000;
    static const float freak84_sigma_ring3 = 0.400000;
    static const float freak84_sigma_ring4 = 0.475000;
    static const float freak84_sigma_ring5 = 0.550000;
    
    /**
     * (x,y) locations of each receptor in the ring.
     */

    static const float freak84_points_ring0[] = {
        0.000000, 0.362783,
        -0.314179, 0.181391,
        -0.314179, -0.181391,
        -0.000000, -0.362783,
        0.314179, -0.181391,
        0.314179, 0.181391
    };
    static const float freak84_points_ring1[] = {
        -0.595502, 0.000000,
        -0.297751, -0.515720,
        0.297751, -0.515720,
        0.595502, -0.000000,
        0.297751, 0.515720,
        -0.297751, 0.515720
    };
    static const float freak84_points_ring2[] = {
        -0.000000, -0.741094,
        0.641806, -0.370547,
        0.641806, 0.370547,
        0.000000, 0.741094,
        -0.641806, 0.370547,
        -0.641806, -0.370547
    };
    static const float freak84_points_ring3[] = {
        0.847306, -0.000000,
        0.423653, 0.733789,
        -0.423653, 0.733789,
        -0.847306, 0.000000,
        -0.423653, -0.733789,
        0.423653, -0.733789
    };
    static const float freak84_points_ring4[] = {
        0.000000, 0.930969,
        -0.806243, 0.465485,
        -0.806243, -0.465485,
        -0.000000, -0.930969,
        0.806243, -0.465485,
        0.806243, 0.465485
    };
    static const float freak84_points_ring5[] = {
        -1.000000, 0.000000,
        -0.500000, -0.866025,
        0.500000, -0.866025,
        1.000000, -0.000000,
        0.500000, 0.866025,
        -0.500000, 0.866025
    };
    
} // vision