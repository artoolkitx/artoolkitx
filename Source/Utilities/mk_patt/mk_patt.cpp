/*
 *  mk_patt.cpp
 *  artoolkitX
 *
 *  Run with "--help" parameter to see usage.
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

// ============================================================================
//    Includes
// ============================================================================

#if defined(_WIN32)
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ARX/AR/ar.h>
#include <ARX/ARVideoSource.h>
#include <ARX/ARVideoView.h>
#include <ARX/ARUtil/time.h>
#include <SDL2/SDL.h>
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#elif defined(__linux) || defined(_WIN32)
#  include <GL/gl.h>
#endif
#include <Eden/EdenGLFont.h>

// ============================================================================
//    Constants
// ============================================================================

#define FONT_SIZE 18.0f

// ============================================================================
//    Global variables
// ============================================================================

static const char *cpara = NULL;
static const char *vconfRGB = 
#if ARX_TARGET_PLATFORM_MACOS || ARX_TARGET_PLATFORM_IOS || ARX_TARGET_PLATFORM_LINUX || ARX_TARGET_PLATFORM_WINDOWS
    "-format=BGRA"
#elif ARX_TARGET_PLATFORM_ANDROID
    "-format=RGBA"
#else
    NULL
#endif
    ;
static const char *vconf = vconfRGB;
static ARdouble gPattRatio = (ARdouble)AR_PATT_RATIO;
static int gLabelingMode = AR_DEFAULT_LABELING_MODE;
static int gPatternDetectionMode = AR_TEMPLATE_MATCHING_COLOR;
static int gPattSize = AR_PATT_SIZE1;

// Video acquisition and rendering.
static ARVideoSource *vs = nullptr;
static ARVideoView *vv = nullptr;
static bool gPostVideoSetupDone = false;
static AR2VideoTimestampT gUpdateFrameStamp = {0, 0};
static ARUint8            *gARImageWithPatternToSave;

// Marker detection.
static ARHandle        *gARHandle = NULL;
static ARMarkerInfo       *target = NULL;

// Drawing.
// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static SDL_Window* gSDLWindow = NULL;
static int gShowHelp = 1;
static int gShowMode = 1;
static GLint gViewport[4];
static bool gDrawPattern = false;
static ARUint8 gPatternImage[AR_PATT_SIZE1_MAX*AR_PATT_SIZE1_MAX*3]; // Buffer big enough to hold largest possible image.

// ============================================================================
//    Function prototypes.
// ============================================================================

static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void startVideo(void);
static void stopVideo(void);
static void initAR(void);
static void quit(int rc);
static void reshape(int w, int h);
static void keyboard(SDL_Keycode key);
static void drawView(void);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys();
static void printMode();

// ============================================================================
//    Functions
// ============================================================================


int main(int argc, char *argv[])
{
#ifdef DEBUG
    arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif
    
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ARLOGe("Error: SDL initialisation failed. SDL error: '%s'.\n", SDL_GetError());
        return -1;
    }
    
    // Preferences.
    processCommandLineOptions(argc, argv);
    
    // Create a window.
    gSDLWindow = SDL_CreateWindow(argv[0],
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  1280, 720,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                  );
    if (!gSDLWindow) {
        ARLOGe("Error creating window: %s.\n", SDL_GetError());
        quit(-1);
    }
    
    // Create an OpenGL context to draw into.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // This is the default.
    SDL_GL_SetSwapInterval(1);
    gSDLContext = SDL_GL_CreateContext(gSDLWindow);
    if (!gSDLContext) {
        ARLOGe("Error creating OpenGL context: %s.\n", SDL_GetError());
        return -1;
    }
    int w, h;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &w, &h);
    reshape(w, h);
    
    EdenGLFontInit(1); // contextsActiveCount=1
    EdenGLFontSetFont(EDEN_GL_FONT_ID_Stroke_Roman);
    EdenGLFontSetSize(FONT_SIZE);
    
    startVideo();
    
    // Main loop.
    bool done = false;
    while (!done) {
        
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                done = true;
                break;
            } else if (ev.type == SDL_WINDOWEVENT) {
                //ARLOGd("Window event %d.\n", ev.window.event);
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED && ev.window.windowID == SDL_GetWindowID(gSDLWindow)) {
                    //int32_t w = ev.window.data1;
                    //int32_t h = ev.window.data2;
                    int w, h;
                    SDL_GL_GetDrawableSize(gSDLWindow, &w, &h);
                    reshape(w, h);
                }
            } else if (ev.type == SDL_KEYDOWN) {
                keyboard(ev.key.keysym.sym);
            }
        }
        
        if (vs->isOpen() && vs->captureFrame()) {
            AR2VideoBufferT *image = vs->checkoutFrameIfNewerThan(gUpdateFrameStamp);
            if (image) {
                gUpdateFrameStamp = image->time;
            
                // Copy the frame for later pattern saving.
                if (gARImageWithPatternToSave) memcpy(gARImageWithPatternToSave, image->buff, vs->getVideoWidth() * vs->getVideoHeight() * arUtilGetPixelSize(vs->getPixelFormat()));
                
                if (!gPostVideoSetupDone) {
                    initAR();
                    
                    // Setup a route for rendering the colour background image.
                    vv = new ARVideoView;
                    if (!vv) {
                        ARLOGe("Error: unable to create video view.\n");
                        quit(-1);
                    }
                    vv->setScalingMode(ARVideoView::ScalingMode::SCALE_MODE_FIT);
                    if (!vv->initWithVideoSource(*vs, contextWidth, contextHeight)) {
                        ARLOGe("Error: unable to init video view.\n");
                        quit(-1);
                    }
                    ARLOGi("Content %dx%d (wxh) will display in GL context %dx%d.\n", vs->getVideoWidth(), vs->getVideoHeight(), contextWidth, contextHeight);
                    vv->getViewport(gViewport);
                    
                    arMalloc(gARImageWithPatternToSave, ARUint8, vs->getVideoWidth() * vs->getVideoHeight() * arUtilGetPixelSize(vs->getPixelFormat()));

                    gPostVideoSetupDone = true;
                }
                if (contextWasUpdated) {
                    vv->setContextSize({contextWidth, contextHeight});
                    vv->getViewport(gViewport);
                }
                
                // Detect the markers in the video frame.
                if (arDetectMarker(gARHandle, image) < 0) {
                    quit(-1);
                }
                int markerNum = arGetMarkerNum(gARHandle);
                ARMarkerInfo *markerInfo = arGetMarker(gARHandle);
                
                int area = arGetAreaMin(gARHandle) - 1;
                target = NULL;
                for (int i = 0; i < markerNum; i++) {
                    if (markerInfo[i].area > area) {
                        area = markerInfo[i].area;
                        target = &(markerInfo[i]);
                        
                        // Get the pattern image.
                        ARdouble vertices[4][2];
                        for (i = 0; i < 4; i++) {
                            vertices[i][0] = target->vertex[(i + 2)%4][0];
                            vertices[i][1] = target->vertex[(i + 2)%4][1];
                        }
                        if (arPattGetImage2(AR_IMAGE_PROC_FRAME_IMAGE, AR_TEMPLATE_MATCHING_COLOR, gPattSize, gPattSize*AR_PATT_SAMPLE_FACTOR1,
                                            image->buff, vs->getVideoWidth(), vs->getVideoHeight(), vs->getPixelFormat(), &(vs->getCameraParameters()->paramLTf),
                                            vertices, arGetPattRatio(gARHandle), gPatternImage) == 0) {
                            gDrawPattern = true;
                        } else {
                            gDrawPattern = false;
                        }
                    }
                }
                
                // Done with frame.
                vs->checkinFrame();
                
                // The display has changed.
                drawView();
                
                arUtilSleep(1); // 1 millisecond.
            } // image
        } // vs->captureFrame()
    } // !done
    
    quit(0);

}

void reshape(int w, int h)
{
    contextWidth = w;
    contextHeight = h;
    ARLOGd("Resized to %dx%d.\n", w, h);
    contextWasUpdated = true;
}

static void quit(int rc)
{
    arDeleteHandle(gARHandle);
    
    stopVideo();
    
    SDL_Quit();
    
    exit(rc);
}

static void usage(char *com)
{
    ARPRINT("Usage: %s [options]\n", com);
    ARPRINT("Options:\n");
    ARPRINT("  --vconf <video parameter for the camera>\n");
    ARPRINT("  --cpara <camera parameter file for the camera>\n");
    ARPRINT("  --pattRatio f: Specify the proportion of the marker width/height, occupied\n");
    ARPRINT("             by the marker pattern. Range (0.0 - 1.0) (not inclusive).\n");
    ARPRINT("             (I.e. 1.0 - 2*borderSize). Default value is 0.5.\n");
    ARPRINT("  --pattSize n: Specify the number of rows and columns in the pattern space\n");
    ARPRINT("             for template (pictorial) markers.\n");
    ARPRINT("             Default value 16 (required for compatibility with ARToolKit prior\n");
    ARPRINT("             to version 5.2). Range is [16, %d] (inclusive).\n", AR_PATT_SIZE1_MAX);
    ARPRINT("  --pattCountMax n: Specify the maximum number of template (pictorial) markers\n");
    ARPRINT("             that may be loaded for use in a single matching pass.\n");
    ARPRINT("             Default value %d. Must be > 0.\n", AR_PATT_NUM_MAX);
    ARPRINT("  --borderSize f: DEPRECATED specify the width of the pattern border, as a\n");
    ARPRINT("             percentage of the marker width. Range (0.0 - 0.5) (not inclusive).\n");
    ARPRINT("             (I.e. (1.0 - pattRatio)/2). Default value is 0.25.\n");
    ARPRINT("  -border=f: Alternate syntax for --borderSize f.\n");
    ARPRINT("  --labelingMode AR_LABELING_BLACK_REGION|AR_LABELING_WHITE_REGION\n");
    ARPRINT("  --patternDetectionMode k: specify the pattern detection mode, where k is one\n");
    ARPRINT("             of: AR_TEMPLATE_MATCHING_COLOR AR_TEMPLATE_MATCHING_MONO\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption;
    float tempF;
    int tempI;

    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--vconf") == 0) {
                i++;
                vconf = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cpara") == 0) {
                i++;
                cpara = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattRatio") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 1.0f) gPattRatio = (ARdouble)tempF;
                else ARLOGe("Error: argument '%s' to --pattRatio invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattSize") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI >= 16 && tempI <= AR_PATT_SIZE1_MAX) gPattSize = tempI;
                else ARLOGe("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--borderSize") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) gPattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to --borderSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--labelingMode") == 0) {
                i++;
                if (strcmp(argv[i], "AR_LABELING_BLACK_REGION") == 0) gLabelingMode = AR_LABELING_BLACK_REGION;
                else if (strcmp(argv[i], "AR_LABELING_WHITE_REGION") == 0) gLabelingMode = AR_LABELING_WHITE_REGION;
                else ARLOGe("Error: argument '%s' to --labelingMode invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--patternDetectionMode") == 0) {
                i++;
                if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_COLOR") == 0) gPatternDetectionMode = AR_TEMPLATE_MATCHING_COLOR;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_MONO") == 0) gPatternDetectionMode = AR_TEMPLATE_MATCHING_MONO;
                else ARLOGe("Error: argument '%s' to --patternDetectionMode invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            }
        }
        if (!gotTwoPartOption) {
            // Look for single-part options.
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARPRINT("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
            } else if( strncmp(argv[i], "-loglevel=", 10) == 0 ) {
                if (strcmp(&(argv[i][10]), "DEBUG") == 0) arLogLevel = AR_LOG_LEVEL_DEBUG;
                else if (strcmp(&(argv[i][10]), "INFO") == 0) arLogLevel = AR_LOG_LEVEL_INFO;
                else if (strcmp(&(argv[i][10]), "WARN") == 0) arLogLevel = AR_LOG_LEVEL_WARN;
                else if (strcmp(&(argv[i][10]), "ERROR") == 0) arLogLevel = AR_LOG_LEVEL_ERROR;
                else usage(argv[0]);
            } else if (strncmp(argv[i], "-border=", 8) == 0) {
                if (sscanf(&(argv[i][8]), "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) gPattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to -border= invalid.\n", argv[i]);
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
    
}

static void startVideo(void)
{
    //char buf[256];
    //snprintf(buf, sizeof(buf), "%s %s", (gPreferenceCameraOpenToken ? gPreferenceCameraOpenToken : ""), (gPreferenceCameraResolutionToken ? gPreferenceCameraResolutionToken : ""));
    
    vs = new ARVideoSource;
    if (!vs) {
        ARLOGe("Error: Unable to create video source.\n");
        quit(-1);
    } else {
        vs->configure(/*buf*/vconf, false, cpara, NULL, 0);
        if (!vs->open()) {
            ARLOGe("Error: Unable to open video source.\n");
        }
    }
    gPostVideoSetupDone = false;
}

static void stopVideo(void)
{
    delete vv;
    vv = nullptr;
    delete vs;
    vs = nullptr;
}

static void initAR(void)
{
    if ((gARHandle = arCreateHandle(vs->getCameraParameters())) == NULL) {
        ARLOGe("Error: arCreateHandle.\n");
        quit(-1);
    }
    arSetPixelFormat(gARHandle, vs->getPixelFormat());
    arSetDebugMode(gARHandle, AR_DEBUG_DISABLE);
    arSetPatternDetectionMode(gARHandle, gPatternDetectionMode);
    arSetMarkerExtractionMode(gARHandle, AR_NOUSE_TRACKING_HISTORY);
    arSetLabelingMode(gARHandle, gLabelingMode);
    arSetPattRatio(gARHandle, gPattRatio);
}

static void get_cpara(ARdouble world[4][2], ARdouble vertex[4][2], ARdouble para[3][3])
{
    ARMat   *a, *b, *c;
    int     i;
    
    a = arMatrixAlloc( 8, 8 );
    b = arMatrixAlloc( 8, 1 );
    c = arMatrixAlloc( 8, 1 );
    for( i = 0; i < 4; i++ ) {
        a->m[i*16+0]  = world[i][0];
        a->m[i*16+1]  = world[i][1];
        a->m[i*16+2]  = 1.0;
        a->m[i*16+3]  = 0.0;
        a->m[i*16+4]  = 0.0;
        a->m[i*16+5]  = 0.0;
        a->m[i*16+6]  = -world[i][0] * vertex[i][0];
        a->m[i*16+7]  = -world[i][1] * vertex[i][0];
        a->m[i*16+8]  = 0.0;
        a->m[i*16+9]  = 0.0;
        a->m[i*16+10] = 0.0;
        a->m[i*16+11] = world[i][0];
        a->m[i*16+12] = world[i][1];
        a->m[i*16+13] = 1.0;
        a->m[i*16+14] = -world[i][0] * vertex[i][1];
        a->m[i*16+15] = -world[i][1] * vertex[i][1];
        b->m[i*2+0] = vertex[i][0];
        b->m[i*2+1] = vertex[i][1];
    }
    arMatrixSelfInv( a );
    arMatrixMul( c, a, b );
    for( i = 0; i < 2; i++ ) {
        para[i][0] = c->m[i*3+0];
        para[i][1] = c->m[i*3+1];
        para[i][2] = c->m[i*3+2];
    }
    para[2][0] = c->m[2*3+0];
    para[2][1] = c->m[2*3+1];
    para[2][2] = 1.0;
    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
}

static int getPatternVerticesFromMarkerVertices(const ARdouble vertex[4][2], GLfloat patternVertex[4][2])
{
    int i;
    ARdouble    world[4][2];
    ARdouble    local[4][2];
    ARdouble    para[3][3];
    ARdouble    d, xw, yw;
    ARdouble    pattRatio1, pattRatio2;
    
    world[0][0] = 100.0;
    world[0][1] = 100.0;
    world[1][0] = 100.0 + 10.0;
    world[1][1] = 100.0;
    world[2][0] = 100.0 + 10.0;
    world[2][1] = 100.0 + 10.0;
    world[3][0] = 100.0;
    world[3][1] = 100.0 + 10.0;
    for( i = 0; i < 4; i++ ) {
        local[i][0] = vertex[i][0];
        local[i][1] = vertex[i][1];
    }
    get_cpara(world, local, para);

    ARdouble pattRatio = arGetPattRatio(gARHandle);
    pattRatio1 = (1.0 - pattRatio)/2.0 * 10.0; // borderSize * 10.0
    pattRatio2 = 10.0*pattRatio;
    
    world[0][0] = 100.0 + pattRatio1;
    world[0][1] = 100.0 + pattRatio1;
    world[1][0] = 100.0 + pattRatio1 + pattRatio2;
    world[1][1] = 100.0 + pattRatio1;
    world[2][0] = 100.0 + pattRatio1 + pattRatio2;
    world[2][1] = 100.0 + pattRatio1 + pattRatio2;
    world[3][0] = 100.0 + pattRatio1;
    world[3][1] = 100.0 + pattRatio1 + pattRatio2;
    
    for (i = 0; i < 4; i++) {
        yw = world[i][1];
        xw = world[i][0];
        d = para[2][0]*xw + para[2][1]*yw + para[2][2];
        if (d == 0) return -1;
        patternVertex[i][0] = (GLfloat)(para[0][0]*xw + para[0][1]*yw + para[0][2])/d;
        patternVertex[i][1] = (GLfloat)(para[1][0]*xw + para[1][1]*yw + para[1][2])/d;
    }
    return (0);
}

static void keyboard(SDL_Keycode key)
{
    int mode, threshChange = 0;
    AR_LABELING_THRESH_MODE modea;
#define NAME_LENGTH_MAX 1024
    char name[NAME_LENGTH_MAX];
    int i;
    
    switch (key) {
        case 0x1B:                        // Quit.
        case 'Q':
        case 'q':
            quit(0);
            break;
        case ' ':
            printf("Enter filename: ");
            if (!fgets(name, sizeof(name), stdin)) break;
            
            // Trim whitespace from end.
            i = (int)strlen(name) - 1;
            while (i >= 0 && (name[i] == '\r' || name[i] == '\n' || name[i] == ' ' || name[i] == '\t')) {
                name[i] = '\0';
                i--;
            }

            if (arPattSave(gARImageWithPatternToSave, vs->getVideoWidth(), vs->getVideoHeight(), vs->getPixelFormat(), &(vs->getCameraParameters()->paramLTf),
                           arGetImageProcMode(gARHandle), target, arGetPattRatio(gARHandle), gPattSize, name) < 0) {
                ARLOGe("ERROR!!\n");
            } else {
                ARPRINT("  Saved\n");
            }
            break;
        case 'X':
        case 'x':
            mode = arGetImageProcMode(gARHandle);
            switch (mode) {
                case AR_IMAGE_PROC_FRAME_IMAGE:  mode = AR_IMAGE_PROC_FIELD_IMAGE; break;
                case AR_IMAGE_PROC_FIELD_IMAGE:
                default: mode = AR_IMAGE_PROC_FRAME_IMAGE; break;
            }
            arSetImageProcMode(gARHandle, mode);
            break;
        case 'a':
        case 'A':
            modea = arGetLabelingThreshMode(gARHandle);
            switch (modea) {
                case AR_LABELING_THRESH_MODE_MANUAL:        modea = AR_LABELING_THRESH_MODE_AUTO_MEDIAN; break;
                case AR_LABELING_THRESH_MODE_AUTO_MEDIAN:   modea = AR_LABELING_THRESH_MODE_AUTO_OTSU; break;
                case AR_LABELING_THRESH_MODE_AUTO_OTSU:     modea = AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE; break;
                case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE: modea = AR_LABELING_THRESH_MODE_AUTO_BRACKETING; break;
                case AR_LABELING_THRESH_MODE_AUTO_BRACKETING:
                default: modea = AR_LABELING_THRESH_MODE_MANUAL; break;
            }
            arSetLabelingThreshMode(gARHandle, modea);
            break;
        case '-':
            threshChange = -5;
            break;
        case '+':
        case '=':
            threshChange = +5;
            break;
        case 'D':
        case 'd':
            mode = arGetDebugMode(gARHandle);
            arSetDebugMode(gARHandle, !mode);
            break;
        case '?':
        case '/':
            gShowHelp++;
            if (gShowHelp > 1) gShowHelp = 0;
            break;
        case 'm':
        case 'M':
            gShowMode = !gShowMode;
            break;
        default:
            break;
    }
    if (threshChange) {
        int threshhold = arGetLabelingThresh(gARHandle);
        threshhold += threshChange;
        if (threshhold < 0) threshhold = 0;
        if (threshhold > 255) threshhold = 255;
        arSetLabelingThresh(gARHandle, threshhold);
    }
    
}

//
// This function is called when the window needs redrawing.
//
static void drawView(void)
{
    SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);
    
    // Clean the OpenGL context.
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (arGetDebugMode(gARHandle) == AR_DEBUG_DISABLE) {
        vv->draw(vs);
    } else {
        vv->drawDebugImage(gARHandle->labelInfo.bwImage, (arGetImageProcMode(gARHandle) == AR_IMAGE_PROC_FIELD_IMAGE ? 1 : 0));
    }
    
    
    // 2D overlays in video frame.
    glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)gARHandle->xsize, 0, (GLdouble)gARHandle->ysize, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    EdenGLFontSetViewSize(contextWidth, contextHeight);
    
    // Draw the largest detected marker.
    if (target) {
        GLfloat vertices[6][2];
        glLoadIdentity();
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertexPointer(2, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        // Corners.
        for (int i = 0; i < 5; i++) {
            vertices[i][0] = (float)target->vertex[(i + 2)%4][0];
            vertices[i][1] = ((float)gARHandle->ysize - (float)target->vertex[(i + 2)%4][1]);
        }
        // Centre.
        vertices[5][0] = (float)target->pos[0];
        vertices[5][1] = ((float)gARHandle->ysize - (float)target->pos[1]);
        glDrawArrays(GL_LINE_STRIP, 0, 6);
        
        // Outline pattern space in blue.
        glColor3f(0.0f, 0.0f, 1.0f);
        getPatternVerticesFromMarkerVertices((const ARdouble (*)[2])target->vertex, vertices);
        for (int i = 0; i < 4; i++) vertices[i][1] = (float)gARHandle->ysize - vertices[i][1];
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // 2D overlays in context space.
    glViewport(0, 0, contextWidth, contextHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)contextWidth, 0, (GLdouble)contextHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw the unwarped pattern.
    if (gDrawPattern) {
        int zoom = 8;
        glLoadIdentity();
        glRasterPos2f((float)(contextWidth - gPattSize*zoom) - 4.0f, (float)(gPattSize*zoom) + 4.0f);
        glPixelZoom((float)zoom, (float)-zoom);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glDrawPixels(gPattSize, gPattSize, GL_BGR_EXT, GL_UNSIGNED_BYTE, gPatternImage);
        glPixelZoom(1.0f, 1.0f);
    }

    // Draw help text and mode.
    if (gShowMode) {
        printMode();
    }
    if (gShowHelp) {
        if (gShowHelp == 1) {
            printHelpKeys();
        }
    }
    
    SDL_GL_SwapWindow(gSDLWindow);
}

//
// The following functions provide the onscreen help text and mode info.
//


static void drawBackground(const float width, const float height, const float x, const float y)
{
    GLfloat vertices[4][2];
    
    vertices[0][0] = x; vertices[0][1] = y;
    vertices[1][0] = width + x; vertices[1][1] = y;
    vertices[2][0] = width + x; vertices[2][1] = height + y;
    vertices[3][0] = x; vertices[3][1] = height + y;
    glLoadIdentity();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);    // 50% transparent black.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
    //glLineWidth(1.0f);
    //glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

static void printHelpKeys()
{
    GLfloat bw, bh;
    const char *helpText[] = {
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode."
    };
#define helpTextLineCount (sizeof(helpText)/sizeof(char *))
    
    bw = EdenGLFontGetBlockWidth((const unsigned char **)helpText, helpTextLineCount);
    bh = EdenGLFontGetBlockHeight((const unsigned char **)helpText, helpTextLineCount);
    drawBackground(bw, bh, 2.0f, 2.0f);
    glDisable(GL_BLEND);
    EdenGLFontDrawBlock(0, NULL, (const unsigned char **)helpText, helpTextLineCount, 2.0f, 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
}

static void printMode()
{
    int len, line;
    char text[256];
    const char *text_p;
    
    glColor3ub(255, 255, 255);
    line = 1;
    
    // Image size and processing mode.
    if (arGetImageProcMode(gARHandle) == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
    else text_p = "even field only";
    snprintf(text, sizeof(text), "Processing %dx%d video frames %s", vs->getVideoWidth(), vs->getVideoHeight(), text_p);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
    // Threshold mode, and threshold, if applicable.
    AR_LABELING_THRESH_MODE threshMode = arGetLabelingThreshMode(gARHandle);
    switch (threshMode) {
        case AR_LABELING_THRESH_MODE_MANUAL: text_p = "MANUAL"; break;
        case AR_LABELING_THRESH_MODE_AUTO_MEDIAN: text_p = "AUTO_MEDIAN"; break;
        case AR_LABELING_THRESH_MODE_AUTO_OTSU: text_p = "AUTO_OTSU"; break;
        case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE: text_p = "AUTO_ADAPTIVE"; break;
        case AR_LABELING_THRESH_MODE_AUTO_BRACKETING: text_p = "AUTO_BRACKETING"; break;
        default: text_p = "UNKNOWN"; break;
    }
    snprintf(text, sizeof(text), "Threshold mode: %s", text_p);
    if (threshMode != AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE) {
        len = (int)strlen(text);
        snprintf(text + len, sizeof(text) - len, ", thresh=%d", arGetLabelingThresh(gARHandle));
    }
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
    // Border size, image processing mode, pattern detection mode.
    snprintf(text, sizeof(text), "Border: %0.2f%%", arGetBorderSize(gARHandle)*100.0);
    switch (arGetPatternDetectionMode(gARHandle)) {
        case AR_TEMPLATE_MATCHING_COLOR: text_p = "Colour template (pattern)"; break;
        case AR_TEMPLATE_MATCHING_MONO: text_p = "Mono template (pattern)"; break;
        default: text_p = "UNKNOWN"; break;
    }
    len = (int)strlen(text);
    snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s", text_p);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", contextWidth, contextHeight);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
}

