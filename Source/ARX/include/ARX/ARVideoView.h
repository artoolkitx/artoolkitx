/*
 *  ARVideoView.h
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
 *
 *  Author(s): Philip Lamb.
 *
 */

#ifndef ARVIDEOVIEW_H
#define ARVIDEOVIEW_H

#include <ARX/Platform.h>
#include <ARX/ARVideoSource.h>

#include <ARX/AR/ar.h>
#include <ARX/ARG/arg.h>

/**
 * ARVideoView draws the output of an ARVideoSource to a rendering context, usually
 * for the purposes of video see-through augmented reality.
 *
 * A rendering context supported by libARG must be active when calling any non-const
 * function in this class. For example, on platforms supporting OpenGL, a valid OpenGL
 * context must be initialised and active at the time of the call. For most OpenGL
 * implementations, such a call can only be made from a rendering thread.
 */
class ARX_EXTERN ARVideoView {

public:

    struct Size {
        int width;
        int height;
    };
    enum class HorizontalAlignment {
        H_ALIGN_LEFT,
        H_ALIGN_CENTRE,
        H_ALIGN_RIGHT
    };
    enum class VerticalAlignment {
        V_ALIGN_TOP,
        V_ALIGN_CENTRE,
        V_ALIGN_BOTTOM
    };
    enum class ScalingMode {
        SCALE_MODE_STRETCH,
        SCALE_MODE_FIT,
        SCALE_MODE_FILL,
        SCALE_MODE_1_TO_1
    };
    
    ARVideoView();
    bool initWithVideoSource(const ARVideoSource& vs, const int contextWidth, const int contextHeight);
    void getViewport(int32_t viewport[4]) const;
    void draw(ARVideoSource* vs);
    void drawDebugImage(const uint8_t *image, int pyramidLevel);
    ~ARVideoView();

    HorizontalAlignment horizontalAlignment() const;
    void setHorizontalAlignment(const HorizontalAlignment hAlign); ///> Defaults to H_ALIGN_CENTRE.
    VerticalAlignment verticalAlignment() const;
    void setVerticalAlignment(const VerticalAlignment vAlign); ///> Defaults to V_ALIGN_CENTRE.
    ScalingMode scalingMode() const;
    void setScalingMode(const ScalingMode scalingMode); ///> Defaults to SCALE_MODE_FIT.
    bool distortionCompensation() const;
    void setDistortionCompensation(const bool distortionCompensation); ///> Defaults to true.
    bool rotate90() const;
    void setRotate90(const bool rotate90); ///> Defaults to false.
    bool flipH() const;
    void setFlipH(const bool flipH); ///> Defaults to false.
    bool flipV() const;
    void setFlipV(const bool flipV); ///> Defaults to false.
    Size contextSize() const;
    void setContextSize(const Size size);
    
private:
    void update();
    ARGL_CONTEXT_SETTINGS_REF m_arglContextSettings;
    enum class DrawType {None, Main, Debug};
    DrawType m_drawTypeSet;
    AR2VideoTimestampT m_drawTime;
    int m_contentWidth;
    int m_contentHeight;
    int m_contextWidth;
    int m_contextHeight;
    int32_t m_viewport[4];
    HorizontalAlignment m_hAlign;
    VerticalAlignment m_vAlign;
    ScalingMode m_scalingMode;
    bool m_distortionCompensation;
    bool m_rotate90;
    bool m_flipH;
    bool m_flipV;
};

#endif // !ARVIDEOVIEW_H
