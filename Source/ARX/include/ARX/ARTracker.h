/*
 *  ARTracker.h
 *  artoolkitX
 *
 *  A C++ class encapsulating core tracker functionality.
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb, Julian Looser.
 *
 */


#ifndef ARTRACKER_H
#define ARTRACKER_H

#include <ARX/Platform.h>
#include <ARX/ARTrackable.h>
#include <vector>
#include <string>

enum class ARTrackerType {
    SQUARE_FIDUCIAL,
    TEXTURE2D_FIDUCIAL,
    GEODETIC,
    INERTIAL,
    VISUAL_SLAM,
    VISUAL_INERTIAL_SLAM
};

class ARTracker {
public:
    ARTracker() {};
    virtual ~ARTracker() {};

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool isRunning() = 0;
    virtual bool update() = 0;
    virtual bool stop() = 0;
    virtual void terminate() = 0;
    
    virtual ARTrackerType type() const = 0;
    virtual std::vector<std::string> trackableConfigurations() const = 0;

    /**
        ARTrackable factory method.
        single;data/hiro.patt;80
        single_buffer;80;buffer=234 221 237...
        single_barcode;0;80
        multi;data/multi/marker.dat
        nft;data/nft/pinball
     */
    virtual ARTrackable *newTrackable(std::vector<std::string> config) = 0;
    virtual void deleteTrackable(ARTrackable **trackable_p) = 0;

};


#endif // !ARTRACKER_H
