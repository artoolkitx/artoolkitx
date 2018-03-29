//
//  feature_point.h
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

//#include <boost/serialization/serialization.hpp>

namespace vision {

    /**
     * Represents a feature point in the visual database.
     */
    class FeaturePoint {
    public:
        
        FeaturePoint() : x(0), y(0), angle(0), scale(0), maxima(true) {}
        FeaturePoint(float _x, float _y, float _angle, float _scale, bool _maxima)
        : x(_x), y(_y), angle(_angle), scale(_scale), maxima(_maxima) {}
        ~FeaturePoint() {}

        /** 
         * The (x,y) location of the center of the feature.
         */
        float x, y;

        /**
         * The orientation of the feature in the range [0,2*pi)
         */
        float angle;

        /**
         * The radius (scale) of the feature in the image.
         */
        float scale;

        /**
         * TRUE if this is maxima, FALSE if a minima.
         */
        bool maxima;
        
        //
        // Serialization
        //
        
        /*template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x;
            ar & y;
            ar & angle;
            ar & scale;
            ar & maxima;
        }*/
        
    }; // FeaturePoint
    
} // vision