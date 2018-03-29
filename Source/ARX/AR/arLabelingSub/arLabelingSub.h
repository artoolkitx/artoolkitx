/*
 *  arLabeling.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
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
 * Revision: 4.0
 * Date: 03/08/13
 *
 *******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // memset()
#include <ARX/AR/ar.h>
#include "arLabelingPrivate.h"

#define AR_PIXEL_SIZE     1

#ifndef AR_LABELING_ADAPTIVE
#  ifndef AR_LABELING_DEBUG_ENABLE_F
#    ifndef AR_LABELING_WHITE_REGION_F
#      ifndef AR_LABELING_FRAME_IMAGE_F
int arLabelingSubDBIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      else
int arLabelingSubDBRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      endif // !AR_LABELING_FRAME_IMAGE_F
#    else
#      ifndef AR_LABELING_FRAME_IMAGE_F
int arLabelingSubDWIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      else
int arLabelingSubDWRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      endif // !AR_LABELING_FRAME_IMAGE_F
#    endif // !AR_LABELING_WHITE_REGION_F
#  else
#    ifndef AR_LABELING_WHITE_REGION_F
#      ifndef AR_LABELING_FRAME_IMAGE_F
int arLabelingSubEBIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      else
int arLabelingSubEBRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      endif // !AR_LABELING_FRAME_IMAGE_F
#    else
#      ifndef AR_LABELING_FRAME_IMAGE_F
int arLabelingSubEWIC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      else
int arLabelingSubEWRC( ARUint8 *image, int xsize, int ysize, int labelingThresh, ARLabelInfo *labelInfo )
#      endif // !AR_LABELING_FRAME_IMAGE_F
#    endif // !AR_LABELING_WHITE_REGION_F
#  endif // !AR_LABELING_DEBUG_ENABLE_F
#else
#  ifndef AR_LABELING_DEBUG_ENABLE_F
#    ifndef AR_LABELING_WHITE_REGION_F
int arLabelingSubDBZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo )
#    else
int arLabelingSubDWZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo )
#    endif // !AR_LABELING_WHITE_REGION_F
#  else
#    ifndef AR_LABELING_WHITE_REGION_F
int arLabelingSubEBZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo )
#    else
int arLabelingSubEWZ( ARUint8 *image, const int xsize, const int ysize, ARUint8* image_thresh, ARLabelInfo *labelInfo )
#    endif // !AR_LABELING_WHITE_REGION_F
#  endif // !AR_LABELING_DEBUG_ENABLE_F
#endif

{
    int       lxsize, lysize;
    ARUint8  *pnt;                     /*  image pointer into source image  */
#ifdef AR_LABELING_ADAPTIVE
    ARUint8  *pnt_thresh;
#endif
    AR_LABELING_LABEL_TYPE  *pnt1, *pnt2;             /*  image pointer into destination (label) image  */
#ifdef AR_LABELING_DEBUG_ENABLE_F
    ARUint8   *dpnt;
#endif
    int      *work, *work2;
    int       wk_max;                   /*  work                */
    int       i,j,k,l;                  /*  for loop            */
    int       *wk;                      /*  pointer for work    */
    int       m,n;                      /*  work                */
    int       *label_num;
    int       *area;
    int       *clip;
    ARdouble  *pos;

#ifdef AR_LABELING_FRAME_IMAGE_F
    lxsize = xsize;
    lysize = ysize;
#else
    lxsize = xsize / 2;
    lysize = ysize / 2;
#endif

#ifdef AR_LABELING_DEBUG_ENABLE_F
    //memset( labelInfo->bwImage, 0, lxsize*lysize );
#endif

	// Set top and bottom rows of labelImage to 0.
    pnt1 = &(labelInfo->labelImage[0]); // Leftmost pixel of top row of image.
    pnt2 = &(labelInfo->labelImage[(lysize - 1)*lxsize]); // Leftmost pixel of bottom row of image.
    for(i = 0; i < lxsize; i++) {
        *(pnt1++) = *(pnt2++) = 0;
    }

	// Set leftmost and rightmost columns of labelImage to 0.
    pnt1 = &(labelInfo->labelImage[0]); // Leftmost pixel of top row of image.
    pnt2 = &(labelInfo->labelImage[lxsize - 1]); // Rightmost pixel of top row of image.
    for(i = 0; i < lysize; i++) {
        *pnt1 = *pnt2 = 0;
        pnt1 += lxsize;
        pnt2 += lxsize;
    }

    wk_max = 0;
    work = labelInfo->work;
    work2 = labelInfo->work2;
    pnt2 = &(labelInfo->labelImage[lxsize + 1]); // Start on 2nd pixel of 2nd row.
#ifdef AR_LABELING_DEBUG_ENABLE_F
    dpnt = &(labelInfo->bwImage[lxsize + 1]);
#  ifdef AR_LABELING_FRAME_IMAGE_F
    pnt = &(image[(xsize + 1)*AR_PIXEL_SIZE]); // Start on 2nd pixel of 2nd row.
#    ifdef AR_LABELING_ADAPTIVE
    pnt_thresh = &(image_thresh[(xsize + 1)*AR_PIXEL_SIZE]);
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*2, pnt_thresh += AR_PIXEL_SIZE*2, pnt2 += 2, dpnt += 2) { // Process rows. At end of each row, skips last pixel of row and first pixel of next row.
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE, pnt_thresh += AR_PIXEL_SIZE, pnt2++, dpnt++) { // Process columns.
#    else
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*2, pnt2 += 2, dpnt += 2) { // Process rows. At end of each row, skips last pixel of row and first pixel of next row.
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE, pnt2++, dpnt++) { // Process columns.
#    endif
#  else
    pnt = &(image[(xsize*2 + 2)*AR_PIXEL_SIZE]);
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*4, pnt2 += 2, dpnt += 2) {
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE*2, pnt2++, dpnt++) {
#  endif
#else
#  ifdef AR_LABELING_FRAME_IMAGE_F
    pnt = &(image[(xsize + 1)*AR_PIXEL_SIZE]); // Start on 2nd pixel of 2nd row.
#    ifdef AR_LABELING_ADAPTIVE
    pnt_thresh = &(image_thresh[(xsize + 1)*AR_PIXEL_SIZE]);
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*2, pnt_thresh += AR_PIXEL_SIZE*2, pnt2 += 2) { // Process rows. At end of each row, skips last pixel of row and first pixel of next row.
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE, pnt_thresh += AR_PIXEL_SIZE, pnt2++) { // Process columns.
#    else
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*2, pnt2 += 2) { // Process rows. At end of each row, skips last pixel of row and first pixel of next row.
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE, pnt2++) { // Process columns.
#    endif
#  else
    pnt = &(image[(xsize*2 + 2)*AR_PIXEL_SIZE]);
    for(j = 1; j < lysize - 1; j++, pnt += AR_PIXEL_SIZE*4, pnt2 += 2) {
        for(i = 1; i < lxsize - 1; i++, pnt += AR_PIXEL_SIZE*2, pnt2++) {
#  endif
#endif // AR_LABELING_DEBUG_ENABLE_F

#ifndef AR_LABELING_WHITE_REGION_F
// Black region.
#  ifndef AR_LABELING_ADAPTIVE
            if( *pnt <= labelingThresh ) {
#  else
            if( *pnt <= *pnt_thresh ) {
#  endif
#else
// White region.
#  ifndef AR_LABELING_ADAPTIVE
            if( *pnt > labelingThresh ) {
#  else
            if( *pnt > *pnt_thresh ) {
#  endif
#endif // !AR_LABELING_WHITE_REGION_F
                // pnt is in region.
#  ifdef AR_LABELING_DEBUG_ENABLE_F
                *dpnt = 255;
#  endif
                pnt1 = &(pnt2[-lxsize]);
                if( *pnt1 > 0 ) {
                    *pnt2 = *pnt1;
                    l = ((*pnt2) - 1) * 7;
                    work2[l+0] ++; // area
                    work2[l+1] += i; // pos[0]
                    work2[l+2] += j; // pos[1]
                    work2[l+6]  = j; // clip[3]
                }
                else if( *(pnt1+1) > 0 ) {
                    if( *(pnt1-1) > 0 ) {
                        m = work[*(pnt1+1)-1];
                        n = work[*(pnt1-1)-1];
                        if( m > n ) {
                            *pnt2 = n;
                            wk = &(work[0]);
                            for(k = 0; k < wk_max; k++) {
                                if( *wk == m ) *wk = n;
                                wk++;
                            }
                        }
                        else if( m < n ) {
                            *pnt2 = m;
                            wk = &(work[0]);
                            for(k = 0; k < wk_max; k++) {
                                if( *wk == n ) *wk = m;
                                wk++;
                            }
                        }
                        else *pnt2 = m;
                        l = ((*pnt2)-1)*7;
                        work2[l+0]  ++; // area.
                        work2[l+1] += i; // pos[0]
                        work2[l+2] += j; // pos[1]
                        work2[l+6]  = j; // clip[3]
                    }
                    else if( *(pnt2-1) > 0 ) {
                        m = work[*(pnt1+1)-1];
                        n = work[*(pnt2-1)-1];
                        if( m > n ) {
                            *pnt2 = n;
                            wk = &(work[0]);
                            for(k = 0; k < wk_max; k++) {
                                if( *wk == m ) *wk = n;
                                wk++;
                            }
                        }
                        else if( m < n ) {
                            *pnt2 = m;
                            wk = &(work[0]);
                            for(k = 0; k < wk_max; k++) {
                                if( *wk == n ) *wk = m;
                                wk++;
                            }
                        }
                        else *pnt2 = m;
                        l = ((*pnt2)-1)*7;
                        work2[l+0] ++; // area
                        work2[l+1] += i; // pos[0]
                        work2[l+2] += j; // pos[1]
                    }
                    else {
                        *pnt2 = *(pnt1+1);
                        l = ((*pnt2)-1)*7;
                        work2[l+0] ++; // area
                        work2[l+1] += i; // pos[0]
                        work2[l+2] += j; // pos[1]
                        if( work2[l+3] > i ) work2[l+3] = i; // clip[0]
                        work2[l+6] = j; // clip [3]
                    }
                }
                else if( *(pnt1-1) > 0 ) {
                    *pnt2 = *(pnt1-1);
                    l = ((*pnt2)-1)*7;
                    work2[l+0] ++; // area
                    work2[l+1] += i; // pos[0]
                    work2[l+2] += j; // pos[1]
                    if( work2[l+4] < i ) work2[l+4] = i; // clip[1]
                    work2[l+6] = j; // clip[3]
                }
                else if( *(pnt2-1) > 0) {
                    *pnt2 = *(pnt2-1);
                    l = ((*pnt2)-1)*7;
                    work2[l+0] ++; // area
                    work2[l+1] += i; // pos[0]
                    work2[l+2] += j; // pos[1]
                    if( work2[l+4] < i ) work2[l+4] = i; // clip[1]
                }
                else {
                    wk_max++;
                    if( wk_max > AR_LABELING_WORK_SIZE ) {
                        ARLOGe("Error: labeling work overflow.\n");
                        return(-1);
                    }
                    work[wk_max-1] = *pnt2 = wk_max;
                    l = (wk_max-1)*7;
                    work2[l+0] = 1; // area
                    work2[l+1] = i; // pos[0]
                    work2[l+2] = j; // pos[1]
                    work2[l+3] = i; // clip[0]
                    work2[l+4] = i; // clip[1]
                    work2[l+5] = j; // clip[2]
                    work2[l+6] = j; // clip[3]
                }
            }
            else {
                // pnt is NOT in region.
                *pnt2 = 0;
#ifdef AR_LABELING_DEBUG_ENABLE_F
				*dpnt = 0;
#endif
            }
        }
#ifndef AR_LABELING_FRAME_IMAGE_F
        pnt += xsize*AR_PIXEL_SIZE;
#endif
    }

    label_num = &(labelInfo->label_num);
    area = &(labelInfo->area[0]);
    clip = &(labelInfo->clip[0][0]);
    pos  = &(labelInfo->pos[0][0]);
    j = 1;
    wk = &(work[0]);
    for(i = 1; i <= wk_max; i++, wk++) {
        *wk = (*wk==i)? j++: work[(*wk)-1];
    }
    *label_num = j - 1;
    if( *label_num == 0 ) {
        return 0;
    }

    memset( (ARUint8 *)area, 0, *label_num *     sizeof(int) );
    memset( (ARUint8 *)pos,  0, *label_num * 2 * sizeof(ARdouble) );
    for(i = 0; i < *label_num; i++) {
        clip[i*4+0] = lxsize;
        clip[i*4+1] = 0;
        clip[i*4+2] = lysize;
        clip[i*4+3] = 0;
    }
    for(i = 0; i < wk_max; i++) {
        j = work[i] - 1;
        area[j]    += work2[i*7+0];
        pos[j*2+0] += work2[i*7+1];
        pos[j*2+1] += work2[i*7+2];
        if( clip[j*4+0] > work2[i*7+3] ) clip[j*4+0] = work2[i*7+3];
        if( clip[j*4+1] < work2[i*7+4] ) clip[j*4+1] = work2[i*7+4];
        if( clip[j*4+2] > work2[i*7+5] ) clip[j*4+2] = work2[i*7+5];
        if( clip[j*4+3] < work2[i*7+6] ) clip[j*4+3] = work2[i*7+6];
    }

    for( i = 0; i < *label_num; i++ ) {
        pos[i*2+0] /= area[i];
        pos[i*2+1] /= area[i];
    }

    return 0;
}
