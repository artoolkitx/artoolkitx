//
//  ARViewController.mm
//  artoolkitX Square Tracking Example
//
//  Copyright 2018 Realmax, Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#import "ARViewController.h"
#import <OpenGLES/ES2/glext.h>
#ifdef DEBUG
#  import <unistd.h>
#  import <sys/param.h>
#endif
#import <string>

#import <ARX/ARController.h>
#import "../draw.h"

struct marker {
    const char *name;
    float height;
    const char *modelDataFileName;
};
static const struct marker markers[] = {
    {"hiro.patt", 80.0, "model0.dat"},
    {"kanji.patt", 80.0, "model1.dat"}
};
static const int markerCount = (sizeof(markers)/sizeof(markers[0]));

@interface ARViewController () {
    ARController *arController;
    long frameNo;
    int contextWidth;
    int contextHeight;
    bool contextRotate90;
    bool contextFlipH;
    bool contextFlipV;
    bool contextWasUpdated;
    int32_t viewport[4];
    float projection[16];
    int markerIDs[markerCount];
    int markerModelIDs[markerCount];
}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;
@end

@implementation ARViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
#ifdef DEBUG
    arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif
    
    // Init instance variables.
    arController = nil;
    frameNo = 0L;
    contextWidth = 0;
    contextHeight = 0;
    contextRotate90 = true; contextFlipH = contextFlipV = false;
    contextWasUpdated = false;
    for (int i = 0; i < markerCount; i++) markerIDs[i] = -1;
    for (int i = 0; i < markerCount; i++) markerModelIDs[i] = -1;
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    [self setupGL];
}

- (void)viewDidLayoutSubviews
{
    GLKView *view = (GLKView *)self.view;
    [view bindDrawable];
    contextWidth = (int)view.drawableWidth;
    contextHeight = (int)view.drawableHeight;
    switch ([[UIApplication sharedApplication] statusBarOrientation]) {
        case UIInterfaceOrientationLandscapeLeft:
            contextRotate90 = false; contextFlipH = contextFlipV = true;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            contextRotate90 = contextFlipH = contextFlipV = true;
            break;
        case UIInterfaceOrientationLandscapeRight:
            contextRotate90 = contextFlipH = contextFlipV = false;
            break;
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationUnknown:
        default:
            contextRotate90 = true; contextFlipH = contextFlipV = false;
            break;
    }
    contextWasUpdated = true;
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setupGL
{
    char vconf[] = "-preset=720p";
    
    [EAGLContext setCurrentContext:self.context];
    
    // Initialise the ARController.
    arController = new ARController();
    if (!arController->initialiseBase()) {
        ARLOGe("Error initialising ARController.\n");
        return;
    }
    
    // Add markers.
#ifdef DEBUG
    char buf[MAXPATHLEN];
    ARLOGe("CWD is '%s'.\n", getcwd(buf, sizeof(buf)));
#endif
    char *resourcesDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
    for (int i = 0; i < markerCount; i++) {
        std::string markerConfig = "single;" + std::string(resourcesDir) + '/' + markers[i].name + ';' + std::to_string(markers[i].height);
        markerIDs[i] = arController->addTrackable(markerConfig);
        if (markerIDs[i] == -1) {
            ARLOGe("Error adding marker.\n");
            return;
        }
    }
    arController->getSquareTracker()->setPatternDetectionMode(AR_TEMPLATE_MATCHING_MONO);
    arController->getSquareTracker()->setThresholdMode(AR_LABELING_THRESH_MODE_AUTO_BRACKETING);

    // Start tracking.
    arController->startRunning(vconf, NULL, NULL, 0);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    drawCleanup();
    if (arController) {
        arController->drawVideoFinal(0);
        arController->shutdown();
        delete arController;
    }

}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    bool gotFrame = arController->capture();
    if (gotFrame) {
        //ARLOGi("Got frame %ld.\n", frameNo);
        frameNo++;
        
        if (!arController->update()) {
            ARLOGe("Error in ARController::update().\n");
            return;
        }
    }

}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (arController->isRunning()) {
        if (contextWasUpdated) {
            arController->drawVideoInit(0);
            arController->drawVideoSettings(0, contextWidth, contextHeight, contextRotate90, contextFlipH, contextFlipV, ARVideoView::HorizontalAlignment::H_ALIGN_CENTRE, ARVideoView::VerticalAlignment::V_ALIGN_CENTRE, ARVideoView::ScalingMode::SCALE_MODE_FIT, viewport);
            drawSetup(ARG_API_GLES2, false, false, false);
            
            ARdouble projectionARD[16];
            arController->projectionMatrix(0, 10.0f, 10000.0f, projectionARD);
            for (int i = 0; i < 16; i++) projection[i] = (float)projectionARD[i];
            
            drawSetViewport(viewport);
            drawSetCamera(projection, NULL);

            for (int i = 0; i < markerCount; i++) {
                markerModelIDs[i] = drawLoadModel([[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:markers[i].modelDataFileName] ofType:NULL].UTF8String);
            }
            contextWasUpdated = false;
        }
        
        // Clean the OpenGL context.
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Display the current video frame to the current OpenGL context.
        arController->drawVideo(0);

        // Look for markers, and draw on each found one.
        for (int i = 0; i < markerCount; i++) {
            
            // Find the marker for the given marker ID.
            ARTrackable *marker = arController->findTrackable(markerIDs[i]);
            float view[16];
            if (marker->visible) {
                //arUtilPrintMtx16(marker->transformationMatrix);
                for (int i = 0; i < 16; i++) view[i] = (float)marker->transformationMatrix[i];
            }
            drawSetModel(markerModelIDs[i], marker->visible, view);
        }
        
        draw();
    }
}

@end
