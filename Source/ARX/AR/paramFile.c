/*
 *  paramFile.c
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
 *  Author(s): Takeshi Mita, Shinsaku Hiura, Hirokazu Kato, Philip Lamb, Julian Looser
 *
 */
/*******************************************************
 *
 * Author: Takeshi Mita, Shinsaku Hiura, Hirokazu Kato
 *
 *         tmita@inolab.sys.es.osaka-u.ac.jp
 *         shinsaku@sys.es.osaka-u.ac.jp
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 4.1
 * Date: 01/12/07
 *
 *******************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ARX/AR/ar.h>
#include <errno.h>

typedef struct {
    int      xsize, ysize;
    double   mat[3][4];                             // Forced to double instead of ARdouble.
    double   dist_factor[AR_DIST_FACTOR_NUM_MAX];   // Forced to double instead of ARdouble.
	int      dist_function_version; // Must be last field in structure (as will not be written to disk).
} ARParamd;

const arParamVersionInfo_t arParamVersionInfo[AR_DIST_FUNCTION_VERSION_MAX] = {
	{4, 136}, 
	{5, 144}, 
	{6, 152}, 
	{9, 176}, 
    {17, 240},
};

#ifdef AR_LITTLE_ENDIAN
typedef union {
	int  x;
	unsigned char y[4];
} SwapIntT;

typedef union {
	float  x;
	unsigned char y[4];
} SwapFloatT;

typedef union {
	double   x;
	unsigned char y[8];
} SwapDoubleT;

static void byteSwapInt( const int *from, int *to )
{
    SwapIntT   *w1, *w2;
    int        i;

    w1 = (SwapIntT *)from;
    w2 = (SwapIntT *)to;
    for( i = 0; i < 4; i++ ) {
        w2->y[i] = w1->y[3-i];
    }

    return;
}

/*static void byteSwapFloat( const float *from, float *to )
{
    SwapFloatT   *w1, *w2;
    int           i;
    
    w1 = (SwapFloatT *)from;
    w2 = (SwapFloatT *)to;
    for( i = 0; i < 4; i++ ) {
        w2->y[i] = w1->y[3-i];
    }
    
    return;
}*/

static void byteSwapDouble( const double *from, double *to )
{
    SwapDoubleT   *w1, *w2;
    int           i;

    w1 = (SwapDoubleT *)from;
    w2 = (SwapDoubleT *)to;
    for( i = 0; i < 8; i++ ) {
        w2->y[i] = w1->y[7-i];
    }

    return;
}

static void byteswap( ARParamd *param )
{
    ARParamd  wparam;
    int      i, j;

    byteSwapInt( &(param->xsize), &(wparam.xsize) );
    byteSwapInt( &(param->ysize), &(wparam.ysize) );

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            byteSwapDouble( &(param->mat[j][i]), &(wparam.mat[j][i]) );
        }
    }

    for( i = 0; i < arParamVersionInfo[param->dist_function_version - 1].dist_factor_num; i++ ) {
        byteSwapDouble( &(param->dist_factor[i]), &(wparam.dist_factor[i]) );
    }
	wparam.dist_function_version = param->dist_function_version;

    *param = wparam;
}
#endif

#ifdef ARDOUBLE_IS_FLOAT
// If ARdouble has been defined to float, these functions are needed
// to convert ARParams for writing out to and reading from disk,
// where floating point values are always stored as doubles.
static ARParamd paramftod(const ARParam *param)
{
    int i, j;
    ARParamd paramd;
    
    paramd.xsize = param->xsize;
    paramd.ysize = param->ysize;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            paramd.mat[j][i] = (double)(param->mat[j][i]);
        }
    }
    for( i = 0; i < arParamVersionInfo[param->dist_function_version - 1].dist_factor_num; i++ ) {
        paramd.dist_factor[i] = (double)(param->dist_factor[i]);
    }
    paramd.dist_function_version = param->dist_function_version;
    return (paramd);
}

static ARParam paramdtof(const ARParamd *paramd)
{
    int i, j;
    ARParam param;
    
    param.xsize = paramd->xsize;
    param.ysize = paramd->ysize;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            param.mat[j][i] = (float)(paramd->mat[j][i]);
        }
    }
    for( i = 0; i < arParamVersionInfo[paramd->dist_function_version - 1].dist_factor_num; i++ ) {
        param.dist_factor[i] = (float)(paramd->dist_factor[i]);
    }
    param.dist_function_version = paramd->dist_function_version;
    return (param);
}

#endif

int    arParamSave( const char *filename, const int num, const ARParam *param, ...)
{
    FILE        *fp;
    va_list     ap;
    ARParam     *param1;
    int         i;
	ARParamd	param_toWrite;
	double		temp;

    if( num < 1 || !filename || !param) return -1;

    fp = fopen( filename, "wb" );
    if( fp == NULL ) {
		ARLOGe("Error (%d): unable to open camera parameters file \"%s\" for writing.\n", errno, filename);
		ARLOGperror(NULL);
		return -1;
	}

#ifdef ARDOUBLE_IS_FLOAT
	param_toWrite = paramftod(param);
#else
    param_toWrite = *((ARParamd *)param);
#endif
	if (param_toWrite.dist_function_version == 1) { // Ensure that version 1 files are compatible with the structure layout in ARToolKit 2.x.
		temp = param_toWrite.dist_factor[2];
		param_toWrite.dist_factor[2] = param_toWrite.dist_factor[3];
		param_toWrite.dist_factor[3] = temp;
	}	
#ifdef AR_LITTLE_ENDIAN
    byteswap(&param_toWrite);
#endif
	
    if (fwrite((void *)&param_toWrite, arParamVersionInfo[param_toWrite.dist_function_version - 1].ARParam_size, 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    va_start(ap, param);
    for( i = 1; i < num; i++ ) {
        param1 = va_arg(ap, ARParam *);
		
#ifdef ARDOUBLE_IS_FLOAT
		param_toWrite = paramftod(param1);
#else
        param_toWrite = *((ARParamd *)param1);
#endif
		if (param_toWrite.dist_function_version == 1) {
			temp = param_toWrite.dist_factor[2];
			param_toWrite.dist_factor[2] = param_toWrite.dist_factor[3];
			param_toWrite.dist_factor[3] = temp;
		}
#ifdef AR_LITTLE_ENDIAN
        byteswap(&param_toWrite);
#endif

		if (fwrite((void *)&param_toWrite, arParamVersionInfo[param_toWrite.dist_function_version - 1].ARParam_size, 1, fp) != 1) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);

    return 0;
}

int arParamLoad( const char *filename, int num, ARParam *param, ...)
{
    //COVHI10334
    int      ret = 0;
    FILE     *fp = NULL;
    int      i = 0;
    va_list  ap;
    ARParam  *param1;
    int      dist_function_version;
    long     flen;
    ARParamd param_wasRead;
    double   temp;

    if (num < 1 || !filename || !param) {
        ret = -1;
        goto done;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
		ARLOGe("Error (%d): unable to open camera parameters file \"%s\" for reading.\n", errno, filename);
		ARLOGperror(NULL);
        ret = -1;
        goto done;
	}
	
	// Determine file length.
	fseek(fp, 0L, SEEK_END);
	if (ferror(fp)) {
		ARLOGe("Error (%d): unable to determine file length.\n", errno);
		ARLOGperror(NULL);
        ret = -1;
        goto done;
    }
	flen = ftell(fp);
	//ARLOGd("Loading a parameter file of length %ld.\n", flen);
	rewind(fp);
	
	// Try to determine distortion function version number.
	// Infer distortion function version number from file length.
	// Assumes all ARParams in file are of same version.
	for (i = 0; i < AR_DIST_FUNCTION_VERSION_MAX; i++) {
		if (flen % arParamVersionInfo[i].ARParam_size == 0) {
			dist_function_version = i + 1;
			break;
		}
	}
	if (i == AR_DIST_FUNCTION_VERSION_MAX) {
		ARLOGe("Error: supplied file does not appear to be an artoolkitX camera parameter file.\n");
        ret = -1;
        goto done;
	}
	ARLOGd("Reading camera parameters from %s (distortion function version %d).\n", filename, dist_function_version);
    
    if (fread((void *)&param_wasRead, arParamVersionInfo[dist_function_version - 1].ARParam_size, 1, fp) != 1) {
		ARLOGe("Error (%d): unable to read from file.\n", errno);
		ARLOGperror(NULL);
        ret = -1;
        goto done;
    }
    param_wasRead.dist_function_version = dist_function_version;
#ifdef AR_LITTLE_ENDIAN
    byteswap( &param_wasRead );
#endif
	if (dist_function_version == 1) { // Ensure that file layout in ARToolKit 2.x.version is modified for ARToolKit 4.x and later.
		temp = param_wasRead.dist_factor[2];
		param_wasRead.dist_factor[2] = param_wasRead.dist_factor[3];
		param_wasRead.dist_factor[3] = temp;
	}	
#ifdef ARDOUBLE_IS_FLOAT
	*param = paramdtof(&param_wasRead);
#else
    *((ARParamd *)param) = param_wasRead;
#endif

    va_start(ap, param);
    for( i = 1; i < num; i++ ) {
        param1 = va_arg(ap, ARParam *);
		param1->dist_function_version = param->dist_function_version;
        if (fread((void *)&param_wasRead, arParamVersionInfo[param->dist_function_version - 1].ARParam_size, 1, fp) != 1) {
            ret = -1;
            goto done;
        }
        param_wasRead.dist_function_version = dist_function_version;
#ifdef AR_LITTLE_ENDIAN
        byteswap( &param_wasRead );
#endif
        if (dist_function_version == 1) { // Ensure that file layout in ARToolKit 2.x.version is modified for ARToolKit 4.x and later.
            temp = param_wasRead.dist_factor[2];
            param_wasRead.dist_factor[2] = param_wasRead.dist_factor[3];
            param_wasRead.dist_factor[3] = temp;
        }	
#ifdef ARDOUBLE_IS_FLOAT
        *param1 = paramdtof(&param_wasRead);
#else
        *((ARParamd *)param1) = param_wasRead;
#endif
    }

done:
    if (fp) {
        fclose(fp);
    }
    
    return ret;
}

int    arParamLoadFromBuffer( const void *buffer, size_t bufsize, ARParam *param)
{
    int         i;
    int         dist_function_version;
	ARParamd	param_wasRead;
	double		temp;
    
    if( !buffer || !param || !bufsize) return -1;
    
	// Try to determine distortion function version number.
	// Infer distortion function version number from buffer length.
	// Assumes all ARParams in file are of same version.
	for (i = 0; i < AR_DIST_FUNCTION_VERSION_MAX; i++) {
		if (bufsize % arParamVersionInfo[i].ARParam_size == 0) {
			dist_function_version = i + 1;
			break;
		}
	}
	if (i == AR_DIST_FUNCTION_VERSION_MAX) {
		ARLOGe("Error: supplied buffer does not appear to be artoolkitX camera parameters.\n");
		//fclose(fp);  // don't need to close file anymore
		return -1;
	}
    
	ARLOGd("Reading camera parameters from buffer (distortion function version %d).\n", dist_function_version);
    
	memcpy((void *)&param_wasRead, buffer, arParamVersionInfo[dist_function_version - 1].ARParam_size);
    
	param_wasRead.dist_function_version = dist_function_version;
    
#ifdef AR_LITTLE_ENDIAN
    byteswap( &param_wasRead );
#endif
	if (dist_function_version == 1) { // Ensure that file layout in ARToolKit 2.x.version is modified for ARToolKit 4.x and later.
		temp = param_wasRead.dist_factor[2];
		param_wasRead.dist_factor[2] = param_wasRead.dist_factor[3];
		param_wasRead.dist_factor[3] = temp;
	}
#ifdef ARDOUBLE_IS_FLOAT
	*param = paramdtof(&param_wasRead);
#else
    *((ARParamd *)param) = param_wasRead;
#endif
    
    return 0;
}

int arParamSaveExt( const char *filename, ARdouble para[3][4] )
{
    FILE        *fp;
    double      para0[3][4];
#ifdef AR_LITTLE_ENDIAN
    double      para1[3][4];
#endif
    int         i, j;

    if (!filename || !filename[0] || !para) return (-1);

    fp = fopen( filename, "wb" );
    if( fp == NULL ) {
		ARLOGe("Error (%d): unable to open external parameters file \"%s\" for writing.\n", errno, filename);
		ARLOGperror(NULL);
		return -1;
	}

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            para0[j][i] = (double)(para[j][i]);
        }
    }

#ifdef AR_LITTLE_ENDIAN
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            byteSwapDouble( &(para0[j][i]), &(para1[j][i]) );
        }
    }

    if( fwrite( para1, sizeof(double), 12, fp ) != 12 ) {
        fclose(fp);
        return -1;
    }
#else
    if( fwrite( para0, sizeof(double), 12, fp ) != 12 ) {
        fclose(fp);
        return -1;
    }
#endif

    fclose(fp);

    return 0;
}

int arParamLoadExt( const char *filename, ARdouble para[3][4] )
{
    FILE        *fp;
    double      para0[3][4];
#ifdef AR_LITTLE_ENDIAN
    double      para1[3][4];
#endif
    int         i, j;

    if (!filename || !filename[0] || !para) return (-1);
    
    fp = fopen( filename, "rb" );
    if( fp == NULL ) {
		ARLOGe("Error (%d): unable to open external parameters file \"%s\" for reading.\n", errno, filename);
		ARLOGperror(NULL);
		return -1;
	}

#ifdef AR_LITTLE_ENDIAN
    if( fread( para1, sizeof(double), 12, fp ) != 12 ) {
        fclose(fp);
        return -1;
    }
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            byteSwapDouble( &(para1[j][i]), &(para0[j][i]) );
        }
    }
#else
    if( fread( para0, sizeof(double), 12, fp ) != 12 ) {
        fclose(fp);
        return -1;
    }
#endif
    
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            para[j][i] = (ARdouble)(para0[j][i]);
        }
    }

    fclose(fp);

    return 0;
}

int arParamLoadExtFromBuffer( const void *buffer, size_t bufsize, ARdouble para[3][4] )
{
    double      para0[3][4];
#ifdef AR_LITTLE_ENDIAN
    double      para1[3][4];
#endif
    int         i, j;
    
    if (!buffer || bufsize != 12*sizeof(double) || !para) return (-1);
    
#ifdef AR_LITTLE_ENDIAN
    memcpy(para1, buffer, 12*sizeof(double));
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            byteSwapDouble( &(para1[j][i]), &(para0[j][i]) );
        }
    }
#else
    memcpy(para0, buffer, 12*sizeof(double));
#endif
    
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            para[j][i] = (ARdouble)(para0[j][i]);
        }
    }
    
    return 0;
}

int arParamSaveOptical(const char *filename, const ARdouble fovy, const ARdouble aspect, const ARdouble m[16])
{
    FILE        *fp;
    double      fovy0, aspect0, m0[16];
#ifdef AR_LITTLE_ENDIAN
	double		fovy1, aspect1, m1[16];
#endif
    int         i;
	
    if (!filename || !filename[0] || !m) return (-1);

    fp = fopen(filename, "wb");
    if( fp == NULL ) {
		ARLOGe("Error (%d): unable to open optical parameters file \"%s\" for writing.\n", errno, filename);
		ARLOGperror(NULL);
		return -1;
	}
	
    fovy0 = (double)fovy;
    aspect0 = (double)aspect;
    for (i = 0; i < 16; i++) m0[i] = (double)(m[i]);
    
#ifdef AR_LITTLE_ENDIAN
	byteSwapDouble(&fovy0, &fovy1);
    if (fwrite(&fovy1, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
	byteSwapDouble(&aspect0, &aspect1);
    if (fwrite(&aspect1, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
    for (i = 0; i < 16; i++) byteSwapDouble(&(m0[i]), &(m1[i]));
    if (fwrite(m1, sizeof(double), 16, fp) != 16) {
        goto bail;
    }
#else
    if (fwrite(&fovy0, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
    if (fwrite(&aspect0, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
    if (fwrite(m0, sizeof(double), 16, fp) != 16) {
        goto bail;
    }
#endif
	
    fclose(fp);	
    return 0;
	
bail:
	fclose(fp);
	return -1;
}

int arParamLoadOptical(const char *filename, ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16])
{
    FILE        *fp;
	double		fovy0, aspect0, m0[16];
#ifdef AR_LITTLE_ENDIAN
	double		fovy1, aspect1, m1[16];
#endif
    int         i;
	
    if (!filename || !filename[0] || !fovy_p || !aspect_p || !m) return (-1);

    fp = fopen(filename, "rb");
    if( fp == NULL ) {
		ARLOGe("Error (%d): unable to open optical parameters file \"%s\" for reading.\n", errno, filename);
		ARLOGperror(NULL);
		return -1;
	}
	
#ifdef AR_LITTLE_ENDIAN
    if (fread(&fovy1, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
	byteSwapDouble(&fovy1, &fovy0);
    if (fread(&aspect1, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
	byteSwapDouble(&aspect1, &aspect0);
    if (fread(m1, sizeof(double), 16, fp) != 16) {
        goto bail;
    }
    for (i = 0; i < 16; i++) byteSwapDouble(&(m1[i]), &(m0[i]));
#else
    if (fread(&fovy0, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
    if (fread(&aspect0, sizeof(double), 1, fp) != 1) {
        goto bail;
    }
    if (fread(m0, sizeof(double), 16, fp) != 16) {
        goto bail;
    }
#endif
    
    *fovy_p = (ARdouble)fovy0;
    *aspect_p = (ARdouble)aspect0;
    for (i = 0; i < 16; i++) m[i] = (ARdouble)(m0[i]);
	
    fclose(fp);
    return 0;
	
bail:
	fclose(fp);
	return -1;
}

int arParamLoadOpticalFromBuffer(const void *buffer, size_t bufsize, ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16])
{
 	double		fovy0, aspect0, m0[16];
#ifdef AR_LITTLE_ENDIAN
	double		fovy1, aspect1, m1[16];
#endif
    int         i;
	
    if (!buffer || bufsize != 18*sizeof(double) || !fovy_p || !aspect_p || !m) return (-1);
    
#ifdef AR_LITTLE_ENDIAN
    memcpy(&fovy1, (unsigned char *)buffer, sizeof(double));
	byteSwapDouble(&fovy1, &fovy0);
    memcpy(&aspect1, (unsigned char *)buffer + sizeof(double), sizeof(double));
	byteSwapDouble(&aspect1, &aspect0);
    memcpy(m1, (unsigned char *)buffer + 2*sizeof(double), 16*sizeof(double));
    for (i = 0; i < 16; i++) byteSwapDouble(&(m1[i]), &(m0[i]));
#else
    memcpy(&fovy0, (unsigned char *)buffer, sizeof(double));
    memcpy(&aspect0, (unsigned char *)buffer + sizeof(double), sizeof(double));
    memcpy(m0, (unsigned char *)buffer + 2*sizeof(double), 16*sizeof(double));
#endif
    
    *fovy_p = (ARdouble)fovy0;
    *aspect_p = (ARdouble)aspect0;
    for (i = 0; i < 16; i++) m[i] = (ARdouble)(m0[i]);
	
    return 0;
}

