/*
 *	MovieVideo.h
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

#import <AVFoundation/AVFoundation.h>

#import <ARX/AR/config.h> // AR_PIXEL_FORMAT

// Notifications.
extern NSString *const MovieVideoMediaLoadedNofication;
extern NSString *const MovieVideoPlaybackEndedNofication;

@interface MovieVideo : NSObject

//
// Options for configuration string 'conf'.
//
// -width=w             Set width of buffer pointed to by bufDataPtr to w. Default is actual width of video.
// -height=h            Set height of buffer pointed to by bufDataPtr to h. Default is actual height of video.
// -pixelformat=pppp    Format for pixels pointed to by bufDataPtr. pppp is either 4 character type, or unsigned 32-bit integer, corresponding to appropriate AVFoundation type. Default is 'BGRA'. Another common format is '420v'.
// -[no]fliph           If -fliph, flips incoming video frames horizontally. Default is -nofliph.
// -[no]flipv           If -flipv, flips incoming video frames vertically. Default is -noflipv.
// -[no]loop            If -loop, video will loop back to beginning when end is reached. Default is -noloop.
// -[no]fill            If -fill, video will be proportionally resized from the centre to fill the buffer. If -nofill, video will be fitted inside buffer, and may be letter-boxed or pillar-boxed. Default is -nofill.
// -[no]1to1            If -1to1, video will be shown inside buffer at original side (may be cropped or displayed with a border). Default is -no1to1.
// -[no]stretch         If -stretch, video will be stretched to fill the buffer. Default is -nostretch.
// -[no]mute            If -mute, any audio in the movie file will be ignored. Default is -nomute.
// -[no]singlebuffer    If -singlebuffer, video frames will be single buffered (may produce tearing). Default is -nosinglebuffer.
// -[no]pause           If -pause, video will be paused when first loaded. Default is -nopause.
// -[no]alpha           Hint that the video contains an alpha channel. Default is -noalpha.
// -transparent=hhhhhh  Transparent colour, expressed as an RGB hex triplet (e.g. ff0000 for red).
// -chromakey=min,max   Perform chroma-keying on video. Chroma values must be expressed as minimum and maximum hue angles 'min' and 'max' in range [0-360), separated by a comma.
//


-(id) initWithURL:(NSURL *)url config:(char *)conf;
-(bool) start;
-(unsigned char *)getFrame;
-(void) stop;

@property(nonatomic, getter=isPaused) BOOL paused;
@property(nonatomic, readonly, getter=isLoaded) BOOL loaded;
@property(readonly) NSInteger contentWidth;
@property(readonly) NSInteger contentHeight;
@property(nonatomic, readonly) AR_PIXEL_FORMAT ARPixelFormat;
@property(readonly) OSType pixFormat;
@property(readonly) size_t bufWidth;
@property(readonly) size_t bufHeight;
@property(readonly) size_t bufRowBytes;
@property(nonatomic, readonly) unsigned char *bufDataPtr;

@property(nonatomic, readonly) BOOL transparent;
-(BOOL) transparencyByTransparentRGBA:(uint32_t *)rgba;
-(void) setTransparencyByTransparent:(BOOL)on RGBA:(uint32_t)rgba;
-(BOOL) transparencyByChromaKeyHueMinAngle:(float *)hueMinAngle hueMaxAngle:(float *)hueMaxAngle;
-(void) setTransparencyByChromaKey:(BOOL)on hueMinAngle:(float)hueMinAngle hueMaxAngle:(float)hueMaxAngle;

@end
