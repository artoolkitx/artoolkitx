/*
 *  AR2/featureSet.c
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
#include <ARX/AR2/featureSet.h>

AR2FeatureSetT *ar2ReadFeatureSet( const char *filename, const char *ext )
{
    AR2FeatureSetT *featureSet = NULL;
    FILE           *fp = NULL;
    int            i, j, l3;

    char buf[512];
    sprintf(buf, "%s.%s", filename, ext);
    if( (fp=fopen(buf, "rb")) == NULL ) {
        ARLOGe("File open error. %s\n", filename);
        return NULL;
    }

    arMalloc( featureSet, AR2FeatureSetT, 1 );

    //COVHI10403
    if( fread(&(featureSet->num), sizeof(featureSet->num), 1, fp) != 1 ) {
        ARLOGe("Read error!!\n");
        goto bail0;
    }

    arMalloc( featureSet->list, AR2FeaturePointsT, featureSet->num );
    for( i = 0; i < featureSet->num; i++ ) {
        if( fread(&(featureSet->list[i].scale), sizeof(featureSet->list[i].scale), 1, fp) != 1 ) {
            ARLOGe("Read error!!\n");
            goto bail1;
        }
        if( fread(&(featureSet->list[i].maxdpi), sizeof(featureSet->list[i].maxdpi), 1, fp) != 1 ) {
            ARLOGe("Read error!!\n");
            goto bail1;
        }
        if( fread(&(featureSet->list[i].mindpi), sizeof(featureSet->list[i].mindpi), 1, fp) != 1 ) {
            ARLOGe("Read error!!\n");
            goto bail1;
        }
        if( fread(&(featureSet->list[i].num), sizeof(featureSet->list[i].num), 1, fp) != 1 ) {
            ARLOGe("Read error!!\n");
            goto bail1;
        }

        arMalloc( featureSet->list[i].coord, AR2FeatureCoordT, featureSet->list[i].num );
        for( j = 0; j < featureSet->list[i].num; j++ ) {
            if( fread(&(featureSet->list[i].coord[j].x), sizeof(featureSet->list[i].coord[j].x), 1, fp) != 1 ) {
                ARLOGe("Read error!!\n");
                goto bail1;
            }
            if( fread(&(featureSet->list[i].coord[j].y), sizeof(featureSet->list[i].coord[j].y), 1, fp) != 1 ) {
                ARLOGe("Read error!!\n");
                goto bail1;
            }
            if( fread(&(featureSet->list[i].coord[j].mx), sizeof(featureSet->list[i].coord[j].mx), 1, fp) != 1 ) {
                ARLOGe("Read error!!\n");
                goto bail1;
            }
            if( fread(&(featureSet->list[i].coord[j].my), sizeof(featureSet->list[i].coord[j].my), 1, fp) != 1 ) {
                ARLOGe("Read error!!\n");
                goto bail1;
            }
            if( fread(&(featureSet->list[i].coord[j].maxSim), sizeof(featureSet->list[i].coord[j].maxSim), 1, fp) != 1 ) {
                ARLOGe("Read error!!\n");
                goto bail1;
            }
        }
    }

    goto done;
    
bail1:
    for(l3=0;l3<i;l3++) {
        free( featureSet->list[l3].coord );
    }
    free( featureSet->list );
bail0:
    free( featureSet );
    featureSet = NULL;

done:
    fclose(fp);
    return featureSet;
}

int ar2SaveFeatureSet( const char *filename, const char *ext, AR2FeatureSetT *featureSet )
{
    FILE    *fp;
    int     i, j;

    char buf[512];
    sprintf(buf, "%s.%s", filename, ext);
    if( (fp=fopen(buf, "wb")) == NULL ) {
        ARLOGe("File open error. %s\n", filename);
        return -1;
    }

    if( fwrite(&(featureSet->num), sizeof(featureSet->num), 1, fp) != 1 ) goto bailBadWrite;

    for( i = 0; i < featureSet->num; i++ ) {
        if( fwrite(&(featureSet->list[i].scale), sizeof(featureSet->list[i].scale), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(&(featureSet->list[i].maxdpi), sizeof(featureSet->list[i].maxdpi), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(&(featureSet->list[i].mindpi), sizeof(featureSet->list[i].mindpi), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(&(featureSet->list[i].num), sizeof(featureSet->list[i].num), 1, fp) != 1 ) goto bailBadWrite;

        for( j = 0; j < featureSet->list[i].num; j++ ) {
            if( fwrite(&(featureSet->list[i].coord[j].x), sizeof(featureSet->list[i].coord[j].x), 1, fp) != 1 ) goto bailBadWrite;
            if( fwrite(&(featureSet->list[i].coord[j].y), sizeof(featureSet->list[i].coord[j].y), 1, fp) != 1 ) goto bailBadWrite;
            if( fwrite(&(featureSet->list[i].coord[j].mx), sizeof(featureSet->list[i].coord[j].mx), 1, fp) != 1 ) goto bailBadWrite;
            if( fwrite(&(featureSet->list[i].coord[j].my), sizeof(featureSet->list[i].coord[j].my), 1, fp) != 1 ) goto bailBadWrite;
            if( fwrite(&(featureSet->list[i].coord[j].maxSim), sizeof(featureSet->list[i].coord[j].maxSim), 1, fp) != 1 ) goto bailBadWrite;
        }
    }

    fclose(fp);
    return 0;
    
bailBadWrite:
    ARLOGe("Error saving feature set: error writing data.\n");
    fclose(fp);
    return (-1);
}

int ar2FreeFeatureSet( AR2FeatureSetT **featureSet )
{
    int     i;

    if( *featureSet == NULL ) return -1;

    for( i = 0; i < (*featureSet)->num; i++ ) {
        free( (*featureSet)->list[i].coord );
    }
    free( (*featureSet)->list );
    free( *featureSet );
    *featureSet = NULL;

    return 0;
}
