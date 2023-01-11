/*
 *  videoWindowsMediaFoundation.cpp
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "videoWindowsMediaFoundation.h"

#ifdef ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION

#if (_WIN32_WINNT < 0x0601) // _WIN32_WINNT_WIN7
#  error error artoolkitX Windows Media Foundation requires Windows 7 or Windows Server 2008 R2 or later. Please compile with Windows Windows 7 or Windows Server 2008 R2 SDK installed and with _WIN32_WINNT=0x0601 or later in your project compiler settings (setting /D_WIN32_WINNT=0x0601).
#endif

#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"strmiids.lib")

#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#include "BufferLock.h"

extern HRESULT LogMediaType(IMFMediaType *pType);

// Private API.
static bool ar2VideoWinMFInit2(void);
static bool ar2VideoWinMFFinal2(void);
static bool ar2VideoWinMFNeedToUninitCOMOnFinal = false;
static void ar2VideoWinMFGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec);
static AR_PIXEL_FORMAT getARPixelFormatForMFMediaType(IMFMediaType *pType);
static bool getMFVideoMediaSubTypeGUIDForARPixelFormat(AR_PIXEL_FORMAT pf, GUID *pguid);
static bool getMediaTypeInfo(IMFMediaType *pType, AR_PIXEL_FORMAT *pf, int *width, int *height, int *rowBytes);
static bool getCurrentMediaTypeInfo(IMFSourceReader *pSourceReader, AR_PIXEL_FORMAT *pf, int *width, int *height, int *rowBytes);
static BOOL Is_Win7_or_WinServer2008R2_or_Later(void);
static char *utf16ws_to_ansis(const wchar_t *wstr);

static int ar2VideoWinMFRefCount = 0;

struct _AR2VideoParamWinMFT {
    int devNum;
    WCHAR *friendlyName;
    AR_PIXEL_FORMAT format;
    int width;
    int height;
    int rowBytes;
    IMFMediaSource *pSource;
    IMFSourceReader *pSourceReader;
    long bufSize;
    AR2VideoBufferT buffer;
	int flipV;
	int flipH;
};

//
// Public functions
//

int ar2VideoDispOptionWinMF(void)
{
    ARPRINT(" -module=WinMF\n");
	ARPRINT(" -devNum=N\n");
    ARPRINT("    Use device number N (default N=1).\n");
    ARPRINT("    e.g. for a second camera input, N=2.\n");
	ARPRINT(" -format=X\n");
    ARPRINT("    Return images with pixels in format X, where X is one of: \n");
    ARPRINT("    BGRA, BGR, NV12/420f, 2vuy/UYVY, yuvs/YUY2, RGB_565, RGBA_5551.\n");
    ARPRINT(" -width=N\n");
    ARPRINT("    Request video format of width N pixels.\n");
    ARPRINT(" -height=N\n");
    ARPRINT("    Request video format of height N pixels.\n");
	ARPRINT(" -flipV\n");
    ARPRINT("    Flip the image vertically.\n");
	ARPRINT(" -noFlipV\n");
    ARPRINT("    Do not flip the image vertically.\n");
	//ARPRINT(" -flipH\n");
    //ARPRINT("    Flip the image horizontally.\n");
	//ARPRINT(" -noFlipH\n");
    //ARPRINT("    Do not flip the image horizontally.\n");
    ARPRINT(" -showDeviceList\n");
    ARPRINT(" -noShowDeviceList\n");
    ARPRINT(" -showFormats\n");
    ARPRINT("    Dump the full list of native formats to the console.\n");
    ARPRINT("    This is helpful in determining device capabilities.\n");
    ARPRINT(" -noShowFormats\n");
    ARPRINT("\n");

    return 0;
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListWinMF(const char *config)
{
#ifdef _WINRT
	ARLOGe("Error: Media Foundation does not support capture from hardware devices on WinRT-based platforms, so we can't either. Sorry about this.\n");
	return NULL;
#else
    int i;
	HRESULT					 hr;
    ARVideoSourceInfoListT *list = NULL;
    
    if (ar2VideoWinMFRefCount == 0) {
        if (!ar2VideoWinMFInit2()) return NULL;
    }
    ar2VideoWinMFRefCount++;
    
    // Enumerate devices.
    
    // Create an attribute store to hold the search criteria, and add a request for video capture capture devices.
    IMFAttributes *pAttributes = NULL;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        ARLOGe("Error: MFCreateAttributes.\n");
        goto bail;
    }
    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        ARLOGe("Error: SetGUID.\n");
        goto bail;
    }
    
    // Get info.
    UINT32 count = 0;
    IMFActivate **ppDevices = NULL;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        ARLOGe("Error: MFEnumDeviceSources.\n");
        goto bail;
    } else if (count <= 0) {
        ARLOGe("Error: no video devices connected.\n");
        goto bail;
    }
    
    // Fill the list.
    arMallocClear(list, ARVideoSourceInfoListT, 1);
    list->count = count;
    arMallocClear(list->info, ARVideoSourceInfoT, count);
    for (i = 0; i < count; i++) {
        WCHAR *szFriendlyName = NULL;
        UINT32 cchFriendlyName; // Number of characters excluding trailing 0.
         if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchFriendlyName))) {
            //ARLOGd("%2d: \"%S\"\n", i + 1, szFriendlyName);
            list->info[i].name = utf16ws_to_ansis(szFriendlyName);
        }
        CoTaskMemFree(szFriendlyName);
        WCHAR *szSymbolicLink = NULL;
        UINT32 cchSymbolicLink; // Number of characters excluding trailing 0.
        if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szSymbolicLink, &cchSymbolicLink))) {
            //ARLOGd("%2d: \"%S\"\n", i + 1, szSymbolicLink);
            list->info[i].UID = utf16ws_to_ansis(szSymbolicLink);
        }
        CoTaskMemFree(szSymbolicLink);
    }
    
    // Done with enumeration-related stuff.
    for (DWORD i = 0; i < count; i++) ppDevices[i]->Release();
    CoTaskMemFree(ppDevices);

bail:
    SafeRelease(&pAttributes);
    ar2VideoWinMFRefCount--;
    if (ar2VideoWinMFRefCount == 0) ar2VideoWinMFFinal2();
    
    return (list);
#endif
}

AR2VideoParamWinMFT *ar2VideoOpenWinMF(const char *config)
{
    AR2VideoParamWinMFT     *vid;
	HRESULT					 hr;
	int						 devNum = 0;
	int						 showList = -1;
	int						 showDialog = -1;
	int                      showFormats = -1;
	const char				*a;
    char                     b[256];
	const char config_default[] = "";
	int                      err_i = 0;

	//ARLOGd("Entering ar2VideoOpenWinMF\n");
	if (ar2VideoWinMFRefCount == 0) {
		if (!ar2VideoWinMFInit2()) return NULL;
	}
    ar2VideoWinMFRefCount++;

    arMallocClear(vid, AR2VideoParamWinMFT, 1);
    vid->format = AR_PIXEL_FORMAT_INVALID;
	vid->width = -1;
	vid->height = -1;

	// Ensure the provided config is valid, otherwise use default config.
	if (!config) a = config_default;
	else if (!config[0]) a = config_default;
    else a = config;
    if (a != NULL) {
        for (;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;
    
            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-devNum=", 8) == 0) {
                if (sscanf(&b[8], "%d", &devNum) != 1) err_i = 1;
                else if (devNum < 0) {
					ARLOGe("Error: device number must be integer beginning with 1, or 0 to use default device.\n");
                    err_i = 1;
				}
            } else if( strncmp( b, "-format=", 8 ) == 0 ) {
                if (strcmp(b+8, "0") == 0) {
                    vid->format = AR_PIXEL_FORMAT_INVALID;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(b+8, "BGRA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGRA;
                    ARLOGi("Requesting images in BGRA format.\n");
                } else if (strcmp(b+8, "BGR") == 0) {
                    vid->format = AR_PIXEL_FORMAT_BGR;
                    ARLOGi("Requesting images in BGR format.\n");
                } else if (strcmp(b+8, "RGB_565") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGB_565;
                    ARLOGi("Requesting images in RGB_565 format.\n");
                } else if (strcmp(b+8, "RGBA_5551") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA_5551;
                    ARLOGi("Requesting images in RGB_5551 format.\n");
                } else if (strcmp(b+8, "2vuy") == 0 || strcmp(b+8, "UYVY") == 0) {
                    vid->format = AR_PIXEL_FORMAT_2vuy;
                    ARLOGi("Requesting images in 2vuy/UYVY format.\n");
                } else if (strcmp(b+8, "yuvs") == 0 || strcmp(b+8, "YUY2") == 0) {
                    vid->format = AR_PIXEL_FORMAT_yuvs;
                    ARLOGi("Requesting images in yuvs/YUY2 format.\n");
                } else if (strcmp(b+8, "NV21") == 0) {
                    vid->format = AR_PIXEL_FORMAT_NV21;
                    ARLOGi("Requesting images in NV21 format.\n");
                } else if (strcmp(b+8, "420f") == 0 || strcmp(b+8, "NV12") == 0) {
                    vid->format = AR_PIXEL_FORMAT_420f;
                    ARLOGi("Requesting images in 420f/NV12 format.\n");
                } else {
                    ARLOGe("Ignoring request for unsupported video format '%s'.\n", b+8);
                }
			} else if( strncmp( b, "-width=", 7 ) == 0 ) {
                if( sscanf( &b[7], "%d", &vid->width ) == 0 ) err_i = 1;
            } else if( strncmp( b, "-height=", 8 ) == 0 ) {
                if( sscanf( &b[8], "%d", &vid->height ) == 0 ) err_i = 1;
            } else if (strcmp(b, "-showDeviceList") == 0) {
				showList = 1;
            } else if (strcmp(b, "-noShowDeviceList") == 0) {
				showList = 0;
            } else if (strcmp(b, "-showFormats") == 0) {
				showFormats = 1;
            } else if (strcmp(b, "-noShowFormats") == 0) {
				showFormats = 0;
            } else if (strcmp(b, "-showDialog") == 0)    {
				showDialog = 1;
            } else if (strcmp(b, "-noShowDialog") == 0)    {
				showDialog = 0;
            } else if (strcmp(b, "-flipH") == 0)    {
				vid->flipH = 1;
            } else if (strcmp(b, "-noFlipH") == 0)    {
				vid->flipH = 0;
            } else if (strcmp(b, "-flipV") == 0)    {
				vid->flipV = 1;
            } else if (strcmp(b, "-noFlipV") == 0)    {
				vid->flipV = 0;
            } else if (strcmp(b, "-module=WinMF") == 0) {
				//ARLOGd("Device set to WinMF\n");
			} else {
				ARLOGe("Unrecognized config token: '%s'\n", b);
                ar2VideoDispOptionWinMF();
                return 0;
            }
            
            if (err_i) goto bail;

            while(*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

	// Although Windows Runtime (Windows Phone 8.1 App / Windows 8.0/8.1 Store App) supports the MFSourceReader
	// interface, it does not expose devices as IMFMediaSource objects, and does not support the API for
	// enumerating device sources. So, it would appear that only URL and byte-stream (i.e. file) sources
	// are supported. So basically, Media Foundation can't be used for capture on WinRT-based platforms.
	// At present, we'll just bail out here. The remaining code can stick around until such time as we support
	// URL and byte-stream sources.
#ifdef _WINRT
	ARLOGe("Error: Media Foundation does not support capture from hardware devices on WinRT-based platforms, so we can't either. Sorry about this.\n");
	goto bail;
#else
	// Enumerate devices.
    
    // Create an attribute store to hold the search criteria, and add a request for video capture capture devices.
    IMFAttributes *pAttributes = NULL;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        ARLOGe("Error: MFCreateAttributes.\n");
        goto bail;
    }
    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        ARLOGe("Error: SetGUID.\n");
        goto bail1;
    }
    
    // Get the list.
    UINT32 count = 0;
    IMFActivate **ppDevices = NULL;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        ARLOGe("Error: MFEnumDeviceSources.\n");
        goto bail1;
    } else if (count <= 0) {
        ARLOGe("Error: no video devices connected.\n");
        goto bail1;
    }
    
    // Report the names if requested.
    WCHAR *szFriendlyName = NULL;
    UINT32 cchFriendlyName; // Number of characters excluding trailing 0.
   	if (showList == 1) {
		for (int i = 0; i < count; i++) {
            if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchFriendlyName))) {
                ARPRINT("%2d: \"%S\"\n", i + 1, szFriendlyName);
            }
            CoTaskMemFree(szFriendlyName);
		}
        goto bail2;
	}
    
    // Device selection. Just based on index at the moment.
    if (devNum > count) {
        ARLOGe("Error: not enough video devices available to satisfy request for device at index %d. Using default device.\n", devNum);
        devNum = 0;
    }
	vid->devNum = (devNum > 0 ? devNum - 1 : 0);
    
    // Get the friendly name for chosen device.
    if (FAILED(ppDevices[vid->devNum]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchFriendlyName))) {
        ARLOGw("Unable to retrieve name of device at index %d.\n", vid->devNum + 1);
        vid->friendlyName = NULL;
    } else {
        vid->friendlyName = _wcsdup(szFriendlyName);
    }
    CoTaskMemFree(szFriendlyName);
 
    // Create a media source for the requested device and save it.
    hr = ppDevices[vid->devNum]->ActivateObject(IID_PPV_ARGS(&(vid->pSource)));
    if (FAILED(hr)) {
        ARLOGe("Error: Unable to create media source.\n");
        goto bail2;
    }
    
    // Done with enumeration-related stuff.
    for (DWORD i = 0; i < count; i++) ppDevices[i]->Release();
    CoTaskMemFree(ppDevices);
    SafeRelease(&pAttributes);

#endif // !_WINRT

    // Create the source reader.
    IMFAttributes *pConfigReader = NULL;
    hr = MFCreateAttributes(&pConfigReader, 2);
    if (FAILED(hr)) {
        ARLOGe("Error: MFCreateAttributes.\n");
		goto bail3;
    }
    hr = pConfigReader->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, (UINT32)1);
    if (FAILED(hr)) {
        ARLOGe("Error: SetUINT32.\n");
        goto bail3;
    }
    hr = pConfigReader->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, (UINT32)1);
    if (FAILED(hr)) {
        ARLOGe("Error: SetUINT32.\n");
        goto bail3;
    }
#ifdef WINDOWS8
    hr = pConfigReader->SetUINT32(MF_LOW_LATENCY, (UINT32)1);
    if (FAILED(hr)) {
        ARLOGe("Error: SetUINT32.\n");
        goto bail3;
    }
#endif
    // For asynchronous reading.
    //hr = pConfigReader->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, (IUnknown*)XXX );
    //if (FAILED(hr)) {
    //    ARLOGe("Error: SetUnknown.\n");
    //    goto bail3;
    //}
    
    hr = MFCreateSourceReaderFromMediaSource(vid->pSource, pConfigReader, &vid->pSourceReader);
    SafeRelease(&pConfigReader);
    if (FAILED(hr)) {
		if (hr == MF_E_DRM_UNSUPPORTED) ARLOGe("Error: The source contains protected content.\n");
		else ARLOGe("Error: MFCreateSourceReaderFromMediaSource (%#x).\n", hr); // See WinError.h. E_INVALIDARG=0x80070057.
		goto bail;
    }
	 
	// If the user asked us for all format info, dump that now.
	if (showFormats == 1) {
		IMFMediaType *pType = NULL;
	    DWORD dwMediaTypeIndex;
        for (dwMediaTypeIndex = 0; ; dwMediaTypeIndex++) {
            hr = vid->pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex, &pType);
            if (hr == MF_E_NO_MORE_TYPES) { // End of enumeration.
                hr = S_OK;
                break;
            } else if (FAILED(hr)) {
                ARLOGe("Error: IMFSourceReader::GetNativeMediaType.\n");
                break;
            }
			ARLOGi("Format %d:\n", dwMediaTypeIndex);
			LogMediaType(pType);
            // Release this type and move onto next index.
            SafeRelease(&pType);
        }
        if (FAILED(hr)) {
            goto bail3;
        }
	}

    // Now configure media source reader.
   
    // Find out the default format.
    AR_PIXEL_FORMAT currentPF;
	int currentWidth, currentHeight, currentRowBytes;
    if (!getCurrentMediaTypeInfo(vid->pSourceReader, &currentPF, &currentWidth, &currentHeight, &currentRowBytes)) {
        ARLOGe("Error: IMFSourceReader::GetCurrentMediaType.\n");
        goto bail3;
    }

	// Check whether current settings are OK.
	bool changeFormat = false;
	if (currentWidth <= 0 || currentHeight <= 0 || currentPF == AR_PIXEL_FORMAT_INVALID) {
		// Current settings are not handleable by artoolkitX.
		changeFormat = true;
	} else {
		if (((vid->width == -1 && vid->height == -1) || (vid->width == currentWidth && vid->height == currentHeight)) && (vid->format == AR_PIXEL_FORMAT_INVALID || vid->format == currentPF)) {
			// Current settings are what was requested.
			vid->format = currentPF;
			vid->width = currentWidth;
			vid->height = currentHeight;
			vid->rowBytes = currentRowBytes;
		} else {
			// Current settings are handleable by artoolkitX, but aren't what was requested.
			changeFormat = true;
		}
	}

	if (changeFormat) {

		// Now look through native formats for a matching pixel formats and sizes.
		IMFMediaType *pType = NULL;
		DWORD dwPFMatchIndex = -1;
		DWORD dwSizeMatchIndex = -1;
		DWORD dwPFAndSizeMatchIndex = -1;
        DWORD dwMediaTypeIndex;
        for (dwMediaTypeIndex = 0; ; dwMediaTypeIndex++) {
            hr = vid->pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex, &pType);
            if (hr == MF_E_NO_MORE_TYPES) { // End of enumeration.
                break;
            } else if (FAILED(hr)) {
                ARLOGe("Error: IMFSourceReader::GetNativeMediaType.\n");
                goto bail3;
            }
			if (!getMediaTypeInfo(pType, &currentPF, &currentWidth, &currentHeight, NULL)) {
                ARLOGe("Error: getMediaTypeInfo.\n");
				SafeRelease(&pType);
                goto bail3;
			}

			bool pfMatch, sizeMatch = false;
            if (currentPF != AR_PIXEL_FORMAT_INVALID && (vid->format == AR_PIXEL_FORMAT_INVALID || vid->format == currentPF)) pfMatch = true;
			else pfMatch = false;
			if (currentWidth >= 0 && currentHeight >= 0 && (vid->width == -1 || vid->width == currentWidth) && (vid->height == -1 || vid->height == currentHeight)) sizeMatch = true;
			else sizeMatch = false;

			if (pfMatch && dwPFMatchIndex == -1) dwPFMatchIndex = dwMediaTypeIndex;
			if (sizeMatch && dwSizeMatchIndex == -1) dwSizeMatchIndex = dwMediaTypeIndex;
			if (pfMatch && sizeMatch && dwPFAndSizeMatchIndex == -1) dwPFAndSizeMatchIndex = dwMediaTypeIndex;

			// Release this type and move onto next index.
            SafeRelease(&pType);
        }

		// Logic to decide how to change the media type.
		GUID subType = GUID_NULL;
		bool setWithNativeFormat = false;
		bool setWithNewType = false;
		if (dwPFAndSizeMatchIndex != -1) {
			// Found a native format with acceptable pixel format and size. Use it.
			dwMediaTypeIndex = dwPFAndSizeMatchIndex;
			setWithNativeFormat = true;
		} else {
			if (dwPFMatchIndex != -1) {
				// A native format with acceptable pixel format was found, but none with acceptable size.
				// We have to ignore the size request, since Media Foundation doesn't do scaling.
				dwMediaTypeIndex = dwPFMatchIndex;
				setWithNativeFormat = true;
			} else {
				// A native format with acceptable size was found, but none with acceptable pixel format OR
				// native formats included neither acceptable sizes nor acceptable pixel formats. In the latter case,
				// we have to ignore the size request, since Media Foundation doesn't do scaling. 
				if (dwSizeMatchIndex != -1) {
					dwMediaTypeIndex = dwSizeMatchIndex;
					setWithNativeFormat = true;
				}
				setWithNewType = true;
				if (vid->format == AR_PIXEL_FORMAT_INVALID) {
					subType = MFVideoFormat_RGB32;
				} else {
					if (!getMFVideoMediaSubTypeGUIDForARPixelFormat(vid->format, &subType)) {
						ARLOGe("Error: pixel format %s not supported by this device.\n", arVideoUtilGetPixelFormatName(vid->format));
						goto bail3;
					}
				}
			}
		}

		// Use one of the native formats.
		if (setWithNativeFormat) {
			// Get the native format once again.
			hr = vid->pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex, &pType);
            if (FAILED(hr)) {
                ARLOGe("Error: IMFSourceReader::GetNativeMediaType.\n");
                goto bail3;
            }

			// Do the change.
			hr = vid->pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
			SafeRelease(&pType);
			if (FAILED(hr)) {
				if (hr == MF_E_INVALIDMEDIATYPE) ARLOGe("Error setting media type: At least one decoder was found for the native stream type, but the requested type specified was rejected.\n");
				else if (hr == MF_E_TOPO_CODEC_NOT_FOUND) ARLOGe("Error setting media type: Could not find a decoder for the native stream type.\n");
				else ARLOGe("Error: SetCurrentMediaType (%d).\n", hr);
				goto bail3;
			}

			// Read back the new format info.
			if (!getCurrentMediaTypeInfo(vid->pSourceReader, &vid->format, &vid->width, &vid->height, &vid->rowBytes)) {
				ARLOGe("Error: IMFSourceReader::GetCurrentMediaType.\n");
				goto bail3;
			}
		} // setWithNativeFormat

		// Set the type to a type not in the native list.
		if (setWithNewType) {
			// Build new format with just the media type and subtype.
            hr = MFCreateMediaType(&pType);
            if (FAILED(hr)) {
                ARLOGe("Error: MFCreateMediaType.\n");
				goto bail3;
            }
			hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            if (FAILED(hr)) {
                ARLOGe("Error: IMFMediaType::SetGUID.\n");
                SafeRelease(&pType);
				goto bail3;
            }
            hr = pType->SetGUID(MF_MT_SUBTYPE, subType);
            if (FAILED(hr)) {
                ARLOGe("Error: IMFMediaType::SetGUID.\n");
                SafeRelease(&pType);
				goto bail3;
            }

			// Do the change.
			hr = vid->pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
			SafeRelease(&pType);
			if (FAILED(hr)) {
				if (hr == MF_E_INVALIDMEDIATYPE) ARLOGe("Error setting media type: At least one decoder was found for the native stream type, but the requested type specified was rejected.\n");
				else if (hr == MF_E_TOPO_CODEC_NOT_FOUND) ARLOGe("Error setting media type: Could not find a decoder for the native stream type.\n");
				else ARLOGe("Error: SetCurrentMediaType (%d).\n", hr);
				goto bail3;
			}

			// Read back the new format info.
			if (!getCurrentMediaTypeInfo(vid->pSourceReader, &vid->format, &vid->width, &vid->height, &vid->rowBytes)) {
				ARLOGe("Error: IMFSourceReader::GetCurrentMediaType.\n");
				goto bail3;
			}

        } // setType

    }
    
    ARLOGi("Video %dx%d (%s), rowBytes=%d.\n", vid->width, vid->height, arVideoUtilGetPixelFormatName(vid->format), vid->rowBytes);
    
    vid->bufSize = vid->width * vid->height * arVideoUtilGetPixelSize(vid->format);
    arMalloc(vid->buffer.buff, ARUint8, vid->bufSize);

	return (AR2VideoParamWinMFT *)vid;

bail3:
	SafeRelease(&vid->pSourceReader);
	goto bail;
#ifndef _WINRT
bail2:
    for (DWORD i = 0; i < count; i++) SafeRelease(&ppDevices[i]);
    CoTaskMemFree(ppDevices);
bail1:
    SafeRelease(&pAttributes);
#endif // !_WINRT
bail:
    free(vid->friendlyName);
    free(vid);
    ar2VideoWinMFRefCount--;
    if (ar2VideoWinMFRefCount == 0) ar2VideoWinMFFinal2();
    
    return NULL;
}

int ar2VideoCloseWinMF(AR2VideoParamWinMFT *vid)
{
    free(vid->buffer.buff);
	vid->buffer.buff = vid->buffer.buffLuma = NULL;
	SafeRelease(&vid->pSourceReader);
    vid->pSource->Shutdown();
    vid->pSource->Release();
	vid->pSource = NULL;
    free(vid->friendlyName);
    vid->friendlyName = NULL;
	free(vid);

    ar2VideoWinMFRefCount--;
    if (ar2VideoWinMFRefCount == 0) ar2VideoWinMFFinal2();
    
    return 0;
} 

int ar2VideoCapStartWinMF(AR2VideoParamWinMFT *vid)
{
    return 0;
}

int ar2VideoCapStopWinMF(AR2VideoParamWinMFT *vid)
{
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWinMF(AR2VideoParamWinMFT *vid)
{
    HRESULT hr;
    DWORD streamIndex, flags;
    LONGLONG llTimeStamp; // 100-nanosecond units.
    IMFSample *pSample;
    
    if (!vid) return NULL;
    if (vid->format == AR_PIXEL_FORMAT_INVALID || vid->width <= 0 || vid->height <= 0) return NULL;
    
    hr = vid->pSourceReader->ReadSample(
                             MF_SOURCE_READER_FIRST_VIDEO_STREAM, // Stream index.
                             0,                              // Flags.
                             &streamIndex,                   // Receives the actual stream index.
                             &flags,                         // Receives status flags.
                             &llTimeStamp,                   // Receives the time stamp.
                             &pSample                        // Receives the sample or NULL.
                             );
    if (FAILED(hr)) {
        ARLOGe("Error: IMFSourceReader::ReadSample.\n");
        return NULL;
    }
   
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM || pSample == NULL) {
         SafeRelease(&pSample);
		 return (NULL);
    }
    
    IMFMediaBuffer *pBuffer = NULL;
    hr = pSample->GetBufferByIndex(0, &pBuffer);
    if (FAILED(hr)) {
        ARLOGe("Error: IMFSample::GetBufferByIndex.\n");
        SafeRelease(&pSample);
        return (NULL);
    }
    
    BYTE *pbScanline0 = NULL;
    LONG lStride = 0;
    // Lock the video buffer. This method returns a pointer to the first scan
    // line in the image, and the stride in bytes.
    VideoBufferLock buffer(pBuffer);
    hr = buffer.LockBuffer(vid->rowBytes, vid->height, &pbScanline0, &lStride);
    if (FAILED(hr)) {
        ARLOGe("Error: VideoBufferLock::LockBuffer.\n");
        SafeRelease(&pSample);
        return (NULL);
    }
    
    if (lStride != vid->rowBytes) {
        ARLOGw("Warning: Incoming frame rowBytes (%ld) differs from expected (%d).\n", lStride, vid->rowBytes);
    } else {
		// the image buffer data is flipped when stride < 0,
		// which means the first pbScanline0 is actually the top line, but in memory
		// the first pbScanline0 points to the last line
		if (lStride > 0) {
			if (!vid->flipV) {
				memcpy(vid->buffer.buff, pbScanline0, vid->bufSize);
			}
			else {
				unsigned char *p0 = pbScanline0;
				unsigned char *p1 = vid->buffer.buff + (vid->height - 1)*vid->rowBytes;
				for (int i = 0; i < vid->height; i++) {
					memcpy(p1, p0, vid->rowBytes);
					p0 += vid->rowBytes;
					p1 -= vid->rowBytes;
				}
			}
		}
		else {
			if (vid->flipV) {
				memcpy(vid->buffer.buff, pbScanline0 + vid->rowBytes * (vid->height - 1), vid->bufSize);
			}
			else {
				BYTE *p0 = pbScanline0 + (vid->height - 1) * vid->rowBytes;
				BYTE *p1 = vid->buffer.buff - (vid->height - 1) * vid->rowBytes;
				for (int i = 0; i<vid->height; i++) {
					memcpy(p1, p0, -vid->rowBytes);
					p0 -= vid->rowBytes;
					p1 += vid->rowBytes;
				}
			}
		}
    }
    
    buffer.UnlockBuffer();
    SafeRelease(&pBuffer);
    
    LONGLONG sec = llTimeStamp / 10000000ll;
    vid->buffer.time.sec = (ARUint32)sec;
    vid->buffer.time.usec = (ARUint32)(llTimeStamp / 10ll - sec * 1000000ll);
    vid->buffer.fillFlag = 1;
    vid->buffer.buffLuma = NULL;
    
    SafeRelease(&pSample);
    return (&vid->buffer);
}

int ar2VideoGetSizeWinMF(AR2VideoParamWinMFT *vid, int *x,int *y)
{
	if (!vid) return -1;
	if (x) *x = vid->width;
	if (y) *y = vid->height;
	return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWinMF(AR2VideoParamWinMFT *vid)
{
	if (!vid) return AR_PIXEL_FORMAT_INVALID;

	return vid->format;
}

int ar2VideoGetIdWinMF(AR2VideoParamWinMFT *vid, ARUint32 *id0, ARUint32 *id1)
{
	if (!vid) return -1;
    return -1;
}

int ar2VideoGetParamiWinMF(AR2VideoParamWinMFT *vid, const int paramName, int *value)
{
    return -1;
}

int ar2VideoSetParamiWinMF(AR2VideoParamWinMFT *vid, const int paramName, const int  value)
{
    return -1;
}

int ar2VideoGetParamdWinMF(AR2VideoParamWinMFT *vid, const int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdWinMF(AR2VideoParamWinMFT *vid, const int paramName, const double  value)
{
    return -1;
}

int ar2VideoGetParamsWinMF(AR2VideoParamWinMFT *vid, const int paramName, char **value)
{
    return -1;
}

int ar2VideoSetParamsWinMF(AR2VideoParamWinMFT *vid, const int paramName, const char *value)
{
    return -1;
}

//
// Private functions.
//

#if 0
class SourceReaderCB : public IMFSourceReaderCallback
{
public:
    SourceReaderCB(HANDLE hEvent) :
    m_nRefCount(1), m_hEvent(hEvent), m_bEOS(FALSE), m_hrStatus(S_OK)
    {
        InitializeCriticalSection(&m_critsec);
    }
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(SourceReaderCB, IMFSourceReaderCallback),
            { 0 },
        };
        return QISearch(this, qit, iid, ppv);
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_nRefCount);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG uCount = InterlockedDecrement(&m_nRefCount);
        if (uCount == 0) {
            delete this;
        }
        return uCount;
    }
    
    // IMFSourceReaderCallback methods
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                              DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample);
    
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *)
    {
        return S_OK;
    }
    
    STDMETHODIMP OnFlush(DWORD)
    {
        return S_OK;
    }
    
public:
    HRESULT Wait(DWORD dwMilliseconds, BOOL *pbEOS)
    {
        *pbEOS = FALSE;
        
        DWORD dwResult = WaitForSingleObject(m_hEvent, dwMilliseconds);
        if (dwResult == WAIT_TIMEOUT) {
            return E_PENDING;
        } else if (dwResult != WAIT_OBJECT_0) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        
        *pbEOS = m_bEOS;
        return m_hrStatus;
    }
    
private:
    
    // Destructor is private. Caller should call Release.
    virtual ~SourceReaderCB()
    {
    }
    
    void NotifyError(HRESULT hr)
    {
        wprintf(L"Source Reader error: 0x%X\n", hr);
    }
    
private:
    long                m_nRefCount;        // Reference count.
    CRITICAL_SECTION    m_critsec;
    HANDLE              m_hEvent;
    BOOL                m_bEOS;
    HRESULT             m_hrStatus;
    
};

// Callback implementation.
// In this minimal example, the OnReadSample method just prints the time stamp to the console window. Then it stores the status code and the end-of-stream flag, and signals the event handle.
HRESULT SourceReaderCB::OnReadSample(
                                     HRESULT hrStatus,
                                     DWORD /* dwStreamIndex */,
                                     DWORD dwStreamFlags,
                                     LONGLONG llTimestamp,
                                     IMFSample *pSample      // Can be NULL
)
{
    EnterCriticalSection(&m_critsec);
    
    if (SUCCEEDED(hrStatus)) {
        if (pSample) {
            // Do something with the sample.
            wprintf(L"Frame @ %I64d\n", llTimestamp);
        }
    } else {
        // Streaming error.
        NotifyError(hrStatus);
    }
    
    if (MF_SOURCE_READERF_ENDOFSTREAM & dwStreamFlags) {
        // Reached the end of the stream.
        m_bEOS = TRUE;
    }
    m_hrStatus = hrStatus;
    
    LeaveCriticalSection(&m_critsec);
    SetEvent(m_hEvent);
    return S_OK;
}
#endif

// One-time initialisation.
static bool	ar2VideoWinMFInit2(void)
{
	HRESULT hr;
    
	//ARLOGd("Entering ar2VideoWinMFInit2\n");
    if (!Is_Win7_or_WinServer2008R2_or_Later()) {
        ARLOGe("Error: Media Foundation video module supported only on Windows 7 or Windows Server 2008 R2 or later.\n");
        return false;
    }
    
    // Initialize the COM runtime.
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
		if (hr != 0x80010106) { // RPC_E_CHANGED_MODE
			ARLOGe("Error: Unable to initialize COM runtime (%#x).\n", hr);
			return false;
		}
    } else {
		ar2VideoWinMFNeedToUninitCOMOnFinal = true;
	}
    
    // Initialize the Media Foundation platform.
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        ARLOGe("Unable to initialize Medoa Foundation platform.\n");
        CoUninitialize();
        return false;
    }
    
    return true;
}

// One-time finalisation.
static bool ar2VideoWinMFFinal2(void)
{
    // Shut down Media Foundation.
    MFShutdown();
    
    // Shut down COM runtime.
	if (ar2VideoWinMFNeedToUninitCOMOnFinal) {
		CoUninitialize();
		ar2VideoWinMFNeedToUninitCOMOnFinal = false;
	}
    
	return true;
}


static void ar2VideoWinMFGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec)
{
    struct _timeb sys_time;   

    _ftime(&sys_time);   
    *t_sec  = (ARUint32)sys_time.time;
    *t_usec = (ARUint32)sys_time.millitm * 1000;

    return;
}

static AR_PIXEL_FORMAT getARPixelFormatForMFMediaType(IMFMediaType *pType)
{
    AR_PIXEL_FORMAT pf = AR_PIXEL_FORMAT_INVALID;
    
    GUID majorType = {0};
    GUID subType = {0};
    HRESULT hr = pType->GetMajorType(&majorType);
    if (FAILED(hr)) {
        ARLOGe("Error: IMFMediaType->GetMajorType.\n");
        return AR_PIXEL_FORMAT_INVALID;
    }
    if (majorType != MFMediaType_Video) {
        ARLOGe("Error: IMFMediaType->GetMajorType returned non-video type.\n");
        return AR_PIXEL_FORMAT_INVALID;
    }
    hr = pType->GetGUID(MF_MT_SUBTYPE, &subType);
    if (FAILED(hr)) {
        ARLOGe("Error: IMFMediaType->GetGUID(MF_MT_SUBTYPE).\n");
        return AR_PIXEL_FORMAT_INVALID;
    }

    if      (subType == MFVideoFormat_RGB32 ||
             subType == MFVideoFormat_ARGB32) return AR_PIXEL_FORMAT_BGRA;
    else if (subType == MFVideoFormat_RGB24) return AR_PIXEL_FORMAT_BGR;
    else if (subType == MFVideoFormat_RGB565) return AR_PIXEL_FORMAT_RGB_565;
    else if (subType == MFVideoFormat_RGB555) return AR_PIXEL_FORMAT_RGBA_5551;
    else if (subType == MFVideoFormat_UYVY) return AR_PIXEL_FORMAT_2vuy;
    else if (subType == MFVideoFormat_YUY2) return AR_PIXEL_FORMAT_yuvs;
    else if (subType == MFVideoFormat_NV12) return AR_PIXEL_FORMAT_420f;
    //else if (subType == MFVideoFormat_NV21) return AR_PIXEL_FORMAT_NV21;
    
    return AR_PIXEL_FORMAT_INVALID;
}

static bool getMediaTypeInfo(IMFMediaType *pType, AR_PIXEL_FORMAT *pf, int *width, int *height, int *rowBytes)
{
    UINT32 w = 0, h = 0;

	if (!pType) return false;

    if (pf) *pf = getARPixelFormatForMFMediaType(pType);
    
    if (width || height || rowBytes) {
        HRESULT hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &w, &h);
        if (FAILED(hr)) {
            ARLOGe("Error: MFGetAttributeSize.\n");
            return false;
        }
        if (width) *width = (int)w;
        if (height) *height = (int)h;
        if (rowBytes) {
            LONG lStride = 0;
            
            // Try to get the default stride from the media type.
            HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride);
            if (FAILED(hr)) {
                // Attribute not set. Try to calculate the default stride.
                GUID subType = GUID_NULL;
                hr = pType->GetGUID(MF_MT_SUBTYPE, &subType);
                if (FAILED(hr)) {
                    ARLOGe("Error: IMFMediaType->GetGUID(MF_MT_SUBTYPE).\n");
                    return false;
                }
#ifndef _WINRT
                hr = MFGetStrideForBitmapInfoHeader(subType.Data1, w, &lStride);
                if (FAILED(hr)) {
                    ARLOGe("Error: MFGetStrideForBitmapInfoHeader.\n");
                    return false;
                }
#else
				if      (subType == MFVideoFormat_NV12)                                        lStride = w;
				else if (subType == MFVideoFormat_YUY2 || subType == MFVideoFormat_UYVY)       lStride = ((w * 2) + 3) & ~3;
				else if (subType == MFVideoFormat_RGB32 || subType ==  MFVideoFormat_ARGB32)   lStride = w * 4;
				else if (subType == MFVideoFormat_RGB24)                                       lStride = ((w * 3) + 3) & ~3;
				else if (subType == MFVideoFormat_RGB565 || subType ==  MFVideoFormat_RGB555)  lStride = ((w * 2) + 3) & ~3;
				else {
					ARLOGe("Error: unsupported media format in manual stride calculation.\n");
					return false;
				}
#endif
                // Set the attribute for later reference.
                (void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
            }
            *rowBytes = (int)lStride;
        }
    }

	return true;
}

static bool getCurrentMediaTypeInfo(IMFSourceReader *pSourceReader, AR_PIXEL_FORMAT *pf, int *width, int *height, int *rowBytes)
{
    bool ret = true;
    DWORD dwStreamIndex = MF_SOURCE_READER_FIRST_VIDEO_STREAM; // Use first video stream provided by device.
    IMFMediaType *pType = NULL;
    
    HRESULT hr = pSourceReader->GetCurrentMediaType(dwStreamIndex, &pType);
    if (FAILED(hr)) {
        ARLOGe("Error: IMFSourceReader::GetCurrentMediaType.\n");
        return false;
    }

	ret = getMediaTypeInfo(pType, pf, width, height, rowBytes);

    SafeRelease(&pType);
    return (ret);
}

static bool getMFVideoMediaSubTypeGUIDForARPixelFormat(AR_PIXEL_FORMAT pf, GUID *pguid)
{
    // Match AR_PIXEL_FORMAT to IMFMediaType. See http://msdn.microsoft.com/en-us/library/windows/desktop/ms698724(v=vs.85).aspx
    switch (pf) {
        case AR_PIXEL_FORMAT_BGRA:
            //*pguid = MFVideoFormat_ARGB32;
            *pguid = MFVideoFormat_RGB32;
            break;
        case AR_PIXEL_FORMAT_BGR:
            *pguid = MFVideoFormat_RGB24;
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            *pguid = MFVideoFormat_RGB565;
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            *pguid = MFVideoFormat_RGB555;
            break;
        case AR_PIXEL_FORMAT_2vuy:
            *pguid = MFVideoFormat_UYVY;
            break;
        case AR_PIXEL_FORMAT_yuvs:
            *pguid = MFVideoFormat_YUY2;
            break;
        case AR_PIXEL_FORMAT_420f:
            *pguid = MFVideoFormat_NV12;
            break;
        //case AR_PIXEL_FORMAT_NV21:
        //    *pguid = MFVideoFormat_NV21;
        //    break;
        //case AR_PIXEL_FORMAT_MONO: // D3DFMT_L8
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_420v:
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_ARGB:
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_RGBA:
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_ABGR:
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_RGB:
        //    *pguid = ?;
        //    break;
        //case AR_PIXEL_FORMAT_RGB4444:
        //    *pguid = ?;
        //    break;
        default:
            return false;
    }
    return true;
}

static BOOL Is_Win7_or_WinServer2008R2_or_Later(void)
{
#ifdef _WINRT
	return true;
#else
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   int op=VER_GREATER_EQUAL;

   // Initialize the OSVERSIONINFOEX structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 6;
   osvi.dwMinorVersion = 1;
   osvi.wServicePackMajor = 0;
   osvi.wServicePackMinor = 0;

   // Initialize the condition mask.

   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, op );

   // Perform the test.

   return VerifyVersionInfo(
      &osvi, 
      VER_MAJORVERSION | VER_MINORVERSION | 
      VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
      dwlConditionMask);
#endif
}

// Convert a UTF16 wchar_t* string to a UTF8 char* string.
// User must call free() when done with result.
static char *utf16ws_to_ansis(const wchar_t *wstr)
{
    char *str;
    if (!wstr) return NULL;
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    str = (char *)malloc(size_needed);
    WideCharToMultiByte                  (CP_UTF8, 0, wstr, -1, str, size_needed, NULL, NULL);
    
    return str;
}

#endif // ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION
