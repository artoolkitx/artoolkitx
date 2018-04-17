//
//  main.cpp
//  artoolkitX 2d Tracking Example
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef DEBUG
#  ifdef _WIN32
#    define MAXPATHLEN MAX_PATH
#    include <direct.h>               // _getcwd()
#    define getcwd _getcwd
#  else
#    include <unistd.h>
#    include <sys/param.h>
#  endif
#endif
#include <string>

#include <ARX/ARController.h>
#include <ARX/ARUtil/time.h>

#ifdef __APPLE__
#  include <SDL2/SDL.h>
#  if (HAVE_GL || HAVE_GL3)
#    include <SDL2/SDL_opengl.h>
#  elif HAVE_GLES2
#    include <SDL2/SDL_opengles2.h>
#  endif
#else
#  include "SDL2/SDL.h"
#  if (HAVE_GL || HAVE_GL3)
#    include "SDL2/SDL_opengl.h"
#  elif HAVE_GLES2
#    include "SDL2/SDL_opengles2.h"
#  endif
#endif

#include "draw.h"

#if ARX_TARGET_PLATFORM_WINDOWS
const char *vconf = "-module=WinMF -format=BGRA";
#else
const char *vconf = NULL;
#endif
const char *cpara = NULL;

// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static int32_t viewport[4];
static float projection[16];
static SDL_Window* gSDLWindow = NULL;

static ARController* arController = NULL;
static ARG_API drawAPI = ARG_API_None;

static long gFrameNo = 0;

struct marker {
    const char *name;
    float height;
};
static const struct marker markers[] = {
    {"pinball.jpg", 1.0}
};
static const int markerCount = (sizeof(markers)/sizeof(markers[0]));

//
//
//

static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void quit(int rc);
static void reshape(int w, int h);

int main(int argc, char *argv[])
{
    processCommandLineOptions(argc, argv);
    
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ARLOGe("Error: SDL initialisation failed. SDL error: '%s'.\n", SDL_GetError());
        quit(1);
        return 1;
    }

    // Create a window.
#if 0
    gSDLWindow = SDL_CreateWindow("artoolkitX 2d Tracking Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              0, 0,
                              SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                              );
#else
    gSDLWindow = SDL_CreateWindow("artoolkitX 2d Tracking Example",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  1280, 720,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                  );
#endif
    if (!gSDLWindow) {
        ARLOGe("Error creating window: %s.\n", SDL_GetError());
        quit(-1);
    }

    // Create an OpenGL context to draw into. If OpenGL 3.2 not available, attempt to fall back to OpenGL 1.5, then OpenGL ES 2.0
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // This is the default.
    SDL_GL_SetSwapInterval(1);
#if HAVE_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    gSDLContext = SDL_GL_CreateContext(gSDLWindow);
    if (gSDLContext) {
        drawAPI = ARG_API_GL3;
        ARLOGi("Created OpenGL 3.2+ context.\n");
    } else {
        ARLOGi("Unable to create OpenGL 3.2 context: %s. Will try OpenGL 1.5.\n", SDL_GetError());
#endif // HAVE_GL3
#if HAVE_GL
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
        gSDLContext = SDL_GL_CreateContext(gSDLWindow);
        if (gSDLContext) {
            drawAPI = ARG_API_GL;
            ARLOGi("Created OpenGL 1.5+ context.\n");
#  if ARX_TARGET_PLATFORM_MACOS
        vconf = "-format=BGRA";
#  endif
        } else {
            ARLOGi("Unable to create OpenGL 1.5 context: %s. Will try OpenGL ES 2.0\n", SDL_GetError());
#endif // HAVE_GL
#if HAVE_GLES2
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            gSDLContext = SDL_GL_CreateContext(gSDLWindow);
            if (gSDLContext) {
                drawAPI = ARG_API_GLES2;
                ARLOGi("Created OpenGL ES 2.0+ context.\n");
            } else {
                ARLOGi("Unable to create OpenGL ES 2.0 context: %s.\n", SDL_GetError());
            }
#endif // HAVE_GLES2
#if HAVE_GL
        }
#endif
#if HAVE_GL3
    }
#endif
    if (drawAPI == ARG_API_None) {
        ARLOGe("No OpenGL context available. Giving up.\n", SDL_GetError());
        quit(-1);
    }

    int w, h;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &w, &h);
    reshape(w, h);

    // Initialise the ARController.
    arController = new ARController();
    if (!arController->initialiseBase()) {
        ARLOGe("Error initialising ARController.\n");
        quit(-1);
    }

#ifdef DEBUG
    arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif


#ifdef DEBUG
    char buf[MAXPATHLEN];
    ARLOGd("CWD is '%s'.\n", getcwd(buf, sizeof(buf)));
#endif
    
    // 1 - Add trackables form markers.
    int markerIDs[markerCount];
    int markerModelIDs[markerCount];
    char *resourcesDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
    for (int i = 0; i < markerCount; i++) {
        std::string markerConfig = "2d;" + std::string(resourcesDir) + '/' + markers[i].name + ';' + std::to_string(markers[i].height);
        markerIDs[i] = arController->addTrackable(markerConfig);
        if (markerIDs[i] == -1) {
            ARLOGe("Error adding marker.\n");
            quit(-1);
        }
    }
    
    //2 - Add trackabales from database file
    //const char *databaseName = "database.xml.gz";
    //char *databaseFullPath;
    //asprintf(&databaseFullPath, "%s/%s", resourcesDir, databaseName);
    //arController->load2DTrackerImageDatabase(databaseFullPath);

    //int markerCount = arController->countTrackables();
    //int markerIDs[markerCount];
    //int markerModelIDs[markerCount];
    //for(int i=0;i<markerCount;i++)
    //{
    //    ARTrackable *trackable = arController->getTrackableAtIndex(i);
    //    markerIDs[i] = trackable->UID;
    //}

#ifdef DEBUG
    ARLOGd("vconf is '%s'.\n", vconf);
#endif
    // Start tracking.
    arController->startRunning(vconf, cpara, NULL, 0);

    // Main loop.
    bool done = false;
    while (!done) {

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)) {
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
                if (ev.key.keysym.sym == SDLK_d) {
                    // No debug mode for 2d tracker yet.
                }
            }
         }

        bool gotFrame = arController->capture();
        if (!gotFrame) {
            arUtilSleep(1);
        } else {
            //ARLOGi("Got frame %ld.\n", gFrameNo);
            gFrameNo++;

            if (!arController->update()) {
                ARLOGe("Error in ARController::update().\n");
                quit(-1);
            }

            if (contextWasUpdated) {
                
                if (!arController->drawVideoInit(0)) {
                    ARLOGe("Error in ARController::drawVideoInit().\n");
                    quit(-1);
                }
                if (!arController->drawVideoSettings(0, contextWidth, contextHeight, false, false, false, ARVideoView::HorizontalAlignment::H_ALIGN_CENTRE, ARVideoView::VerticalAlignment::V_ALIGN_CENTRE, ARVideoView::ScalingMode::SCALE_MODE_FIT, viewport)) {
                    ARLOGe("Error in ARController::drawVideoSettings().\n");
                    quit(-1);
                }
                drawSetup(drawAPI, false, false, false);
                
                drawSetViewport(viewport);
                ARdouble projectionARD[16];
                arController->projectionMatrix(0, 10.0f, 10000.0f, projectionARD);
                for (int i = 0; i < 16; i++) projection[i] = (float)projectionARD[i];
                drawSetCamera(projection, NULL);
                
                for (int i = 0; i < markerCount; i++) {
                    markerModelIDs[i] = drawLoadModel(NULL);
                }
                contextWasUpdated = false;
            }

            SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);

            // Clear the context.
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Display the current video frame to the current OpenGL context.
            arController->drawVideo(0);

            // Look for trackables, and draw on each found one.
            for (int i = 0; i < markerCount; i++) {

                // Find the trackable for the given trackable ID.
                ARTrackable *marker = arController->findTrackable(markerIDs[i]);
                float view[16];
                if (marker->visible) {
                    //arUtilPrintMtx16(marker->transformationMatrix);
                    for (int i = 0; i < 16; i++) view[i] = (float)marker->transformationMatrix[i];
                }
                drawSetModel(markerModelIDs[i], marker->visible, view);
            }

            draw();
            
            SDL_GL_SwapWindow(gSDLWindow);
        } // if (gotFrame)
    } // while (!done)

    free(resourcesDir);
    
    quit(0);
    return 0;
}

static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption;
    
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
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
}

static void usage(char *com)
{
    ARPRINT("Usage: %s [options]\n", com);
    ARPRINT("Options:\n");
    ARPRINT("  --vconf <video parameter for the camera>\n");
    ARPRINT("  --cpara <camera parameter file for the camera>\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static void quit(int rc)
{
    drawCleanup();
    if (arController) {
        arController->drawVideoFinal(0);
        arController->shutdown();
        delete arController;
    }
    if (gSDLContext) {
        SDL_GL_MakeCurrent(0, NULL);
        SDL_GL_DeleteContext(gSDLContext);
    }
    if (gSDLWindow) {
        SDL_DestroyWindow(gSDLWindow);
    }
    SDL_Quit();
    exit(rc);
}

static void reshape(int w, int h)
{
    contextWidth = w;
    contextHeight = h;
    ARLOGd("Resized to %dx%d.\n", w, h);
    contextWasUpdated = true;
}

