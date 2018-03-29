//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Portions Copyright (c) Microsoft Open Technologies, Inc. 
//
//*@@@---@@@@******************************************************************

#pragma once

#include "MFIncludes.h"


namespace Media {

class MediaSink;

enum class CaptureStreamType
{
    Preview = 0,
    Record
};

ref class CaptureFrameGrabber sealed
{
public:

    // IClosable
    virtual ~CaptureFrameGrabber();

    void ShowCameraSettings();

internal:

    static concurrency::task<CaptureFrameGrabber^> CreateAsync(_In_ WMC::MediaCapture^ capture, _In_ WMMp::VideoEncodingProperties^ props)
    {
        return CreateAsync(capture, props, CaptureStreamType::Preview);
    }

    static concurrency::task<CaptureFrameGrabber^> CreateAsync(_In_ WMC::MediaCapture^ capture, _In_ WMMp::VideoEncodingProperties^ props, CaptureStreamType streamType);

    concurrency::task<MW::ComPtr<IMF2DBuffer2>> GetFrameAsync();
    concurrency::task<void> FinishAsync();

private:

    CaptureFrameGrabber(_In_ WMC::MediaCapture^ capture, _In_ WMMp::VideoEncodingProperties^ props, CaptureStreamType streamType);

    void ProcessSample(_In_ MediaSample^ sample);

    Platform::Agile<WMC::MediaCapture> _capture;
    ::Windows::Media::IMediaExtension^ _mediaExtension;

    MW::ComPtr<MediaSink> _mediaSink;

    CaptureStreamType _streamType;

    enum class State
    {
        Created,
        Started,
        Closing,
        Closed
    } _state;

    std::queue<concurrency::task_completion_event<MW::ComPtr<IMF2DBuffer2>>> _videoSampleRequestQueue;
    AutoMF _mf;
    MWW::SRWLock _lock;
};

}