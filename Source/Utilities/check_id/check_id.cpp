/*
 *  check_id.cpp
 *  artoolkitX
 *
 *  Allows visual verification of artoolkitX pattern ID's, including
 *  failure modes, ID numbers, and poses.
 *
 *  Press '?' while running for help on available key commands.
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
 *  Author(s): Philip Lamb.
 *
 */


// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#  define snprintf _snprintf
#endif
#include <stdlib.h>					// malloc(), free()
#include <inttypes.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/paramGL.h>
#include <ARX/AR/arMulti.h>
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
//	Constants
// ============================================================================

#define FONT_SIZE 18.0f

#define VIEW_DISTANCE_MIN		1.0			// Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX		10000.0		// Objects further away from the camera than this will not be displayed.

typedef struct _cutoffPhaseColours {
    int cutoffPhase;
    GLubyte colour[3];
} cutoffPhaseColours_t ;

const cutoffPhaseColours_t cutoffPhaseColours[AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT] = {
    {AR_MARKER_INFO_CUTOFF_PHASE_NONE,                               {0xff, 0x0,  0x0 }},  // Red.
    {AR_MARKER_INFO_CUTOFF_PHASE_PATTERN_EXTRACTION,                 {0x95, 0xd6, 0xf6}},  // Light blue.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_GENERIC,                      {0x0,  0x0,  0xff}},  // Blue.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONTRAST,                     {0x99, 0x66, 0x33}},  // Brown.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_NOT_FOUND,            {0x7f, 0x0,  0x7f}},  // Purple.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_EDC_FAIL,             {0xff, 0x0,  0xff}},  // Magenta.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE,                   {0x0,  0xff, 0x0 }},  // Green.
    {AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR,                         {0xff, 0x7f, 0x0 }},  // Orange.
    {AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR_MULTI,                   {0xff, 0xff, 0x0 }},  // Yellow.
    {AR_MARKER_INFO_CUTOFF_PHASE_HEURISTIC_TROUBLESOME_MATRIX_CODES, {0xc6, 0xdc, 0x6a}},  // Khaki.
};

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static char *cpara = NULL;
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
static AR_MATRIX_CODE_TYPE gMatrixCodeType = AR_MATRIX_CODE_TYPE_DEFAULT;
static int gLabelingMode = AR_DEFAULT_LABELING_MODE;
static int gPatternDetectionMode = AR_DEFAULT_PATTERN_DETECTION_MODE;
static int gPattSize = AR_PATT_SIZE1;
static int gPattCountMax = AR_PATT_NUM_MAX;


// Video acquisition and rendering.
static ARVideoSource *vs = nullptr;
static ARVideoView *vv = nullptr;
static bool gPostVideoSetupDone = false;
static AR2VideoTimestampT gUpdateFrameStamp = {0, 0};
static int gARTImageSavePlease = FALSE;
static AR2VideoTimestampT gFPSCalcFrameStamp = {0, 0};
static double gFPS = 0.0;

// Marker detection.
#define CHECK_ID_MULTIMARKERS_MAX 16
static int patt_names_count = 0;
static char *patt_names[CHECK_ID_MULTIMARKERS_MAX] = {NULL};
static ARHandle		*gARHandle = NULL;
static ARPattHandle	*gARPattHandle = NULL;
static long			gFrameCount = 0;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandle = NULL;
static int          gRobustFlag = TRUE;
static int gMultiConfigCount = 0;
static ARMultiMarkerInfoT *gMultiConfigs[CHECK_ID_MULTIMARKERS_MAX] = {NULL};
static ARdouble gMultiErrs[CHECK_ID_MULTIMARKERS_MAX];

// Drawing.
// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static SDL_Window* gSDLWindow = NULL;
static ARdouble gProjection[16];
static int gShowHelp = 1;
static int gShowMode = 1;
static GLint gViewport[4];
static int gDrawPatternSize = 0;
static ARUint8 ext_patt[AR_PATT_SIZE2_MAX*AR_PATT_SIZE2_MAX*3]; // Holds unwarped pattern extracted from image.


// ============================================================================
//	Function prototypes.
// ============================================================================

static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void startVideo(void);
static void stopVideo(void);
static void initAR(void);
static int setupMarkers(const int patt_count, const char *patt_names[], ARMultiMarkerInfoT *multiConfigs[], ARHandle *arhandle, ARPattHandle **pattHandle_p);
static void quit(int rc);
static void reshape(int w, int h);
static void keyboard(SDL_Keycode key);
static void drawView(void);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys();
static void printMode();

// ============================================================================
//	Functions
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
    arUtilTimerReset();
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
                
                // FPS calculations.
                gFrameCount++;
                if (gFrameCount % 10 == 0) {
                    double d = (double)(gUpdateFrameStamp.sec - gFPSCalcFrameStamp.sec) + (double)((int32_t)gUpdateFrameStamp.usec - (int32_t)gFPSCalcFrameStamp.usec)*0.000001;
                    if (d) gFPS = 1.0/d*10;
                    gFPSCalcFrameStamp = gUpdateFrameStamp;
                }

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
                    gPostVideoSetupDone = true;
                }
                if (contextWasUpdated) {
                    vv->setContextSize({contextWidth, contextHeight});
                    vv->getViewport(gViewport);
                }
                
                int i, j;
                static int imageNumber = 0;
                
                if (gARTImageSavePlease) {
                    char imageNumberText[15];
                    sprintf(imageNumberText, "image-%04d.jpg", imageNumber++);
                    if (arVideoSaveImageJPEG(gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, image->buff, imageNumberText, 75, 0) < 0) {
                        ARLOGe("Error saving video image.\n");
                    }
                    gARTImageSavePlease = FALSE;
                }
                
                // Detect the markers in the video frame.
                if (arDetectMarker(gARHandle, image) < 0) {
                    quit(-1);
                }
                
                // If  marker config files were specified, evaluate detected patterns against them now.
                for (i = 0; i < gMultiConfigCount; i++) {
                    if (gRobustFlag) gMultiErrs[i] = arGetTransMatMultiSquareRobust(gAR3DHandle, arGetMarker(gARHandle), arGetMarkerNum(gARHandle), gMultiConfigs[i]);
                    else gMultiErrs[i] = arGetTransMatMultiSquare(gAR3DHandle, arGetMarker(gARHandle), arGetMarkerNum(gARHandle), gMultiConfigs[i]);
                    //if (gMultiConfigs[i]->prevF != 0) ARLOGe("Found multimarker set %d, err=%0.3f\n", i, gMultiErrs[i]);
                }
                
                
                // For matrix mode, draw the pattern image of the largest marker.
                int pattDetectMode = arGetPatternDetectionMode(gARHandle);
                if (pattDetectMode == AR_MATRIX_CODE_DETECTION || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
                    
                    int markerNum = arGetMarkerNum(gARHandle);
                    ARMarkerInfo *markerInfo = arGetMarker(gARHandle);
                    
                    int area = 0, biggestMarker = -1;
                    
                    for (j = 0; j < markerNum; j++) if (markerInfo[j].area > area) {
                        area = markerInfo[j].area;
                        biggestMarker = j;
                    }
                    if (area >= arGetAreaMin(gARHandle)) {
                        
                        // Reorder vertices based on dir.
                        ARdouble vertexUpright[4][2];
                        for (i = 0; i < 4; i++) {
                            int dir = markerInfo[biggestMarker].dir;
                            vertexUpright[i][0] = markerInfo[biggestMarker].vertex[(i + 4 - dir)%4][0];
                            vertexUpright[i][1] = markerInfo[biggestMarker].vertex[(i + 4 - dir)%4][1];
                        }
                        int imageProcMode = arGetImageProcMode(gARHandle);
                        ARdouble pattRatio = arGetPattRatio(gARHandle);
                        AR_MATRIX_CODE_TYPE matrixCodeType = arGetMatrixCodeType(gARHandle);
                        if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID) {
                            gDrawPatternSize = 14;
                            arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, gDrawPatternSize, gDrawPatternSize * AR_PATT_SAMPLE_FACTOR2,
                                            image->buff, gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, &gARHandle->arParamLT->paramLTf, vertexUpright, (ARdouble)14/(ARdouble)(14 + 2), ext_patt);
                        } else {
                            gDrawPatternSize = matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK;
                            arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, gDrawPatternSize, gDrawPatternSize * AR_PATT_SAMPLE_FACTOR2,
                                            image->buff, gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, &gARHandle->arParamLT->paramLTf, vertexUpright, pattRatio, ext_patt);
                        }
                    } else {
                        gDrawPatternSize = 0;
                    }
                } else {
                    gDrawPatternSize = 0;
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
    int i;
    
    ar3DDeleteHandle(&gAR3DHandle);
    arPattDetach(gARHandle);
    for (i = 0; i < gMultiConfigCount; i++) {
        arMultiFreeConfig(gMultiConfigs[i]);
    }
    if (gARPattHandle) arPattDeleteHandle(gARPattHandle);
    arDeleteHandle(gARHandle);
    
    stopVideo();

    SDL_Quit();
    
    exit(rc);
}


static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption;
    float tempF;
    int tempI;

    //
    // Command-line options.
    //
    
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
            } else if (strcmp(argv[i], "--pattCountMax") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI > 0) gPattCountMax = tempI;
                else ARLOGe("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--borderSize") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) gPattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to --borderSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--matrixCodeType") == 0) {
                i++;
                if (strcmp(argv[i], "AR_MATRIX_CODE_3x3") == 0) gMatrixCodeType = AR_MATRIX_CODE_3x3;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_3x3_HAMMING63") == 0) gMatrixCodeType = AR_MATRIX_CODE_3x3_HAMMING63;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_3x3_PARITY65") == 0) gMatrixCodeType = AR_MATRIX_CODE_3x3_PARITY65;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4") == 0) gMatrixCodeType = AR_MATRIX_CODE_4x4;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4_BCH_13_9_3") == 0) gMatrixCodeType = AR_MATRIX_CODE_4x4_BCH_13_9_3;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4_BCH_13_5_5") == 0) gMatrixCodeType = AR_MATRIX_CODE_4x4_BCH_13_5_5;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_5x5") == 0) gMatrixCodeType = AR_MATRIX_CODE_5x5;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_6x6") == 0) gMatrixCodeType = AR_MATRIX_CODE_6x6;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_GLOBAL_ID") == 0) gMatrixCodeType = AR_MATRIX_CODE_GLOBAL_ID;
                else ARLOGe("Error: argument '%s' to --matrixCodeType invalid.\n", argv[i]);
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
                else if (strcmp(argv[i], "AR_MATRIX_CODE_DETECTION") == 0) gPatternDetectionMode = AR_MATRIX_CODE_DETECTION;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX") == 0) gPatternDetectionMode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_MONO_AND_MATRIX") == 0) gPatternDetectionMode = AR_TEMPLATE_MATCHING_MONO_AND_MATRIX;
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
                if (patt_names_count < CHECK_ID_MULTIMARKERS_MAX) {
                    patt_names[patt_names_count] = argv[i];
                    patt_names_count++;
                }
            //} else {
            //    ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
            //    usage(argv[0]);
            }
        }
        i++;
    }
}

static void usage(char *com)
{
    ARPRINT("Usage: %s [options] [Multimarker config. file [Multimarker config. file 2]]\n", com);
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
    ARPRINT("  --matrixCodeType k: specify the type of matrix code used, where k is one of:\n");
    ARPRINT("             AR_MATRIX_CODE_3x3 AR_MATRIX_CODE_3x3_HAMMING63\n");
    ARPRINT("             AR_MATRIX_CODE_3x3_PARITY65 AR_MATRIX_CODE_4x4\n");
    ARPRINT("             AR_MATRIX_CODE_4x4_BCH_13_9_3 AR_MATRIX_CODE_4x4_BCH_13_5_5\n");
    ARPRINT("             AR_MATRIX_CODE_5x5 AR_MATRIX_CODE_6x6 AR_MATRIX_CODE_GLOBAL_ID\n");
    ARPRINT("  --labelingMode AR_LABELING_BLACK_REGION|AR_LABELING_WHITE_REGION\n");
    ARPRINT("  --patternDetectionMode k: specify the pattern detection mode, where k is one\n");
    ARPRINT("             of: AR_TEMPLATE_MATCHING_COLOR AR_TEMPLATE_MATCHING_MONO\n");
    ARPRINT("             AR_MATRIX_CODE_DETECTION AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX\n");
    ARPRINT("             AR_TEMPLATE_MATCHING_MONO_AND_MATRIX\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
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
    arglCameraFrustumRH(&(vs->getCameraParameters()->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, gProjection);

    if ((gARHandle = arCreateHandle(vs->getCameraParameters())) == NULL) {
        ARLOGe("Error: arCreateHandle.\n");
        quit(-1);
    }
    arSetPixelFormat(gARHandle, vs->getPixelFormat());
    arSetDebugMode(gARHandle, AR_DEBUG_DISABLE);
    arSetPatternDetectionMode(gARHandle, gPatternDetectionMode);
    arSetLabelingMode(gARHandle, gLabelingMode);
    arSetPattRatio(gARHandle, gPattRatio);
    arSetMatrixCodeType(gARHandle, gMatrixCodeType);
    if ((gAR3DHandle = ar3DCreateHandle(&(vs->getCameraParameters()->param))) == NULL) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        quit(-1);
    }
    
    //
    // Load marker(s).
    //
    
    if (!setupMarkers(patt_names_count, (const char **)patt_names, gMultiConfigs, gARHandle, &gARPattHandle)) {
        ARLOGe("main(): Unable to set up AR marker(s).\n");
        quit(-1);
    }
    gMultiConfigCount = patt_names_count;
}

static int setupMarkers(const int patt_count, const char *patt_names[], ARMultiMarkerInfoT *multiConfigs[], ARHandle *arhandle, ARPattHandle **pattHandle_p)
{
    int i;
    
    if (!patt_count) {
        // Default behaviour is to default to matrix mode.
        *pattHandle_p = NULL;
        arSetPatternDetectionMode( arhandle, AR_MATRIX_CODE_DETECTION ); // If no markers specified, default to matrix mode.
    } else {
        // If marker configs have been specified, attempt to load them.
        
        int mode = -1, nextMode;
        
        // Need a pattern handle because the config file could specify matrix or template markers.
        if ((*pattHandle_p = arPattCreateHandle2(gPattSize, gPattCountMax)) == NULL) {
            ARLOGe("setupMarkers(): Error: arPattCreateHandle2.\n");
            return (FALSE);
        }
        
        for (i = 0; i < patt_count; i++) {
            
            if (!(multiConfigs[i] = arMultiReadConfigFile(patt_names[i], *pattHandle_p))) {
                ARLOGe("setupMarkers(): Error reading multimarker config file '%s'.\n", patt_names[i]);
                for (i--; i >= 0; i--) {
                    arMultiFreeConfig(multiConfigs[i]);
                }
                arPattDeleteHandle(*pattHandle_p);
                return (FALSE);
            }

            if (multiConfigs[i]->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE) {
                nextMode = AR_TEMPLATE_MATCHING_COLOR;
            } else if (multiConfigs[i]->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX) {
                nextMode = AR_MATRIX_CODE_DETECTION;
            } else { // AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX or mixed.
                nextMode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
            }
            
            if (mode == -1) {
                mode = nextMode;
            } else if (mode != nextMode) {
                mode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
            }
        }
        arSetPatternDetectionMode(arhandle, mode);
        
        arPattAttach(arhandle, *pattHandle_p);
    }
    
	return (TRUE);
}

static void keyboard(SDL_Keycode key)
{
	int mode, threshChange = 0;
    AR_LABELING_THRESH_MODE modea;
	
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			quit(0);
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
		case 'C':
		case 'c':
			ARLOGe("*** Camera - %f (frame/sec)\n", (double)gFrameCount/arUtilTimer());
			gFrameCount = 0;
			arUtilTimerReset();
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
        case 's':
        case 'S':
            if (!gARTImageSavePlease) gARTImageSavePlease = TRUE;
            break;
		case '?':
		case '/':
            gShowHelp++;
            if (gShowHelp > 2) gShowHelp = 0;
			break;
        case 'r':
        case 'R':
            gRobustFlag = !gRobustFlag;
            break;
        case 't':
        case 'T':
            mode = arGetCornerRefinementMode(gARHandle);
            arSetCornerRefinementMode(gARHandle, !mode);
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

static void drawAxes()
{
    GLfloat vertices[6][3] = {
        {0.0f, 0.0f, 0.0f}, {10.0f,  0.0f,  0.0f},
        {0.0f, 0.0f, 0.0f},  {0.0f, 10.0f,  0.0f},
        {0.0f, 0.0f, 0.0f},  {0.0f,  0.0f, 10.0f}
    };
    GLubyte colours[6][4] = {
        {255,0,0,255}, {255,0,0,255},
        {0,255,0,255}, {0,255,0,255},
        {0,0,255,255}, {0,0,255,255}
    };

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colours);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glLineWidth(2.0f);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, 6);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

static void pointTransformf(float v[4], const float M[16])
{
    float v0[4];
    
    // Copy v to v0 so that result can be returned in v.
    v0[0] = v[0]; v0[1] = v[1]; v0[2] = v[2]; v0[3] = v[3];
    
    v[0] = M[ 0]*v0[0] + M[ 4]*v0[1] + M[ 8]*v0[2] + M[12]*v0[3];
    v[1] = M[ 1]*v0[0] + M[ 5]*v0[1] + M[ 9]*v0[2] + M[13]*v0[3];
    v[2] = M[ 2]*v0[0] + M[ 6]*v0[1] + M[10]*v0[2] + M[14]*v0[3];
    v[2] = M[ 3]*v0[0] + M[ 7]*v0[1] + M[11]*v0[2] + M[15]*v0[3];
}

static bool pointProjectf(float objX, float objY, float objZ, const float model[16], const float proj[16], int view[4], float *winX, float *winY, float *winZ)
{
    float v[4];
    v[0] = objX;
    v[1] = objY;
    v[2] = objZ;
    v[3] = 1.0f;
    pointTransformf(v, model);
    pointTransformf(v, proj);
    if (v[3] == 0.0f) return false;
    v[0] /= v[3];
    v[1] /= v[3];
    v[2] /= v[3];
    if (winX) *winX = view[0] + view[2]*(v[0]*0.5f + 0.5f);
    if (winY) *winY = view[1] + view[3]*(v[1]*0.5f + 0.5f);
    if (winZ) *winZ = v[2]*0.5f + 0.5f;
    return true;
}


//
// This function is called when the window needs redrawing.
//
static void drawView(void)
{
    ARdouble m[16];
    int i, j, k;
    GLubyte pixels[300];
    char text[256];
    float winX, winY, winZ;
    int showMErr[CHECK_ID_MULTIMARKERS_MAX];
    GLdouble MX[CHECK_ID_MULTIMARKERS_MAX];
    GLdouble MY[CHECK_ID_MULTIMARKERS_MAX];
    
    SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);
    
    // Clean the OpenGL context.
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (arGetDebugMode(gARHandle) == AR_DEBUG_DISABLE) {
        vv->draw(vs);
    } else {
        vv->drawDebugImage(gARHandle->labelInfo.bwImage, (arGetImageProcMode(gARHandle) == AR_IMAGE_PROC_FIELD_IMAGE ? 1 : 0));
    }

    if (gMultiConfigCount) {
        glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
        glLoadMatrixf(gProjection);
#else
        glLoadMatrixd(gProjection);
#endif
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);

        // If we have multi-configs, show their origin onscreen.
        for (k = 0; k < gMultiConfigCount; k++) {
            showMErr[k] = FALSE;
            if (gMultiConfigs[k]->prevF != 0) {
                arglCameraViewRH((const ARdouble (*)[4])gMultiConfigs[k]->trans, m, 1.0);
#ifdef ARDOUBLE_IS_FLOAT
                glLoadMatrixf(m);
#else
                glLoadMatrixd(m);
#endif
                drawAxes();
#ifndef ARDOUBLE_IS_FLOAT
                float p0[16], m0[16];
                for (i = 0; i < 16; i++) p0[i] = (float)gProjection[i];
                for (i = 0; i < 16; i++) m0[i] = (float)m[i];
                if (pointProjectf(0, 0, 0, m0, p0, gViewport, &winX, &winY, &winZ) == GL_TRUE)
#else
                if (pointProjectf(0, 0, 0, m, gProjection, gViewport, &winX, &winY, &winZ) == GL_TRUE)
#endif
                {
                    showMErr[k] = TRUE;
                    MX[k] = winX; MY[k] = winY;
                }
            }
            
        } // for k
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

    int pattDetectMode = arGetPatternDetectionMode(gARHandle);
    AR_MATRIX_CODE_TYPE matrixCodeType = arGetMatrixCodeType(gARHandle);

    // For all markers, draw onscreen position.
    // Colour based on cutoffPhase.
    GLfloat vertices[6][2];
    glLoadIdentity();
    glLineWidth(2.0f);
    for (j = 0; j < gARHandle->marker_num; j++) {
        glVertexPointer(2, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glColor3ubv(cutoffPhaseColours[gARHandle->markerInfo[j].cutoffPhase].colour);
        // Corners, starting in top-left.
        for (i = 0; i < 5; i++) {
            int dir = gARHandle->markerInfo[j].dir;
            vertices[i][0] = (float)gARHandle->markerInfo[j].vertex[(i + 4 - dir)%4][0];
            vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].vertex[(i + 4 - dir)%4][1]);
        }
        // Centre.
        vertices[i][0] = (float)gARHandle->markerInfo[j].pos[0];
        vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].pos[1]);
        glDrawArrays(GL_LINE_STRIP, 0, 6);
        // For markers that have been identified, draw the ID number.
        if (gARHandle->markerInfo[j].id >= 0) {
            glColor3ub(255, 0, 0);
            if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID && (pattDetectMode == AR_MATRIX_CODE_DETECTION || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX)) snprintf(text, sizeof(text), "%" PRIu64 " (err=%d)", gARHandle->markerInfo[j].globalID, gARHandle->markerInfo[j].errorCorrected);
            else snprintf(text, sizeof(text), "%d", gARHandle->markerInfo[j].id);
            EdenGLFontDrawLine(0, NULL, (unsigned char *)text, (float)gARHandle->markerInfo[j].pos[0], ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].pos[1]), H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    
    // For matrix mode, draw the pattern image of the largest marker.
    if (gDrawPatternSize) {
        int zoom = 4;
        glRasterPos2f((float)(contextWidth - gDrawPatternSize*zoom) - 4.0f, (float)(gDrawPatternSize*zoom) + 4.0f);
        glPixelZoom((float)zoom, (float)-zoom);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glDrawPixels(gDrawPatternSize, gDrawPatternSize, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt);
        glPixelZoom(1.0f, 1.0f);
    }

    
    // Draw error value for multimarker pose.
    for (k = 0; k < gMultiConfigCount; k++) {
        if (showMErr[k]) {
            snprintf(text, sizeof(text), "err=%0.3f", gMultiErrs[k]);
             EdenGLFontDrawLine(0, NULL, (unsigned char *)text, MX[k], MY[k], H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
        }
    }
    
    // 2D overlays in context space.
    glViewport(0, 0, contextWidth, contextHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)contextWidth, 0, (GLdouble)contextHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw help text and mode.
    if (gShowMode) {
        printMode();
    }
    if (gShowHelp) {
        if (gShowHelp == 1) {
            printHelpKeys();
        } else if (gShowHelp == 2) {
            GLfloat bw = EdenGLFontGetBlockWidth((const unsigned char **)arMarkerInfoCutoffPhaseDescriptions, AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT);
            bw += 12.0f; // Space for color block.
            GLfloat bh = EdenGLFontGetBlockHeight((const unsigned char **)arMarkerInfoCutoffPhaseDescriptions, AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT);
            GLfloat lineh = EdenGLFontGetSize() * EdenGLFontGetLineSpacing();
            drawBackground(bw, bh, 2.0f, 2.0f);
            glDisable(GL_BLEND);
            
            // Draw the colour block and text, line by line.
            for (i = 0; i < AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT; i++) {
                for (j = 0; j < 300; j += 3) {
                    pixels[j    ] = cutoffPhaseColours[i].colour[0];
                    pixels[j + 1] = cutoffPhaseColours[i].colour[1];
                    pixels[j + 2] = cutoffPhaseColours[i].colour[2];
                }
                glRasterPos2f(2.0f, (AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT - 1 - i) * lineh + 2.0f);
                glPixelZoom(1.0f, 1.0f);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glDrawPixels(10, 10, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                EdenGLFontDrawBlock(0, NULL, (const unsigned char **)arMarkerInfoCutoffPhaseDescriptions, AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT, 14.0f, 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
             }
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
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
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
        " ? or /        Show/hide this help / marker cutoff phase key.",
        " q or [esc]    Quit program.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " c             Calulcate frame rate.",
        " r             Toggle robust multi-marker mode on/off.",
        " t             Toggle corner refinement  mode on/off.",
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
    int len, line, textPatternCount;
    char text[256];
    const char *text_p;

    glColor3ub(255, 255, 255);
    line = 1;
    
    // Image size, frame rate, and processing mode.
	if (arGetImageProcMode(gARHandle) == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
	else text_p = "even field only";
    snprintf(text, sizeof(text), "Processing %dx%d@%.1f fps video frames %s", vs->getVideoWidth(), vs->getVideoHeight(), gFPS, text_p);
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
    textPatternCount = 0;
    switch (arGetPatternDetectionMode(gARHandle)) {
        case AR_TEMPLATE_MATCHING_COLOR: text_p = "Colour template (pattern)"; break;
        case AR_TEMPLATE_MATCHING_MONO: text_p = "Mono template (pattern)"; break;
        case AR_MATRIX_CODE_DETECTION: text_p = "Matrix (barcode)"; textPatternCount = -1; break;
        case AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX: text_p = "Colour template + Matrix (2 pass, pattern + barcode)"; break;
        case AR_TEMPLATE_MATCHING_MONO_AND_MATRIX: text_p = "Mono template + Matrix (2 pass, pattern + barcode "; break;
        default: text_p = "UNKNOWN"; textPatternCount = -1; break;
    }
    if (textPatternCount != -1) textPatternCount = gARPattHandle->patt_num;
    len = (int)strlen(text);
    if (textPatternCount != -1) snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s, %d patterns loaded", text_p, textPatternCount);
    else snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s", text_p);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
    // Robust mode.
    if (gMultiConfigCount) {
        snprintf(text, sizeof(text), "Robust multi-marker pose estimation %s", (gRobustFlag ? "ON" : "OFF"));
        if (gRobustFlag) {
            ARdouble prob;
            icpGetInlierProbability(gAR3DHandle->icpHandle, &prob);
            len = (int)strlen(text);
            snprintf(text + len, sizeof(text) - len, ", inliner prob. %0.1f%%", prob*100.0f);
        }
        EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
        line++;
    }

    // Corner refinement mode.
    if (arGetCornerRefinementMode(gARHandle) == AR_CORNER_REFINEMENT_ENABLE) {
        EdenGLFontDrawLine(0, NULL, (unsigned char *)"Subpixel corner refinement enabled", 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
        line++;
    }

    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", contextWidth, contextHeight);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
    
}
