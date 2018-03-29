/*
 *  genTexData.c
 *  artoolkitX
 *
 *  Generates image sets and texture data.
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifdef _WIN32
#include <windows.h>
#  define truncf(x) floorf(x) // These are the same for positive numbers.
#endif
#include <stdio.h>
#include <string.h>
#include <ARX/AR/ar.h>
#include <ARX/AR2/config.h>
#include <ARX/AR2/imageFormat.h>
#include <ARX/AR2/imageSet.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/util.h>
#include <ARX/KPM/kpm.h>
#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#if defined(__APPLE__) || defined(__linux__)
#  define HAVE_DAEMON_FUNC 1
#  include <unistd.h>
#endif
#include <time.h> // time(), localtime(), strftime()

#define          KPM_SURF_FEATURE_DENSITY_L0    70
#define          KPM_SURF_FEATURE_DENSITY_L1   100
#define          KPM_SURF_FEATURE_DENSITY_L2   150
#define          KPM_SURF_FEATURE_DENSITY_L3   200

#define          TRACKING_EXTRACTION_LEVEL_DEFAULT 2
#define          INITIALIZATION_EXTRACTION_LEVEL_DEFAULT 1
#define KPM_MINIMUM_IMAGE_SIZE 28 // Filter size for 1 octaves plus 1.
//#define KPM_MINIMUM_IMAGE_SIZE 196 // Filter size for 4 octaves plus 1.

#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

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

static int                  genfset = 1;
static int                  genfset3 = 1;

static char                 filename[MAXPATHLEN] = "";
static AR2JpegImageT       *jpegImage;
//static ARUint8             *image;
static int                  xsize, ysize;
static int                  nc;
static float                dpi = -1.0f;

static float                dpiMin = -1.0f;
static float                dpiMax = -1.0f;
static float               *dpi_list;
static int                  dpi_num = 0;

static float                sd_thresh  = -1.0f;
static float                min_thresh = -1.0f;
static float                max_thresh = -1.0f;
static int                  featureDensity = -1;
static int                  occ_size = -1;
static int                  tracking_extraction_level = -1; // Allows specification from command-line.
static int                  initialization_extraction_level = -1;

static int                  background = 0;
static char                 logfile[MAXPATHLEN] = "";
static char                 exitcodefile[MAXPATHLEN] = "";
static char                 exitcode = -1;
#define EXIT(c) {exitcode=c;exit(c);}


static void  usage( char *com );
static int   readImageFromFile(const char *filename, ARUint8 **image_p, int *xsize_p, int *ysize_p, int *nc_p, float *dpi_p);
static int   setDPI( void );
static void  write_exitcode(void);

int main( int argc, char *argv[] )
{
    AR2JpegImageT       *jpegImage = NULL;
    ARUint8             *image = NULL;
    AR2ImageSetT        *imageSet = NULL;
    AR2FeatureMapT      *featureMap = NULL;
    AR2FeatureSetT      *featureSet = NULL;
    KpmRefDataSet       *refDataSet = NULL;
    float                scale1, scale2;
    int                  procMode;
    char                 buf[1024];
    int                  num;
    int                  i, j;
    char                *sep = NULL;
	time_t				 clock;
    int                  maxFeatureNum;
    int                  err;

    for( i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "-dpi=", 5) == 0 ) {
            if( sscanf(&argv[i][5], "%f", &dpi) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-sd_thresh=", 11) == 0 ) {
            if( sscanf(&argv[i][11], "%f", &sd_thresh) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-max_thresh=", 12) == 0 ) {
            if( sscanf(&argv[i][12], "%f", &max_thresh) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-min_thresh=", 12) == 0 ) {
            if( sscanf(&argv[i][12], "%f", &min_thresh) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-feature_density=", 17) == 0 ) {
            if( sscanf(&argv[i][17], "%d", &featureDensity) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-level=", 7) == 0 ) {
            if( sscanf(&argv[i][7], "%d", &tracking_extraction_level) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-leveli=", 8) == 0 ) {
            if( sscanf(&argv[i][8], "%d", &initialization_extraction_level) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-max_dpi=", 9) == 0 ) {
            if( sscanf(&argv[i][9], "%f", &dpiMax) != 1 ) usage(argv[0]);
        } else if( strncmp(argv[i], "-min_dpi=", 9) == 0 ) {
            if( sscanf(&argv[i][9], "%f", &dpiMin) != 1 ) usage(argv[0]);
        } else if( strcmp(argv[i], "-background") == 0 ) {
            background = 1;
        } else if( strcmp(argv[i], "-nofset") == 0 ) {
            genfset = 0;
        } else if( strcmp(argv[i], "-fset") == 0 ) {
            genfset = 1;
        } else if( strcmp(argv[i], "-nofset2") == 0 ) {
            ARPRINTE("Error: -nofset2 option no longer supported as of ARToolKit v5.3.\n");
            exit(-1);
        } else if( strcmp(argv[i], "-fset2") == 0 ) {
            ARPRINTE("Error: -fset2 option no longer supported as of ARToolKit v5.3.\n");
            exit(-1);
        } else if( strcmp(argv[i], "-nofset3") == 0 ) {
            genfset3 = 0;
        } else if( strcmp(argv[i], "-fset3") == 0 ) {
            genfset3 = 1;
        } else if( strncmp(argv[i], "-log=", 5) == 0 ) {
            strncpy(logfile, &(argv[i][5]), sizeof(logfile) - 1);
            logfile[sizeof(logfile) - 1] = '\0'; // Ensure NULL termination.
        } else if( strncmp(argv[i], "-loglevel=", 10) == 0 ) {
            if (strcmp(&(argv[i][10]), "DEBUG") == 0) arLogLevel = AR_LOG_LEVEL_DEBUG;
            else if (strcmp(&(argv[i][10]), "INFO") == 0) arLogLevel = AR_LOG_LEVEL_INFO;
            else if (strcmp(&(argv[i][10]), "WARN") == 0) arLogLevel = AR_LOG_LEVEL_WARN;
            else if (strcmp(&(argv[i][10]), "ERROR") == 0) arLogLevel = AR_LOG_LEVEL_ERROR;
            else usage(argv[0]);
         } else if( strncmp(argv[i], "-exitcode=", 10) == 0 ) {
            strncpy(exitcodefile, &(argv[i][10]), sizeof(exitcodefile) - 1);
            exitcodefile[sizeof(exitcodefile) - 1] = '\0'; // Ensure NULL termination.
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
            ARPRINT("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
            exit(0);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0) {
            usage(argv[0]);
        } else if( filename[0] == '\0' ) {
            strncpy(filename, argv[i], sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0'; // Ensure NULL termination.
        } else {
            ARPRINTE("Error: unrecognised option '%s'\n", argv[i]);
            usage(argv[0]);
        }
    }
    
    // Do some checks on the input.
    if (filename[0] == '\0') {
        ARPRINTE("Error: no input file specified. Exiting.\n");
        usage(argv[0]);
    }
    sep = strrchr(filename, '.');
    if (!sep || (strcmp(sep, ".jpeg") && strcmp(sep, ".jpg") && strcmp(sep, ".jpe") && strcmp(sep, ".JPEG") && strcmp(sep, ".JPE") && strcmp(sep, ".JPG"))) {
        ARPRINTE("Error: input file must be a JPEG image (with suffix .jpeg/.jpg/.jpe). Exiting.\n");
        usage(argv[0]);
    }
    if (background) {
#if HAVE_DAEMON_FUNC
        if (filename[0] != '/' || logfile[0] != '/' || exitcodefile[0] != '/') {
            ARPRINTE("Error: -background flag requires full pathname of files (input, -log or -exitcode) to be specified. Exiting.\n");
            EXIT(E_BAD_PARAMETER);
        }
        if (tracking_extraction_level == -1 && (sd_thresh == -1.0 || min_thresh == -1.0 || max_thresh == -1.0)) {
            ARPRINTE("Error: -background flag requires -level or -sd_thresh, -min_thresh and -max_thresh -to be set. Exiting.\n");
            EXIT(E_BAD_PARAMETER);
        }
        if (initialization_extraction_level == -1 && (featureDensity == -1)) {
            ARPRINTE("Error: -background flag requires -leveli or -surf_thresh to be set. Exiting.\n");
            EXIT(E_BAD_PARAMETER);
        }
        if (dpi == -1.0) {
            ARPRINTE("Error: -background flag requires -dpi to be set. Exiting.\n");
            EXIT(E_BAD_PARAMETER);
        }
        if (dpiMin != -1.0f && (dpiMin <= 0.0f || dpiMin > dpi)) {
            ARPRINTE("Error: -min_dpi must be greater than 0 and less than or equal to -dpi. Exiting.n\n");
            EXIT(E_BAD_PARAMETER);
        }
        if (dpiMax != -1.0f && (dpiMax < dpiMin || dpiMax > dpi)) {
            ARPRINTE("Error: -max_dpi must be greater than or equal to -min_dpi and less than or equal to -dpi. Exiting.n\n");
            EXIT(E_BAD_PARAMETER);
        }
#else
        ARPRINTE("Error: -background flag not supported on this operating system. Exiting.\n");
        exit(E_BACKGROUND_OPERATION_UNSUPPORTED);
#endif
    }
    
    if (background) {
#if HAVE_DAEMON_FUNC
        // Daemonize.
        if (daemon(0, 0) == -1) {
            perror("Unable to detach from controlling terminal");
            EXIT(E_UNABLE_TO_DETACH_FROM_CONTROLLING_TERMINAL);
        }
        // At this point, stdin, stdout and stderr point to /dev/null.
#endif
    }
    
    if (logfile[0]) {
        if (!freopen(logfile, "a", stdout) ||
            !freopen(logfile, "a", stderr)) ARPRINTE("Unable to redirect stdout or stderr to logfile.\n");
    }
    if (exitcodefile[0]) {
        atexit(write_exitcode);
    }
    
    // Print the start date and time.
    clock = time(NULL);
    if (clock != (time_t)-1) {
        struct tm *timeptr = localtime(&clock);
        if (timeptr) {
            char stime[26+8] = "";
            if (strftime(stime, sizeof(stime), "%Y-%m-%d %H:%M:%S %z", timeptr)) // e.g. "1999-12-31 23:59:59 NZDT".
                ARPRINT("--\nGenerator started at %s\n", stime);
        }
    }

    if (genfset) {
        if (tracking_extraction_level == -1 && (sd_thresh  == -1.0 || min_thresh == -1.0 || max_thresh == -1.0 || occ_size == -1)) {
            do {
                printf("Select extraction level for tracking features, 0(few) <--> 4(many), [default=%d]: ", TRACKING_EXTRACTION_LEVEL_DEFAULT);
                if( fgets(buf, sizeof(buf), stdin) == NULL ) EXIT(E_USER_INPUT_CANCELLED);
                if (buf[0] == '\n') tracking_extraction_level = TRACKING_EXTRACTION_LEVEL_DEFAULT;
                else sscanf(buf, "%d", &tracking_extraction_level);
            } while (tracking_extraction_level < 0 || tracking_extraction_level > 4);
        }
        switch (tracking_extraction_level) {
            case 0:
                if( sd_thresh  == -1.0f ) sd_thresh  = AR2_DEFAULT_SD_THRESH_L0;
                if( min_thresh == -1.0f ) min_thresh = AR2_DEFAULT_MIN_SIM_THRESH_L0;
                if( max_thresh == -1.0f ) max_thresh = AR2_DEFAULT_MAX_SIM_THRESH_L0;
                if( occ_size   == -1    ) occ_size   = AR2_DEFAULT_OCCUPANCY_SIZE;
                break;
            case 1:
                if( sd_thresh  == -1.0f ) sd_thresh  = AR2_DEFAULT_SD_THRESH_L1;
                if( min_thresh == -1.0f ) min_thresh = AR2_DEFAULT_MIN_SIM_THRESH_L1;
                if( max_thresh == -1.0f ) max_thresh = AR2_DEFAULT_MAX_SIM_THRESH_L1;
                if( occ_size   == -1    ) occ_size   = AR2_DEFAULT_OCCUPANCY_SIZE;
                break;
            case 2:
                if( sd_thresh  == -1.0f ) sd_thresh  = AR2_DEFAULT_SD_THRESH_L2;
                if( min_thresh == -1.0f ) min_thresh = AR2_DEFAULT_MIN_SIM_THRESH_L2;
                if( max_thresh == -1.0f ) max_thresh = AR2_DEFAULT_MAX_SIM_THRESH_L2;
                if( occ_size   == -1    ) occ_size   = AR2_DEFAULT_OCCUPANCY_SIZE*2/3;
                break;
            case 3:
                if( sd_thresh  == -1.0f ) sd_thresh  = AR2_DEFAULT_SD_THRESH_L3;
                if( min_thresh == -1.0f ) min_thresh = AR2_DEFAULT_MIN_SIM_THRESH_L3;
                if( max_thresh == -1.0f ) max_thresh = AR2_DEFAULT_MAX_SIM_THRESH_L3;
                if( occ_size   == -1    ) occ_size   = AR2_DEFAULT_OCCUPANCY_SIZE*2/3;
                break;
            case 4: // Same as 3, but with smaller AR2_DEFAULT_OCCUPANCY_SIZE.
                if( sd_thresh  == -1.0f ) sd_thresh  = AR2_DEFAULT_SD_THRESH_L3;
                if( min_thresh == -1.0f ) min_thresh = AR2_DEFAULT_MIN_SIM_THRESH_L3;
                if( max_thresh == -1.0f ) max_thresh = AR2_DEFAULT_MAX_SIM_THRESH_L3;
                if( occ_size   == -1    ) occ_size   = AR2_DEFAULT_OCCUPANCY_SIZE*1/2;
                break;
             default: // We only get to here if the parameters are already set.
                break;
        }
        ARPRINT("MAX_THRESH  = %f\n", max_thresh);
        ARPRINT("MIN_THRESH  = %f\n", min_thresh);
        ARPRINT("SD_THRESH   = %f\n", sd_thresh);
    }
    if (genfset3) {
        if (initialization_extraction_level == -1 && featureDensity == -1) {
            do {
                printf("Select extraction level for initializing features, 0(few) <--> 3(many), [default=%d]: ", INITIALIZATION_EXTRACTION_LEVEL_DEFAULT);
                if( fgets(buf,1024,stdin) == NULL ) EXIT(E_USER_INPUT_CANCELLED);
                if (buf[0] == '\n') initialization_extraction_level = INITIALIZATION_EXTRACTION_LEVEL_DEFAULT;
                else sscanf(buf, "%d", &initialization_extraction_level);
            } while (initialization_extraction_level < 0 || initialization_extraction_level > 3);
        }
        switch(initialization_extraction_level) {
            case 0:
                if( featureDensity  == -1 ) featureDensity  = KPM_SURF_FEATURE_DENSITY_L0;
                break;
            default:
            case 1:
                if( featureDensity  == -1 ) featureDensity  = KPM_SURF_FEATURE_DENSITY_L1;
                break;
            case 2:
                if( featureDensity  == -1 ) featureDensity  = KPM_SURF_FEATURE_DENSITY_L2;
                break;
            case 3:
                if( featureDensity  == -1 ) featureDensity  = KPM_SURF_FEATURE_DENSITY_L3;
                break;
        }
        ARPRINT("SURF_FEATURE = %d\n", featureDensity);
    }

    if ((err = readImageFromFile(filename, &image, &xsize, &ysize, &nc, &dpi)) != 0) {
        ARPRINTE("Error reading image from file '%s'.\n", filename);
        EXIT(err);
    }

    setDPI();

    ARPRINT("Generating ImageSet...\n");
    ARPRINT("   (Source image xsize=%d, ysize=%d, channels=%d, dpi=%.1f).\n", xsize, ysize, nc, dpi);
    imageSet = ar2GenImageSet( image, xsize, ysize, nc, dpi, dpi_list, dpi_num );
    ar2FreeJpegImage(&jpegImage);
    if( imageSet == NULL ) {
        ARPRINTE("ImageSet generation error!!\n");
        EXIT(E_DATA_PROCESSING_ERROR);
    }
    ARPRINT("  Done.\n");
    ar2UtilRemoveExt( filename );
    ARPRINT("Saving to %s.iset...\n", filename);
    if( ar2WriteImageSet( filename, imageSet ) < 0 ) {
        ARPRINTE("Save error: %s.iset\n", filename );
        EXIT(E_DATA_PROCESSING_ERROR);
    }
    ARPRINT("  Done.\n");

    if (genfset) {
        arMalloc( featureSet, AR2FeatureSetT, 1 );                      // A featureSet with a single image,
        arMalloc( featureSet->list, AR2FeaturePointsT, imageSet->num ); // and with 'num' scale levels of this image.
        featureSet->num = imageSet->num;
        
        ARPRINT("Generating FeatureList...\n");
        for( i = 0; i < imageSet->num; i++ ) {
            ARPRINT("Start for %f dpi image.\n", imageSet->scale[i]->dpi);
            
            featureMap = ar2GenFeatureMap( imageSet->scale[i],
                                          AR2_DEFAULT_TS1*AR2_TEMP_SCALE, AR2_DEFAULT_TS2*AR2_TEMP_SCALE,
                                          AR2_DEFAULT_GEN_FEATURE_MAP_SEARCH_SIZE1, AR2_DEFAULT_GEN_FEATURE_MAP_SEARCH_SIZE2,
                                          AR2_DEFAULT_MAX_SIM_THRESH2, AR2_DEFAULT_SD_THRESH2 );
            if( featureMap == NULL ) {
                ARPRINTE("Error!!\n");
                EXIT(E_DATA_PROCESSING_ERROR);
            }
            ARPRINT("  Done.\n");
            
            
            featureSet->list[i].coord = ar2SelectFeature2( imageSet->scale[i], featureMap,
                                                          AR2_DEFAULT_TS1*AR2_TEMP_SCALE, AR2_DEFAULT_TS2*AR2_TEMP_SCALE, AR2_DEFAULT_GEN_FEATURE_MAP_SEARCH_SIZE2,
                                                          occ_size,
                                                          max_thresh, min_thresh, sd_thresh, &num );
            if( featureSet->list[i].coord == NULL ) num = 0;
            featureSet->list[i].num   = num;
            featureSet->list[i].scale = i;
            
            scale1 = 0.0f;
            for( j = 0; j < imageSet->num; j++ ) {
                if( imageSet->scale[j]->dpi < imageSet->scale[i]->dpi ) {
                    if( imageSet->scale[j]->dpi > scale1 ) scale1 = imageSet->scale[j]->dpi;
                }
            }
            if( scale1 == 0.0f ) {
                featureSet->list[i].mindpi = imageSet->scale[i]->dpi * 0.5f;
            }
            else {
                /*
                 scale2 = imageSet->scale[i]->dpi;
                 scale = sqrtf( scale1 * scale2 );
                 featureSet->list[i].mindpi = scale2 / ((scale2/scale - 1.0f)*1.1f + 1.0f);
                 */
                featureSet->list[i].mindpi = scale1;
            }
            
            scale1 = 0.0f;
            for( j = 0; j < imageSet->num; j++ ) {
                if( imageSet->scale[j]->dpi > imageSet->scale[i]->dpi ) {
                    if( scale1 == 0.0f || imageSet->scale[j]->dpi < scale1 ) scale1 = imageSet->scale[j]->dpi;
                }
            }
            if( scale1 == 0.0f ) {
                featureSet->list[i].maxdpi = imageSet->scale[i]->dpi * 2.0f;
            }
            else {
                //scale2 = imageSet->scale[i]->dpi * 1.2f;
                scale2 = imageSet->scale[i]->dpi;
                /*
                 scale = sqrtf( scale1 * scale2 );
                 featureSet->list[i].maxdpi = scale2 * ((scale/scale2 - 1.0f)*1.1f + 1.0f);
                 */
                featureSet->list[i].maxdpi = scale2*0.8f + scale1*0.2f;
            }
            
            ar2FreeFeatureMap( featureMap );
        }
        ARPRINT("  Done.\n");
        
        ARPRINT("Saving FeatureSet...\n");
        if( ar2SaveFeatureSet( filename, "fset", featureSet ) < 0 ) {
            ARPRINTE("Save error: %s.fset\n", filename );
            EXIT(E_DATA_PROCESSING_ERROR);
        }
        ARPRINT("  Done.\n");
        ar2FreeFeatureSet( &featureSet );
    }
    
    if (genfset3) {
        ARPRINT("Generating FeatureSet3...\n");
        refDataSet  = NULL;
        procMode    = KpmProcFullSize;
        for( i = 0; i < imageSet->num; i++ ) {
            //if( imageSet->scale[i]->dpi > 100.0f ) continue;
            
            maxFeatureNum = featureDensity * imageSet->scale[i]->xsize * imageSet->scale[i]->ysize / (480*360);
            ARPRINT("(%d, %d) %f[dpi]\n", imageSet->scale[i]->xsize, imageSet->scale[i]->ysize, imageSet->scale[i]->dpi);
            if( kpmAddRefDataSet (
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
                                  imageSet->scale[i]->imgBWBlur[1],
#else
                                  imageSet->scale[i]->imgBW,
#endif
                                  imageSet->scale[i]->xsize,
                                  imageSet->scale[i]->ysize,
                                  imageSet->scale[i]->dpi,
                                  procMode, KpmCompNull, maxFeatureNum, 1, i, &refDataSet) < 0 ) { // Page number set to 1 by default.
                ARPRINTE("Error at kpmAddRefDataSet.\n");
                EXIT(E_DATA_PROCESSING_ERROR);
            }
        }
        ARPRINT("  Done.\n");
        ARPRINT("Saving FeatureSet3...\n");
        if( kpmSaveRefDataSet(filename, "fset3", refDataSet) != 0 ) {
            ARPRINTE("Save error: %s.fset2\n", filename );
            EXIT(E_DATA_PROCESSING_ERROR);
        }
        ARPRINT("  Done.\n");
        kpmDeleteRefDataSet( &refDataSet );
    }
    
    ar2FreeImageSet( &imageSet );

    // Print the start date and time.
    clock = time(NULL);
    if (clock != (time_t)-1) {
        struct tm *timeptr = localtime(&clock);
        if (timeptr) {
            char stime[26+8] = "";
            if (strftime(stime, sizeof(stime), "%Y-%m-%d %H:%M:%S %z", timeptr)) // e.g. "1999-12-31 23:59:59 NZDT".
                ARPRINT("Generator finished at %s\n--\n", stime);
        }
    }

    exitcode = E_NO_ERROR;
    return (exitcode);
}

// Reads dpiMinAllowable, xsize, ysize, dpi, background, dpiMin, dpiMax.
// Sets dpiMin, dpiMax, dpi_num, dpi_list.
static int setDPI( void )
{
    float       dpiWork, dpiMinAllowable;
    char		buf1[256];
    int			i;

    // Determine minimum allowable DPI, truncated to 3 decimal places.
    dpiMinAllowable = truncf(((float)KPM_MINIMUM_IMAGE_SIZE / (float)(MIN(xsize, ysize))) * dpi * 1000.0) / 1000.0f;
    
    if (background) {
        if (dpiMin == -1.0f) dpiMin = dpiMinAllowable;
        if (dpiMax == -1.0f) dpiMax = dpi;
    }

    if (dpiMin == -1.0f) {
        for (;;) {
            printf("Enter the minimum image resolution (DPI, in range [%.3f, %.3f]): ", dpiMinAllowable, (dpiMax == -1.0f ? dpi : dpiMax));
            if( fgets( buf1, 256, stdin ) == NULL ) EXIT(E_USER_INPUT_CANCELLED);
            if( sscanf(buf1, "%f", &dpiMin) == 0 ) continue;
            if (dpiMin >= dpiMinAllowable && dpiMin <= (dpiMax == -1.0f ? dpi : dpiMax)) break;
            else printf("Error: you entered %.3f, but value must be greater than or equal to %.3f and less than or equal to %.3f.\n", dpiMin, dpiMinAllowable, (dpiMax == -1.0f ? dpi : dpiMax));
        }
    } else if (dpiMin < dpiMinAllowable) {
        ARPRINTE("Warning: -min_dpi=%.3f smaller than minimum allowable. Value will be adjusted to %.3f.\n", dpiMin, dpiMinAllowable);
        dpiMin = dpiMinAllowable;
    }
    if (dpiMax == -1.0f) {
        for (;;) {
            printf("Enter the maximum image resolution (DPI, in range [%.3f, %.3f]): ", dpiMin, dpi);
            if( fgets( buf1, 256, stdin ) == NULL ) EXIT(E_USER_INPUT_CANCELLED);
            if( sscanf(buf1, "%f", &dpiMax) == 0 ) continue;
            if (dpiMax >= dpiMin && dpiMax <= dpi) break;
            else printf("Error: you entered %.3f, but value must be greater than or equal to minimum resolution (%.3f) and less than or equal to image resolution (%.3f).\n", dpiMax, dpiMin, dpi);
        }
    } else if (dpiMax > dpi) {
        ARPRINTE("Warning: -max_dpi=%.3f larger than maximum allowable. Value will be adjusted to %.3f.\n", dpiMax, dpi);
        dpiMax = dpi;
    }
    
    // Decide how many levels we need.
    if (dpiMin == dpiMax) {
        dpi_num = 1;
    } else {
        dpiWork = dpiMin;
        for( i = 1;; i++ ) {
            dpiWork *= powf(2.0f, 1.0f/3.0f); // *= 1.25992104989487
            if( dpiWork >= dpiMax*0.95f ) {
                break;
            }
        }
        dpi_num = i + 1;
    }
    arMalloc(dpi_list, float, dpi_num);
    
    // Determine the DPI values of each level.
    dpiWork = dpiMin;
    for( i = 0; i < dpi_num; i++ ) {
        ARPRINT("Image DPI (%d): %f\n", i+1, dpiWork);
        dpi_list[dpi_num - i - 1] = dpiWork; // Lowest value goes at tail of array, highest at head.
        dpiWork *= powf(2.0f, 1.0f/3.0f);
        if( dpiWork >= dpiMax*0.95f ) dpiWork = dpiMax;
    }

    return 0;
}

static void usage( char *com )
{
    if (!background) {
        ARPRINT("%s <filename>\n", com);
        ARPRINT("    -level=n\n"
              "         (n is an integer in range 0 (few) to 4 (many). Default %d.'\n", TRACKING_EXTRACTION_LEVEL_DEFAULT);
        ARPRINT("    -sd_thresh=<sd_thresh>\n");
        ARPRINT("    -max_thresh=<max_thresh>\n");
        ARPRINT("    -min_thresh=<min_thresh>\n");
        ARPRINT("    -leveli=n\n"
              "         (n is an integer in range 0 (few) to 3 (many). Default %d.'\n", INITIALIZATION_EXTRACTION_LEVEL_DEFAULT);
        ARPRINT("    -feature_density=<feature_density>\n");
        ARPRINT("    -dpi=f: Override embedded JPEG DPI value.\n");
        ARPRINT("    -max_dpi=<max_dpi>\n");
        ARPRINT("    -min_dpi=<min_dpi>\n");
        ARPRINT("    -background\n");
        ARPRINT("         Run in background, i.e. as daemon detached from controlling terminal. (macOS and Linux only.)\n");
        ARPRINT("    -log=<path>\n");
        ARPRINT("    -loglevel=x\n");
        ARPRINT("         x is one of: DEBUG, INFO, WARN, ERROR. Default is %s.\n", (AR_LOG_LEVEL_DEFAULT == AR_LOG_LEVEL_DEBUG ? "DEBUG" : (AR_LOG_LEVEL_DEFAULT == AR_LOG_LEVEL_INFO ? "INFO" : (AR_LOG_LEVEL_DEFAULT == AR_LOG_LEVEL_WARN ? "WARN" : (AR_LOG_LEVEL_DEFAULT == AR_LOG_LEVEL_ERROR ? "ERROR" : "UNKNOWN")))));
        ARPRINT("    -exitcode=<path>\n");
        ARPRINT("    --help -h -?  Display this help\n");
    }

    EXIT(E_BAD_PARAMETER);
}

static int readImageFromFile(const char *filename, ARUint8 **image_p, int *xsize_p, int *ysize_p, int *nc_p, float *dpi_p)
{
    char *ext;
    char buf[256];
    char buf1[512], buf2[512];
    
    if (!filename || !image_p || !xsize_p || !ysize_p || !nc_p || !dpi_p) return (E_BAD_PARAMETER);

    ext = arUtilGetFileExtensionFromPath(filename, 1);
    if (!ext) {
        ARPRINTE("Error: unable to determine extension of file '%s'. Exiting.\n", filename);
        EXIT(E_INPUT_DATA_ERROR);
    }
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0 || strcmp(ext, "jpe") == 0) {
        
        ARPRINT("Reading JPEG file...\n");
        ar2UtilDivideExt( filename, buf1, buf2 );
        jpegImage = ar2ReadJpegImage( buf1, buf2 );
        if( jpegImage == NULL ) {
            ARPRINTE("Error: unable to read JPEG image from file '%s'. Exiting.\n", filename);
            EXIT(E_INPUT_DATA_ERROR);
        }
        ARPRINT("   Done.\n");
        
        *image_p = jpegImage->image;
        if (jpegImage->nc != 1 && jpegImage->nc != 3) {
            ARPRINTE("Error: Input JPEG image is in neither RGB nor grayscale format. %d bytes/pixel %sformat is unsupported. Exiting.\n", jpegImage->nc, (jpegImage->nc == 4 ? "(possibly CMYK) " : ""));
            EXIT(E_INPUT_DATA_ERROR);
        }
        *nc_p    = jpegImage->nc;
        ARPRINT("JPEG image '%s' is %dx%d.\n", filename, jpegImage->xsize, jpegImage->ysize);
        if (jpegImage->xsize < KPM_MINIMUM_IMAGE_SIZE || jpegImage->ysize < KPM_MINIMUM_IMAGE_SIZE) {
            ARPRINTE("Error: JPEG image width and height must be at least %d pixels. Exiting.\n", KPM_MINIMUM_IMAGE_SIZE);
            EXIT(E_INPUT_DATA_ERROR);
        }
        *xsize_p = jpegImage->xsize;
        *ysize_p = jpegImage->ysize;
        if (*dpi_p == -1.0) {
            if( jpegImage->dpi == 0.0f ) {
                for (;;) {
                    printf("JPEG image '%s' does not contain embedded resolution data, and no resolution specified on command-line.\nEnter resolution to use (in decimal DPI): ", filename);
                    if( fgets( buf, 256, stdin ) == NULL ) {
                        EXIT(E_USER_INPUT_CANCELLED);
                    }
                    if( sscanf(buf, "%f", &(jpegImage->dpi)) == 1 ) break;
                }
            }
            *dpi_p   = jpegImage->dpi;
        }

    //} else if (strcmp(ext, "png") == 0) {
        
        
        
    } else {
        ARPRINTE("Error: file '%s' has extension '%s', which is not supported for reading. Exiting.\n", filename, ext);
        free(ext);
        EXIT(E_INPUT_DATA_ERROR);
    }
    free(ext);
    
    return 0;
}

static void write_exitcode(void)
{
    if (exitcodefile[0]) {
        FILE *fp = fopen(exitcodefile, "w");
        fprintf(fp, "%d\n", exitcode);
        fclose(fp);
    }
}

