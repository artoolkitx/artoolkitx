/*
 *  arMultiEditConfig.c
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
 *  Copyright 2018 Philip Lamb
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/AR/ar.h>
#include <ARX/AR/arMulti.h>
#include <string.h> // memset()

ARMultiMarkerInfoT *arMultiAllocConfig(void)
{
    ARMultiMarkerInfoT *marker_info = (ARMultiMarkerInfoT *)malloc(sizeof(ARMultiMarkerInfoT));
    if (!marker_info) {
        ARLOGe("arMultiAllocConfig out of memory!!\n");
        return NULL;
    }
    
    marker_info->marker = NULL;
    marker_info->marker_num = 0;
    marker_info->prevF = 0;
    marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_NONE;
    marker_info->cfPattCutoff = AR_MULTI_CONFIDENCE_PATTERN_CUTOFF_DEFAULT;
    marker_info->cfMatrixCutoff = AR_MULTI_CONFIDENCE_MATRIX_CUTOFF_DEFAULT;
    marker_info->min_submarker = 0;
    marker_info->minInlierProb = ICP_INLIER_PROBABILITY;
    
    return (marker_info);
}

ARMultiMarkerInfoT *arMultiCopyConfig(const ARMultiMarkerInfoT *marker_info)
{
    if (!marker_info) return NULL;
    
    ARMultiMarkerInfoT *mi = (ARMultiMarkerInfoT *)malloc(sizeof(ARMultiMarkerInfoT));
    if (!mi) {
        ARLOGe("arMultiCopyConfig out of memory!!\n");
        return NULL;
    }
    
    size_t emi_size = marker_info->marker_num * sizeof(ARMultiEachMarkerInfoT);
    mi->marker = (ARMultiEachMarkerInfoT *)malloc(emi_size);
    if (!mi->marker) {
        ARLOGe("arMultiCopyConfig out of memory!!\n");
        free(mi);
        return NULL;
    }
    memcpy(mi->marker, marker_info->marker, emi_size);
    mi->marker_num = marker_info->marker_num;
    
    memcpy(mi->trans, marker_info->trans, 12*sizeof(ARdouble));
    mi->prevF = marker_info->prevF;
    mi->patt_type = marker_info->patt_type;
    mi->cfPattCutoff = marker_info->cfPattCutoff;
    mi->cfMatrixCutoff = marker_info->cfMatrixCutoff;
    mi->min_submarker = marker_info->min_submarker;
    mi->minInlierProb = marker_info->minInlierProb;

    return (mi);
}

// patt_type: Either AR_MULTI_PATTERN_TYPE_TEMPLATE or AR_MULTI_PATTERN_TYPE_MATRIX.
int arMultiAddOrUpdateSubmarker(ARMultiMarkerInfoT *marker_info, int patt_id, int patt_type, ARdouble width, const ARdouble trans[3][4], uint64_t globalID)
{
    int i;
    
    // Look for matching marker already in set.
    for (i = 0; i < marker_info->marker_num; i++) {
        if (marker_info->marker[i].patt_type == patt_type && marker_info->marker[i].patt_id == patt_id) {
            if (patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE || (patt_type == AR_MULTI_PATTERN_TYPE_MATRIX && marker_info->marker[i].globalID == globalID)) break;
        }
    }
    
    if (i == marker_info->marker_num) { // Not found, need to add to it.
        
        // Increase the array size.
        ARMultiEachMarkerInfoT *emi;
        if (!marker_info->marker) emi = (ARMultiEachMarkerInfoT *)malloc(sizeof(ARMultiEachMarkerInfoT));
        else emi = (ARMultiEachMarkerInfoT *)realloc(marker_info->marker, sizeof(ARMultiEachMarkerInfoT) * (marker_info->marker_num + 1));
        if (!emi) {
            ARLOGe("arMultiAddOrUpdateSubmarker out of memory!!\n");
            return (-1);
        }
        marker_info->marker = emi;
        marker_info->marker_num++;

        // As we've enlarged the array, i is now a valid index.
        memset(&marker_info->marker[i], 0, sizeof(ARMultiEachMarkerInfoT));
        marker_info->marker[i].patt_id = patt_id;
        marker_info->marker[i].patt_type = patt_type;
        marker_info->marker[i].width = width;
        if (patt_type == AR_MULTI_PATTERN_TYPE_MATRIX) {
            marker_info->marker[i].globalID = globalID;
        }
    }
    
    arMultiUpdateSubmarkerPose(&marker_info->marker[i], trans);
    
    if (patt_type == AR_MULTI_PATTERN_TYPE_MATRIX) {
        if (marker_info->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE) {
            marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX;
        } else {
            marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_MATRIX;
        }
    } else { // patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE
        if (marker_info->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX) {
            marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX;
        } else {
            marker_info->patt_type = AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE;
        }
    }
    
    return 0;
}

void arMultiUpdateSubmarkerPose(ARMultiEachMarkerInfoT *submarker, const ARdouble trans[3][4])
{
    int i, j;
    ARdouble wpos3d[4][2];
    
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            submarker->trans[j][i] = trans[j][i];
        }
    }
    
    arUtilMatInv((const ARdouble (*)[4])submarker->trans, submarker->itrans);
    
    wpos3d[0][0] =  -submarker->width/2.0;
    wpos3d[0][1] =   submarker->width/2.0;
    wpos3d[1][0] =   submarker->width/2.0;
    wpos3d[1][1] =   submarker->width/2.0;
    wpos3d[2][0] =   submarker->width/2.0;
    wpos3d[2][1] =  -submarker->width/2.0;
    wpos3d[3][0] =  -submarker->width/2.0;
    wpos3d[3][1] =  -submarker->width/2.0;
    for (j = 0; j < 4; j++) {
        submarker->pos3d[j][0] = submarker->trans[0][0] * wpos3d[j][0]
                               + submarker->trans[0][1] * wpos3d[j][1]
                               + submarker->trans[0][3];
        submarker->pos3d[j][1] = submarker->trans[1][0] * wpos3d[j][0]
                               + submarker->trans[1][1] * wpos3d[j][1]
                               + submarker->trans[1][3];
        submarker->pos3d[j][2] = submarker->trans[2][0] * wpos3d[j][0]
                               + submarker->trans[2][1] * wpos3d[j][1]
                               + submarker->trans[2][3];
    }
}

int arMultiRemoveSubmarker(ARMultiMarkerInfoT *marker_info, int patt_id, int patt_type, uint64_t globalID)
{
    int i;
    
    // Look for matching marker already in set.
    for (i = 0; i < marker_info->marker_num; i++) {
        if (marker_info->marker[i].patt_type == patt_type && marker_info->marker[i].patt_id == patt_id) {
            if (patt_type == AR_MULTI_PATTERN_TYPE_TEMPLATE || (patt_type == AR_MULTI_PATTERN_TYPE_MATRIX && marker_info->marker[i].globalID == globalID)) break;
        }
    }

    if (i == marker_info->marker_num) return -1; // Not found.
    
    // Shuffle the rest down.
    for (i++; i < marker_info->marker_num; i++) {
        marker_info->marker[i - 1] = marker_info->marker[i];
    }
    
    // Reduce the array size. Because we're shrinking, realloc failure isn't fatal.
    ARMultiEachMarkerInfoT *emi = (ARMultiEachMarkerInfoT *)realloc(marker_info->marker, sizeof(ARMultiEachMarkerInfoT) * (marker_info->marker_num - 1));
    if (!emi) {
        ARLOGw("arMultiRemoveSubmarker out of memory!!\n.");
    } else {
        marker_info->marker = emi;
    }
    marker_info->marker_num--;

    return 0;
}
