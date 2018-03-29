/*
 *  arDetectMarker2.c
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
 * Revision: 2.11
 * Date: 00/05/06
 *
 ******************************************************/

#include <ARX/AR/ar.h>

static int check_square( int area, ARMarkerInfo2 *marker_info2, ARdouble factor );

static int get_vertex( int x_coord[], int y_coord[], int st, int ed,
                       ARdouble thresh, int vertex[], int *vnum );

int arDetectMarker2( int xsize, int ysize, ARLabelInfo *labelInfo, int imageProcMode,
                     int areaMax, int areaMin, ARdouble squareFitThresh,
                     ARMarkerInfo2 *markerInfo2, int *marker2_num )
{
    ARMarkerInfo2     *pm;
    int               i, j, ret;
    ARdouble            d;

    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
        areaMin /= 4;
        areaMax /= 4;
        xsize /=  2;
        ysize /=  2;
    }

    *marker2_num = 0;
    for( i = 0; i < labelInfo->label_num; i++ ) {
        if( labelInfo->area[i] < areaMin || labelInfo->area[i] > areaMax ) continue;
        if( labelInfo->clip[i][0] == 1 || labelInfo->clip[i][1] == xsize-2 ) continue;
        if( labelInfo->clip[i][2] == 1 || labelInfo->clip[i][3] == ysize-2 ) continue;

        ret = arGetContour( labelInfo->labelImage, xsize, ysize, labelInfo->work, i+1,
                            labelInfo->clip[i], &(markerInfo2[*marker2_num]));
        if( ret < 0 ) continue;

        ret = check_square( labelInfo->area[i], &(markerInfo2[*marker2_num]), squareFitThresh );
        if( ret < 0 ) continue;

        markerInfo2[*marker2_num].area   = labelInfo->area[i];
        markerInfo2[*marker2_num].pos[0] = labelInfo->pos[i][0];
        markerInfo2[*marker2_num].pos[1] = labelInfo->pos[i][1];
        (*marker2_num)++;
        if( *marker2_num == AR_SQUARE_MAX ) break;
    }

    for( i = 0; i < *marker2_num; i++ ) {
        for( j = i+1; j < *marker2_num; j++ ) {
            d = (markerInfo2[i].pos[0] - markerInfo2[j].pos[0])
              * (markerInfo2[i].pos[0] - markerInfo2[j].pos[0])
              + (markerInfo2[i].pos[1] - markerInfo2[j].pos[1])
              * (markerInfo2[i].pos[1] - markerInfo2[j].pos[1]);
            if( markerInfo2[i].area > markerInfo2[j].area ) {
                if( d < markerInfo2[i].area / 4 ) {
                    markerInfo2[j].area = 0;
                }
            }
            else {
                if( d < markerInfo2[j].area / 4 ) {
                    markerInfo2[i].area = 0;
                }
            }
        }
    }
    for( i = 0; i < *marker2_num; i++ ) {
        if( markerInfo2[i].area == 0.0 ) {
            for( j=i+1; j < *marker2_num; j++ ) {
                markerInfo2[j-1] = markerInfo2[j];
            }
            (*marker2_num)--;
        }
    }

    if( imageProcMode == AR_IMAGE_PROC_FIELD_IMAGE ) {
        pm = &(markerInfo2[0]);
        for( i = 0; i < *marker2_num; i++ ) {
            pm->area *= 4;
            pm->pos[0] *= 2.0;
            pm->pos[1] *= 2.0;
            for( j = 0; j< pm->coord_num; j++ ) {
                pm->x_coord[j] *= 2;
                pm->y_coord[j] *= 2;
            }
            pm++;
        }
    }

    return 0;
}

int arGetContour( AR_LABELING_LABEL_TYPE *limage, int xsize, int ysize, int *label_ref, int label,
                  int clip[4], ARMarkerInfo2 *marker_info2)
{
    int        xdir[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
    int        ydir[8] = {-1,-1, 0, 1, 1, 1, 0,-1};
    int        wx[AR_CHAIN_MAX];
    int        wy[AR_CHAIN_MAX];
    AR_LABELING_LABEL_TYPE   *p1;
    int        sx, sy, dir;
    int        dmax, d, v1 = 0 /*COVHI10455*/;
    int        i, j;

    sx = -1;
    j = clip[2];
    p1 = &(limage[j*xsize + clip[0]]);
    for ( i = clip[0]; i <= clip[1]; i++, p1++ ) {
        if ( *p1 > 0 && label_ref[(*p1)-1] == label ) {
            sx = i; sy = j;
            break;
        }
    }
    if ( i > clip[1] || -1 == sx ) {
        ARLOGe("??? 1\n"); return -1;
    }

    marker_info2->coord_num = 1;
    marker_info2->x_coord[0] = sx;
    marker_info2->y_coord[0] = sy;
    dir = 5;
    for(;;) {
        p1 = &(limage[marker_info2->y_coord[marker_info2->coord_num-1] * xsize
                    + marker_info2->x_coord[marker_info2->coord_num-1]]);
        dir = (dir+5)%8;
        for(i=0;i<8;i++) {
            if( p1[ydir[dir]*xsize+xdir[dir]] > 0 ) break;
            dir = (dir+1)%8;
        }
        if( i == 8 ) {
            ARLOGe("??? 2\n"); return -1;
        }
        marker_info2->x_coord[marker_info2->coord_num]
            = marker_info2->x_coord[marker_info2->coord_num-1] + xdir[dir];
        marker_info2->y_coord[marker_info2->coord_num]
            = marker_info2->y_coord[marker_info2->coord_num-1] + ydir[dir];
        if( marker_info2->x_coord[marker_info2->coord_num] == sx
         && marker_info2->y_coord[marker_info2->coord_num] == sy ) break;
        marker_info2->coord_num++;
        if( marker_info2->coord_num == AR_CHAIN_MAX-1 ) {
            ARLOGe("??? 3\n"); return -1;
        }
    }

    dmax = 0;
    for(i=1;i<marker_info2->coord_num;i++) {
        d = (marker_info2->x_coord[i]-sx)*(marker_info2->x_coord[i]-sx)
          + (marker_info2->y_coord[i]-sy)*(marker_info2->y_coord[i]-sy);
        if( d > dmax ) {
            dmax = d;
            v1 = i;
        }
    }

    for(i=0;i<v1;i++) {
        wx[i] = marker_info2->x_coord[i];
        wy[i] = marker_info2->y_coord[i];
    }
    for(i=v1;i<marker_info2->coord_num;i++) {
        marker_info2->x_coord[i-v1] = marker_info2->x_coord[i];
        marker_info2->y_coord[i-v1] = marker_info2->y_coord[i];
    }
    for(i=0;i<v1;i++) {
        marker_info2->x_coord[i-v1+marker_info2->coord_num] = wx[i];
        marker_info2->y_coord[i-v1+marker_info2->coord_num] = wy[i];
    }
    marker_info2->x_coord[marker_info2->coord_num] = marker_info2->x_coord[0];
    marker_info2->y_coord[marker_info2->coord_num] = marker_info2->y_coord[0];
    marker_info2->coord_num++;

    return 0;
}

static int check_square( int area, ARMarkerInfo2 *marker_info2, ARdouble factor )
{
    int             sx, sy;
    int             dmax, d, v1;
    int             vertex[10];
    int             wv1[10], wvnum1, wv2[10], wvnum2, v2;
    ARdouble          thresh;
    int             i;

    dmax = 0;
    v1 = 0;
    sx = marker_info2->x_coord[0];
    sy = marker_info2->y_coord[0];
    for(i=1;i<marker_info2->coord_num-1;i++) {
        d = (marker_info2->x_coord[i]-sx)*(marker_info2->x_coord[i]-sx)
          + (marker_info2->y_coord[i]-sy)*(marker_info2->y_coord[i]-sy);
        if( d > dmax ) {
            dmax = d;
            v1 = i;
        }
    }

    thresh = (area/0.75) * 0.01 * factor;
    vertex[0] = 0;
    wvnum1 = 0;
    wvnum2 = 0;
    if( get_vertex(marker_info2->x_coord, marker_info2->y_coord, 0,  v1,
                   thresh, wv1, &wvnum1) < 0 ) {
        return -1;
    }
    if( get_vertex(marker_info2->x_coord, marker_info2->y_coord,
                   v1,  marker_info2->coord_num-1, thresh, wv2, &wvnum2) < 0 ) {
        return -1;
    }

    if( wvnum1 == 1 && wvnum2 == 1 ) {
        vertex[1] = wv1[0];
        vertex[2] = v1;
        vertex[3] = wv2[0];
    }
    else if( wvnum1 > 1 && wvnum2 == 0 ) {
        v2 = v1 / 2;
        wvnum1 = wvnum2 = 0;
        if( get_vertex(marker_info2->x_coord, marker_info2->y_coord,
                       0,  v2, thresh, wv1, &wvnum1) < 0 ) {
            return -1;
        }
        if( get_vertex(marker_info2->x_coord, marker_info2->y_coord,
                       v2,  v1, thresh, wv2, &wvnum2) < 0 ) {
            return -1;
        }
        if( wvnum1 == 1 && wvnum2 == 1 ) {
            vertex[1] = wv1[0];
            vertex[2] = wv2[0];
            vertex[3] = v1;
        }
        else {
            return -1;
        }
    }
    else if( wvnum1 == 0 && wvnum2 > 1 ) {
        v2 = (v1 + marker_info2->coord_num-1) / 2;
        wvnum1 = wvnum2 = 0;
        if( get_vertex(marker_info2->x_coord, marker_info2->y_coord,
                   v1, v2, thresh, wv1, &wvnum1) < 0 ) {
            return -1;
        }
        if( get_vertex(marker_info2->x_coord, marker_info2->y_coord,
                   v2, marker_info2->coord_num-1, thresh, wv2, &wvnum2) < 0 ) {
            return -1;
        }
        if( wvnum1 == 1 && wvnum2 == 1 ) {
            vertex[1] = v1;
            vertex[2] = wv1[0];
            vertex[3] = wv2[0];
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }

    marker_info2->vertex[0] = vertex[0];
    marker_info2->vertex[1] = vertex[1];
    marker_info2->vertex[2] = vertex[2];
    marker_info2->vertex[3] = vertex[3];
    marker_info2->vertex[4] = marker_info2->coord_num-1;

    return 0;
}

static int get_vertex( int x_coord[], int y_coord[], int st,  int ed,
                       ARdouble thresh, int vertex[], int *vnum)
{
    ARdouble d, dmax;
    ARdouble a, b, c;
    int      i, v1;

    a = y_coord[ed] - y_coord[st];
    b = x_coord[st] - x_coord[ed];
    c = x_coord[ed] * y_coord[st] - y_coord[ed] * x_coord[st];
    dmax = 0;
    for (i = v1 = (st + 1) /*COVHI10453*/; i < ed; i++) {
        d = a*x_coord[i] + b*y_coord[i] + c;
        if( d*d > dmax ) {
            dmax = d*d;
            v1 = i;
        }
    }
    if( dmax/(a*a+b*b) > thresh ) {
        if( get_vertex(x_coord, y_coord, st,  v1, thresh, vertex, vnum) < 0 )
            return -1;

        if( (*vnum) > 5 ) return(-1);
        vertex[(*vnum)] = v1;
        (*vnum)++;

        if( get_vertex(x_coord, y_coord, v1,  ed, thresh, vertex, vnum) < 0 )
            return -1;
    }

    return 0;
}
