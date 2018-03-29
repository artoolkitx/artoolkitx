/*
 *  WindowsMediaCapture.h
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

#pragma once

#include <ARX/AR/config.h>

#include <wrl/client.h>
#include <agile.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <stdint.h>
#include "CaptureFrameGrabber/CaptureFrameGrabber.h"

class WindowsMediaCapture
{

public:
    WindowsMediaCapture(void)/* : m_stopLockCondVar {}*/;
	virtual ~WindowsMediaCapture(void);

	// Setup/tear down functions
	bool StartCapture(int width,
					  int height,
					  Platform::String^ pixelFormat = Windows::Media::MediaProperties::MediaEncodingSubtypes::Rgb24, // Raw formats: Rgb24, Rgb32, Nv12, Argb32, Bgra8, Yv12, Yuy2, Iyuv
					  int preferredDeviceIndex = 0,
					  Windows::Devices::Enumeration::Panel preferredLocation = Windows::Devices::Enumeration::Panel::Unknown,
					  void (*errorCallback)(void *) = NULL,
				      void *errorCallbackUserdata = NULL
					);
	bool Capturing() const; // Returns true if between StartCapture() and StopCapture().
	uint8_t *GetFrame(); // Returns NULL if no new frame available, or pointer to frame buffer if new frame is available. Dimensions of buffer are width()*height()*Bpp(). If non-NULL buffer returned, then any previous non-NULL buffer is invalidated.
	void StopCapture();

	int width() const;
	int height() const;
	int Bpp() const;
    bool flipV() const;
    void setFlipV(bool flag);
    
	Windows::Devices::Enumeration::Panel deviceLocation() const;
    std::string deviceName() const;
	void WindowsMediaCapture::showSettings(void);

private:
    //Disable Copy ctor and assignment
	WindowsMediaCapture(const WindowsMediaCapture& other) = delete; // Copy constructor not implemented.
	auto operator=(const WindowsMediaCapture& other) -> WindowsMediaCapture& = delete; // Copy assignment operator not implemented.
    //Disable Move ctor and assignment
    WindowsMediaCapture(WindowsMediaCapture&&) = delete;
    auto operator=(WindowsMediaCapture&&) -> WindowsMediaCapture& = delete;

	bool initDevices();
	void _GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber);

	bool m_started;
	int m_width;
	int m_height;
	int m_Bpp;
    bool m_flipV;
	Platform::String^ m_pixelFormat;
	Platform::Agile<Windows::Devices::Enumeration::DeviceInformationCollection> m_devices;
	Platform::Agile<Windows::Media::Capture::MediaCapture> m_mediaCapture;
	int m_deviceIndex;
	Windows::Devices::Enumeration::Panel m_deviceLocation;
	std::string m_deviceName;
	::Media::CaptureFrameGrabber^ m_frameGrabber;
	bool m_frameGrabberInited;
	long m_frameCountIn;
	long m_frameCountOut;
	uint8_t *m_buf0;
	uint8_t *m_buf1;
	int m_bufNext;
	std::mutex m_bufLock;
    void (*m_errorCallback)(void *);
    void *m_errorCallbackUserdata;
    bool m_frameGrabberIsDone;
    std::mutex m_stopLockMutex;
    std::condition_variable m_stopLockCondVar;
};