//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Portions Copyright (c) Microsoft Open Technologies, Inc. 
//
//*@@@---@@@@******************************************************************

#include <stdexcept>
#include <ARX/AR/ar.h>

#include "MediaStreamSink.h"
#include "MediaSink.h"
#include "CaptureFrameGrabber.h"

using namespace Media;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Media;
using namespace Windows::Media::Capture;
using namespace Windows::Media::MediaProperties;
using namespace concurrency;
using namespace Microsoft::WRL::Details;
using namespace Microsoft::WRL;

task<Media::CaptureFrameGrabber^> Media::CaptureFrameGrabber::CreateAsync(_In_ MediaCapture^ capture, _In_ VideoEncodingProperties^ props, CaptureStreamType streamType)
{
    auto reader = ref new Media::CaptureFrameGrabber(capture, props, streamType);

    auto profile = ref new MediaEncodingProfile();
    profile->Video = props;

    task<void> task;
    if (reader->_streamType == CaptureStreamType::Preview)
    {
        task = create_task(capture->StartPreviewToCustomSinkAsync(profile, reader->_mediaExtension));
    }
    else
    {
        task = create_task(capture->StartRecordToCustomSinkAsync(profile, reader->_mediaExtension));
    }

    return task.then([reader]()
    {
        reader->_state = State::Started;
        return reader;
    });
}

Media::CaptureFrameGrabber::CaptureFrameGrabber(_In_ MediaCapture^ capture, _In_ VideoEncodingProperties^ props, CaptureStreamType streamType)
: _state(State::Created)
, _streamType(streamType)
, _capture(capture)
{
    auto videoSampleHandler = ref new MediaSampleHandler(this, &Media::CaptureFrameGrabber::ProcessSample);

    _mediaSink = Make<MediaSink>(nullptr, props, nullptr, videoSampleHandler);
    _mediaExtension = reinterpret_cast<IMediaExtension^>(static_cast<AWM::IMediaExtension*>(_mediaSink.Get()));
}

Media::CaptureFrameGrabber::~CaptureFrameGrabber()
{
    if (_state == State::Started)
    {
        if (_streamType == CaptureStreamType::Preview)
        {
            (void)_capture->StopPreviewAsync();
        }
        else
        {
            (void)_capture->StopRecordAsync();
        }
    }

    if (_mediaSink != nullptr)
    {
        (void)_mediaSink->Shutdown();
        _mediaSink = nullptr;
    }
    _mediaExtension = nullptr;
    _capture = nullptr;
}

void Media::CaptureFrameGrabber::ShowCameraSettings()
{
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
    throw std::runtime_error("ShowCameraSettings not implemented on Windows Phone");
#else
    if (_state == State::Started)
    {
        CameraOptionsUI::Show(_capture.Get());
    }
#endif
}

task<void> Media::CaptureFrameGrabber::FinishAsync()
{
    auto lock = _lock.LockExclusive();

    if (_state != State::Started)
    {
        throw ref new COMException(E_UNEXPECTED, L"State");
    }
    _state = State::Closing;

    if (_mediaSink != nullptr)
    {
        (void)_mediaSink->Shutdown();
        _mediaSink = nullptr;
    }
    _mediaExtension = nullptr;

    task<void> task;
    if (_streamType == CaptureStreamType::Preview)
    {
        task = create_task(_capture->StopPreviewAsync());
    }
    else
    {
        task = create_task(_capture->StopRecordAsync());
    }

    return task.then([this]()
    {
        auto lock = _lock.LockExclusive();
        _state = State::Closed;
        _capture = nullptr;
    });
}

task<ComPtr<IMF2DBuffer2>> Media::CaptureFrameGrabber::GetFrameAsync()
{
    ARLOGd("Media::CaptureFrameGrabber::GetFrameAsync(): called.\n");
    auto lock = _lock.LockExclusive();

    if (_state != State::Started)
    {
        ARLOGe("Media::CaptureFrameGrabber::GetFrameAsync(): throwing E_UNEXPECTED State.\n");
        throw ref new COMException(E_UNEXPECTED, L"State");
    }

    _mediaSink->RequestVideoSample();

    task_completion_event<ComPtr<IMF2DBuffer2>> taskEvent;
    _videoSampleRequestQueue.push(taskEvent);

    return create_task(taskEvent);
    ARLOGd("Media::CaptureFrameGrabber::GetFrameAsync(): exiting.\n");
}

void Media::CaptureFrameGrabber::ProcessSample(_In_ MediaSample^ sample)
{
    task_completion_event<ComPtr<IMF2DBuffer2>> t;

    {
        auto lock = _lock.LockExclusive();

        t = _videoSampleRequestQueue.front();
        _videoSampleRequestQueue.pop();
    }

    ComPtr<IMFMediaBuffer> buffer;
    CHK(sample->Sample->ConvertToContiguousBuffer(&buffer));

    // Dispatch without the lock taken to avoid deadlocks
    t.set(As<IMF2DBuffer2>(buffer));
}
