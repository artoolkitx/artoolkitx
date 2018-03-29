/*
 *  dispTexData.cpp
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
#include <ARX/AR2/config.h>
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/util.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/KPM/kpm.h>
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
static int              display_fset = 0;
static int              display_fset3 = 0;
static char            *inputFilePath = NULL;

// Input.
static AR2ImageSetT            *imageSet;
static AR2FeatureSetT          *featureSet;
static KpmRefDataSet           *refDataSet;
static int                      page = 0;
static double                   imageZoom = 1.0f;


// Drawing.
// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static SDL_Window* gSDLWindow = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglContextSettings = NULL;
static int gShowHelp = 1;
static int gShowMode = 1;

static char             exitcode = -1;
#define EXIT(c) {exitcode=c;exit(c);}

// ============================================================================
//    Function prototypes.
// ============================================================================

static void loadImage(void);
static void setImagePage(int page_in);
static void quit(int rc);
static void reshape(int w, int h);
static void keyboard(SDL_Keycode key);
static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
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
            }
        }
        
    } // !done
    
    quit(0);
}

static void reshape(int w, int h)
{
    contextWidth = w;
    contextHeight = h;
    ARLOGd("Resized to %dx%d.\n", w, h);
    drawView();
}

static void quit(int rc)
{
    arglCleanup(gArglContextSettings);
    ar2FreeImageSet(&imageSet);
    ar2FreeFeatureSet(&featureSet);
    kpmDeleteRefDataSet(&refDataSet);

    free(inputFilePath);

    SDL_Quit();
    
    exit(rc);
}

static void loadImage(void)
{
    ARPRINT("Read ImageSet.\n");
    ar2UtilRemoveExt(inputFilePath);
    imageSet = ar2ReadImageSet(inputFilePath);
    if (!imageSet) {
        ARLOGe("file open error: %s.iset\n", inputFilePath);
        exit(0);
    }
    ARPRINT("  end.\n");

    if (display_fset) {
        ARPRINT("Read FeatureSet.\n");
        featureSet = ar2ReadFeatureSet(inputFilePath, "fset");
        if (!featureSet) {
            ARLOGe("file open error: %s.fset\n", inputFilePath);
            exit(0);
        }
        ARPRINT("  end.\n");
    }
 
    if (display_fset3) {
        ARPRINT("Read FeatureSet3.\n");
        kpmLoadRefDataSet(inputFilePath, "fset3", &refDataSet);
        if (!refDataSet) {
            ARLOGe("file open error: %s.fset3\n", inputFilePath);
            exit(0);
        }
        ARPRINT("  end.\n");
        ARPRINT("num = %d\n", refDataSet->num);
    }
    
    setImagePage(0);
}

static void setImagePage(int page_in)
{
    page = page_in;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    page = page % (imageSet->num*(AR2_BLUR_IMAGE_MAX));
#else
    page = page % (imageSet->num);
#endif

    // Update the image to be drawn.
    if (gArglContextSettings) {
        arglCleanup(gArglContextSettings);
        gArglContextSettings = NULL;
    }
    ARParam cparam;
    arParamClear(&cparam, imageSet->scale[page]->xsize, imageSet->scale[page]->ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);
    gArglContextSettings = arglSetupForCurrentContext(&cparam, AR_PIXEL_FORMAT_MONO);
    arglDistortionCompensationSet(gArglContextSettings, FALSE);
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    arglPixelBufferDataUpload(gArglContextSettings, imageSet->scale[page/AR2_BLUR_IMAGE_MAX]->imgBWBlur[page%AR2_BLUR_IMAGE_MAX]);
#else
    arglPixelBufferDataUpload(gArglContextSettings, imageSet->scale[page]->imgBW);
#endif
    drawView();
}

static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption, display_defaults = 1;
    //int tempI;
    
    //
    // Command-line options.
    //
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            //if (strcmp(argv[i], "--someOption") == 0) {
            //    i++;
            //    if (sscanf(argv[i], "%d", &tempI) == 1 && tempI > 0) gSomeVar = tempI;
            //    else ARPRINTE("Error: argument '%s' to --someOption invalid.\n", argv[i]);
            //    gotTwoPartOption = TRUE;
            //}
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
            } else if (strcmp(argv[i], "-iset") == 0) {
                display_defaults = 0;
            } else if (strcmp(argv[i], "-fset") == 0) {
                display_defaults = 0;
                display_fset = 1;
            } else if( strcmp(argv[i], "-fset2") == 0 ) {
                ARPRINTE("Error: -fset2 option no longer supported as of ARToolKit v5.3.\n");
                exit(-1);
            } else if( strcmp(argv[i], "-fset3") == 0 ) {
                display_defaults = 0;
                display_fset3 = 1;
            } else {
                if (!inputFilePath) inputFilePath = strdup(argv[i]);
                else usage(argv[0]);
            }
        }
        i++;
    }
    if (!inputFilePath) usage(argv[0]);
    if (display_defaults) display_fset = display_fset3 = 1;
}

static void usage( char *com )
{
    ARPRINT("Usage: %s [options] <filename>\n\n", com);
    ARPRINT("Where <filename> is path to a JPEG or iset file.\n\n");
    ARPRINT("Options:\n");
    ARPRINT("  -iset     Show image set.\n");
    ARPRINT("  -fset     Show fset features.\n");
    ARPRINT("  -fset3    Show fset3 features.\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static void keyboard(SDL_Keycode key)
{
    bool redraw = false;
    
    switch (key) {
        case 0x1B:						// Quit.
        case 'Q':
        case 'q':
            quit(0);
            break;
        case ' ':
            setImagePage(page + 1);
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
    if (redraw) {
        drawView();
    }
}

//
// This function is called when the window needs redrawing.
//
static void drawView(void)
{
    int i;
    int viewport[4];
    
    if (!imageSet || !gArglContextSettings) return;
    
    SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);
    
    float xzoom, yzoom;
    xzoom = (float)contextWidth / (float)imageSet->scale[page]->xsize;
    yzoom = (float)contextHeight / (float)imageSet->scale[page]->ysize;
    imageZoom = (xzoom > yzoom ? yzoom : xzoom);
    ARPRINT("%dx%d input image will display in %dx%d window at %.1f%% size\n", imageSet->scale[page]->xsize, imageSet->scale[page]->ysize, contextWidth, contextHeight, imageZoom*100.0f);
    
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = (int)(imageSet->scale[page]->xsize * imageZoom);
    viewport[3] = (int)(imageSet->scale[page]->ysize * imageZoom);
    
    // Clean the OpenGL context.
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    arglDispImage(gArglContextSettings, viewport);
    
    // 2D overlays in image frame.
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)imageSet->scale[page]->xsize, 0, (GLdouble)imageSet->scale[page]->ysize, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    EdenGLFontSetViewSize(imageSet->scale[page]->xsize, imageSet->scale[page]->ysize);
    
    if (display_fset) {
 
        // Draw red boxes around features, and number.
        glLineWidth(2.0f);
        float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        glColor4fv(red);
        EdenGLFontSetColor(red);
        
        for (i = 0; i < featureSet->list[page].num; i++) {
            int x = featureSet->list[page].coord[i].x;
            int y = featureSet->list[page].coord[i].y;
            
            GLfloat vertices[4][2];
            vertices[0][0] = x - AR2_DEFAULT_TS1;
            vertices[0][1] = y - AR2_DEFAULT_TS1;
            vertices[1][0] = x - AR2_DEFAULT_TS1;
            vertices[1][1] = y + AR2_DEFAULT_TS2;
            vertices[2][0] = x + AR2_DEFAULT_TS2;
            vertices[2][1] = y + AR2_DEFAULT_TS2;
            vertices[3][0] = x + AR2_DEFAULT_TS2;
            vertices[3][1] = y - AR2_DEFAULT_TS1;
            glVertexPointer(2, GL_FLOAT, 0, vertices);
            glEnableClientState(GL_VERTEX_ARRAY);
            glDrawArrays(GL_LINE_LOOP, 0, 4);

            glLineWidth(1.0f);
            char text[16];
            snprintf(text, sizeof(text), "%d", i);
            EdenGLFontDrawLine(0, NULL, (unsigned char *)text, (float)x, (float)y, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
            
            glDisableClientState(GL_VERTEX_ARRAY);
        }
        ARPRINT("fset:  Num of feature points: %d\n", featureSet->list[page].num);
    }
    
    if (display_fset3) {
        
        // Draw green crosses on features.
        glLineWidth(2.0f);
        float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};
        glColor4fv(green);
        
        for (int i = 0; i < refDataSet->num; i++) {
            if (refDataSet->refPoint[i].refImageNo != page) continue;
            int x = refDataSet->refPoint[i].coord2D.x;
            int y = refDataSet->refPoint[i].coord2D.y;
            GLfloat vertices[4][2];
            vertices[0][0] = x - 5;
            vertices[0][1] = y - 5;
            vertices[1][0] = x + 5;
            vertices[1][1] = y + 5;
            vertices[2][0] = x + 5;
            vertices[2][1] = y - 5;
            vertices[3][0] = x - 5;
            vertices[3][1] = y + 5;
            glVertexPointer(2, GL_FLOAT, 0, vertices);
            glEnableClientState(GL_VERTEX_ARRAY);
            glDrawArrays(GL_LINES, 0, 4);
            glDisableClientState(GL_VERTEX_ARRAY);
        }
#if 0
        for (i = 0; i < refDataSet->pageNum; i++) {
            for (j = 0; j < refDataSet->pageInfo[i].imageNum; j++) {
                if (refDataSet->pageInfo[i].imageInfo[j].imageNo == page) {
                    ARPRINT("fset3: Image size: %dx%d\n", refDataSet->pageInfo[i].imageInfo[j].width, refDataSet->pageInfo[i].imageInfo[j].height);
                }
            }
        }
#endif
    }
    
    // 2D overlays in context space.
    glViewport(0, 0, contextWidth, contextHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)contextWidth, 0, (GLdouble)contextHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    EdenGLFontSetViewSize(contextWidth, contextHeight);
    glLineWidth(1.0f);
    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
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
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " [space]       Page through all texture data resolutions."
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
    int line;
    char text[256];
    
    glColor3ub(255, 255, 255);
    line = 1;
    
    // Feature set resolution.
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    snprintf(text, sizeof(text), "%f[dpi] image. Size = (%d,%d)\n", imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->dpi,
             imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->xsize,
             imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->ysize);
#else
    snprintf(text, sizeof(text), "%f[dpi] image. Size = (%d,%d)\n", imageSet->scale[page]->dpi,
             imageSet->scale[page]->xsize,
             imageSet->scale[page]->ysize);
#endif
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;

    if (display_fset) {
        snprintf(text, sizeof(text), "fset:  Num of feature points: %d\n", featureSet->list[page].num);
        EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
        line++;
    }
    
    if (display_fset3) {
        
        // Count fset3 feature points that belong to current page.
        int co = 0;
        for (int i = 0; i < refDataSet->num; i++) if (refDataSet->refPoint[i].refImageNo == page) co++;

        snprintf(text, sizeof(text), "fset3: Num of feature points: %d\n", co);
        EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
        line++;
    }

    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", contextWidth, contextHeight);
    EdenGLFontDrawLine(0, NULL, (unsigned char *)text, 2.0f,  (line - 1)*FONT_SIZE + 2.0f, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_TEXT_TOP_TO_VIEW_TOP);
    line++;

}
