/*
 *  arMultiReadConfigFile.c
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 1.0
 * Date: 01/09/05
 *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/arMulti.h>

static char *get_buff( char *buf, int n, FILE *fp );

ARMultiMarkerInfoT *arMultiReadConfigFile( const char *filename, ARPattHandle *pattHandle )
{
    FILE                   *fp;
    ARMultiEachMarkerInfoT *marker;
    ARMultiMarkerInfoT     *marker_info;
    ARdouble               trans[3][4];
    char                   buf[256], pattPath[2048], dummy;
    int                    num;
    bool                   have_template_submarker = false;
    bool                   have_matrix_submarker = false;
    int                    i, j;

    if ((fp = fopen(filename, "r")) == NULL) {
        ARLOGe("Error: unable to open multimarker config file '%s'.\n", filename);
        ARLOGperror(NULL);
        return NULL;
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%d", &num) != 1 ) {
        ARLOGe("Error processing multimarker config file '%s': First line must be number of marker configs to read.\n", filename);
        fclose(fp);
        return NULL;
    }
    ARLOGd("Reading %d markers from multimarker file '%s'\n", num, filename);

    arMalloc(marker, ARMultiEachMarkerInfoT, num);

    for( i = 0; i < num; i++ ) {
        get_buff(buf, 256, fp);
        if (sscanf(buf, "%" SCNu64 " %c", &(marker[i].globalID), &dummy) != 1) { // Try first as matrix code.
            
            if (!pattHandle) {
                ARLOGe("Error processing multimarker config file '%s': pattern '%s' specified in multimarker configuration while in barcode-only mode.\n", filename, buf);
                goto bail1;
            }
            if (!arUtilGetDirectoryNameFromPath(pattPath, filename, sizeof(pattPath), 1)) { // Get directory prefix.
                ARLOGe("Error processing multimarker config file '%s': Unable to determine directory name.\n", filename);
                goto bail1;
            }
            strncat(pattPath, buf, sizeof(pattPath) - strlen(pattPath) - 1); // Add name of file to open.
            if ((marker[i].patt_id = arPattLoad(pattHandle, pattPath)) < 0) {
                ARLOGe("Error processing multimarker config file '%s': Unable to load pattern '%s'.\n", filename, pattPath);
                goto bail1;
            }
            marker[i].patt_type = AR_MULTI_PATTERN_TYPE_TEMPLATE;
            have_template_submarker = true;
        } else {
            
            if ((marker[i].globalID & 0xffffffff80000000ULL) == 0ULL) marker[i].patt_id = (int)(marker[i].globalID & 0x7fffffffULL); // If upper 33 bits are zero, use lower 31 bits as regular matrix code.
            else marker[i].patt_id = 0;
            ARLOGd("Marker %3d is matrix code %" PRIu64 ".\n", i + 1, marker[i].globalID);
            marker[i].patt_type = AR_MULTI_PATTERN_TYPE_MATRIX;
            have_matrix_submarker = true;
        }

        get_buff(buf, 256, fp);
        if( sscanf(buf,
#ifdef ARDOUBLE_IS_FLOAT
                   "%f",
#else
                   "%lf",
#endif
                   &marker[i].width) != 1 ) {
            ARLOGe("Error processing multimarker config file '%s', marker definition %3d: First line must be pattern width.\n", filename, i + 1);
            goto bail1;
        }
        
        j = 0;
        get_buff(buf, 256, fp);
        if( sscanf(buf,
#ifdef ARDOUBLE_IS_FLOAT
                   "%f %f %f %f",
#else
                   "%lf %lf %lf %lf",
#endif
                   &trans[j][0],
                   &trans[j][1],
                   &trans[j][2],
                   &trans[j][3]) != 4 ) {
            // Perhaps this is an old ARToolKit v2.x multimarker file?
            // If so, then the next line is two values (center) and should be skipped.
            float t1, t2;
            if( sscanf(buf,
                       "%f %f",
                       &t1, &t2) != 2 ) {
                ARLOGe("Error processing multimarker config file '%s', marker definition %3d: Lines 2 - 4 must be marker transform.\n", filename, i + 1);
                goto bail1;
            }
        } else j++;
        do {
            get_buff(buf, 256, fp);
            if( sscanf(buf, 
#ifdef ARDOUBLE_IS_FLOAT
                       "%f %f %f %f",
#else
                       "%lf %lf %lf %lf",
#endif
                       &trans[j][0],
                       &trans[j][1],
                       &trans[j][2],
                       &trans[j][3]) != 4 ) {
                ARLOGe("Error processing multimarker config file '%s', marker definition %3d: Lines 2 - 4 must be marker transform.\n", filename, i + 1);
                goto bail1;
            }
            j++;
        } while (j < 3);
        
        arMultiUpdateSubmarkerPose(&marker[i], trans);
    }

    fclose(fp);

    marker_info = arMultiAllocConfig();
    if (!marker_info) goto bail;
    marker_info->marker     = marker;
    marker_info->marker_num = num;
    if (have_template_submarker && have_matrix_submarker) marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX;
    else if (have_template_submarker) marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE;
    else if (have_matrix_submarker) marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_MATRIX;

    return marker_info;
    
bail1:
    fclose(fp);
bail:
    free(marker);
    return NULL;
}

static char *get_buff(char *buf, int n, FILE *fp)
{
    char *ret;
    size_t l;
    
    do {
        ret = fgets(buf, n, fp);
        if (ret == NULL) return (NULL); // EOF or error.
        
        // Remove NLs and CRs from end of string.
        l = strlen(buf);
        while (l > 0) {
            if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
            l--;
            buf[l] = '\0';
        }
    } while (buf[0] == '#' || buf[0] == '\0'); // Reject comments and blank lines.
    
    return (ret);
}
