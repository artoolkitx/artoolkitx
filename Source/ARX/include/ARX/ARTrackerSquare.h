/*
 *  ARTrackerSquare.h
 *  artoolkitX
 *
 *  A C++ class implementing the artoolkitX square fiducial marker tracker.
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


#ifndef ARTRACKERSQUARE_H
#define ARTRACKERSQUARE_H

#include <ARX/ARTrackableSquare.h>
#include <ARX/ARTrackableMultiSquare.h>
#include <ARX/ARTrackerVideo.h>
#include <ARX/AR/ar.h>

class ARTrackerSquare : public ARTrackerVideo {
public:
    ARTrackerSquare();
    ~ARTrackerSquare();
    
    ARTrackerType type() const override {
        return ARTrackerType::SQUARE_FIDUCIAL;
    }
    
    std::vector<std::string> trackableConfigurations() const override {
        std::vector<std::string> sv;
        sv.push_back("single");
        sv.push_back("single_barcode");
        sv.push_back("single_buffer");
        sv.push_back("multi");
        return sv;
    }

    bool initialize() override;

    /**
     * Enables or disables debug mode in the tracker. When enabled, a black and white debug
     * image is generated during marker detection. The debug image is useful for visualising
     * the binarization process and choosing a threshold value.
     * @param    debug        true to enable debug mode, false to disable debug mode
     * @see                    getDebugMode()
     */
    void setDebugMode(bool debug);
    
    /**
     * Returns whether debug mode is currently enabled.
     * @return                true when debug mode is enabled, false when debug mode is disabled
     * @see                    setDebugMode()
     */
    bool debugMode() const;
    
    void setImageProcMode(int mode);
    
    int imageProcMode() const;
    
    /**
     * Sets the threshold value used for image binarization.
     * @param    thresh    The new threshold value to use
     * @see                    getThreshold()
     */
    void setThreshold(int thresh);
    
    /**
     * Returns the current threshold value used for image binarization.
     * @return                The current threshold value
     * @see                    setThreshold()
     */
    int threshold() const;
    
    /**
     * Sets the thresholding mode to use.
     * @param mode            The new thresholding mode to use.
     * @see                    getThresholdMode()
     */
    void setThresholdMode(AR_LABELING_THRESH_MODE mode);
    
    /**
     * Returns the current thresholding mode.
     * @return                The current thresholding mode.
     * @see                    setThresholdMode()
     */
    AR_LABELING_THRESH_MODE thresholdMode() const;
    
    /**
     * Sets the labeling mode to use.
     * @param mode            The new labeling mode to use.
     * @see                    getLabelingMode()
     */
    void setLabelingMode(int mode);
    
    /**
     * Returns the current labeling mode.
     * @return                The current labeling mode.
     * @see                    setLabelingMode()
     */
    int labelingMode() const;
    
    void setPatternDetectionMode(int mode);
    
    int patternDetectionMode() const;
    
    void setPattRatio(float ratio);
    
    float pattRatio() const;
    
    void setMatrixCodeType(AR_MATRIX_CODE_TYPE type);
    
    AR_MATRIX_CODE_TYPE matrixCodeType() const;
    
    void setPatternSize(int patternSize);
    
    int patternSize() const;
    
    void setPatternCountMax(int patternCountMax);
    
    int patternCountMax() const;
    
    void setMatrixModeAutoCreateNewTrackables(bool on) { m_matrixModeAutoCreateNewTrackables = on; }

    bool matrixModeAutoCreateNewTrackables() const { return m_matrixModeAutoCreateNewTrackables; }

    typedef void (*MatrixModeAutoCreateNewTrackablesCallback_t)(const ARTrackableSquare& trackable);
    void setMatrixModeAutoCreateNewTrackablesCallback(MatrixModeAutoCreateNewTrackablesCallback_t callback) { m_matrixModeAutoCreateNewTrackablesCallback = callback; }
    MatrixModeAutoCreateNewTrackablesCallback_t matrixModeAutoCreateNewTrackablesCallback() const { return m_matrixModeAutoCreateNewTrackablesCallback; }


    static constexpr float k_matrixModeAutoCreateNewTrackablesDefaultWidth_default = 80.0f;
    void setMatrixModeAutoCreateNewTrackablesDefaultWidth(ARdouble width) { m_matrixModeAutoCreateNewTrackablesDefaultWidth = width; }

    float matrixModeAutoCreateNewTrackablesDefaultWidth() const { return m_matrixModeAutoCreateNewTrackablesDefaultWidth; }

    bool start(ARParamLT *paramLT, AR_PIXEL_FORMAT pixelFormat) override;
    bool start(ARParamLT *paramLT0, AR_PIXEL_FORMAT pixelFormat0, ARParamLT *paramLT1, AR_PIXEL_FORMAT pixelFormat1, const ARdouble transL2R[3][4]) override;
    bool isRunning() override;
    bool wantsUpdate() override;
    bool update(AR2VideoBufferT *buff) override;
    bool update(AR2VideoBufferT *buff0, AR2VideoBufferT *buff1) override;
    bool stop() override;
    void terminate() override;

    int newTrackable(std::vector<std::string> config) override;
    unsigned int countTrackables() override;
    std::shared_ptr<ARTrackable> getTrackable(int UID) override;
    std::vector<std::shared_ptr<ARTrackable>> getAllTrackables() override;
    bool deleteTrackable(int UID) override;
    void deleteAllTrackables() override;

    bool updateDebugTextureRGBA32(const int videoSourceIndex, uint32_t* buffer);
    
private:
    std::vector<std::shared_ptr<ARTrackable>> m_trackables;
    int m_threshold;
    AR_LABELING_THRESH_MODE m_thresholdMode;
    int m_imageProcMode;
    int m_labelingMode;
    ARdouble m_pattRatio;
    int m_patternDetectionMode;
    AR_MATRIX_CODE_TYPE m_matrixCodeType;
    bool m_debugMode;
    int m_patternSize;
    int m_patternCountMax;
    bool m_matrixModeAutoCreateNewTrackables;
    float m_matrixModeAutoCreateNewTrackablesDefaultWidth;
    MatrixModeAutoCreateNewTrackablesCallback_t m_matrixModeAutoCreateNewTrackablesCallback;

    ARHandle *m_arHandle0;              ///< Structure containing square tracker state.
    ARHandle *m_arHandle1;              ///< For stereo tracking, structure containing square tracker state for second tracker in stereo pair.
    ARPattHandle *m_arPattHandle;       ///< Structure containing information about trained patterns.
    AR3DHandle *m_ar3DHandle;           ///< Structure used to compute 3D poses from tracking data.
    ARdouble m_transL2R[3][4];          ///< For stereo tracking, transformation matrix from left camera to right camera.
    AR3DStereoHandle *m_ar3DStereoHandle; ///< For stereo tracking, additional tracker state.
};

#endif // !ARTRACKERSQUARE_H
