/*
 *  ARVideoView.cpp
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
 
#include <ARX/ARVideoView.h>
#if USE_GL_STATE_CACHE
#  include <ARX/ARG/glStateCache2.h>
#endif
#include <algorithm>

ARVideoView::ARVideoView() :
    m_arglContextSettings(NULL),
    m_drawTypeSet(DrawType::None),
    m_drawTime({0, 0}),
    m_contentWidth(0),
    m_contentHeight(0),
    m_contextWidth(0),
    m_contextHeight(0),
    m_viewport{0},
    m_hAlign(HorizontalAlignment::H_ALIGN_CENTRE),
    m_vAlign(VerticalAlignment::V_ALIGN_CENTRE),
    m_scalingMode(ScalingMode::SCALE_MODE_FIT),
    m_distortionCompensation(true),
    m_rotate90(false),
    m_flipH(false),
    m_flipV(false)
{
}

bool ARVideoView::initWithVideoSource(const ARVideoSource& vs, const int contextWidth, const int contextHeight)
{
    ARLOGd("ARVideoView::initWithVideoSource.\n");

    // If glStateCache is in use, need to flush it here, since we can't guarantee external users will have been using it.
#if USE_GL_STATE_CACHE
    glStateCacheFlush();
#endif
    // Allow re-initialization.
    if (m_arglContextSettings) {
        arglCleanup(m_arglContextSettings);
    }
    ARParamLT *paramLT = vs.getCameraParameters();
    if (paramLT) {
        m_arglContextSettings = arglSetupForCurrentContext(&(paramLT->param), vs.getPixelFormat());
    } else {
        ARParam param;
        arParamClear(&param, vs.getVideoWidth(), vs.getVideoHeight(), AR_DIST_FUNCTION_VERSION_DEFAULT);
        m_arglContextSettings = arglSetupForCurrentContext(&param, vs.getPixelFormat());
        m_distortionCompensation = false;
    }
    if (!m_arglContextSettings) {
        ARLOGe("Error initializing display of video frames from source.\n");
        return false;
    }
    m_drawTypeSet = DrawType::Main;
    m_contentWidth = vs.getVideoWidth();
    m_contentHeight = vs.getVideoHeight();
    m_contextWidth = contextWidth;
    m_contextHeight = contextHeight;
    update();
    
    return true;
}

void ARVideoView::update()
{
    if (m_arglContextSettings) {
        arglSetRotate90(m_arglContextSettings, m_rotate90 ? 1 : 0);
        arglSetFlipH(m_arglContextSettings, m_flipH ? 1 : 0);
        arglSetFlipV(m_arglContextSettings, m_flipV ? 1 : 0);
        arglDistortionCompensationSet(m_arglContextSettings, m_distortionCompensation ? 1 : 0);
    }
    
    // Calculate viewPort.
    int left, bottom, w, h;
    if (m_scalingMode == ScalingMode::SCALE_MODE_STRETCH) {
        w = m_contextWidth;
        h = m_contextHeight;
    } else {
        int contentWidthFinalOrientation = (m_rotate90 ? m_contentHeight : m_contentWidth);
        int contentHeightFinalOrientation = (m_rotate90 ? m_contentWidth : m_contentHeight);
        if (m_scalingMode == ScalingMode::SCALE_MODE_FIT || m_scalingMode == ScalingMode::SCALE_MODE_FILL) {
            float scaleRatioWidth, scaleRatioHeight, scaleRatio;
            scaleRatioWidth = (float)m_contextWidth / (float)contentWidthFinalOrientation;
            scaleRatioHeight = (float)m_contextHeight / (float)contentHeightFinalOrientation;
            if (m_scalingMode == ScalingMode::SCALE_MODE_FILL) scaleRatio = (std::max)(scaleRatioHeight, scaleRatioWidth);
            else scaleRatio = (std::min)(scaleRatioHeight, scaleRatioWidth);
            w = (int)((float)contentWidthFinalOrientation * scaleRatio);
            h = (int)((float)contentHeightFinalOrientation * scaleRatio);
        } else {
            w = contentWidthFinalOrientation;
            h = contentHeightFinalOrientation;
        }
    }

    if (m_hAlign == HorizontalAlignment::H_ALIGN_LEFT) left = 0;
    else if (m_hAlign == HorizontalAlignment::H_ALIGN_RIGHT) left = m_contextWidth - w;
    else left = (m_contextWidth - w) / 2;
    
    if (m_vAlign == VerticalAlignment::V_ALIGN_BOTTOM) bottom = 0;
    else if (m_vAlign == VerticalAlignment::V_ALIGN_TOP) bottom = m_contextHeight - h;
    else bottom = (m_contextHeight - h) / 2;
    
    m_viewport[0] = left;
    m_viewport[1] = bottom;
    m_viewport[2] = w;
    m_viewport[3] = h;
    ARLOGd("ARVideoView::update() calculated viewport {%d, %d, %d, %d}\n", left, bottom, w, h);
}

bool ARVideoView::distortionCompensation() const
{
    return m_distortionCompensation;
}

void ARVideoView::setDistortionCompensation(const bool distortionCompensation)
{
    if (m_distortionCompensation != distortionCompensation) {
        m_distortionCompensation = distortionCompensation;
        update();
    }
}

bool ARVideoView::rotate90() const
{
    return m_rotate90;
}

void ARVideoView::setRotate90(const bool rotate90)
{
    if (m_rotate90 != rotate90) {
        m_rotate90 = rotate90;
        update();
    }
}

bool ARVideoView::flipH() const
{
    return m_flipH;
}

void ARVideoView::setFlipH(const bool flipH)
{
    if (m_flipH != flipH) {
        m_flipH = flipH;
        update();
    }
}

bool ARVideoView::flipV() const
{
    return m_flipV;
}

void ARVideoView::setFlipV(const bool flipV)
{
    if (m_flipV != flipV) {
        m_flipV = flipV;
        update();
    }
}

ARVideoView::HorizontalAlignment ARVideoView::horizontalAlignment() const
{
    return m_hAlign;
}

void ARVideoView::setHorizontalAlignment(const HorizontalAlignment hAlign)
{
    if (m_hAlign != hAlign) {
        m_hAlign = hAlign;
        update();
    }
}

ARVideoView::VerticalAlignment ARVideoView::verticalAlignment() const
{
    return m_vAlign;
}

void ARVideoView::setVerticalAlignment(const VerticalAlignment vAlign)
{
    if (m_vAlign != vAlign) {
        m_vAlign = vAlign;
        update();
    }
}

ARVideoView::ScalingMode ARVideoView::scalingMode() const
{
    return m_scalingMode;
}

void ARVideoView::setScalingMode(const ScalingMode scaling)
{
    if (m_scalingMode != scaling) {
        m_scalingMode = scaling;
        update();
    }
}

ARVideoView::Size ARVideoView::contextSize() const
{
    return {m_contextWidth, m_contextHeight};
}

void ARVideoView::setContextSize(const Size size)
{
    if (m_contextWidth != size.width || m_contextHeight != size.height) {
        m_contextWidth = size.width;
        m_contextHeight = size.height;
        update();
    }
}

void ARVideoView::getViewport(int32_t viewport[4]) const
{
    if (!viewport) return;
    viewport[0] = m_viewport[0];
    viewport[1] = m_viewport[1];
    viewport[2] = m_viewport[2];
    viewport[3] = m_viewport[3];
}

void ARVideoView::draw(ARVideoSource* vs)
{
    // If glStateCache is in use, need to flush it here, since we can't guarantee external users will have been using it.
#if USE_GL_STATE_CACHE
    glStateCacheFlush();
#endif
    AR2VideoBufferT *frame = vs->checkoutFrameIfNewerThan(m_drawTime);
    if (frame) {
        m_drawTime = frame->time;
        if (m_drawTypeSet != DrawType::Main) {
            arglPixelFormatSet(m_arglContextSettings, vs->getPixelFormat());
            arglPixelBufferSizeSet(m_arglContextSettings, m_contentWidth, m_contentHeight);
            arglSetPixelZoom(m_arglContextSettings, 1.0f);
            m_drawTypeSet = DrawType::Main;
        }
        if (frame->bufPlaneCount == 2) {
            if (!arglPixelBufferDataUploadBiPlanar(m_arglContextSettings, frame->bufPlanes[0], frame->bufPlanes[1])) {
                ARLOGe("arglPixelBufferDataUploadBiPlanar.\n");
            }
        } else {
            if (!arglPixelBufferDataUpload(m_arglContextSettings, frame->buff)) {
                ARLOGe("arglPixelBufferDataUpload.\n");
            }
        }
        vs->checkinFrame();
    } else {
        ARLOGd("ARVideoView::draw frame=NULL.\n");
    }
    arglDispImage(m_arglContextSettings, m_viewport);
}

void ARVideoView::drawDebugImage(const uint8_t *image, int pyramidLevel)
{
    // If glStateCache is in use, need to flush it here, since we can't guarantee external users will have been using it.
#if USE_GL_STATE_CACHE
    glStateCacheFlush();
#endif
    if (m_drawTypeSet != DrawType::Debug) {
        arglPixelFormatSet(m_arglContextSettings, AR_PIXEL_FORMAT_MONO);
        arglPixelBufferSizeSet(m_arglContextSettings, m_contentWidth >> pyramidLevel, m_contentHeight >> pyramidLevel);
        arglSetPixelZoom(m_arglContextSettings, (float)(1 << pyramidLevel));
        m_drawTypeSet = DrawType::Debug;
    }
    if (image) {
        if (!arglPixelBufferDataUploadBiPlanar(m_arglContextSettings, (ARUint8 *)image, NULL)) {
            ARLOGe("arglPixelBufferDataUploadBiPlanar.\n");
        }
    } else {
        ARLOGd("ARVideoView::drawDebugImage image=NULL.\n");
    }
    arglDispImage(m_arglContextSettings, m_viewport);
}

ARVideoView::~ARVideoView()
{
    arglCleanup(m_arglContextSettings);
}


