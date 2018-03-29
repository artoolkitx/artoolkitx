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

#include <ARX/AR/sys/WindowsMediaCapture.h>
#include <ARX/AR/ar.h>
#include <ppltasks.h>
#include <cstdio>


using namespace concurrency;						// task, create_task(), wait(), get()
using namespace Platform;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Capture;			// MediaCapture, MediaCaptureInitializationSettings
using namespace Windows::Media::Devices;			// 
using namespace Windows::Media::MediaProperties;	// VideoEncodingProperties
using namespace Windows::Foundation;				// IAsyncOperation
using namespace Windows::Devices::Enumeration;		// DeviceInformation::FindAllAsync(), DeviceInformationCollection, Panel
using namespace Microsoft::WRL;						// ComPtr

//---------------------------------------------------------

// Convert a UTF16 std::wstring to an ANSI std::string.
static std::string utf16ws_to_ansis(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte                  (CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert a Platform::String to an ANSI std::string.
static std::string ps_to_ansis(Platform::String^ ps)
{
	if (ps == nullptr) return std::string();
	return utf16ws_to_ansis(std::wstring(ps->Data()));
}

// Convert a Platform::String to an ANSI C string.
static const char *ps_to_ansicstr(Platform::String^ ps)
{
	if (ps == nullptr) return NULL;
	return ps_to_ansis(ps).c_str();
}

const char* GetTimeStamp()
{
    const double NANOSECONDS_TO_MILLISECONDS = (1.0 / 1000000.0);
    Windows::Globalization::Calendar^ CInstance = ref new Windows::Globalization::Calendar;
    wchar_t Buf[4];
    _snwprintf(Buf, 4, L"%.3d", (int)(CInstance->Nanosecond * NANOSECONDS_TO_MILLISECONDS));
    String^ TS = "TS:" + CInstance->HourAsPaddedString(2) + "h:" + CInstance->MinuteAsPaddedString(2) + "m:" +
                     CInstance->SecondAsPaddedString(2) + "s:" +
                     CInstance->PeriodAsString() + ":" + ref new String(Buf) + "ms";
    return(ps_to_ansis(TS).c_str());
}

static const char *locationNameFromEnum(const Windows::Devices::Enumeration::Panel location)
{
    static const char back[] = "back";
    static const char front[] = "front";
    static const char top[] = "top";
    static const char bottom[] = "bottom";
    static const char left[] = "left";
    static const char right[] = "right";
    static const char unknown[] = "unknown";
    
    switch (location) {
        case Windows::Devices::Enumeration::Panel::Back:    return back;    break;
        case Windows::Devices::Enumeration::Panel::Front:   return front;   break;
        case Windows::Devices::Enumeration::Panel::Top:     return top;     break;
        case Windows::Devices::Enumeration::Panel::Bottom:  return bottom;  break;
        case Windows::Devices::Enumeration::Panel::Left:    return left;    break;
        case Windows::Devices::Enumeration::Panel::Right:   return right;   break;
        case Windows::Devices::Enumeration::Panel::Unknown:
        default:                                            return unknown; break;
    }
}

//---------------------------------------------------------

WindowsMediaCapture::WindowsMediaCapture(void) :
	m_started(false),
	m_frameGrabberInited(false),
    m_frameGrabberIsDone(false),
    m_stopLockCondVar {}
{
    ARLOGd("WindowsMediaCapture::ctor(): called.\n");
}

WindowsMediaCapture::~WindowsMediaCapture(void)
{
    ARLOGi("WindowsMediaCapture::~dtor(): called.\n");
	if (m_started) {
        ARLOGd("WindowsMediaCapture::~dtor(): calling StopCapture(%s).\n", GetTimeStamp());
        StopCapture();
    }
}

bool WindowsMediaCapture::initDevices()
{
	IAsyncOperation<DeviceInformationCollection^>^ deviceOp = DeviceInformation::FindAllAsync(DeviceClass::VideoCapture);
	auto deviceEnumTask = create_task(deviceOp);
	auto deviceEnumTask2 = deviceEnumTask.
        then([this](DeviceInformationCollection^ devices) // value-based continuation, does not run if
        {                                                 // FindAllAsync() throws.
            ARLOGi("WindowsMediaCapture::initDevices(): found %d video capture devices.\n", devices->Size);
		    m_devices = devices;
	    });
	deviceEnumTask2.wait();

	return true;
}

// This must initialize the camera, get the resolutions and prep for the start.
bool WindowsMediaCapture::StartCapture(int width, int height, String^ pixelFormat, int preferredDeviceIndex, Windows::Devices::Enumeration::Panel preferredLocation,
                                       void (*errorCallback)(void *), void *errorCallbackUserdata) 
{
    ARLOGd("WindowsMediaCapture::StartCapture(): called (ThdID-%d, %s).\n", GetCurrentThreadId(), GetTimeStamp());

	if (m_started) {
		ARLOGe("WindowsMediaCapture::StartCapture(): error-capture already started.\n");
	}
	m_started = true;

	if (!m_devices.Get()) {
		initDevices();
		if (!m_devices.Get()) {
			ARLOGe("WindowsMediaCapture::StartCapture(): error-unable to list video capture devices, exiting returning false.\n");
			//rootPage->NotifyUser("Error getting list of camera devices.", NotifyType::ErrorMessage);
            m_started = false;
			return false;
		}
	}

	// Stash parameters.
	m_width = width;
	m_height = height;
	m_pixelFormat = pixelFormat;
    m_errorCallback = errorCallback;
    m_errorCallbackUserdata = errorCallbackUserdata;
    
	// Pragmatic device selection.
	int deviceIndex0 = -1;
	if (preferredLocation != Windows::Devices::Enumeration::Panel::Unknown) {

		int i = 0, j = 0;
		while (i < m_devices->Size) {
			DeviceInformation^ di = m_devices->GetAt(i);
            if (di->EnclosureLocation == nullptr) {
                ARLOGw("WindowsMediaCapture::StartCapture(): warning-requested device location %s but device %d '%s' cannot supply a location.\n",
                       locationNameFromEnum(preferredLocation), i + 1, ps_to_ansicstr(di->Name));
            } else {
                if (di->EnclosureLocation->Panel == preferredLocation) {
                    deviceIndex0 = i;
                    if (j == preferredDeviceIndex) break;
                    j++;
                }
            }
			i++;
		}
	} else {
		deviceIndex0 = (preferredDeviceIndex >= m_devices->Size ? m_devices->Size - 1 : preferredDeviceIndex);
	}

	if (deviceIndex0 < 0) {
		ARLOGe("WindowsMediaCapture::StartCapture(): error-no video devices available, exiting returning false.\n");
		//rootPage->NotifyUser("No camera device found.", NotifyType::ErrorMessage);
        m_started = false;
		return false;
	}

	DeviceInformation^ di = m_devices->GetAt(deviceIndex0);
	if (di == nullptr) { // Sanity check.
		ARLOGe("WindowsMediaCapture::StartCapture(): error-null DeviceInformation, exiting returning false.\n");
        m_started = false;
		return false;
	}
	m_deviceIndex = deviceIndex0;
	m_deviceName = ps_to_ansis(di->Name);
	if (di->EnclosureLocation == nullptr) m_deviceLocation = Windows::Devices::Enumeration::Panel::Unknown;
	else m_deviceLocation = di->EnclosureLocation->Panel;
	ARLOGi("WindowsMediaCapture::StartCapture(): using device %d '%s' (location:%s).\n",
                deviceIndex0 + 1, m_deviceName.c_str(), locationNameFromEnum(m_deviceLocation));

	Windows::Media::Capture::MediaCaptureInitializationSettings^ captureInitSettings = ref new MediaCaptureInitializationSettings();
	captureInitSettings->VideoDeviceId = di->Id;
	captureInitSettings->AudioDeviceId = nullptr;
	captureInitSettings->StreamingCaptureMode = StreamingCaptureMode::Video;

	try {
		auto mediaCapture = ref new Windows::Media::Capture::MediaCapture();
        if (mediaCapture == nullptr) {
            ARLOGe("WindowsMediaCapture::StartCapture(): error-null MediaCapture, exiting returning false.\n");
            m_started = false;
            return false;
        }
		m_mediaCapture = mediaCapture;

		Windows::ApplicationModel::Core::CoreApplicationView^ cav = Windows::ApplicationModel::Core::CoreApplication::MainView;
		if (cav == nullptr) {
			ARLOGe("WindowsMediaCapture::StartCapture(): error-null CoreApplicationView, exiting returning false.\n");
            m_started = false;
			return false;
		}
		Windows::UI::Core::CoreWindow^ cw = cav->CoreWindow;
		if (cw == nullptr) {
			ARLOGe("WindowsMediaCapture::StartCapture(): error-null CoreWindow, exiting returning false.\n");
            m_started = false;
			return false;
		}
		Windows::UI::Core::CoreDispatcher^ cd = cw->Dispatcher;
		if (cd == nullptr) {
			ARLOGe("WindowsMediaCapture::StartCapture(): error-null CoreDispatcher, exiting returning false.\n");
            m_started = false;
			return false;
		}

		cd->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
			         ref new Windows::UI::Core::DispatchedHandler(
            [this, captureInitSettings]()
		    {
                ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda) executing, (ThdID-%d, %s).\n", GetCurrentThreadId(), GetTimeStamp());
			    // Code here will execute on UI thread.
		
			    // InitializeAsync must run on UI STA thread, so that it can get access to the WebCam.
                task<void> initTask = create_task(m_mediaCapture->InitializeAsync(captureInitSettings));
			    auto initTask2 = initTask.then(
				    //.then([this](task<void> initTask) // Task-based continuation
				    [this]() // Value-based continuation, does not run if InitializeAsync() throws.
			        {
                        ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda).task(initTask).then(initTask2) executing, (ThdID-%d).\n", GetCurrentThreadId());
				        //try {
				        //	initTask.get(); // Also needs to be surrounded with try{} catch() {}.
				        //} catch (Exception ^ e) {
				        //	ARLOGe("Error: unable to initialise Windows::Media::Capture::MediaCapture.\n");
				        //	ARLOGe(ps_to_ansicstr(e->Message));
				        //	//if (m_errorCallback) (*m_errorCallback)(m_errorCallbackUserdata);
				        //	return;
				        //}
			
                        ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda).task(initTask).then(initTask2) before Agile<Windows::MediaCapture>.Get().\n");
				        auto mediaCapture = m_mediaCapture.Get();
                        ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda).task(initTask).then(initTask2) after Agile<Windows::MediaCapture>.Get().\n");

				        //ShowStatusMessage("Device initialized OK");
				
				        // Can't currently use this function, as we're not a ref class.
				        //mediaCapture->Failed += ref new Windows::Media::Capture::MediaCaptureFailedEventHandler(this, &WindowsMediaCapture::Failed);

				        //IVectorView<IMediaEncodingProperties^>^ res = mediaCapture->VideoDeviceController->GetAvailableMediaStreamProperties(MediaStreamType::Photo);
				        // Now iterate through and examine supported resolutions.
				        // NOT YET IMPLEMENTED. Instead:
				        auto props = safe_cast<VideoEncodingProperties^>(mediaCapture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
				        props->Subtype = m_pixelFormat;
				        props->Width = m_width;
				        props->Height = m_height;

				        // Allocate buffers.
				        if      (m_pixelFormat == MediaEncodingSubtypes::Rgb24) m_Bpp = 3;
				        else if	(m_pixelFormat == MediaEncodingSubtypes::Rgb32 || m_pixelFormat == MediaEncodingSubtypes::Argb32 || m_pixelFormat == MediaEncodingSubtypes::Bgra8) m_Bpp = 4;
				        else if (m_pixelFormat == MediaEncodingSubtypes::Yuy2) m_Bpp = 2;
				        //else if (m_pixelFormat == MediaEncodingSubtypes::Nv12) m_Bpp = 1.5; // 2 planes.
				        //else if (m_pixelFormat == MediaEncodingSubtypes::Yv12 || m_pixelFormat == MediaEncodingSubtypes::Iyuv) m_Bpp = 1.5; // 3 planes. Also, Iyuv == I420. 
				        else {
					        ARLOGe("WindowsMediaCapture::StartCapture(): error-request for unsupported pixel format.\n");
					        throw ref new InvalidArgumentException("WindowsMediaCapture::StartCapture(): error-request for unsupported pixel format.");
				        }
				        size_t bufSize = m_width * m_Bpp * m_height;
				        m_buf0 = (uint8_t *)malloc(bufSize);
				        m_buf1 = (uint8_t *)malloc(bufSize);
				        if (!m_buf0 || !m_buf1) {
					        ARLOGe("WindowsMediaCapture::StartCapture(): error-out of memory while attempting to allocate frame buffers.\n");
					        throw ref new OutOfMemoryException("WindowsMediaCapture::StartCapture(): error-out of memory while attempting to allocate frame buffers.");
				        }
				        m_frameCountOut = m_frameCountIn = 0L;
				        m_bufNext = 0;

				        return ::Media::CaptureFrameGrabber::CreateAsync(mediaCapture, props);

			        }).then(
                        [this](::Media::CaptureFrameGrabber^ frameGrabber) // Value-based continuation, does not run if preceding block throws.
			            {
                            ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda).task(initTask).then(initTask2).then(lambda) executing, m_frameGrabberInited=true, m_frameGrabberIsDone=false, (ThdID-%d).\n", GetCurrentThreadId());
				            m_frameGrabber = frameGrabber;
				            m_frameGrabberInited = true;
                            m_frameGrabberIsDone = false;
                            ARLOGd("WindowsMediaCapture::StartCapture(): calling _GrabFrameAsync() to start frame grabber task (%s).\n", GetTimeStamp().\n);
				            _GrabFrameAsync(frameGrabber);
                            ARLOGd("WindowsMediaCapture::StartCapture(): RunAsync(lambda).task(initTask).then(initTask2).then(lambda) exiting(End of StartAR(%s)).\n", GetTimeStamp().\n);
			            });
		    }));//end: cd->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                //                  ref new Windows::UI::Core::DispatchedHandler(
                //                      [this, captureInitSettings]()
	} catch (Exception ^ e) {
		ARLOGe("WindowsMediaCapture::StartCapture(): error-unable to initialise Windows::Media::Capture::MediaCapture, exiting returning false.\n");
		ARLOGe(ps_to_ansicstr(e->Message));
        m_started = false;
        return false;
	}

    ARLOGd("WindowsMediaCapture::StartCapture(): exiting returning true.\n");
	return true;
}

bool WindowsMediaCapture::Capturing() const
{
	return m_started;
}

void WindowsMediaCapture::_GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber)
{
    ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): called (ThdID-%d).\n", GetCurrentThreadId());
	if (frameGrabber == nullptr) { // Sanity check.
		ARLOGe("WindowsMediaCapture::_GrabFrameAsync(): error-NULL frameGrabber, exiting.\n");
		return;
	}
	
    task<ComPtr<IMF2DBuffer2>> getFrameTask = create_task(frameGrabber->GetFrameAsync());
    auto getFrameTask2 = getFrameTask.then(
        [this, frameGrabber](task<ComPtr<IMF2DBuffer2>> task1)
        {
            bool grabbedOK = false;
            ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): getFrameTask2 (frameGrabber->GetFrameAsync() task).then(lambda exp) entered (ThdID-%d).\n", GetCurrentThreadId());
        
            ComPtr<IMF2DBuffer2>& buffer = task1.get();
		    if (!buffer) { // If CaptureFrameGrabber::GetFrameAsync() fails, buffer will be NULL.
                ARLOGe("WindowsMediaCapture::_GrabFrameAsync(): error-NULL task<ComPtr<IMF2DBuffer2>> param.\n");
            } else {
                if (m_frameGrabberInited) {
                    std::lock_guard<std::mutex> lock(m_bufLock); // Locks when constructed, unlocks when deleted (deleted when it goes out of scope).

                    // Copy the pixels from the IMF2DBuffer2
                    uint8_t* buf = (m_bufNext ? m_buf1 : m_buf0);
                    if (!buf || !m_width || !m_height || !m_Bpp) {
                        ARLOGe("WindowsMediaCapture::_GrabFrameAsync(): error-video parameters are incomplete.\n");
                    } else {
                        size_t bufRowBytes = m_width * m_Bpp;

                        BYTE *pbScanline0;
                        LONG lPitch;
                        BYTE *pbBufferStart;
                        DWORD cbBufferLength;

                        // Lock2DSize is more optimal than Lock2D(&pbScanline0, &lPitch).
                        CHK(buffer->Lock2DSize(MF2DBuffer_LockFlags_Read, &pbScanline0, &lPitch, &pbBufferStart, &cbBufferLength));
                    
                        if (pbBufferStart + m_height*bufRowBytes > pbBufferStart + cbBufferLength) {
                            ARLOGe("WindowsMediaCapture::_GrabFrameAsync error BUFFER OVERFLOW.\n");
                        } else {
                            //ARLOGi("Copying buffer (scanline0=%p, pitch=%ld, start=%p, len=%d) to buf%d (rowBytes=%ld) frameCount=%d.\n", pbScanline0, lPitch, pbBufferStart, cbBufferLength, m_bufNext, bufRowBytes, m_frameCountIn + 1);
                        
                            if (!m_flipV && lPitch == bufRowBytes) {
                                // Optimised case, copy entire buffer in one call.
                                memcpy(buf, pbScanline0, m_height * bufRowBytes);
                            } else {
                                // Line by line copy.
                                if (m_flipV) {
                                    // Do v flip by reversing order of writes.
                                    buf = buf + (m_height - 1) * bufRowBytes;
                                    for (unsigned int row = 0; row < m_height; row++) {
                                        memcpy(buf, pbScanline0, bufRowBytes);
                                        pbScanline0 += lPitch;
                                        buf -= bufRowBytes;
                                    }
                                } else {
                                    for (unsigned int row = 0; row < m_height; row++) {
                                        memcpy(buf, pbScanline0, bufRowBytes);
                                        pbScanline0 += lPitch;
                                        buf += bufRowBytes;
                                    }
                                }
                            }
                        
                            m_frameCountIn++;
                            grabbedOK = true;
                        }
                    
                        CHK(buffer->Unlock2D());
                    }
                } //end: if (m_frameGrabberInited)
            } //end: else of if (!buffer)

            // Provided we haven't been asked to stop, and we ran successfully, initiate a new
            // frame grab. Note that because the task initiated has
            // task_continuation_context::use_current(), it won't actually begin to execute
            // until this task is done.
            if (m_frameGrabberInited && grabbedOK) {
                ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): if (m_frameGrabberInited && grabbedOK) true.\n");
                _GrabFrameAsync(frameGrabber);
            } else {
                ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): done, locking m_stopLockMutex, setting m_frameGrabberIsDone=true, notifying m_stopLockCondVar wait thread.\n");
                std::unique_lock<std::mutex> lock(m_stopLockMutex);
			    m_frameGrabberIsDone = true;
			    lock.unlock(); // Manual unlocking is done before notifying, to avoid waking up any waiting thread only to block again (see notify_one for details).
                m_stopLockCondVar.notify_one();
                ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): notified m_stopLockCondVar StopCapture thread.\n");
		    }
        }, //end: continuation task param arg 1: [this, frameGrabber](task<ComPtr<IMF2DBuffer2>> task1){}
        task_continuation_context::use_current() //end: continuation task param arg 2
    ); //end: getFrameTask2([this, frameGrabber](task<ComPtr<IMF2DBuffer2>> task1), task_continuation_context::use_current())
    ARLOGd("WindowsMediaCapture::_GrabFrameAsync(): exiting (ThdID-%d).\n", GetCurrentThreadId());
}

uint8_t *WindowsMediaCapture::GetFrame()
{
	uint8_t *buf = NULL;
	if (m_started && m_frameGrabberInited && (m_frameCountOut != m_frameCountIn)) {
		//ARLOGd("Will check out frame (frameCount=%d).\n", m_frameCountIn);
		std::lock_guard<std::mutex> lock(m_bufLock); // Locks when constructed, unlocks when deleted (deleted when it goes out of scope).
		if (m_bufNext) {
			buf = m_buf1;
			m_bufNext = 0;
		} else {
			buf = m_buf0;
			m_bufNext = 1;
		}
		m_frameCountOut = m_frameCountIn;
	}
	return buf;
}

void WindowsMediaCapture::StopCapture()
{
    ARLOGi("WindowsMediaCapture::StopCapture(): called (ThdID-%d)\n", GetCurrentThreadId());
	if (!m_started) {
		ARLOGe("WindowsMediaCapture::StopCapture(): Error-capture already stopped\n");
		return;
	}
	m_started = false;

	m_frameGrabberInited = false; // At the end of the current grab, framegrabber will not start a new grab.
    ARLOGd("WindowsMediaCapture::StopCapture(): m_frameGrabberInited was just set to false\n");
    if (m_frameGrabber != nullptr) {
        ARLOGd("WindowsMediaCapture::StopCapture(): if (m_frameGrabber != nullptr) true\n");
        // Wait for framegrabber to exit.
        {
            std::unique_lock<std::mutex> lock(m_stopLockMutex);
            if (!m_frameGrabberIsDone) {
                ARLOGd("WindowsMediaCapture::StopCapture(): if (!m_frameGrabberIsDone) true, calling m_stopLockCondVar.wait()\n");
                // Atomically releases lock and enters a wait/sleep state. Atomically grabs lock and comes out of sleep state when predicate becomes true.
                // While passed in predicate evaluates to false, m_stopLockCondVar continues to wait.
                m_stopLockCondVar.wait(lock,
                                       [this]()
                                       {
                                           return m_frameGrabberIsDone;
                                       });
            }
		}

        ARLOGi("WindowsMediaCapture::StopCapture(): m_frameGrabberIsDone must be true (%s)\n", ((m_frameGrabberIsDone)? "true" : "false"));
		auto finishTask = create_task(m_frameGrabber->FinishAsync());
		auto finishTask2 = finishTask.then([this]
		                                   {
			                                   m_frameGrabber = nullptr; // Remove our reference. At the end of the current grab,
                                                                         // no new task will hold a reference and it will be deleted.
		                                   });
		finishTask2.wait();
    }

	{
		std::lock_guard<std::mutex> lock(m_bufLock); // Locks when constructed, unlocks when deleted (deleted when it goes out of scope).
		free(m_buf0);
		m_buf0 = NULL;
		free(m_buf1);
		m_buf1 = NULL;
	}
    ARLOGd("WindowsMediaCapture::StopCapture(): exiting(End of StopAR(%s))\n", GetTimeStamp());
}

int WindowsMediaCapture::width() const
{
	return m_width;
}

int WindowsMediaCapture::height() const
{
	return m_height;
}

int WindowsMediaCapture::Bpp() const
{
	return m_Bpp;
}

bool WindowsMediaCapture::flipV() const
{
    return m_flipV;
}

void WindowsMediaCapture::setFlipV(bool flag)
{
    m_flipV = flag;
}

Windows::Devices::Enumeration::Panel  WindowsMediaCapture::deviceLocation() const
{
	return m_deviceLocation;
}

std::string  WindowsMediaCapture::deviceName() const
{
	return m_deviceName;
}

void WindowsMediaCapture::showSettings()
{
    if (m_frameGrabberInited) {
        m_frameGrabber->ShowCameraSettings();
    }
}

//void WindowsMediaCapture::Failed(Windows::Media::Capture::MediaCapture ^currentCaptureObject,
//                                 Windows::Media::Capture::MediaCaptureFailedEventArgs^ currentFailure)
//{
//    String ^message = "Fatal error: " + currentFailure->Message;
//    ARLOGe(ps_to_ansis(message).c_str());
//    //create_task(Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High,
//    //                                 ref new Windows::UI::Core::DispatchedHandler([this, message]()
//    //                                                                              {
//    //                                                                                  ShowStatusMessage(message);
//    //                                                                              })));
//    if (m_errorCallback) (*m_errorCallback)(m_errorCallbackUserdata);
//}
