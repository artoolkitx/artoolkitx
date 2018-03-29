/*
 *  MovieVideo.m
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#import <ARX/ARVideo/video.h>

#ifdef ARVIDEO_INPUT_AVFOUNDATION

#import "MovieVideo.h"

#define MIN3(x,y,z)  ((y) <= (z) ? ((x) <= (y) ? (x) : (y)) : ((x) <= (z) ? (x) : (z)))
#define MAX3(x,y,z)  ((y) >= (z) ? ((x) >= (y) ? (x) : (y)) : ((x) >= (z) ? (x) : (z)))

/* Asset keys */
NSString * const kTracksKey         = @"tracks";
NSString * const kPlayableKey		= @"playable"; // Can be used to initialize an AVPlayerItem. (iOS 4.3 and later.)
NSString * const kReadableKey		= @"readable"; // Can be used to initialize an AVAssetReader. (iOS 4.3 and later.)
NSString * const kStatusKey         = @"status";

NSString *const MovieVideoMediaLoadedNofication = @"MovieVideoMediaLoadedNofication";
NSString *const MovieVideoPlaybackEndedNofication = @"MovieVideoPlaybackEndedNofication";

static BOOL inited = FALSE;

//#define MOVIEVIDEO_TEST_STREAMING 1

#ifdef MOVIEVIDEO_TEST_STREAMING
static const NSString *observingContext;
#endif

@interface MovieVideo (MovieVideoPrivate)
- (void)createTransferFunctionChromaKeyAlphaTableHueMinAngle:(float)hueMinAngle hueMaxAngle:(float)hueMaxAngle;
- (void)prerollAsyncWithKeys:(NSArray *)keys;
- (void)preroll;
- (void)play;
//- (void)playerItemDidReachEnd:(NSNotification *)notification;
- (void)dumpFrame;
@end

@implementation MovieVideo
{
    BOOL _loaded;
    NSURL *_url;
    
    // Configuration.
    int _height; // Requested frame width.
    int _width; // Requested frame height.
    BOOL _paused;
    BOOL _flipV;
    BOOL _flipH;
    BOOL _loop;
    BOOL _noSound;
    BOOL _scaleFill, _scale1to1, _scaleStretch;
    OSType _pixFormat;
    
	NSInteger _contentWidth;		// The width of the video.
	NSInteger _contentHeight;		// The height of the video.
    
	// Will be used to manage a consistent frame rate.
    CFAbsoluteTime _playTime;
    CFAbsoluteTime _pausedTime;
	CGFloat _nominalFrameRate;
	NSInteger _frameCounter;
    
    AVURLAsset *_asset;
    AVAssetTrack *_videoTrack;
    AVPlayer *_audioPlayer;
    AVAssetReader *_assetReader;
    AVAssetReaderTrackOutput *_videoTrackOutput;
#ifdef MOVIEVIDEO_TEST_STREAMING
    AVPlayerItem *_playerItem;
    AVPlayer *_player;
    BOOL _playerItemIsBeingObserved;
#endif
    
    size_t _bufWidth;
    size_t _bufHeight;
    size_t _bufRowBytes;
    unsigned char *_bufDataPtr;

    BOOL _transparent;
    BOOL _transferFunctionTransparency;
    uint32_t _transferFunctionTransparencyColor;
    uint32_t _transferFunctionTransparencyColorMask;
    BOOL _transferFunctionChromaKey;
    float _transferFunctionChromaKeyHueMinAngle;
    float _transferFunctionChromaKeyHueMaxAngle;
#define TRANSFER_FUNCTION_ALPHA_TABLE_BITS 6 // 6 bits will result in an alpha table of 32Kb. (((2^6)^3)/8)
#define TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS (1 << TRANSFER_FUNCTION_ALPHA_TABLE_BITS)
    uint8_t *_transferFunctionChromaKeyAlphaTable;
}

@synthesize bufWidth = _bufWidth, bufHeight = _bufHeight, bufRowBytes = _bufRowBytes, bufDataPtr = _bufDataPtr;
@synthesize contentWidth = _contentWidth, contentHeight = _contentHeight;
@synthesize pixFormat = _pixFormat;
@synthesize transparent = _transparent;
@synthesize loaded = _loaded;

-(id) initWithURL:(NSURL *)url config:(char *)conf
{
#ifdef DEBUG
    NSLog(@"MovieVideo -initWithURL:config:\n");
#endif
    if ((self = [super init])) {
        
        _loaded = FALSE;
        _url = nil;
        
        // Configuration options.
        _width = 0;
        _height = 0;
        _paused = FALSE; // Not paused by default.
        _loop = TRUE;
        _flipH = _flipV = FALSE;
        _noSound = FALSE;
        _scaleFill = _scale1to1 = _scaleStretch = FALSE;
        _pixFormat = kCVPixelFormatType_32BGRA; // kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
        _transparent = FALSE;
        _transferFunctionTransparency = FALSE;
        _transferFunctionChromaKey = FALSE;
        _transferFunctionChromaKeyAlphaTable = NULL;
       
        // Information read from loaded media.
        _nominalFrameRate = -1;
		_frameCounter = -1;
		_contentWidth = 0;
		_contentHeight = 0;
        
        // OS playback objects.
        _asset = nil;
        _videoTrack = nil;
        _audioPlayer = nil;
        _assetReader = nil;
        _videoTrackOutput = nil;
#ifdef MOVIEVIDEO_TEST_STREAMING
        _playerItem = nil;
        _player = nil;
        _playerItemIsBeingObserved = FALSE;
#endif

        _bufDataPtr = NULL;
        _bufHeight = _bufWidth = _bufRowBytes = 0;
        
        // Process configuration options.
        char			*a, line[256];
        int				err_i = 0;
        //unsigned int	singleBuffer = 0;
        if (conf) {
            a = conf;
            err_i = 0;
            for (;;) {
                while (*a == ' ' || *a == '\t') a++; // Skip whitespace.
                if (*a == '\0') break;
                
                if (strncmp(a, "-width=", 7) == 0) {
                    sscanf(a, "%s", line);
                    if (strlen(line) <= 7 || sscanf(&line[7], "%d", &_width) == 0) err_i = 1;
                } else if (strncmp(a, "-height=", 8) == 0) {
                    sscanf(a, "%s", line);
                    if (strlen(line) <= 8 || sscanf(&line[8], "%d", &_height) == 0) err_i = 1;
                } else if (strncmp(a, "-pixelformat=", 13) == 0) {
                    sscanf(a, "%s", line);
                    if (strlen(line) <= 13) err_i = 1;
                    else {
#ifdef AR_BIG_ENDIAN
                        if (strlen(line) == 17) err_i = (sscanf(&line[13], "%4c", (char *)&_pixFormat) < 1);
#else
                        if (strlen(line) == 17) err_i = (sscanf(&line[13], "%c%c%c%c", &(((char *)&_pixFormat)[3]), &(((char *)&_pixFormat)[2]), &(((char *)&_pixFormat)[1]), &(((char *)&_pixFormat)[0])) < 1);
#endif
                        else err_i = (sscanf(&line[13], "%li", (long *)&_pixFormat) < 1); // Integer.
                    }
                } else if (strncmp(a, "-fliph", 6) == 0) {
                    _flipH = TRUE;
                } else if (strncmp(a, "-nofliph", 8) == 0) {
                    _flipH = FALSE;
                } else if (strncmp(a, "-flipv", 6) == 0) {
                    _flipV = TRUE;
                } else if (strncmp(a, "-noflipv", 8) == 0) {
                    _flipV = FALSE;
                } else if (strncmp(a, "-loop", 5) == 0) {
                    _loop = TRUE;
                } else if (strncmp(a, "-noloop", 7) == 0) {
                    _loop = FALSE;
                } else if (strncmp(a, "-fill", 5) == 0) {
                    _scaleFill = TRUE;
                } else if (strncmp(a, "-nofill", 7) == 0) {
                    _scaleFill = FALSE;
                } else if (strncmp(a, "-1to1", 4) == 0) {
                    _scale1to1 = TRUE;
                } else if (strncmp(a, "-no1to1", 6) == 0) {
                    _scale1to1 = FALSE;
                } else if (strncmp(a, "-stretch", 8) == 0) {
                    _scaleStretch = TRUE;
                } else if (strncmp(a, "-nostretch", 10) == 0) {
                    _scaleStretch = FALSE;
                } else if (strncmp(a, "-mute", 5) == 0) {
                    _noSound = TRUE;
                } else if (strncmp(a, "-nomute", 7) == 0) {
                    _noSound = FALSE;
                } else if (strncmp(a, "-singlebuffer", 13) == 0) {
                    //singleBuffer = 1;
                } else if (strncmp(a, "-nosinglebuffer", 15) == 0) {
                    //singleBuffer = 0;
                } else if (strncmp(a, "-pause", 6) == 0) {
                    _paused = TRUE;
                } else if (strncmp(a, "-nopause", 8) == 0) {
                    _paused = FALSE;
                } else if (strncmp(a, "-alpha", 6) == 0) { // Force on (for some future date when iOS video codecs support an alpha channel ?).
                    _transparent = TRUE;
                } else if (strncmp(a, "-noalpha", 8) == 0) { // Force off.
                    _transparent = FALSE;
                } else if (strncmp(a, "-transparent=", 13) == 0) {
                    uint32_t c;
                    if (sscanf(&a[13], "%x", &c) != 1) {
                        NSLog(@"Error: transparency must be expressed as an RGB hex triplet (e.g. ff0000 for red).\n");
                    } else {
                        [self setTransparencyByTransparent:TRUE RGBA:c << 8];
                    }
                } else if (strncmp(a, "-chromakey=", 11) == 0) {
                    float min;
                    float max;
                    if (sscanf(&a[11], "%f,%f", &min, &max) != 2
                        || min < 0.0f
                        || min >= 360.0f
                        || max < 0.0f
                        || max >= 360.f) {
                        NSLog(@"Error: chroma key values must be expressed as minimum and maximum hue angles [0-360), separated by a comma.\n");
                    } else {
                        [self setTransparencyByChromaKey:TRUE hueMinAngle:min hueMaxAngle:max];
                    }
                } else {
                    err_i = 1;
                }
                
                if (err_i) {
                    NSLog(@"Error with configuration option. Ignoring.\n");
                }
                
                while (*a != ' ' && *a != '\t' && *a != '\0') a++; // Skip to next whitespace.
            }
        }
        
        if (_url) [_url release];
        _url = [url copy];

        @synchronized(self) {
            if (!inited) {
                inited = TRUE;
            }
        }
    }
    return (self);
}

-(bool) start
{
#ifdef DEBUG
    NSLog(@"MovieVideo -start\n");
#endif
    /*
     Create an asset for inspection of a resource referenced by a given URL.
     Load the values for the asset keys "tracks", "playable".
     */
    _asset = [[AVURLAsset URLAssetWithURL:_url options:nil] retain];
    
    // We need to know about 'playable' because we're using it with an AVPlayerItem.
    // We need to know about 'readable' because we're using it with an AVAssetReader.
    NSArray *keys = [NSArray arrayWithObjects:kTracksKey, kPlayableKey, kReadableKey, nil];
    
    /* Tells the asset to load the values of any of the specified keys that are not already loaded. */
    [_asset loadValuesAsynchronouslyForKeys:keys completionHandler:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            /* IMPORTANT: Must dispatch to main queue in order to operate on the AVPlayer and AVPlayerItem. */
            [self prerollAsyncWithKeys:keys];
        });
     }];

    return (TRUE);
}

/*
 Invoked at the completion of the loading of the values for all keys on the asset that we require.
 Checks whether loading was successfull= and whether the asset is playable.
 If so, sets up an AVPlayerItem and an AVPlayer to play the asset.
 */
- (void)prerollAsyncWithKeys:(NSArray *)keys
{
#ifdef DEBUG
    NSLog(@"MovieVideo -prerollAsyncWithKeys:\n");
#endif
    /* Make sure that the value of each key has loaded successfully. */
	for (NSString *key in keys) {
		NSError *error = nil;
		AVKeyValueStatus keyStatus = [_asset statusOfValueForKey:key error:&error];
		if (keyStatus == AVKeyValueStatusFailed) {
			NSLog(@"Error reading status of key '%@': %@.\n", key, [error localizedDescription]);
			return;
		} else if (keyStatus == AVKeyValueStatusUnknown) {
            NSLog(@"Warning: unknown status of key '%@'.\n", key);
        }
		/* If you are also implementing -[AVAsset cancelLoading], add your code here to bail out properly in the case of cancellation. */
	}
    
    /* Use the AVAsset playable property to detect whether the asset can be played. */
    if ([_asset respondsToSelector:@selector(isPlayable)]) { // iOS v4.3 and later.
        if (!_asset.playable) {
            NSLog(@"Error: asset's tracks were loaded, but could not be made playable.\n");
            return;
        }
    }

    // Streaming assets have [_asset tracks].count == 0.
#ifdef DEBUG
    NSArray *tracks = [_asset tracks]; // array of (AVAssetTrack *).
    NSLog(@"Media has %lu tracks:\n", (unsigned long)tracks.count);
    for (AVAssetTrack *track in tracks) {
        NSLog(@" - trackID=%d, mediaType=%@. playable=%s.\n", track.trackID, track.mediaType, (track.isPlayable ? "YES" : "NO"));
    }
#endif

#ifndef MOVIEVIDEO_TEST_STREAMING
    [self mediaIsReady];
#else
    // This approach handles both file-based and streaming media consistently. iOS 4.3 and later only.
    // Create a player item, and a player. The player will load the media of its bound player item.
    _playerItem = [AVPlayerItem playerItemWithAsset:_asset];
    [_playerItem addObserver:self forKeyPath:kStatusKey options:0 context:&observingContext];
    _playerItemIsBeingObserved = TRUE;
    if (_player) {
        [_player release];
        _player = nil;
    }
    _player = [[AVPlayer playerWithPlayerItem:_playerItem] retain];
#endif
}

#ifdef MOVIEVIDEO_TEST_STREAMING
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == &observingContext) {
        switch ([(AVPlayerItem *)object status]) {
            case AVPlayerItemStatusReadyToPlay:
                dispatch_async(dispatch_get_main_queue(),
                               ^{
                                   [self mediaIsReady];
                               });
                return;
                break;
            case AVPlayerItemStatusFailed:
                NSLog(@"Player item failed.\n");
                // Should cleanup and bail out.
                break;
            case AVPlayerItemStatusUnknown:
                // Do nothing yet.
                break;
                
        }
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
    return;
}
#endif

- (void) mediaIsReady
{
#ifndef MOVIEVIDEO_TEST_STREAMING
    // File-based media: get video tracks directly from asset.
    // This doesn't work with streaming media; the asset is only the media
    // references. In those cases, we'd need to get the asset from the player.
    NSArray *videoTracks = [_asset tracksWithMediaType:AVMediaTypeVideo];
    if (videoTracks.count == 0) {
        NSLog(@"Media does not contain any video.\n");
        return;
    }
    _videoTrack = [[videoTracks objectAtIndex:0] retain];
#else
    NSArray *playerItemTracks = [_playerItem tracks];
    for (AVPlayerItemTrack *pit in playerItemTracks) {
        if ([pit.assetTrack.mediaType isEqualToString:AVMediaTypeVideo]) {
            _videoTrack = [pit.assetTrack retain];
            break;
        }
    }
    if (!_videoTrack) {
        NSLog(@"Media does not contain any video.\n");
        return;
    }
#endif
    
    _contentWidth = _videoTrack.naturalSize.width;
    _contentHeight = _videoTrack.naturalSize.height;
    _nominalFrameRate = _videoTrack.nominalFrameRate;
    // duration = videoTrack.timeRange.duration.value/videoTrack.timeRange.duration.timescale; // microseconds.
#ifdef DEBUG
    NSLog(@"Media video is %ldx%ld@%.3f fps.\n", (long)_contentWidth, (long)_contentHeight, _nominalFrameRate);
#endif
    if (_audioPlayer) {
        [_audioPlayer release];
        _audioPlayer = nil;
    }
    if (!_noSound) {
        AVPlayerItem *audioPlayerItem = [AVPlayerItem playerItemWithAsset:_asset];
        _audioPlayer = [[AVPlayer playerWithPlayerItem:audioPlayerItem] retain];
        //CGFloat totalSeconds = 0.0f;
        //CMTime duration = [_audioPlayer currentItem].duration;
        //totalSeconds = (float)duration.value / (float)duration.timescale;
    }
    
    [self play];
}

- (void)play
{
#ifdef DEBUG
    NSLog(@"MovieVideo -play\n");
#endif
    NSError *error = nil;

    // Rewind the audio.
    if (_audioPlayer) [_audioPlayer seekToTime:kCMTimeZero];
    
    // Create a new AVAssetReader, disposing of any existing one.
    if (_assetReader) {
        [_assetReader release];
        _assetReader = nil;
    }
    _assetReader = [[AVAssetReader assetReaderWithAsset:_videoTrack.asset error:&error] retain];
    if (error) {
        NSLog(@"%@", [error localizedDescription]);
    }
    
    NSDictionary *settings = nil;
    if (_pixFormat) {
        if (_width && _height) {
            settings = [NSDictionary dictionaryWithObjectsAndKeys:
                        [NSNumber numberWithInteger:_pixFormat], kCVPixelBufferPixelFormatTypeKey,
                        [NSNumber numberWithInteger:_width], kCVPixelBufferWidthKey,
                        [NSNumber numberWithInteger:_height], kCVPixelBufferHeightKey,
                        nil];
        } else {
            settings = [NSDictionary dictionaryWithObjectsAndKeys:
                        [NSNumber numberWithInteger:_pixFormat], kCVPixelBufferPixelFormatTypeKey,
                        nil];
        }
    }
    
    if (_videoTrackOutput) {
        [_videoTrackOutput release];
        _videoTrackOutput = nil;
    }
    _videoTrackOutput = [[AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:_videoTrack outputSettings:settings] retain];
    if ([_videoTrackOutput respondsToSelector:@selector(setAlwaysCopiesSampleData:)]) { // iOS 5.0, macOS 10.8 and later.
        _videoTrackOutput.alwaysCopiesSampleData = NO; // We won't ever modify the buffer (in fact we copy it) so don't require the OS to copy it.
    }
    [_assetReader addOutput:_videoTrackOutput];
    [_assetReader startReading];
    
    _frameCounter = 0;
    _playTime = _pausedTime = CFAbsoluteTimeGetCurrent();
    _loaded = TRUE;
    [[NSNotificationCenter defaultCenter] postNotificationName:MovieVideoMediaLoadedNofication object:self];
    
    if (_audioPlayer && !_paused) [_audioPlayer play];
}

/*- (void)playerItemDidReachEnd:(NSNotification *)notification {
#ifdef DEBUG
    NSLog(@"MovieVideo -playerItemDidReachEnd:\n");
#endif
    if (_loop) [self play];
}
*/

-(unsigned char *)getFrame
{
	if (!_loaded) {
#ifdef DEBUG
        NSLog(@"MovieVideo -getFrame !LOADED\n");
#endif
        return (NULL);
    }
     if (_paused) {
#ifdef DEBUG
        NSLog(@"MovieVideo -getFrame PAUSED\n");
#endif
		return (NULL);
	}
#ifdef DEBUG
    NSLog(@"MovieVideo -getFrame\n");
#endif
    
    // Read a frame from our AVAssetReaderTrackOutput, and update our
    // _frameCounter depending on how many frames are consumed in this update pass.
	
	// Determine what frame we should be on.
	NSInteger targetFrameNumber;
	CGFloat elapsedSeconds;
	//if (_audioPlayer) {
    //    // Calculate from audio player timestamp and _nomimalFramerate.
	//    CMTime audioTime = [_audioPlayer currentTime];
	//    elapsedSeconds = (float)audioTime.value / (float)audioTime.timescale;
    //} else {
        elapsedSeconds = (float)(CFAbsoluteTimeGetCurrent() - _playTime);
	//}
    targetFrameNumber = floor(elapsedSeconds * _nominalFrameRate);
	
	// Compare the target frame number with the current frame number. It there is
	// a disparity between the two, we'll either...
	//	1) skip this update, if we're updating faster than the video's framerate.
	//	2) discard frames without processing until we catch up, if we're rendering slower than the video's framerate.
	NSInteger frameDisparity = targetFrameNumber - _frameCounter;
	if (frameDisparity <= 0) { // && elapsedSeconds != totalSeconds) {
#ifdef DEBUG
        NSLog(@"MovieVideo -getFrame: NOP (video is %ld frames ahead of audio).\n", (long)-frameDisparity);
#endif
		return (NULL);
	} else if (frameDisparity > 1) {
        int i;
		for (i = 0; i < frameDisparity; i++) {
			[self dumpFrame];
		}
	}
        
    CMSampleBufferRef sampleBuffer = [_videoTrackOutput copyNextSampleBuffer];
	if (!sampleBuffer) {
        switch (_assetReader.status) {
            case AVAssetReaderStatusUnknown: // 0
				NSLog(@"AVAssetReaderStatusUnknown"); // Have not yet started reading.
				break;
            case AVAssetReaderStatusReading: // 1
				NSLog(@"AVAssetReaderStatusReading"); // Reading but frame not yet ready.
				break;
            case AVAssetReaderStatusCompleted: // 2
                [[NSNotificationCenter defaultCenter] postNotificationName:MovieVideoPlaybackEndedNofication object:self];
                if (_loop) [self play];
                break;
            case AVAssetReaderStatusFailed: // 3
				NSLog(@"AVAssetReaderStatusFailed");
				NSLog(@"%@", _assetReader.error);
                break;
            case AVAssetReaderStatusCancelled: // 4
				NSLog(@"AVAssetReaderStatusCancelled");
                break;
        }
        return (NULL);
	}

	// Get and lock the pixel buffer.
    CVPixelBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    
    /*OSType pixelFormat = CVPixelBufferGetPixelFormatType(imageBuffer); // 'BGRA', 'y420'.
    NSLog(@"Pixel format:  0x%08x = %u = '%c%c%c%c'.\n", (unsigned int)pixelFormat, (unsigned int)pixelFormat,
#ifdef AR_BIG_ENDIAN
            ((char *)&pixelFormat)[0], ((char *)&pixelFormat)[1], ((char *)&pixelFormat)[2], ((char *)&pixelFormat)[3]
#else
            ((char *)&pixelFormat)[3], ((char *)&pixelFormat)[2], ((char *)&pixelFormat)[1], ((char *)&pixelFormat)[0]
#endif
			);*/
    
    uint8_t *baseAddress;
    size_t bytesPerRow, width, height;
    size_t planes = CVPixelBufferGetPlaneCount(imageBuffer);
    if (!planes) {
        baseAddress = (uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
        bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
        width = CVPixelBufferGetWidth(imageBuffer);
        height = CVPixelBufferGetHeight(imageBuffer);
    } else {
        baseAddress = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
        bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
        width = CVPixelBufferGetWidthOfPlane(imageBuffer, 0);
        height = CVPixelBufferGetHeightOfPlane(imageBuffer, 0);
    }
    
    if (!_bufDataPtr) {
        _bufDataPtr = (unsigned char *)valloc(bytesPerRow*height);
        if (!_bufDataPtr) {
            CFRelease(sampleBuffer);
            return (NULL);
        }
        _bufWidth = width;
        _bufHeight = height;
        _bufRowBytes = bytesPerRow;
    }
    
    if (_transferFunctionChromaKey) {
        int i, j;
        uint8_t *src = baseAddress, *dest = (_flipV ? _bufDataPtr + (height - 1)*bytesPerRow : _bufDataPtr);
        for (i = 0; i < height; i++) {
            uint32_t *srcbytep = (uint32_t *)src;
            uint32_t *destbytep = (uint32_t *)dest;
            uint8_t alpha;
            switch (_pixFormat) {
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32ABGR:
#else
                case kCVPixelFormatType_32RGBA:
#endif
                    for (j = 0; j < width; j++) {
                        unsigned int alphaTableIndex = (*srcbytep & 0xfc000000) >> (32 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*3) | (*srcbytep & 0x00fc0000) >> (24 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*2) | (*srcbytep & 0x0000fc00) >> (16 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS);
                        alpha = _transferFunctionChromaKeyAlphaTable[alphaTableIndex / 8] & (1 << (alphaTableIndex % 8));
                        *destbytep = (alpha ? *srcbytep & 0xffffff00 : *srcbytep);
                        srcbytep++;
                        destbytep++;
                    }
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32ARGB:
#else
                case kCVPixelFormatType_32BGRA:
#endif
                    for (j = 0; j < width; j++) {
                        unsigned int alphaTableIndex = (*srcbytep & 0xfc000000) >> (32 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS) | (*srcbytep & 0x00fc0000) >> (24 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*2) | (*srcbytep & 0x0000fc00) << (TRANSFER_FUNCTION_ALPHA_TABLE_BITS*3 - 16);
                        alpha = _transferFunctionChromaKeyAlphaTable[alphaTableIndex / 8] & (1 << (alphaTableIndex % 8));
                        *destbytep = (alpha ? *srcbytep & 0xffffff00 : *srcbytep);
                        srcbytep++;
                        destbytep++;
                    }
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32BGRA:
#else
                case kCVPixelFormatType_32ARGB:
#endif
                    for (j = 0; j < width; j++) {
                        unsigned int alphaTableIndex = (*srcbytep & 0x00fc0000) >> (24 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*3) | (*srcbytep & 0x0000fc00) >> (16 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*2) | (*srcbytep & 0x000000fc) >> (8 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS);
                        alpha = _transferFunctionChromaKeyAlphaTable[alphaTableIndex / 8] & (1 << (alphaTableIndex % 8));
                        *destbytep = (alpha ? *srcbytep & 0x00ffffff : *srcbytep);
                        srcbytep++;
                        destbytep++;
                    }
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32RGBA:
#else
                case kCVPixelFormatType_32ABGR:
#endif
                    for (j = 0; j < width; j++) {
                        unsigned int alphaTableIndex = (*srcbytep & 0x00fc0000) >> (24 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS) | (*srcbytep & 0x0000fc00) >> (16 - TRANSFER_FUNCTION_ALPHA_TABLE_BITS*2) | (*srcbytep & 0x000000fc) << (TRANSFER_FUNCTION_ALPHA_TABLE_BITS*3 - 8);
                        alpha = _transferFunctionChromaKeyAlphaTable[alphaTableIndex / 8] & (1 << (alphaTableIndex % 8));
                        *destbytep = (alpha ? *srcbytep & 0x00ffffff : *srcbytep);
                        srcbytep++;
                        destbytep++;
                    }
                    break;
            }
            src += bytesPerRow;
            dest += (_flipV ? -bytesPerRow : bytesPerRow);
        }

    } else if (_transferFunctionTransparency) {
        int i, j;
        uint8_t *src = baseAddress, *dest = (_flipV ? _bufDataPtr + (height - 1)*bytesPerRow : _bufDataPtr);
        for (i = 0; i < height; i++) {
            uint32_t *srcbytep = (uint32_t *)src;
            uint32_t *destbytep = (uint32_t *)dest;
            for (j = 0; j < width; j++) {
                uint32_t srcbyte = *srcbytep;
                uint32_t srcbyteMasked = srcbyte & _transferFunctionTransparencyColorMask;
                *destbytep = (srcbyteMasked == _transferFunctionTransparencyColor ? srcbyteMasked : srcbyte);
                srcbytep++;
                destbytep++;
            }
            src += bytesPerRow;
            dest += (_flipV ? -bytesPerRow : bytesPerRow);
        }
    } else {
        if (!_flipV) memcpy(_bufDataPtr, baseAddress, bytesPerRow*height);
        else {
            int i;
            uint8_t *src = baseAddress, *dest = _bufDataPtr + (height - 1)*bytesPerRow;
            for (i = 0; i < height; i++) {
                memcpy(dest, src, bytesPerRow);
                src += bytesPerRow;
                dest -= bytesPerRow;
            }
        }
    }

    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

    CFRelease(sampleBuffer); // We got it via copy, so release it.
	_frameCounter++;

    return (_bufDataPtr);
}

- (void)dumpFrame {
#ifdef DEBUG
    NSLog(@"MovieVideo -dumpFrame\n");
#endif
	// Ignores the next frame in the video track by copying it and immediately releasing it.
    // Updates our _frameCounter to reflect the discarded frames.
    // Since we told the OS we don't need it to vend a copied buffer,
    // (see '_videoTrackOutput.alwaysCopiesSampleData = NO' above) this should be quite lightweight.
    CMSampleBufferRef sampleBuffer = [_videoTrackOutput copyNextSampleBuffer];
	if (sampleBuffer) {
		CFRelease(sampleBuffer);
		_frameCounter++;
	} else {
        switch (_assetReader.status) {
            case AVAssetReaderStatusUnknown: // 0
				NSLog(@"AVAssetReaderStatusUnknown");
				break;
            case AVAssetReaderStatusReading: // 1
				NSLog(@"AVAssetReaderStatusReading");
				break;
            case AVAssetReaderStatusCompleted: // 2
                [[NSNotificationCenter defaultCenter] postNotificationName:MovieVideoPlaybackEndedNofication object:self];
                if (_loop) [self play];
                break;
            case AVAssetReaderStatusFailed: // 3
				NSLog(@"AVAssetReaderStatusFailed");
				NSLog(@"%@", _assetReader.error);
                break;
            case AVAssetReaderStatusCancelled: // 4
				NSLog(@"AVAssetReaderStatusCancelled");
                break;
        }
	}
}

-(AR_PIXEL_FORMAT) ARPixelFormat
{
    switch (_pixFormat) {
        case kCVPixelFormatType_32BGRA:
            return (AR_PIXEL_FORMAT_BGRA);
            break;
        case kCVPixelFormatType_32ARGB:
            return (AR_PIXEL_FORMAT_ARGB);
            break; 
        case kCVPixelFormatType_422YpCbCr8:
            return (AR_PIXEL_FORMAT_2vuy);
            break;
        case kCVPixelFormatType_422YpCbCr8_yuvs:
            return (AR_PIXEL_FORMAT_yuvs);
            break; 
        case kCVPixelFormatType_24RGB:
            return (AR_PIXEL_FORMAT_RGB);
            break; 
        case kCVPixelFormatType_24BGR:
            return (AR_PIXEL_FORMAT_BGR);
            break; 
        case kCVPixelFormatType_32ABGR:
            return (AR_PIXEL_FORMAT_ABGR);
            break; 
        case kCVPixelFormatType_32RGBA:
            return (AR_PIXEL_FORMAT_RGBA);
            break; 
        case kCVPixelFormatType_8Indexed:
            return (AR_PIXEL_FORMAT_MONO);
            break;
        default:
            return (AR_PIXEL_FORMAT)-1;
            break;
    }
}

- (void) setPaused:(BOOL)paused
{
#ifdef DEBUG
    NSLog(@"MovieVideo -setPaused:%s\n", (paused ? "TRUE" : "FALSE"));
#endif
    if (_paused != paused) {
        _paused = paused;
        if (_loaded) {
            if (_audioPlayer) {
                if (_paused) [_audioPlayer pause];
                else [_audioPlayer play];
            }
            // Adjust "start" time by the amount of time we spent paused.
            CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
            if (_paused) _pausedTime = now;
            else _playTime += (now - _pausedTime);
        }
    }
}

- (BOOL) isPaused
{
    return (_paused);
}

- (void) stop
{
#ifdef DEBUG
    NSLog(@"MovieVideo -stop\n");
#endif
    [_assetReader cancelReading];
    if (_audioPlayer) [_audioPlayer pause];
    _loaded = FALSE;
}

-(void) dealloc
{
#ifdef DEBUG
    NSLog(@"MovieVideo -dealloc\n");
#endif
    [_assetReader release];
    [_videoTrackOutput release];
    
    if (_audioPlayer) {
        [_audioPlayer release];
        _audioPlayer = nil;
    }
    [_videoTrack release];
    
#ifdef MOVIEVIDEO_TEST_STREAMING
    if (_player) {
        [_player release];
        _player = nil;
    }
    if (_playerItemIsBeingObserved) {
        [self removeObserver:self forKeyPath:kStatusKey context:&observingContext];
        _playerItemIsBeingObserved = FALSE;
    }
    _playerItem = nil;
#endif
    
    [_asset release];
    
    [_url release];
    
    if (_transferFunctionChromaKeyAlphaTable) free(_transferFunctionChromaKeyAlphaTable);
	
    [super dealloc];
}

-(BOOL) transparencyByTransparentRGBA:(uint32_t *)rgba
{
    if (_transferFunctionTransparency) {
        if (rgba) {
            uint32_t r, g, b;
            switch (_pixFormat) {
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32ABGR:
#else
                case kCVPixelFormatType_32RGBA:
#endif
                    *rgba = _transferFunctionTransparencyColor;
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32ARGB:
#else
                case kCVPixelFormatType_32BGRA:
#endif
                    r = (_transferFunctionTransparencyColor & 0x0000ff00) >> 8;
                    g = (_transferFunctionTransparencyColor & 0x00ff0000) >> 16;
                    b = (_transferFunctionTransparencyColor & 0xff000000) >> 24;
                    *rgba = (r << 24 | g << 16 | b << 8);
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32BGRA:
#else
                case kCVPixelFormatType_32ARGB:
#endif
                    r = (_transferFunctionTransparencyColor & 0x00ff0000) >> 16;
                    g = (_transferFunctionTransparencyColor & 0x0000ff00) >> 8;
                    b = _transferFunctionTransparencyColor & 0x000000ff;
                    *rgba = (r << 24 | g << 16 | b << 8);
                    break;
#ifdef AR_LITTLE_ENDIAN
                case kCVPixelFormatType_32RGBA:
#else
                case kCVPixelFormatType_32ABGR:
#endif
                    r = _transferFunctionTransparencyColor & 0x000000ff;
                    g = (_transferFunctionTransparencyColor & 0x0000ff00) >> 8;
                    b = (_transferFunctionTransparencyColor & 0x00ff0000) >> 16;
                    *rgba = (r << 24 | g << 16 | b << 8);
                    break;
                default:
                    NSLog(@"Cannot use transparency transfer function with non 32-bit pixel format.\n");
            }
        }
    }
    return _transferFunctionTransparency;
}

-(void) setTransparencyByTransparent:(BOOL)on RGBA:(uint32_t)rgba
{
    _transparent = _transferFunctionTransparency = on;
    if (on) {
        uint32_t r = (rgba & 0xff000000) >> 24;
        uint32_t g = (rgba & 0x00ff0000) >> 16;
        uint32_t b = (rgba & 0x0000ff00) >> 8;
        switch (_pixFormat) {
#ifdef AR_LITTLE_ENDIAN
            case kCVPixelFormatType_32RGBA:
#else
            case kCVPixelFormatType_32ABGR:
#endif
                _transferFunctionTransparencyColor = b << 16 | g << 8 | r;
                _transferFunctionTransparencyColorMask = 0x00ffffff;
                break;
#ifdef AR_LITTLE_ENDIAN
            case kCVPixelFormatType_32BGRA:
#else
            case kCVPixelFormatType_32ARGB:
#endif
                _transferFunctionTransparencyColor = r << 16 | g << 8 | b;
                _transferFunctionTransparencyColorMask = 0x00ffffff;
                break;
#ifdef AR_LITTLE_ENDIAN
            case kCVPixelFormatType_32ARGB:
#else
            case kCVPixelFormatType_32BGRA:
#endif
                _transferFunctionTransparencyColor = b << 24 | g << 16 | r << 8;
                _transferFunctionTransparencyColorMask = 0xffffff00;
                break;
#ifdef AR_LITTLE_ENDIAN
            case kCVPixelFormatType_32ABGR:
#else
            case kCVPixelFormatType_32RGBA:
#endif
                _transferFunctionTransparencyColor = rgba & 0xffffff00;
                _transferFunctionTransparencyColorMask = 0xffffff00;
                break;
            default:
                NSLog(@"Cannot use transparency transfer function with non 32-bit pixel format.\n");
        }
    }
}

-(BOOL) transparencyByChromaKeyHueMinAngle:(float *)hueMinAngle hueMaxAngle:(float *)hueMaxAngle
{
    if (_transferFunctionChromaKey) {
        if (hueMinAngle) *hueMinAngle = _transferFunctionChromaKeyHueMinAngle;
        if (hueMaxAngle) *hueMaxAngle = _transferFunctionChromaKeyHueMaxAngle;
    }
    return (_transferFunctionChromaKey);
}

-(void) setTransparencyByChromaKey:(BOOL)on hueMinAngle:(float)hueMinAngle hueMaxAngle:(float)hueMaxAngle
{
    _transparent = _transferFunctionChromaKey = on;
    if (!on) {
        if (_transferFunctionChromaKeyAlphaTable) {
            free(_transferFunctionChromaKeyAlphaTable);
            _transferFunctionChromaKeyAlphaTable = NULL;
        }
    } else {
        int ri, gi, bi;
        float r, g, b;
        const int size = TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS*TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS*TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS/8;
        
        if (hueMinAngle < 0.0f || hueMinAngle >= 360.0f || hueMaxAngle < 0.0f || hueMaxAngle >= 360.0f) return; // Sanity check.
        _transferFunctionChromaKeyHueMinAngle = hueMinAngle;
        _transferFunctionChromaKeyHueMaxAngle = hueMaxAngle;
        
        if (_transferFunctionChromaKeyAlphaTable) memset(_transferFunctionChromaKeyAlphaTable, 0, size); // Reset to 0.
        else _transferFunctionChromaKeyAlphaTable = (uint8_t *)calloc(1, size*sizeof(uint8_t)); // allocate.
        
        for (ri = 0; ri < TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS; ri++) {
            r = (float)ri/(TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS - 1); // -1 ensures that value == 1.0 on last pass.
            for (gi = 0; gi < TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS; gi++) {
                g = (float)gi/(TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS - 1);
                for (bi = 0; bi < TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS; bi++) {
                    b = (float)bi/(TRANSFER_FUNCTION_ALPHA_TABLE_LEVELS - 1);
                    
                    // Calculate hue [0, 360], sat [0, 1], val [0,1].
                    float hue/*, sat, val*/;
                    float max = MAX3(r, g, b);
                    // val = max;
                    if (max == 0.0f) continue; // black, hue is undefined.
                    float min = MIN3(r, g, b);
                    float delta = max - min;
                    if (delta == 0.0f) continue; // greyscale.
                    //sat = delta/max;
                    if (max == r) hue = (g - b)/delta;
                    else if (max == g) hue = 2.0f + (b - r)/delta;
                    else /* (max == b) */ hue = 4.0f + (r - g)/delta;
                    hue *= 60.0f;
                    if (hue < 0.0f) hue += 360.0f;
                    
                    int setIt = FALSE;
                    if (hueMaxAngle < hueMinAngle) { // Hue range spans 0/360 degree boundary.
                        if (!(hue >= hueMaxAngle && hue <= hueMinAngle)) setIt = TRUE;
                    } else {
                        if (hue > hueMinAngle && hue < hueMaxAngle) setIt = TRUE;
                    }
                    if (setIt) {
                        unsigned int alphaTableIndex = ri << (TRANSFER_FUNCTION_ALPHA_TABLE_BITS*2) | gi << TRANSFER_FUNCTION_ALPHA_TABLE_BITS | bi;
                        _transferFunctionChromaKeyAlphaTable[alphaTableIndex / 8] += (1 << alphaTableIndex % 8); // Set the bit to indicate alpha should be zeroed.
                    }
                }
            }
        }
    }
}

@end

#endif //  ARVIDEO_INPUT_AVFOUNDATION
