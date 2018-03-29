/*
 *  AR2/marker.c
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/AR/ar.h>
#include <stdio.h>
#include <stdlib.h>
#include <ARX/AR2/marker.h>
#include <ARX/AR2/util.h>

static char *get_buff( char *buf, int n, FILE *fp );

int ar2FreeMarkerSet( AR2MarkerSetT **markerSet )
{
    if( *markerSet == NULL ) return -1;

    free( (*markerSet)->marker );
    free( *markerSet );
    *markerSet = NULL;

    return 0;
}

AR2MarkerSetT *ar2ReadMarkerSet( char *filename, char *ext, ARPattHandle  *pattHandle )
{
    //COVHI10394
    FILE          *fp = NULL;
    AR2MarkerSetT *markerSet = NULL;
    char          buf[256], buf1[256]/*, buf2[256]*/;
    int           i, j;

    char namebuf[512];
    sprintf(namebuf, "%s.%s", filename, ext);
    if( (fp=fopen(namebuf, "r")) == NULL ) return NULL;

    arMalloc( markerSet, AR2MarkerSetT, 1 );

    if( get_buff(buf, 256, fp) == NULL ) {
        free( markerSet );
        markerSet = NULL;
        goto done;
    }
    if( sscanf(buf, "%d", &(markerSet->num)) != 1 ) {
        free( markerSet );
        markerSet = NULL;
        goto done;
    }
    if( markerSet->num <= 0 ) {
        free(markerSet);
        markerSet = NULL;
        goto done;
    }

    arMalloc( markerSet->marker, AR2MarkerT, markerSet->num );

    for( i = 0; i < markerSet->num; i++ ) {
        if( get_buff(buf, 256, fp) == NULL ) {
            free( markerSet->marker );
            free( markerSet );
            markerSet = NULL;
            goto done;
        }
        if( sscanf(buf, "%s", buf1) != 1 ) {
            free( markerSet->marker );
            free( markerSet );
            markerSet = NULL;
            goto done;
        }
        //ar2UtilDivideExt(buf1, buf, buf2);
        if( (markerSet->marker[i].pattId = arPattLoad(pattHandle, buf1)) < 0 ) {
            free( markerSet->marker );
            free( markerSet );
            markerSet = NULL;
            goto done;
        }

        if( get_buff(buf, 256, fp) == NULL ) {
            free( markerSet->marker );
            free( markerSet );
            markerSet = NULL;
            goto done;
        }
        if( sscanf(buf, "%f", &(markerSet->marker[i].width)) != 1 ) {
            free( markerSet->marker );
            free( markerSet );
            markerSet = NULL;
            goto done;
        }

        for( j = 0; j < 3; j++ ) {
            if( get_buff(buf, 256, fp) == NULL ) {
                free( markerSet->marker );
                free( markerSet );
                markerSet = NULL;
                goto done;
            }
            if( sscanf(buf, "%f %f %f %f",
                            &(markerSet->marker[i].transI2M[j][0]),
                            &(markerSet->marker[i].transI2M[j][1]),
                            &(markerSet->marker[i].transI2M[j][2]),
                            &(markerSet->marker[i].transI2M[j][3])) != 4 ) {
                free( markerSet->marker );
                free( markerSet );
                markerSet = NULL;
                goto done;
            }
        }
    }

done:
    fclose(fp);
    return markerSet;
}

static char *get_buff( char *buf, int n, FILE *fp )
{
    char *ret;

    for(;;) {
        ret = fgets( buf, n, fp );
        if( ret == NULL ) return(NULL);
        if( buf[0] != '\n' && buf[0] != '#' ) return(ret);
    }
}

