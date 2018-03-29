/*
 *  genMarkerSet.c
 *  artoolkitX
 *
 *  Identifies markers in texture image and generates marker set files.
 *
 *  Run with "--help" parameter to see usage.
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

// ============================================================================
//    Includes
// ============================================================================

#ifdef _WIN32
#  include <windows.h>
//#  define snprintf _snprintf
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#include <stdlib.h>                    // malloc(), free()
#include <ARX/AR/ar.h>
#include <ARX/ARG/arg.h>
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/marker.h>
#include <ARX/AR2/util.h>
#include <ARX/AR2/imageFormat.h>
#include <ARX/ARVideo/video.h>  // ARVideoLumaInfo
#include <SDL2/SDL.h>
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#elif defined(__linux) || defined(_WIN32)
#  include <GL/gl.h>
#endif
#include <Eden/EdenGLFont.h>
#include <vector>
#include <algorithm>

// ============================================================================
//    Constants
// ============================================================================

#define FONT_SIZE 18.0f
#define THRESH 100

enum {
    E_NO_ERROR = 0,
    E_BAD_PARAMETER = 64,
    E_INPUT_DATA_ERROR = 65,
    E_USER_INPUT_CANCELLED = 66,
    E_BACKGROUND_OPERATION_UNSUPPORTED = 69,
    E_DATA_PROCESSING_ERROR = 70,
    E_UNABLE_TO_DETACH_FROM_CONTROLLING_TERMINAL = 71,
    E_GENERIC_ERROR = 255
};

// ============================================================================
//    Global variables
// ============================================================================

// Preferences.
static ARdouble gPattRatio = (ARdouble)AR_PATT_RATIO;
static int gLabelingMode = AR_DEFAULT_LABELING_MODE;
static int gPattSize = AR_PATT_SIZE1;
static int gPattCountMax = AR_PATT_NUM_MAX;
static char            *inputFilePath = NULL;

// Input image.
static AR2ImageSetT    *imageSet = NULL;
static AR2JpegImageT   *jpegImage = NULL;
static AR2VideoBufferT  image = {0};
static ARVideoLumaInfo *imageLumaInfo = NULL;
static int              imageWidth = 0, imageHeight = 0; // Input image size.
static AR_PIXEL_FORMAT  imagePixelFormat = AR_PIXEL_FORMAT_INVALID;
static float            imageDPI = -1.0f;
static double           imageZoom = 1.0f;

// Marker detection.
static ARParam          cparam;
static ARParamLT       *cparamLT = NULL;
static ARHandle        *gARHandle = NULL;
static float           *gMarkerWidths = NULL; // Allocated in detect() to an array of length arGetMarkerNum(gARHandle).
static std::vector<ARMarkerInfo *> gSelectedMarkers;


static int              detectedMarkerNum = 0;

// Drawing.
// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static SDL_Window* gSDLWindow = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglContextSettings = NULL;
static int gShowHelp = 1;
static int gShowMode = 1;
static GLint gViewport[4];

static char             exitcode = -1;
#define EXIT(c) {exitcode=c;exit(c);}

// ============================================================================
//    Function prototypes.
// ============================================================================

static void loadImage(void);
static void quit(int rc);
static void reshape(int w, int h);
static void keyboard(SDL_Keycode key);
static void mouse(Uint8 button, Sint32 x, Sint32 y); // button = SDL_BUTTON_LEFT|SDL_BUTTON_MIDDLE|SDL_BUTTON_RIGHT etc.
static int detect(float **markerWidths_p);
static void saveFiles(void);
static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void check_square(ARMarkerInfo *markerInfo, int markerNum, float *markerWidths);
static void pixel2mm(float px, float py, float *mx, float *my);
//static void mm2pixel( float mx, float my, float *px, float *py );
static void drawView(void);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys(void);
static void printMode(void);

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
    
    loadImage();

    // Main loop.
    bool done = false;
    while (!done) {
        
        SDL_Event ev;
        while (SDL_WaitEvent(&ev)) {
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
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.windowID == SDL_GetWindowID(gSDLWindow)) {
                mouse(ev.button.button, ev.button.x, ev.button.y);
            }
        }
        
    } // !done
    
    quit(0);
}

void reshape(int w, int h)
{
    contextWidth = w;
    contextHeight = h;
    ARLOGd("Resized to %dx%d.\n", w, h);
    contextWasUpdated = true;
    drawView();
}

static void quit(int rc)
{
    if (gMarkerWidths) {
        free(gMarkerWidths);
        gMarkerWidths = NULL;
    }
    arglCleanup(gArglContextSettings);
    if (imageLumaInfo) {
        arVideoLumaFinal(&imageLumaInfo);
        imageLumaInfo = NULL;
    }
    ar2FreeImageSet(&imageSet);
    ar2FreeJpegImage(&jpegImage);
    free(inputFilePath);

    SDL_Quit();
    
    exit(rc);
}

static void loadImage(void)
{
    char *ext = arUtilGetFileExtensionFromPath(inputFilePath, 1);
    if (!ext) {
        ARPRINTE("Error: unable to determine extension of file '%s'. Exiting.\n", inputFilePath);
        EXIT(E_INPUT_DATA_ERROR);
    }
    
    if (strcmp(ext, "iset") == 0) {
        
        int targetScale;
		char *filenameISet;
        
        ARPRINT("Reading ImageSet...\n");
        filenameISet = strdup(inputFilePath);
        ar2UtilRemoveExt(filenameISet);
        imageSet = ar2ReadImageSet(filenameISet);
        if( imageSet == NULL ) {
            ARPRINTE("Error: unable to read ImageSet from file '%s.iset'\n", filenameISet);
            free(ext);
            free(filenameISet);
            EXIT(E_INPUT_DATA_ERROR);
        }
        free(filenameISet);
        ARPRINT("   Done.\n");
       
        targetScale = 0;
        for (int i = 1; i < imageSet->num; i++) {
            if (imageSet->scale[i]->dpi > imageSet->scale[targetScale]->dpi) targetScale = i;
        }
        imagePixelFormat = AR_PIXEL_FORMAT_MONO;
        
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        image.buff = imageSet->scale[targetScale]->imgBWBlur[1];
        image.fillFlag = 1;
#else
        image.buff = imageSet->scale[targetScale]->imgBW;
        image.fillFlag = 1;
#endif
        imageWidth = imageSet->scale[targetScale]->xsize;
        imageHeight = imageSet->scale[targetScale]->ysize;
        imageDPI   = imageSet->scale[targetScale]->dpi;
        ARPRINT("iset image '%s' is %dx%d@%f dpi.\n", inputFilePath, imageWidth, imageHeight, imageDPI);
        
    } else if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0 || strcmp(ext, "jpe") == 0) {
        char buf[256];
        char buf1[MAXPATHLEN], buf2[MAXPATHLEN];
        
        ARPRINT("Reading JPEG file...\n");
        ar2UtilDivideExt(inputFilePath, buf1, buf2);
        jpegImage = ar2ReadJpegImage(buf1, buf2);
        if (!jpegImage) {
            ARPRINTE("Error: unable to read JPEG image from file '%s'. Exiting.\n", inputFilePath);
            free(ext);
            EXIT(E_INPUT_DATA_ERROR);
        }
        ARPRINT("   Done.\n");
        
        ARPRINT("JPEG image '%s' is %dx%d.\n", inputFilePath, jpegImage->xsize, jpegImage->ysize);
        image.buff = jpegImage->image;
        image.fillFlag = 1;
        if (jpegImage->nc == 1) imagePixelFormat = AR_PIXEL_FORMAT_MONO;
        else if (jpegImage->nc == 3) imagePixelFormat = AR_PIXEL_FORMAT_RGB;
        else {
            ARPRINTE("Error: 2 byte/pixel JPEG files not currently supported. Exiting.\n");
            ar2FreeJpegImage(&jpegImage);
            free(ext);
            EXIT(E_INPUT_DATA_ERROR);
        }
        imageWidth = jpegImage->xsize;
        imageHeight = jpegImage->ysize;
        if (imageDPI == -1.0f) {
            if (jpegImage->dpi == 0.0f) {
                for (;;) {
                    ARPRINT("JPEG image '%s' does not contain embedded resolution data, and no resolution specified on command-line.\nEnter resolution to use (in decimal DPI): ", inputFilePath);
                    if (fgets(buf, sizeof(buf), stdin) == NULL) {
                        ar2FreeJpegImage(&jpegImage);
                        free(ext);
                        EXIT(E_USER_INPUT_CANCELLED);
                    }
                    if (sscanf(buf, "%f", &(jpegImage->dpi)) == 1) break;
                }
            }
            imageDPI = jpegImage->dpi;
        }

        // Create luma-only buffer.
        if (imagePixelFormat == AR_PIXEL_FORMAT_MONO) {
            image.buffLuma = image.buff;
        } else {
            imageLumaInfo = arVideoLumaInit(imageWidth, imageHeight, imagePixelFormat);
            if (!imageLumaInfo) {
                ARPRINTE("Error: unable to initialise luma conversion.\n");
                ar2FreeJpegImage(&jpegImage);
                free(ext);
                EXIT(E_INPUT_DATA_ERROR);
            }
            image.buffLuma = arVideoLuma(imageLumaInfo, image.buff);
        }

    } else {
        ARPRINTE("Error: file '%s' has extension '%s', which is not supported for reading. Exiting.\n", inputFilePath, ext);
        free(ext);
        EXIT(E_INPUT_DATA_ERROR);
    }
    free(ext);

    arParamClear(&cparam, imageWidth, imageHeight, AR_DIST_FUNCTION_VERSION_DEFAULT);
    cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET);
    gARHandle = arCreateHandle(cparamLT);
    arSetDebugMode(gARHandle, AR_DEBUG_DISABLE);
    arSetLabelingMode(gARHandle, gLabelingMode);
    arSetLabelingThresh(gARHandle, THRESH);
    arSetImageProcMode(gARHandle, AR_IMAGE_PROC_FRAME_IMAGE);
    arSetMarkerExtractionMode(gARHandle, AR_NOUSE_TRACKING_HISTORY);
    arSetPixelFormat(gARHandle, imagePixelFormat);

    gArglContextSettings = arglSetupForCurrentContext(&cparam, imagePixelFormat);
    arglDistortionCompensationSet(gArglContextSettings, FALSE);
    arglPixelBufferDataUpload(gArglContextSettings, image.buff);

    detectedMarkerNum = detect(&gMarkerWidths);
    drawView();
}

static int detect(float **markerWidths_p)
{
    if (!markerWidths_p) return 0;
    
    gSelectedMarkers.clear();
    
    arDetectMarker(gARHandle, &image);
    ARPRINT("Pass 1: detected %d marker candidates.\n", arGetMarkerNum(gARHandle));
    
    if (*markerWidths_p) free(*markerWidths_p);
    arMalloc(*markerWidths_p, float, arGetMarkerNum(gARHandle));
    check_square(arGetMarker(gARHandle), arGetMarkerNum(gARHandle), *markerWidths_p);
    int co = 0;
    for (int i = 0; i < arGetMarkerNum(gARHandle); i++) {
        if ((*markerWidths_p)[i] > 0.0f) co++;
    }
    ARPRINT("Pass 2: %d detected marker candidates are square.\n", co);
    
    return co;
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
            if (strcmp(argv[i], "--pattRatio") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 1.0f) gPattRatio = (ARdouble)tempF;
                else ARPRINTE("Error: argument '%s' to --pattRatio invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattSize") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI >= 16 && tempI <= AR_PATT_SIZE1_MAX) gPattSize = tempI;
                else ARPRINTE("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattCountMax") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI > 0) gPattCountMax = tempI;
                else ARPRINTE("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--borderSize") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) gPattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARPRINTE("Error: argument '%s' to --borderSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--labelingMode") == 0) {
                i++;
                if (strcmp(argv[i], "AR_LABELING_BLACK_REGION") == 0) gLabelingMode = AR_LABELING_BLACK_REGION;
                else if (strcmp(argv[i], "AR_LABELING_WHITE_REGION") == 0) gLabelingMode = AR_LABELING_WHITE_REGION;
                else ARPRINTE("Error: argument '%s' to --labelingMode invalid.\n", argv[i]);
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
                else ARPRINTE("Error: argument '%s' to -border= invalid.\n", argv[i]);
            } else if (strncmp(argv[i], "-dpi=", 5) == 0) {
                if (sscanf(&(argv[i][5]), "%f", &tempF) == 1 && tempF > 0.0f) imageDPI = tempF;
                else ARPRINTE("Error: argument '%s' to -dpi= invalid.\n", argv[i]);
            } else {
                if (!inputFilePath) inputFilePath = strdup(argv[i]);
                else usage(argv[0]);
            }
        }
        i++;
    }
    if (!inputFilePath) usage(argv[0]);
}

static void usage( char *com )
{
    ARPRINT("Usage: %s [options] <filename>\n\n", com);
    ARPRINT("Where <filename> is path to a JPEG or iset file.\n\n");
    ARPRINT("Options:\n");
    ARPRINT("  --pattRatio f: Specify the proportion of the marker width/height, occupied\n");
    ARPRINT("             by the marker pattern. Range (0.0 - 1.0) (not inclusive).\n");
    ARPRINT("             (I.e. 1.0 - 2*borderSize). Default value is 0.5.\n");
    ARPRINT("  --borderSize f: DEPRECATED specify the width of the pattern border, as a\n");
    ARPRINT("             percentage of the marker width. Range (0.0 - 0.5) (not inclusive).\n");
    ARPRINT("             (I.e. (1.0 - pattRatio)/2). Default value is 0.25.\n");
    ARPRINT("  -border=f: Alternate syntax for --borderSize f.\n");
    ARPRINT("  --pattSize n: Specify the number of rows and columns in the pattern space\n");
    ARPRINT("             for template (pictorial) markers.\n");
    ARPRINT("             Default value 16 (required for compatibility with ARToolKit prior\n");
    ARPRINT("             to version 5.2). Range is [16, %d] (inclusive).\n", AR_PATT_SIZE1_MAX);
    ARPRINT("  --pattCountMax n: Specify the maximum number of template (pictorial) markers\n");
    ARPRINT("             that may be loaded for use in a single matching pass.\n");
    ARPRINT("             Default value %d. Must be > 0.\n", AR_PATT_NUM_MAX);
    ARPRINT("  --labelingMode AR_LABELING_BLACK_REGION|AR_LABELING_WHITE_REGION\n");
    ARPRINT("  -dpi=f:    Override embedded JPEG DPI value.\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static void keyboard(SDL_Keycode key)
{
    int mode, threshChange = 0;
    bool redetect = false, redraw = false;
    AR_LABELING_THRESH_MODE modea;
    
    switch (key) {
        case 0x1B:						// Quit.
        case 'Q':
        case 'q':
            quit(0);
            break;
        case ' ':
            if (gSelectedMarkers.size() > 0) {
                saveFiles();
            };
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
            redetect = redraw = true;
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
            redetect = redraw = true;
            break;
        case 'b':
        case 'B':
            mode = arGetLabelingMode(gARHandle);
            if (mode == AR_LABELING_BLACK_REGION) mode = AR_LABELING_WHITE_REGION;
            else mode = AR_LABELING_BLACK_REGION;
            arSetLabelingMode(gARHandle, mode);
            redetect = redraw = true;
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
            mode = !arGetDebugMode(gARHandle);
            arSetDebugMode(gARHandle, mode);
            if (mode == AR_DEBUG_ENABLE) {
                arglPixelFormatSet(gArglContextSettings, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
                arglPixelBufferDataUpload(gArglContextSettings, gARHandle->labelInfo.bwImage);
            } else {
                arglPixelFormatSet(gArglContextSettings, imagePixelFormat); // Drawing the input image.
                arglPixelBufferDataUpload(gArglContextSettings, image.buff);
            }
            redraw = true;
            break;
        case '?':
        case '/':
            gShowHelp++;
            if (gShowHelp > 1) gShowHelp = 0;
            redraw = true;
            break;
        case 'm':
        case 'M':
            gShowMode = !gShowMode;
            redraw = true;
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
        ARPRINT("thresh = %d\n", threshhold);
        redetect = redraw = 1;
    }

    if (redetect) {
        detectedMarkerNum = detect(&gMarkerWidths);
    }
    if (redraw) {
        drawView();
    }
}

static void mouse(Uint8 button, Sint32 x, Sint32 y)
{
    ARLOGd("Mouse button %d at window %d,%d.\n", button, x, y);
    
    int winW, winH;
    SDL_GetWindowSize(gSDLWindow, &winW, &winH);
    float px = x * (contextWidth/winW) / imageZoom;
    float py = y * (contextHeight/winH) / imageZoom;
    
    // Determine if the click falls inside a marker square, and if so, which one.
    int markerNum = arGetMarkerNum(gARHandle);
    ARMarkerInfo *markerInfo = arGetMarker(gARHandle);
    ARMarkerInfo *markerClicked = NULL;
    for (int i = 0; i < markerNum; i++) {
        if (gMarkerWidths[i] <= 0.0) continue;
        
        ARdouble (*v)[2] = markerInfo[i].vertex;
        // Test whether point (px, py) is inside marker. A point will be inside if it is on the same side of
        // all line segments.
        bool sides[4];
        for (int j = 0; j < 4; j++) {
            sides[j] = ((px - v[j%4][0])*(v[(j + 1)%4][1] - v[j%4][1]) - (py - v[j%4][1])*(v[(j + 1)%4][0] - v[j%4][0])) > 0.0;
            //ARLOGd("Point (%.1f,%1f) is %s of line (%.1f,%1f) to (%.1f,%1f)", px, py, sides[j] ? "left" : "right", v[j%4][0], v[j%4][1], v[(j + 1)%4][0], v[(j + 1)%4][1]);
        }
        if ((sides[0] == sides[1]) && (sides[0] == sides[2]) && (sides[0] == sides[3])) {
            markerClicked = &markerInfo[i];
            break;
        }
    }
    if (markerClicked) {
        std::vector<ARMarkerInfo *>::iterator it = std::find(gSelectedMarkers.begin(), gSelectedMarkers.end(), markerClicked);
        if (it == gSelectedMarkers.end()) {
            gSelectedMarkers.push_back(markerClicked);
        } else {
            gSelectedMarkers.erase(it);
        }
        drawView();
    }
}

static void saveFiles(void)
{
    int     ret = 0;
    char    path[MAXPATHLEN] = "";
    size_t  pathLen = 0L;
    char   *basename = NULL;
    const char markerExt[] = "mrk";
    const char patternExt[] = "pat";
    FILE    *fp;
    int     x, y;
    int     co;
    float   vec[2][2], center[2], length;
    float   trans1[3][4];
    float   mx1, my1, mx2, my2;
    //static int r = 0;

    basename = arUtilGetFileBasenameFromPath(inputFilePath, 0);
    
    arUtilGetDirectoryNameFromPath(path, inputFilePath, sizeof(path), 1);
    pathLen = strlen(path);
    snprintf(path + pathLen, sizeof(path) - pathLen, "%s.%s", basename, markerExt);
    if (!(fp = fopen(path, "w"))) {
        ARPRINTE("Error opening output marker file '%s' for writing.\n", path);
        ARLOGperror(NULL);
        ret = -1;
        goto done0;
    }

    co = 0;
    for (std::vector<ARMarkerInfo *>::iterator it = gSelectedMarkers.begin(); it != gSelectedMarkers.end(); it++) {
        
        arUtilGetDirectoryNameFromPath(path, inputFilePath, sizeof(path), 1);
        pathLen = strlen(path);
        snprintf(path + pathLen, sizeof(path) - pathLen, "%s-%02d.%s", basename, co, patternExt);
        if (arPattSave(image.buff, imageWidth, imageHeight, imagePixelFormat, &(cparamLT->paramLTf), AR_IMAGE_PROC_FRAME_IMAGE, *it, arGetPattRatio(gARHandle), gPattSize, path) < 0) {
            ARPRINTE("Error saving pattern file '%s'.\n", path);
            ret = -1;
            goto done;
        } else {
            ARPRINT("Saved pattern file '%s'.\n", path);
        }
        co++;
    }

    // Write the marker file.
    fprintf(fp, "%d\n\n", co);
    co = 0;
    for (std::vector<ARMarkerInfo *>::iterator it = gSelectedMarkers.begin(); it != gSelectedMarkers.end(); ++it) {

        // Find the marker width.
        int markerNum = arGetMarkerNum(gARHandle);
        ARMarkerInfo *markerInfo = arGetMarker(gARHandle);
        float markerWidth = 0.0f;
        for (int i = 0; i < markerNum; i++) {
            if (*it == (&markerInfo[i])) {
                markerWidth = gMarkerWidths[i];
                break;
            }
        }
        
        snprintf(path, sizeof(path), "%s-%02d.%s", basename, co, patternExt);
        fprintf(fp, "%s\n", path);
        fprintf(fp, "%f\n", markerWidth);

        // Print vertices in pixel coords.
        // Origin at UL of image.
        ARPRINT("-- %s --\n", path);
        ARPRINT("Upper-left:  {%f, %f}\n", (*it)->vertex[2][0], (*it)->vertex[2][1]);
        ARPRINT("Upper-right: {%f, %f}\n", (*it)->vertex[3][0], (*it)->vertex[3][1]);
        ARPRINT("Lower-right: {%f, %f}\n", (*it)->vertex[0][0], (*it)->vertex[0][1]);
        ARPRINT("Lower-left:  {%f, %f}\n", (*it)->vertex[1][0], (*it)->vertex[1][1]);

        // UR (for axis-aligned upright marker) to m1, UL (for axis-aligned upright marker) to m2
        // Origin at LL of image.
        // vec[0] is unit vector from UL to UR (for axis-aligned upright marker).
        pixel2mm((*it)->vertex[3][0], (*it)->vertex[3][1], &mx1, &my1);
        pixel2mm((*it)->vertex[2][0], (*it)->vertex[2][1], &mx2, &my2);
        vec[0][0] = mx1 - mx2;
        vec[0][1] = my1 - my2;
        length = sqrt(vec[0][0]*vec[0][0] + vec[0][1]*vec[0][1]);
        vec[0][0] /= length;
        vec[0][1] /= length;

        // LL (for axis-aligned upright marker) to m1, UL (for axis-aligned upright marker) to m2
        // Origin at LL of image.
        // vec[1] is unit vector from LL to UL (for axis-aligned upright marker).
        pixel2mm((*it)->vertex[1][0], (*it)->vertex[1][1], &mx1, &my1);
        pixel2mm((*it)->vertex[2][0], (*it)->vertex[2][1], &mx2, &my2);
        vec[1][0] = mx2 - mx1;
        vec[1][1] = my2 - my1;
        length = sqrt(vec[1][0]*vec[1][0] + vec[1][1]*vec[1][1]);
        vec[1][0] /= length;
        vec[1][1] /= length;

        mx1 = ((*it)->vertex[0][0] + (*it)->vertex[1][0] + (*it)->vertex[2][0] + (*it)->vertex[3][0]) / 4;
        my1 = ((*it)->vertex[0][1] + (*it)->vertex[1][1] + (*it)->vertex[2][1] + (*it)->vertex[3][1]) / 4;
        pixel2mm(mx1, my1, &mx2, &my2);
        center[0] = mx2;
        center[1] = my2;

        trans1[0][0] = vec[0][0];
        trans1[1][0] = vec[0][1];
        trans1[2][0] = 0.0;
        trans1[0][1] = vec[1][0];
        trans1[1][1] = vec[1][1];
        trans1[2][1] = 0.0;
        trans1[0][2] = 0.0;
        trans1[1][2] = 0.0;
        trans1[2][2] = 1.0;
        trans1[0][3] = center[0];
        trans1[1][3] = center[1];
        trans1[2][3] = 0.0;
        for (y = 0; y < 3; y++) {
            for (x = 0; x < 4; x++) fprintf(fp, " %15.7f", trans1[y][x]);
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");

        co++;
    }

done:
    fclose(fp);
    
done0:
    free(basename);
    quit(ret);
}

static void check_square(ARMarkerInfo *markerInfo, int markerNum, float *markerWidths)
{
    float   wlen, len[4], ave, sd;
    float   vec[4][2], ip1, ip2;
    int     i, j;

    for( i = 0; i < markerNum; i++ ) {
        ave = 0.0;
        for( j = 0; j < 4; j++ ) {
            vec[j][0] = markerInfo[i].vertex[(j+1)%4][0] - markerInfo[i].vertex[j][0];
            vec[j][1] = markerInfo[i].vertex[(j+1)%4][1] - markerInfo[i].vertex[j][1];
            wlen = vec[j][0] * vec[j][0] + vec[j][1] * vec[j][1];
            ave += len[j] = sqrt( wlen );
            vec[j][0] /= len[j];
            vec[j][1] /= len[j];
        }
        ave /= 4.0;
        sd = 0.0;
        for( j = 0; j < 4; j++ ) {
            sd += (len[j] - ave)*(len[j] - ave);
        }
        sd = sqrt( sd/4.0 );



        ip1 = vec[0][0]*vec[2][0] + vec[0][1]*vec[2][1];
        ip2 = vec[0][0]*vec[1][0] + vec[0][1]*vec[1][1];
        if( sd/ave > 0.01 || ip1 > -0.99 || ip2 < -0.01 || ip2 > 0.01 ) {
			markerWidths[i] = 0.0;
        }
		else {
	        markerWidths[i] = ave/imageDPI * 25.4;
		}
    }
}

static void pixel2mm(float px, float py, float *mx, float *my)
{
    *mx =  px / imageDPI * 25.4f;
    *my = (imageHeight - py) / imageDPI * 25.4f;
}

/*static void mm2pixel(float mx, float my, float *px, float *py)
{
    *px = mx * imageDPI / 25.4f;
    *py = imageHeight - my * imageDPI / 25.4f;
}*/

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

//
// This function is called when the window needs redrawing.
//
static void drawView(void)
{
    int i;
    
    if (!imageWidth || !imageHeight || !gArglContextSettings) return;
    
    SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);
    
    if (contextWasUpdated) {
        float xzoom, yzoom;
        xzoom = (float)contextWidth / (float)imageWidth;
        yzoom = (float)contextHeight / (float)imageHeight;
        imageZoom = (xzoom > yzoom ? yzoom : xzoom);
        ARPRINT("%dx%d input image will display in %dx%d window at %.1f%% size\n", imageWidth, imageHeight, contextWidth, contextHeight, imageZoom*100.0f);
        
        gViewport[0] = 0;
        gViewport[1] = 0;
        gViewport[2] = (int)(imageWidth * imageZoom);
        gViewport[3] = (int)(imageHeight * imageZoom);
        contextWasUpdated = false;
    }
    
    // Clean the OpenGL context.
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    arglDispImage(gArglContextSettings, gViewport);

    // 2D overlays in image frame.
    glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)imageWidth, 0, (GLdouble)imageHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    EdenGLFontSetViewSize(imageWidth, imageHeight);
    
    // Draw boxes around detected square markers.
    glLoadIdentity();
    glLineWidth(2.0f);
    GLfloat vertices[6][2];
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    for (int j = 0; j < arGetMarkerNum(gARHandle); j++) {
        if (gMarkerWidths[j] <= 0.0f) continue;

        // Corners, starting in top-left.
        glColor4ub(0, 255, 0, 255);
        for (i = 0; i < 5; i++) {
            vertices[i][0] = (float)gARHandle->markerInfo[j].vertex[(i + 2)%4][0];
            vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].vertex[(i + 2)%4][1]);
        }
        // Centre.
        vertices[i][0] = (float)gARHandle->markerInfo[j].pos[0];
        vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].pos[1]);
        glDrawArrays(GL_LINE_STRIP, 0, 6);

        // Outline pattern space in blue.
        glColor4ub(0, 0, 255, 255);
        getPatternVerticesFromMarkerVertices((const ARdouble (*)[2])gARHandle->markerInfo[j].vertex, vertices);
        for (int i = 0; i < 4; i++) vertices[i][1] = (float)gARHandle->ysize - vertices[i][1];
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    // Draw red boxes around selected markers, with numbers.
    float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glColor4fv(red);
    EdenGLFontSetColor(red);
    int co = 0;
    for (std::vector<ARMarkerInfo *>::iterator it = gSelectedMarkers.begin(); it != gSelectedMarkers.end(); ++it) {
        glVertexPointer(2, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        // Corners.
        for (i = 0; i < 4; i++) {
            vertices[i][0] = (float)((*it)->vertex[i%4][0]);
            vertices[i][1] = (float)gARHandle->ysize - (float)((*it)->vertex[i%4][1]);
        }
        glLineWidth(4.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glLineWidth(1.0f);
        char text[16];
        snprintf(text, sizeof(text), "%d", co);
        
        float markerHeightInPoints = ((*it)->vertex[0][1] - (*it)->vertex[2][1]) * 72.0f / imageDPI;
        EdenGLFontSetSize(markerHeightInPoints * 0.8f);
        EdenGLFontDrawLine(0, NULL, (unsigned char *)text, (float)((*it)->pos[0]), (float)gARHandle->ysize - (float)((*it)->pos[1]), H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
        co++;
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    
    // 2D overlays in context space.
    glViewport(0, 0, contextWidth, contextHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)contextWidth, 0, (GLdouble)contextHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    EdenGLFontSetViewSize(contextWidth, contextHeight);
    glLineWidth(1.0f);
    EdenGLFontSetColor(white);
    EdenGLFontSetSize(FONT_SIZE);

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
        "Use the mouse to select markers in order.\n",
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " [space]       Save the selected markers."
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " b             Change labeling mode.",
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
    snprintf(text, sizeof(text), "Processing %dx%d input image %s", imageWidth, imageHeight, text_p);
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
    len = (int)strlen(text);
    snprintf(text + len, sizeof(text) - len, ", debug mode %s", (arGetDebugMode(gARHandle) == AR_DEBUG_ENABLE ? "ON" : "OFF"));
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;

    // Border size, image processing mode, pattern detection mode.
    snprintf(text, sizeof(text), "Border: %0.2f%%", arGetBorderSize(gARHandle)*100.0);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;

    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", contextWidth, contextHeight);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;

    // Detected markers.
    snprintf(text, sizeof(text), "Detected %d square markers.\n", detectedMarkerNum);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;
}
