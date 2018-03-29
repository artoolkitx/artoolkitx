/*
 *  arPattSave.c
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

#include <stdio.h>
#include <ARX/AR/ar.h>

int arPattSave( ARUint8 *image, int xsize, int ysize, int pixelFormat, ARParamLTf *paramLTf,
                int imageProcMode, ARMarkerInfo *marker_info, ARdouble pattRatio, int pattSize, const char *filename )
{
    FILE      *fp;
    ARUint8   *ext_pat[4];
    ARdouble    vertex[4][2];
    int       i, j, k, x, y;

    for (i = 0; i < 4; i++) {
        arMalloc(ext_pat[i], ARUint8, pattSize*pattSize*3);
    }
    
    for( j = 0; j < 4; j++ ) {
        for( k = 0; k < 4; k++ ) {
            vertex[k][0] = marker_info->vertex[(k+j+2)%4][0];
            vertex[k][1] = marker_info->vertex[(k+j+2)%4][1];
        }
        arPattGetImage2( imageProcMode, AR_TEMPLATE_MATCHING_COLOR, pattSize, pattSize*AR_PATT_SAMPLE_FACTOR1,
                         image, xsize, ysize, pixelFormat, paramLTf, vertex, pattRatio, ext_pat[j] );
    }

    fp = fopen( filename, "w" );
    if( fp == NULL ) return -1;

	// Write out in order AR_PATT_SIZE_X columns x AR_PATT_SIZE_Y rows x 3 colours x 4 orientations.
    for( i = 0; i < 4; i++ ) {
        for( j = 0; j < 3; j++ ) {
            for( y = 0; y < pattSize; y++ ) {
                for( x = 0; x < pattSize; x++ ) {
                    fprintf(fp, "%4d", ext_pat[i][(y*pattSize + x)*3 + j] );
                }
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "\n");
    }

    fclose( fp );

    for (i = 0; i < 4; i++) {
        free(ext_pat[i]);
    }

    return 0;
}
