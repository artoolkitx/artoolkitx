/*
 *  AR2/surface.c
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
#include <string.h>
#include <stdlib.h>
#include <ARX/AR2/coord.h>
#include <ARX/AR2/featureSet.h>
#include <ARX/AR2/template.h>
#include <ARX/AR2/tracking.h>
#include <ARX/AR2/util.h>

static char *get_buff( char *buf, int n, FILE *fp );

AR2SurfaceSetT *ar2ReadSurfaceSet( const char *filename, const char *ext, ARPattHandle *pattHandle )
{
    AR2SurfaceSetT  *surfaceSet;
    FILE            *fp = NULL;
    int              readMode;
    char             buf[256], name[256];
    int              i, j, k;

    if( ext == NULL || *ext == '\0' || strcmp(ext,"fset") == 0 ) {
        strncpy(name, filename, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        readMode = 0;
    }
    else {
        char namebuf[512];
        sprintf(namebuf, "%s.%s", filename, ext);
        if ((fp = fopen(namebuf,"r")) == NULL) {
            ARLOGe("Error opening file '%s'.\n", filename);
            ARLOGperror(NULL);
            return (NULL);
        }
        readMode = 1;
    }
    arMalloc(surfaceSet, AR2SurfaceSetT, 1);

    if( readMode ) {
        if( get_buff(buf, 256, fp) == NULL ) {
            fclose(fp);
            free(surfaceSet);
            return (NULL);
        }
        if( sscanf(buf, "%d", &i) != 1 ) {
            fclose(fp);
            free(surfaceSet);
            return (NULL);
        }
        if( i < 1 ) {
            fclose(fp);
            free(surfaceSet);
            return (NULL);
        }
        surfaceSet->num     = i;
        surfaceSet->contNum = 0;
    }
    else {
        surfaceSet->num     = 1;
        surfaceSet->contNum = 0;
    }
    arMalloc(surfaceSet->surface, AR2SurfaceT, surfaceSet->num);

    for( i = 0; i < surfaceSet->num; i++ ) {
        ARLOGi("\n### Surface No.%d ###\n", i+1);
        if( readMode ) {
            if( get_buff(buf, 256, fp) == NULL ) break;
            if( sscanf(buf, "%s", name) != 1 ) break;
            ar2UtilRemoveExt( name );
        }
        ARLOGi("  Read ImageSet.\n");
        surfaceSet->surface[i].imageSet = ar2ReadImageSet( name );
        if( surfaceSet->surface[i].imageSet == NULL ) {
            ARLOGe("Error opening file '%s.iset'.\n", name);
            free(surfaceSet->surface);
            free(surfaceSet);
			if (fp) fclose(fp); //COVHI10426
            return (NULL);
        }
        ARLOGi("    end.\n");

        ARLOGi("  Read FeatureSet.\n");
        surfaceSet->surface[i].featureSet = ar2ReadFeatureSet( name, "fset" );
        if( surfaceSet->surface[i].featureSet == NULL ) {
            ARLOGe("Error opening file '%s.fset'.\n", name);
            ar2FreeImageSet(&surfaceSet->surface[i].imageSet);
            free(surfaceSet->surface);
            free(surfaceSet);
			if (fp) fclose(fp); //COVHI10426
            return (NULL);
        }
        ARLOGi("    end.\n");

        if (pattHandle) {
            ARLOGi("  Read MarkerSet.\n");
            ar2UtilRemoveExt( name );
            surfaceSet->surface[i].markerSet = ar2ReadMarkerSet( name, "mrk", pattHandle );
            if( surfaceSet->surface[i].markerSet == NULL ) {
                ARLOGe("Error opening file '%s.mrk'.\n", name);
                ar2FreeFeatureSet(&surfaceSet->surface[i].featureSet);
                ar2FreeImageSet(&surfaceSet->surface[i].imageSet);
                free(surfaceSet->surface);
                free(surfaceSet);
                if (fp) fclose(fp); //COVHI10426
                return (NULL);
            }
            ARLOGi("    end.\n");
        } else {
            surfaceSet->surface[i].markerSet = NULL;
        }

        if (readMode) {
            if( get_buff(buf, 256, fp) == NULL ) break;
            if( sscanf(buf, "%f %f %f %f",
                       &(surfaceSet->surface[i].trans[0][0]),
                       &(surfaceSet->surface[i].trans[0][1]),
                       &(surfaceSet->surface[i].trans[0][2]),
                       &(surfaceSet->surface[i].trans[0][3])) != 4 ) {
                ARLOGe("Transformation matrix read error!!\n");
                fclose(fp);
                exit(0);
            }
            if( get_buff(buf, 256, fp) == NULL ) break;
            if( sscanf(buf, "%f %f %f %f",
                       &(surfaceSet->surface[i].trans[1][0]),
                       &(surfaceSet->surface[i].trans[1][1]),
                       &(surfaceSet->surface[i].trans[1][2]),
                       &(surfaceSet->surface[i].trans[1][3])) != 4 ) {
                ARLOGe("Transformation matrix read error!!\n");
                fclose(fp);
                exit(0);
            }
            if( get_buff(buf, 256, fp) == NULL ) break;
            if( sscanf(buf, "%f %f %f %f",
                       &(surfaceSet->surface[i].trans[2][0]),
                       &(surfaceSet->surface[i].trans[2][1]),
                       &(surfaceSet->surface[i].trans[2][2]),
                       &(surfaceSet->surface[i].trans[2][3])) != 4 ) {
                ARLOGe("Transformation matrix read error!!\n");
                fclose(fp);
                exit(0);
            }
        } else {
            for( j = 0; j < 3; j++ ) {
                for( k = 0; k < 4; k++ ) {
                    surfaceSet->surface[i].trans[j][k] = (j == k)? 1.0f: 0.0f;
                }
            }
        }
        arUtilMatInvf( (const float (*)[4])surfaceSet->surface[i].trans, surfaceSet->surface[i].itrans );

        ar2UtilReplaceExt( name, 256, "jpg");
        arMalloc( surfaceSet->surface[i].jpegName, char, 256);
        strncpy( surfaceSet->surface[i].jpegName, name, 256 );
    }

    if (fp) fclose(fp); //COVHI10459

    if (i < surfaceSet->num) exit(0);

    return surfaceSet;
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

int ar2FreeSurfaceSet( AR2SurfaceSetT **surfaceSet )
{
    int     i;

    if( *surfaceSet == NULL ) return -1;

    for( i = 0; i < (*surfaceSet)->num; i++ ) {
        ar2FreeImageSet( &((*surfaceSet)->surface[i].imageSet) );
        ar2FreeFeatureSet( &((*surfaceSet)->surface[i].featureSet) );
        if( (*surfaceSet)->surface[i].markerSet != NULL ) {
            ar2FreeMarkerSet( &((*surfaceSet)->surface[i].markerSet) );
        }
        free( (*surfaceSet)->surface[i].jpegName );
    }
    free( (*surfaceSet)->surface );
    free( *surfaceSet );
    *surfaceSet = NULL;

    return 0;
}


int ar2SetInitTrans( AR2SurfaceSetT *surfaceSet, float  trans[3][4] )
{
    int    i, j;

    if( surfaceSet == NULL ) return -1;
    surfaceSet->contNum = 1;
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) surfaceSet->trans1[j][i] = trans[j][i];
    }
    surfaceSet->prevFeature[0].flag = -1;

    return 0;
}
