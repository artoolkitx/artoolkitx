/*
 *  imagedatabase_2d.cpp
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
#include <ARX/AR/ar.h>
#include <ARX/ARController.h>
#include <ARX/ARUtil/log.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <dirent.h>
// ============================================================================
//    Constants
// ============================================================================


// ============================================================================
//    Global variables
// ============================================================================

static const char *vconf = "-module=Dummy";
static ARController* arController;
static const char *imgDir = "";
static const char *outputFilename = "";

// ============================================================================
//    Function prototypes.
// ============================================================================
std::vector<std::string> getFiles(const char *nameIn, bool want_path);
static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void quit(int rc);
static std::string getFileExtension(std::string filename);
// ============================================================================
//    Functions
// ============================================================================


int main(int argc, char *argv[])
{
    arLogLevel = AR_LOG_LEVEL_DEBUG;
    processCommandLineOptions(argc, argv);

    ARPRINT("Initialising ARController.\n");
    arController = new ARController();
    if (!arController->initialiseBase()) {
        ARPRINT("Error initialising ARController.\n");
        quit(1);
    }
    
    // Start tracking.
    if (!arController->startRunning(vconf, NULL, NULL, 0)) {
        ARPRINT("Error starting ARController.\n");
        quit(1);
    }
    
    if (arController->capture())
    {
        ARPRINT("Searching for images in - %s.\n",imgDir);
        std::vector<std::string> fileNames = getFiles(imgDir, true);
        ARPRINT("Found %zu images in %s.\n", fileNames.size(), imgDir);
        if (fileNames.size() > 0) {
            int parsedFiles = 0;
            for (int i = 0; i < fileNames.size(); i++) {
                if (getFileExtension(fileNames[i]) == "jpg") {
                    ARPRINT("Loading image %i - %s.\n", i, fileNames[i].c_str());
                    std::string markerConfig = "2d;" + fileNames[i] + ";1.0";
                    int markerID = arController->addTrackable(markerConfig);
                    if (markerID == -1) {
                        ARPRINT("Error adding marker.\n");
                        quit(-1);
                    }
                }
                parsedFiles++;
            }
            if ((parsedFiles > 0) && (arController->update())) {
                ARPRINT("Outputting database to - %s.\n", outputFilename);
                arController->save2DTrackerImageDatabase(outputFilename);
                ARPRINT("Database saved.\n");
            } else {
                ARPRINT("Input directory %s does not contain valid jpg images.\n", imgDir);
            }
        } else {
            ARPRINT("Input directory %s does not contain jpg images.\n", imgDir);
        }
    } else {
        ARPRINT("Could not initialise 2D tracker.\n");
    }
    quit(0);
}

std::vector<std::string> getFiles(const char *nameIn, bool want_path)
{
    std::string dirname = std::string(nameIn);
    //DLOG(INFO) << "Getting files inside `" << dirname << "`";
    DIR* dir;
    struct dirent* ent;
    std::vector<std::string> newFiles;
    if ((dir = opendir(dirname.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0) {
                continue;
            } else if (strcmp(ent->d_name, "..") == 0) {
                continue;
            } else if (strcmp(ent->d_name, ".DS_Store") == 0) {
                continue;
            }
            if (want_path) {
                newFiles.push_back(dirname + "/" + ent->d_name);
            } else {
                newFiles.push_back(ent->d_name);
            }
        }
        closedir(dir);
    }
    return newFiles;
}

static void usage(char *com)
{
    ARPRINT("Usage: %s [options]\n", com);
    ARPRINT("Options:\n");
    ARPRINT("  --imgDir <Image directory to generate image database>\n");
    ARPRINT("  --fileOut <Output file name to generate image database .xml/.yml with *.gz forcing compression i.e. .xml.gz/.yml.gz>\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static std::string getFileExtension(std::string filename)
{
    return filename.substr(filename.find_last_of(".") + 1);
}

static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption;
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--imgDir") == 0) {
                i++;
                imgDir = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--fileOut") == 0) {
                i++;
                outputFilename = argv[i];
                std::string outfileNameString(outputFilename);
                if (!(getFileExtension(outfileNameString) == "xaml")) {
                    ARPRINT("Database file extension should be of .xaml type.  Name given - %s.\n", outputFilename);
                    quit(1);
                }
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
            } else {
                ARLOGi("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
    if (i == 1) {
        usage(argv[0]);
    }
}

static void quit(int rc)
{
    if (arController) {
        arController->shutdown();
        delete arController;
    }
    exit(rc);
}
