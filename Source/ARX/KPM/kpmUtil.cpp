/*
 *  kpmUtil.cpp
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
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <ARX/AR/ar.h>
#include <ARX/AR/icp.h>
#include <ARX/KPM/kpm.h>
#include <ARX/KPM/kpmType.h>

#if BINARY_FEATURE
#include <facade/visual_database_facade.h>
#else
#include <ARX/KPM/surfSub.h>
#endif

static ARUint8 *genBWImageFull      ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageHalf      ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageOneThird  ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageTwoThird  ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageQuart     ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize );


#if !BINARY_FEATURE
static int kpmUtilGetInitPoseHomography( float *sCoord, float *wCoord, int num, float initPose[3][4] );
#endif

int kpmUtilGetCorner( ARUint8 *inImage, int xsize, int ysize, int procMode, int maxPointNum,
                      CornerPoints *cornerPoints )
{
    ARUint8        *inImageBW;
    int            xsize2, ysize2;
    int            cornerNum;
    int            i;

    inImageBW = kpmUtilResizeImage( inImage, xsize, ysize, procMode, &xsize2, &ysize2 ); //Eventually returns a
                                                                                                   //malloc()'ed buffer
    if( inImageBW == NULL ) return -1;
    
#if BINARY_FEATURE
    vision::VisualDatabaseFacade *freakMatcher = new vision::VisualDatabaseFacade;
    freakMatcher->addImage(inImageBW, xsize, ysize, 1);
    const std::vector<vision::FeaturePoint>& points = freakMatcher->getQueryFeaturePoints();
    cornerNum = (int)freakMatcher->getQueryFeaturePoints().size();
#else
    SurfSubHandleT *surfHandle;
    surfHandle = surfSubCreateHandle(xsize2, ysize2, AR_PIXEL_FORMAT_MONO);
    if (!surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        free( inImageBW ); //COVHI10283
        return -1;
    }
    surfSubSetMaxPointNum( surfHandle, maxPointNum );
    surfSubExtractFeaturePoint( surfHandle, inImageBW, NULL, 0 );
    cornerNum = surfSubGetFeaturePointNum( surfHandle );
#endif
    
    if( procMode == KpmProcFullSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
#if BINARY_FEATURE
            x = points[i].x, y = points[i].y;
#else
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
#endif
            cornerPoints->pt[i].x = (int)x;
            cornerPoints->pt[i].y = (int)y;
        }
    }
    else if( procMode == KpmProcTwoThirdSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
#if BINARY_FEATURE
            x = points[i].x, y = points[i].y;
#else
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
#endif
            cornerPoints->pt[i].x = (int)(x * 1.5f);
            cornerPoints->pt[i].y = (int)(y * 1.5f);
        }
    }
    else if( procMode == KpmProcHalfSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
#if BINARY_FEATURE
            x = points[i].x, y = points[i].y;
#else
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
#endif
            cornerPoints->pt[i].x = (int)(x * 2.0f);
            cornerPoints->pt[i].y = (int)(y * 2.0f);
        }
    }
    else if( procMode == KpmProcOneThirdSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
#if BINARY_FEATURE
            x = points[i].x, y = points[i].y;
#else
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
#endif
            cornerPoints->pt[i].x = (int)(x * 3.0f);
            cornerPoints->pt[i].y = (int)(y * 3.0f);
        }
    }
    else {      
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
#if BINARY_FEATURE
            x = points[i].x, y = points[i].y;
#else
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
#endif
            cornerPoints->pt[i].x = (int)(x * 4.0f);
            cornerPoints->pt[i].y = (int)(y * 4.0f);
        }
    }
    cornerPoints->num = cornerNum;

    free( inImageBW );
#if BINARY_FEATURE
    delete freakMatcher;
#else
    surfSubDeleteHandle( &surfHandle );
#endif

    return 0;
}

ARUint8 *kpmUtilResizeImage( ARUint8 *image, int xsize, int ysize, int procMode, int *newXsize, int *newYsize )
{
    if( procMode == KpmProcFullSize ) {
        return genBWImageFull( image, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcTwoThirdSize ) {
        return genBWImageTwoThird( image, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcHalfSize ) {
        return genBWImageHalf( image, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcOneThirdSize ) {
        return genBWImageOneThird( image, xsize, ysize, newXsize, newYsize );
    }
    else {
        return genBWImageQuart( image, xsize, ysize, newXsize, newYsize );
    }
}

#if !BINARY_FEATURE
int kpmUtilGetPose( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{
    ICPHandleT    *icpHandle;
    ICPDataT       icpData;
    ICP2DCoordT   *sCoord;
    ICP3DCoordT   *wCoord;
    ARdouble       initMatXw2Xc[3][4];
    ARdouble       err;
    int            i;

    if( matchData->num < 4 ) return -1;

    arMalloc( sCoord, ICP2DCoordT, matchData->num );
    arMalloc( wCoord, ICP3DCoordT, matchData->num );

    for( i = 0; i < matchData->num; i++ ) {
        sCoord[i].x = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i].y = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i].x = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.x;
        wCoord[i].y = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.y;
        wCoord[i].z = 0.0;
        //printf("%3d: (%f %f) - (%f %f)\n", i, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
    }

    icpData.num = i;
    icpData.screenCoord = &sCoord[0];
    icpData.worldCoord  = &wCoord[0];

    if( icpGetInitXw2Xc_from_PlanarData( cparamLT->param.mat, sCoord, wCoord, matchData->num, initMatXw2Xc ) < 0 ) {
        //printf("Error!! at icpGetInitXw2Xc_from_PlanarData.\n");
        free( sCoord );
        free( wCoord );
        return -1;
    }
/*
    printf("--- Init pose ---\n");
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  printf(" %8.3f", initMatXw2Xc[j][i]);
        printf("\n"); 
    } 
*/
    if( (icpHandle = icpCreateHandle( cparamLT->param.mat )) == NULL ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
#if 0
    if( icpData.num > 10 ) {
        icpSetInlierProbability( icpHandle, 0.7 );
        if( icpPointRobust( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
    else {
        if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
#else
#  ifdef ARDOUBLE_IS_FLOAT
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
#  else
    ARdouble camPosed[3][4];
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPosed, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) camPose[r][c] = (float)camPosed[r][c];
#  endif
#endif
    icpDeleteHandle( &icpHandle );

/*
    printf("error = %f\n", err);
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  printf(" %8.3f", camPose[j][i]);
        printf("\n"); 
    } 
    if( err > 10.0f ) {
        for( i = 0; i < matchData->num; i++ ) {
            printf("%d\t%f\t%f\t%f\t%f\n", i+1, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
        }
    }
*/


    free( sCoord );
    free( wCoord );

    *error = (float)err;
    if( *error > 10.0f ) return -1;

    return 0;
}


int kpmUtilGetPose2( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, int *redDataIndex, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{
    ICPHandleT    *icpHandle;
    ICPDataT       icpData;
    ICP2DCoordT   *sCoord;
    ICP3DCoordT   *wCoord;
    ARdouble       initMatXw2Xc[3][4];
    ARdouble       err;
    int            i;
    
    if( matchData->num < 4 ) return -1;
    
    arMalloc( sCoord, ICP2DCoordT, matchData->num );
    arMalloc( wCoord, ICP3DCoordT, matchData->num );
    
    for( i = 0; i < matchData->num; i++ ) {
        sCoord[i].x = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i].y = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i].x = refDataSet->refPoint[redDataIndex[matchData->match[i].refIndex]].coord3D.x;
        wCoord[i].y = refDataSet->refPoint[redDataIndex[matchData->match[i].refIndex]].coord3D.y;
        wCoord[i].z = 0.0;
    }
    
    icpData.num = i;
    icpData.screenCoord = &sCoord[0];
    icpData.worldCoord  = &wCoord[0];
    
    if( icpGetInitXw2Xc_from_PlanarData( cparamLT->param.mat, sCoord, wCoord, matchData->num, initMatXw2Xc ) < 0 ) {
        //ARLOGe("Error!! at icpGetInitXw2Xc_from_PlanarData.\n");
        free( sCoord );
        free( wCoord );
        return -1;
    }
/*
    ARLOGd("--- Init pose ---\n");
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  ARLOGd(" %8.3f", initMatXw2Xc[j][i]);
        ARLOGd("\n");
    } 
*/
    if( (icpHandle = icpCreateHandle( cparamLT->param.mat )) == NULL ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
#if 0
    if( icpData.num > 10 ) {
        icpSetInlierProbability( icpHandle, 0.7 );
        if( icpPointRobust( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
    else {
        if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
#else
#  ifdef ARDOUBLE_IS_FLOAT
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
#  else
    ARdouble camPosed[3][4];
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPosed, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) camPose[r][c] = (float)camPosed[r][c];
#  endif
#endif
    icpDeleteHandle( &icpHandle );

/*
    ARLOGd("error = %f\n", err);
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  ARLOGd(" %8.3f", camPose[j][i]);
        ARLOGd("\n");
    } 
    if( err > 10.0 ) {
        for( i = 0; i < matchData->num; i++ ) {
            ARLOGd("%d\t%f\t%f\t%f\t%f\n", i+1, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
        }
    }
*/


    free( sCoord );
    free( wCoord );

    *error = (float)err;
    if( *error > 10.0f ) return -1;

    return 0;
}

int kpmUtilGetPoseHomography( KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{   
    float   *sCoord;
    float   *wCoord;
    float    initPose[3][4];
    int      num;
    int      i;
    
    if( matchData->num < 4 ) return -1;
    num = matchData->num;
    
    arMalloc( sCoord, float, num*2 );
    arMalloc( wCoord, float, num*2 );
    
    for( i = 0; i < num; i++ ) {
        sCoord[i*2+0] = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i*2+1] = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i*2+0] = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.x;
        wCoord[i*2+1] = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.y;
    }
    
    if( kpmUtilGetInitPoseHomography( sCoord, wCoord, num, initPose ) < 0 ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
    
    for( int j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ )  camPose[j][i] = initPose[j][i];
    }
    
    *error = 0.0;
    float  *p1 = sCoord;
    float  *p2 = wCoord;
    for( i = 0; i < num; i++ ) {
        float  x, y, w;
        x = camPose[0][0] * *p2 + camPose[0][1] * *(p2+1) + camPose[0][3];
        y = camPose[1][0] * *p2 + camPose[1][1] * *(p2+1) + camPose[1][3];
        w = camPose[2][0] * *p2 + camPose[2][1] * *(p2+1) + camPose[2][3];
        if( w == 0.0 ) {
            free( sCoord );
            free( wCoord );
            return -1;
        }
        x /= w;
        y /= w;
        *error += (*p1 - x)*(*p1 - x) + (*(p1+1) - y)*(*(p1+1) - y);
    }
    *error /= num;
    
    free( sCoord );
    free( wCoord );
    if( *error > 10.0 ) return -1;

    return 0;
}
#endif

static ARUint8 *genBWImageFull( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    int       xsize2, ysize2;

    *newXsize = xsize2 = xsize;
    *newYsize = ysize2 = ysize;
    arMalloc( newImage, ARUint8, xsize*ysize );
    memcpy(newImage, image, xsize*ysize);

    return newImage;
}

static ARUint8 *genBWImageHalf( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/2;
    *newYsize = ysize2 = ysize/2;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    p  = newImage;
    for( j = 0; j < ysize2; j++ ) {
        p1 = image + xsize*(j*2+0);
        p2 = image + xsize*(j*2+1);
        for( i = 0; i < xsize2; i++ ) {
            *(p++) = ( (int)*(p1+0) + (int)*(p1+1)
                     + (int)*(p2+0) + (int)*(p2+1) ) / 4;
            p1+=2;
            p2+=2;
        }
    }

    return newImage;
}

static ARUint8 *genBWImageQuart( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2, *p3, *p4;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/4;
    *newYsize = ysize2 = ysize/4;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );
    
    p  = newImage;
    for( j = 0; j < ysize2; j++ ) {
        p1 = image + xsize*(j*4+0);
        p2 = image + xsize*(j*4+1);
        p3 = image + xsize*(j*4+2);
        p4 = image + xsize*(j*4+3);
        for( i = 0; i < xsize2; i++ ) {
            *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3)
                     + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3)
                     + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3)
                     + (int)*(p4+0) + (int)*(p4+1) + (int)*(p4+2) + (int)*(p4+3)) / 16;
            p1+=4;
            p2+=4;
            p3+=4;
            p4+=4;
        }
    }

    return newImage;
}


static ARUint8 *genBWImageOneThird( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2, *p3;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/3;
    *newYsize = ysize2 = ysize/3;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    p  = newImage;
    for( j = 0; j < ysize2; j++ ) {
        p1 = image + xsize*(j*3+0);
        p2 = image + xsize*(j*3+1);
        p3 = image + xsize*(j*3+2);
        for( i = 0; i < xsize2; i++ ) {
            *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                     + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                     + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2) ) / 9;
            p1+=3;
            p2+=3;
            p3+=3;
        }
    }

    return newImage;
}

static ARUint8 *genBWImageTwoThird  ( ARUint8 *image, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *q1, *q2, *p1, *p2, *p3;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/3*2;
    *newYsize = ysize2 = ysize/3*2;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    q1  = newImage;
    q2  = newImage + xsize2;
    for( j = 0; j < ysize2/2; j++ ) {
        p1 = image + xsize*(j*3+0);
        p2 = image + xsize*(j*3+1);
        p3 = image + xsize*(j*3+2);
        for( i = 0; i < xsize2/2; i++ ) {
            *(q1++) = ( (int)*(p1+0)   + (int)*(p1+1)/2
                      + (int)*(p2+0)/2 + (int)*(p2+1)/4 ) *4/9;
            *(q2++) = ( (int)*(p2+0)/2 + (int)*(p2+1)/4
                      + (int)*(p3+0)   + (int)*(p3+1)/2 ) *4/9;
            p1++;
            p2++;
            p3++;
            *(q1++) = ( (int)*(p1+0)/2 + (int)*(p1+1)
                      + (int)*(p2+0)/4 + (int)*(p2+1)/2 ) *4/9;
            *(q2++) = ( (int)*(p2+0)/4 + (int)*(p2+1)/2
                      + (int)*(p3+0)/2 + (int)*(p3+1)   ) *4/9;
            p1+=2;
            p2+=2;
            p3+=2;
        }
        q1 += xsize2;
        q2 += xsize2;
    }

    return newImage;
}

#if !BINARY_FEATURE
static int kpmUtilGetInitPoseHomography( float *sCoord, float *wCoord, int num, float initPose[3][4] )
{
    float  *A, *B;
    ARMatf  matA, matB;
    ARMatf *matAt, *matAtA, *matAtB, *matH;
    int     i;
    int     ret = 0;

    arMalloc( A, float, num*8*2 );
    arMalloc( B, float, num*2 );

    for( i = 0; i < num; i++ ) {
        A[i*16+ 0] = wCoord[i*2+0];
        A[i*16+ 1] = wCoord[i*2+1];
        A[i*16+ 2] = 1.0;
        A[i*16+ 3] = 0.0;
        A[i*16+ 4] = 0.0;
        A[i*16+ 5] = 0.0;
        A[i*16+ 6] = -sCoord[i*2+0]*wCoord[i*2+0];
        A[i*16+ 7] = -sCoord[i*2+0]*wCoord[i*2+1];
        A[i*16+ 8] = 0.0;
        A[i*16+ 9] = 0.0;
        A[i*16+10] = 0.0;
        A[i*16+11] = wCoord[i*2+0];
        A[i*16+12] = wCoord[i*2+1];
        A[i*16+13] = 1.0;
        A[i*16+14] = -sCoord[i*2+1]*wCoord[i*2+0];
        A[i*16+15] = -sCoord[i*2+1]*wCoord[i*2+1];
        B[i*2+0]   = sCoord[i*2+0];
        B[i*2+1]   = sCoord[i*2+1];
    }
    
    matA.row = num*2;
    matA.clm = 8;
    matA.m   = A;

    matB.row = num*2;
    matB.clm = 1;
    matB.m   = B;

    matAt = arMatrixAllocTransf( &matA );
    if( matAt == NULL ) {
        ret = -1;
        goto bail;
    }
    matAtA = arMatrixAllocMulf( matAt, &matA );
    if( matAtA == NULL ) {
        ret = -1;
        goto bail1;
    }
    matAtB = arMatrixAllocMulf( matAt, &matB );
    if( matAtB == NULL ) {
        ret = -1;
        goto bail2;
    }
    if( arMatrixSelfInvf(matAtA) < 0 ) {
        ret = -1;
        goto bail3;
    }

    matH = arMatrixAllocMulf( matAtA, matAtB );
    if( matH == NULL ) {
        ret = -1;
        goto bail3;
    }

    initPose[0][0] = matH->m[0];
    initPose[0][1] = matH->m[1];
    initPose[0][2] = 0.0;
    initPose[0][3] = matH->m[2];
    initPose[1][0] = matH->m[3];
    initPose[1][1] = matH->m[4];
    initPose[1][2] = 0.0;
    initPose[1][3] = matH->m[5];
    initPose[2][0] = matH->m[6];
    initPose[2][1] = matH->m[7];
    initPose[2][2] = 0.0;
    initPose[2][3] = 1.0;

    arMatrixFreef( matH );
bail3:
    arMatrixFreef( matAtB );
bail2:
    arMatrixFreef( matAtA );
bail1:
    arMatrixFreef( matAt );
bail:
    free(B);
    free(A);

    return (ret);
}
#endif
