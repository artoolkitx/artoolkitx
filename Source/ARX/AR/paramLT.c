/*
 *  paramLT.c
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/param.h>


int arParamLTSave( char *filename, char *ext, ARParamLT *paramLT )
{
    FILE  *fp;
    char *buf;
    size_t len;

    len = strlen(filename) + strlen(ext) + 2;
    arMalloc(buf, char, len);
    sprintf(buf, "%s.%s", filename, ext);
    if( (fp=fopen(buf, "wb")) == NULL ) {
        ARLOGe("Error: Unable to open file '%s' for writing.\n", buf);
        free(buf);
        return -1;
    }
    free(buf);

    if( fwrite( paramLT, sizeof(ARParamLT), 1, fp ) != 1 ) {
        fclose(fp);
        return -1;
    }
    if( fwrite( paramLT->paramLTf.i2o, sizeof(float), paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2, fp )
       != paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2 ) {
        fclose(fp);
        return -1;
    }
    if( fwrite( paramLT->paramLTf.o2i, sizeof(float), paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2, fp )
       != paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2 ) {
        fclose(fp);
        return -1;
    }
    //if( fwrite( paramLT->paramLTi.i2o, sizeof(short), paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2, fp )
    //   != paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2 ) {
    //    fclose(fp);
    //    return -1;
    //}
    //if( fwrite( paramLT->paramLTi.o2i, sizeof(short), paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2, fp )
    //   != paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2 ) {
    //    fclose(fp);
    //    return -1;
    //}

    fclose(fp);
    
    return 0;
}

ARParamLT *arParamLTLoad( char *filename, char *ext )
{
    FILE        *fp;
    ARParamLT   *paramLT;
    char *buf;
    size_t len;

    len = strlen(filename) + strlen(ext) + 2;
    arMalloc(buf, char, len);
    sprintf(buf, "%s.%s", filename, ext);
    if( (fp=fopen(buf, "rb")) == NULL ) {
        ARLOGe("Error: Unable to open file '%s' for reading.\n", buf);
        free(buf);
        return NULL;
    }
    free(buf);
    
    arMalloc(paramLT, ARParamLT, 1);
    
    if( fread( paramLT, sizeof(ARParamLT), 1, fp ) != 1 ) {
        fclose(fp);
        free(paramLT);
        return NULL;
    }

    arMalloc(paramLT->paramLTf.i2o, float, paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2);
    arMalloc(paramLT->paramLTf.o2i, float, paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2);
    //arMalloc(paramLT->paramLTi.i2o, short, paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2);
    //arMalloc(paramLT->paramLTi.o2i, short, paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2);

    if( fread( paramLT->paramLTf.i2o, sizeof(float), paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2, fp )
       != paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2 ) {
        free(paramLT->paramLTf.i2o);
        free(paramLT->paramLTf.o2i);
        //free(paramLT->paramLTi.i2o);
        //free(paramLT->paramLTi.o2i);
        free(paramLT);
        fclose(fp);
        return NULL;
    }
    if( fread( paramLT->paramLTf.o2i, sizeof(float), paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2, fp )
       != paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2 ) {
        free(paramLT->paramLTf.i2o);
        free(paramLT->paramLTf.o2i);
        //free(paramLT->paramLTi.i2o);
        //free(paramLT->paramLTi.o2i);
        free(paramLT);
        fclose(fp);
        return NULL;
    }
    //if( fread( paramLT->paramLTi.i2o, sizeof(short), paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2, fp )
    //   != paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2 ) {
    //    free(paramLT->paramLTf.i2o);
    //    free(paramLT->paramLTf.o2i);
    //    free(paramLT->paramLTi.i2o);
    //    free(paramLT->paramLTi.o2i);
    //    free(paramLT);
    //    fclose(fp);
    //    return NULL;
    //}
    //if( fread( paramLT->paramLTi.o2i, sizeof(short), paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2, fp )
    //   != paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2 ) {
    //    free(paramLT->paramLTf.i2o);
    //    free(paramLT->paramLTf.o2i);
    //    free(paramLT->paramLTi.i2o);
    //    free(paramLT->paramLTi.o2i);
    //    free(paramLT);
    //    fclose(fp);
    //    return NULL;
    //}
    
    fclose(fp);
    
    return paramLT;

}

ARParamLT  *arParamLTCreate( ARParam *param, int offset )
{
    ARParamLT   *paramLT;
    ARdouble    *dist_factor;
    int          dist_function_version;
    ARdouble     ix, iy;
    ARdouble     ox, oy;
    float       *i2of, *o2if;
    //short       *i2oi, *o2ii;
    int          i, j;
    
    arMalloc(paramLT, ARParamLT, 1);
    paramLT->param = *param;
    
    paramLT->paramLTf.xsize = param->xsize + offset*2;
    paramLT->paramLTf.ysize = param->ysize + offset*2;
    paramLT->paramLTf.xOff = offset;
    paramLT->paramLTf.yOff = offset;
    arMalloc(paramLT->paramLTf.i2o, float, paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2);
    arMalloc(paramLT->paramLTf.o2i, float, paramLT->paramLTf.xsize*paramLT->paramLTf.ysize*2);
    
    //paramLT->paramLTi.xsize = param->xsize + offset*2;
    //paramLT->paramLTi.ysize = param->ysize + offset*2;
    //paramLT->paramLTi.xOff = offset;
    //paramLT->paramLTi.yOff = offset;
    //arMalloc(paramLT->paramLTi.i2o, short, paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2);
    //arMalloc(paramLT->paramLTi.o2i, short, paramLT->paramLTi.xsize*paramLT->paramLTi.ysize*2);
    
    dist_factor = param->dist_factor;
    dist_function_version = param->dist_function_version;
    i2of = paramLT->paramLTf.i2o;
    o2if = paramLT->paramLTf.o2i;
    //i2oi = paramLT->paramLTi.i2o;
    //o2ii = paramLT->paramLTi.o2i;
    for( j = 0; j < paramLT->paramLTf.ysize; j++ ) {
        for( i = 0; i < paramLT->paramLTf.xsize; i++ ) {
            arParamIdeal2Observ( dist_factor, (float)(i-offset), (float)(j-offset), &ox, &oy, dist_function_version);
            *(i2of++) = (float)ox;
            //*(i2oi++) = (int)(ox+0.5F);
            *(i2of++) = (float)oy;
            //*(i2oi++) = (int)(oy+0.5F);
            arParamObserv2Ideal( dist_factor, (float)(i-offset), (float)(j-offset), &ix, &iy, dist_function_version);
            *(o2if++) = (float)ix;
            //*(o2ii++) = (int)(ix+0.5F);
            *(o2if++) = (float)iy;
            //*(o2ii++) = (int)(iy+0.5F);
        }
    }
    
    return paramLT;
}

int arParamLTFree( ARParamLT **paramLT_p )
{
    if (!paramLT_p || !(*paramLT_p)) return (-1);
    
    free((*paramLT_p)->paramLTf.i2o);
    free((*paramLT_p)->paramLTf.o2i);
    //free((*paramLT_p)->paramLTi.i2o);
    //free((*paramLT_p)->paramLTi.o2i);
    free(*paramLT_p);
    *paramLT_p = NULL;
    return 0;
}

/*
int arParamIdeal2ObservLTi( const ARParamLTi *paramLTi, const int    ix, const int    iy, int    *ox, int    *oy)
{
    int      px, py;
    short   *lt;
    
    px = ix + paramLTi->xOff;
    py = iy + paramLTi->yOff;
    if( px < 0 || px >= paramLTi->xsize ||
        py < 0 || py >= paramLTi->ysize ) return -1;
    
    lt = paramLTi->i2o+ (py*paramLTi->xsize + px)*2;
    *ox = *(lt++);
    *oy = *lt;
    return 0;
}
*/

int arParamIdeal2ObservLTf( const ARParamLTf *paramLTf, const float  ix, const float  iy, float  *ox, float  *oy)
{
    int      px, py;
    float   *lt;
    
    px = (int)(ix+0.5F) + paramLTf->xOff;
    py = (int)(iy+0.5F) + paramLTf->yOff;
    if( px < 0 || px >= paramLTf->xsize ||
        py < 0 || py >= paramLTf->ysize ) return -1;
    
    lt = paramLTf->i2o+ (py*paramLTf->xsize + px)*2;
    *ox = *(lt++);
    *oy = *lt;
    return 0;
}

/*
int arParamObserv2IdealLTi( const ARParamLTi *paramLTi, const int    ox, const int    oy, int    *ix, int    *iy)
{
    int      px, py;
    short   *lt;
    
    px = ox + paramLTi->xOff;
    py = oy + paramLTi->yOff;
    if( px < 0 || px >= paramLTi->xsize ||
        py < 0 || py >= paramLTi->ysize ) return -1;

    lt = paramLTi->o2i+ (py*paramLTi->xsize + px)*2;
    *ix = *(lt++);
    *iy = *lt;
    return 0;
}
*/

int arParamObserv2IdealLTf( const ARParamLTf *paramLTf, const float  ox, const float  oy, float  *ix, float  *iy)
{
    int      px, py;
    float   *lt;
    
    px = (int)(ox+0.5F) + paramLTf->xOff;
    py = (int)(oy+0.5F) + paramLTf->yOff;
    if( px < 0 || px >= paramLTf->xsize ||
        py < 0 || py >= paramLTf->ysize ) return -1;
    
    lt = paramLTf->o2i+ (py*paramLTf->xsize + px)*2;
    *ix = *(lt++);
    *iy = *lt;
    return 0;
}

