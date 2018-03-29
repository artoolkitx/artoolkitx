/*
 *  videoAspectRatio.c
 *  artoolkitX
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#include <ARX/ARVideo/video.h>
#include <stdio.h>
#include <string.h> // strdup(), asprintf()

struct _ASPECT_RATIOS_ENTRY {
    int width;
    int height;
    AR_VIDEO_ASPECT_RATIO aspectRatio;
    char *name;
};

const static struct _ASPECT_RATIOS_ENTRY aspectRatios[] =
{
    {1,  1,  AR_VIDEO_ASPECT_RATIO_1_1, "1:1"},    // 1.0:
    {11, 9,  AR_VIDEO_ASPECT_RATIO_11_9, "11:9"},  // 1.222: 176x144 (QCIF), 352x288 (CIF)
    {5,  4,  AR_VIDEO_ASPECT_RATIO_5_4, "5:4"},    // 1.25:  1280x1024 (SXGA), 2560x2048
    {4,  3,  AR_VIDEO_ASPECT_RATIO_4_3, "4:3"},    // 1.333: 320x240 (QVGA), 480x360, 640x480 (VGA), 768x576 (576p), 800x600 (SVGA), 832x624, 960x720, 1024x768 (XGA), 1152x864, 1280x960, 1400x1050, 1600x1200, 2048x1536
    {3,  2,  AR_VIDEO_ASPECT_RATIO_3_2, "3:2"},    // 1.5:   240x160, 480x320, 960x640, 720x480 (480p), 1152x768, 1280x854, 1440x960
    {14, 9,  AR_VIDEO_ASPECT_RATIO_14_9, "14:9"},  // 1.556:
    {8,  5,  AR_VIDEO_ASPECT_RATIO_8_5, "8:5"},    // 1.6:   320x200, 1280x800, 1440x900, 1680x1050, 1920x1200, 2560x1600
    {5,  3,  AR_VIDEO_ASPECT_RATIO_5_3, "5:3"},    // 1.667: 800x480, 1280x768, 1600x960
    {16, 9,  AR_VIDEO_ASPECT_RATIO_16_9, "16:9"},  // 1.778: 640x360, 960x540, 1024x576, 1280x720 (720p), 1600x900, 1920x1080 (1080p)
    {9,  5,  AR_VIDEO_ASPECT_RATIO_9_5, "9:5"},    // 1.8:   864x480
    {17, 9,  AR_VIDEO_ASPECT_RATIO_17_9, "17:9"},  // 1.889: 2040x1080
    {21, 9,  AR_VIDEO_ASPECT_RATIO_21_9, "21:9"},  // 2.333: 2560x1080
    
    // Some values that are close to standard ratios.
    {683, 384, AR_VIDEO_ASPECT_RATIO_16_9, "16:9"}, // ~1.778: 1366x768
    {85,  48,  AR_VIDEO_ASPECT_RATIO_16_9, "16:9"}, // ~1.778: 1360x768
    {256, 135, AR_VIDEO_ASPECT_RATIO_17_9, "17:9"}, // ~1.889: 2048x1080 (2K)
    {512, 307, AR_VIDEO_ASPECT_RATIO_5_3, "5:3"},   // ~1.667: 1024x614
    {30,  23,  AR_VIDEO_ASPECT_RATIO_4_3, "4:3"},   // ~1.333: 480x368
    {128, 69,  AR_VIDEO_ASPECT_RATIO_17_9, "17:9"}, // ~1.889: 1024x552
    {30,  23,  AR_VIDEO_ASPECT_RATIO_11_9, "11:9"}, // ~1.222: 592x480, 480x368
    {53,  30,  AR_VIDEO_ASPECT_RATIO_16_9, "16:9"}, // ~1.767: 848x480
    {37,  30,  AR_VIDEO_ASPECT_RATIO_11_9, "11:9"}, // ~1.233: 592x480
    {192, 145, AR_VIDEO_ASPECT_RATIO_4_3, "4:3"},   // ~1.324: 1152x870
    {640, 427, AR_VIDEO_ASPECT_RATIO_3_2, "3:2"},   // ~1.499: 1280x854
    {427, 240, AR_VIDEO_ASPECT_RATIO_16_9, "16:9"}  // ~1.779: 854x480
};
#define _ASPECT_RATIOS_COUNT (sizeof(aspectRatios)/sizeof(aspectRatios[0]))

AR_VIDEO_ASPECT_RATIO arVideoUtilFindAspectRatio(int w, int h)
{
    int i;
    
    // Reduce.
    int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
#define PRIMES_COUNT (sizeof(primes)/sizeof(primes[0]))

    int w_lcd = w, h_lcd = h;
    for (i = 0; i < PRIMES_COUNT; i++) {
        int prime = primes[i];
        while (w_lcd >= prime && h_lcd >= prime && w_lcd % prime == 0 && h_lcd % prime == 0) {
            w_lcd /= prime; h_lcd /= prime;
        }
    }
    
    // Find.
    for (i = 0; i < _ASPECT_RATIOS_COUNT; i++) {
        if (w_lcd == aspectRatios[i].width && h_lcd == aspectRatios[i].height) return aspectRatios[i].aspectRatio;
    }
    return (AR_VIDEO_ASPECT_RATIO_UNIQUE);
}

char *arVideoUtilFindAspectRatioName(int w, int h)
{
    int i;
    const char format[] = "%d:%d";
#ifdef _WIN32
    int len;
#endif
    char *ret;
    
    // Reduce.
    int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
#define PRIMES_COUNT (sizeof(primes)/sizeof(primes[0]))
    
    int w_lcd = w, h_lcd = h;
    for (i = 0; i < PRIMES_COUNT; i++) {
        int prime = primes[i];
        while (w_lcd >= prime && h_lcd >= prime && w_lcd % prime == 0 && h_lcd % prime == 0) {
            w_lcd /= prime; h_lcd /= prime;
        }
    }
    
    // Find.
    for (i = 0; i < _ASPECT_RATIOS_COUNT; i++) {
        if (w_lcd == aspectRatios[i].width && h_lcd == aspectRatios[i].height) return (strdup(aspectRatios[i].name));
    }
#ifdef _WIN32
    len = _scprintf(format, w, h);
    if (len >= 0) {
        ret = (char *)malloc((len + 1)*sizeof(char)); // +1 for nul-term.
        if (!ret) return (NULL);
        sprintf(ret, format, w, h);
    }
#else
    if (asprintf(&ret, format, w, h) == -1) return (NULL);
#endif
    return (ret);
}
