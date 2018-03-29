/*
 *  kpmRefDataSet.cpp
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
#include <stdint.h>
#include <ARX/AR/ar.h>
#include <ARX/KPM/kpm.h>
#include <ARX/KPM/kpmType.h>
#include "kpmPrivate.h"
#include "kpmFopen.h"

#if BINARY_FEATURE
#include <facade/visual_database_facade.h>
#else
#include <ARX/KPM/surfSub.h>
#endif



int kpmGenRefDataSet ( ARUint8 *refImage, int xsize, int ysize, float dpi, int procMode, int compMode, int maxFeatureNum,
                       int pageNo, int imageNo, KpmRefDataSet **refDataSetPtr )
{
    ARUint8         *refImageBW;
    KpmRefDataSet   *refDataSet;
    int              xsize2, ysize2;
    int              i, j;

    if (!refDataSetPtr || !refImage) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr/refImage.\n");
        return (-1);
    }
    if (!xsize || !ysize || !dpi) {
        ARLOGe("kpmDeleteRefDataSet(): 0 xsize/ysize/dpi.\n");
        return (-1);
    }

    arMalloc( refDataSet, KpmRefDataSet, 1 );
    
    refDataSet->pageNum = 1; // I.e. number of pages = 1.
    arMalloc( refDataSet->pageInfo, KpmPageInfo, 1 );
    refDataSet->pageInfo[0].pageNo = pageNo;
    refDataSet->pageInfo[0].imageNum = 1; // I.e. number of images = 1.
    arMalloc( refDataSet->pageInfo[0].imageInfo, KpmImageInfo, 1 );
    refDataSet->pageInfo[0].imageInfo[0].imageNo = imageNo;
    refImageBW = kpmUtilResizeImage( refImage, xsize, ysize, procMode, &xsize2, &ysize2 );
    refDataSet->pageInfo[0].imageInfo[0].width   = xsize2;
    refDataSet->pageInfo[0].imageInfo[0].height  = ysize2;

    if( compMode == KpmCompY ) {
        ARUint8 *refImageBW2 = refImageBW;
        refImageBW = (ARUint8 *)malloc(sizeof(ARUint8)*xsize2*(ysize2/2));
        if( refImageBW == NULL ) exit(0);

        ARUint8 *p1 = refImageBW;
        ARUint8 *p2 = refImageBW2;
        for( j = 0; j < ysize2/2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p1++) = ( (int)*p2 + (int)*(p2+xsize2) ) / 2;
                p2++;
            }
            p2 += xsize2;
        }
        free( refImageBW2 );
    }

#if BINARY_FEATURE
    vision::VisualDatabaseFacade freakMatcher;
    std::vector<vision::FeaturePoint> points;
    std::vector<unsigned char> descriptors;
    freakMatcher.computeFreakFeaturesAndDescriptors(refImageBW, xsize2, ysize2, points, descriptors);
    ARLOGi("Freak features - %d\n",points.size());
    //freakMatcher.addImage(refImageBW, xsize2, ysize2, imageNo);
    //const std::vector<vision::FeaturePoint>& points = freakMatcher.getFeaturePoints(imageNo);
    //const std::vector<unsigned char>& descriptors = freakMatcher.getDescriptors(imageNo);
    refDataSet->num = (int)points.size();
#else
    SurfSubHandleT   *surfHandle;
    surfHandle = surfSubCreateHandle(xsize2, (compMode == KpmCompY)? ysize2/2: ysize2, AR_PIXEL_FORMAT_MONO);
    if (!surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        exit(-1);
    }
    surfSubSetMaxPointNum( surfHandle, maxFeatureNum );
    surfSubExtractFeaturePoint( surfHandle, refImageBW, NULL, 0 );
    refDataSet->num = surfSubGetFeaturePointNum( surfHandle );
#endif
    
    if( refDataSet->num != 0 ) {
        arMalloc( refDataSet->refPoint, KpmRefData, refDataSet->num );
        if( procMode == KpmProcFullSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < FREAK_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = descriptors[i*FREAK_SUB_DIMENSION+j];
                }
                refDataSet->refPoint[i].featureVec.angle = points[i].angle;
                refDataSet->refPoint[i].featureVec.scale = points[i].scale;
                refDataSet->refPoint[i].featureVec.maxima = (int)points[i].maxima;
#else
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
#endif
                
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x + 0.5f) / dpi * 25.4f;               // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-0.5f) - y) / dpi * 25.4f;       // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcTwoThirdSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < FREAK_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = descriptors[i*FREAK_SUB_DIMENSION+j];
                }
                refDataSet->refPoint[i].featureVec.angle = points[i].angle;
                refDataSet->refPoint[i].featureVec.scale = points[i].scale;
                refDataSet->refPoint[i].featureVec.maxima = (int)points[i].maxima;
#else
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
#endif
                
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*1.5f + 0.75f) / dpi * 25.4f;         // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-0.75f) - y*1.5f) / dpi * 25.4f; // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcHalfSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < FREAK_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = descriptors[i*FREAK_SUB_DIMENSION+j];
                }
                refDataSet->refPoint[i].featureVec.angle = points[i].angle;
                refDataSet->refPoint[i].featureVec.scale = points[i].scale;
                refDataSet->refPoint[i].featureVec.maxima = (int)points[i].maxima;
#else
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
#endif
                
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*2.0f + 1.0f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-1.0f) - y*2.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcOneThirdSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < FREAK_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = descriptors[i*FREAK_SUB_DIMENSION+j];
                }
                refDataSet->refPoint[i].featureVec.angle = points[i].angle;
                refDataSet->refPoint[i].featureVec.scale = points[i].scale;
                refDataSet->refPoint[i].featureVec.maxima = (int)points[i].maxima;
#else
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
#endif
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*3.0f + 1.5f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-1.5f) - y*3.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < FREAK_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = descriptors[i*FREAK_SUB_DIMENSION+j];
                }
                refDataSet->refPoint[i].featureVec.angle = points[i].angle;
                refDataSet->refPoint[i].featureVec.scale = points[i].scale;
                refDataSet->refPoint[i].featureVec.maxima = (int)points[i].maxima;
#else
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
#endif
                
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*4.0f + 2.0f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-2.0f) - y*4.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
    }
    else {
        refDataSet->refPoint = NULL;
    }
    free(refImageBW);
#if !BINARY_FEATURE
    surfSubDeleteHandle( &surfHandle );
#endif
    
    *refDataSetPtr = refDataSet;

    return 0;
}

int kpmAddRefDataSet ( ARUint8 *refImage, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                              int pageNo, int imageNo, KpmRefDataSet **refDataSetPtr )
{
    KpmRefDataSet  *refDataSetPtr2;
    int ret;

    ret =  kpmGenRefDataSet(refImage, xsize, ysize, dpi, procMode, compMode, maxFeatureNum, pageNo, imageNo, &refDataSetPtr2);
    if (ret < 0) {
        ARLOGe("Error while adding reference data set: kpmGenRefDataSet() failed.\n");
        return (ret);
    }

    ARLOGi("========= %d ===========\n", refDataSetPtr2->num);

    ret = kpmMergeRefDataSet( refDataSetPtr, &refDataSetPtr2 );
    if (ret < 0) {
        ARLOGe("Error while adding reference data set: kpmMergeRefDataSet() failed.\n");
    }
    return (ret);
}

int kpmMergeRefDataSet ( KpmRefDataSet **refDataSetPtr1, KpmRefDataSet **refDataSetPtr2 )
{
    KpmRefData    *refPoint;
    KpmPageInfo   *pageInfo;
    int            pageNum;
    int            imageNum;
    int            num1, num2, num3;
    int            i, j, k, l;

    if (!refDataSetPtr1 || !refDataSetPtr2) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr1/refDataSetPtr2.\n");
        return (-1);
    }

    if (!*refDataSetPtr1) {
        arMalloc( *refDataSetPtr1, KpmRefDataSet, 1 );
        (*refDataSetPtr1)->num          = 0;
        (*refDataSetPtr1)->refPoint     = NULL;
        (*refDataSetPtr1)->pageNum      = 0;
        (*refDataSetPtr1)->pageInfo     = NULL;
    }
    if (!*refDataSetPtr2) return 0;
    
    // Merge KpmRefData.
    num1 = (*refDataSetPtr1)->num;
    num2 = (*refDataSetPtr2)->num;
    arMalloc( refPoint, KpmRefData, num1+num2 );
    for( i = 0; i < num1; i++ ) {
        refPoint[i] = (*refDataSetPtr1)->refPoint[i];
    }
    for( i = 0; i < num2; i++ ) {
        refPoint[num1+i] = (*refDataSetPtr2)->refPoint[i];
    }
    if( (*refDataSetPtr1)->refPoint != NULL ) free((*refDataSetPtr1)->refPoint);
    (*refDataSetPtr1)->refPoint = refPoint;
    (*refDataSetPtr1)->num      = num1 + num2;
    
    // Allocate pageInfo for the combined sets.
    num1 = (*refDataSetPtr1)->pageNum;
    num2 = (*refDataSetPtr2)->pageNum;
    num3 = 0;
    for( i = 0; i < num2; i++ ) {
        for( j = 0; j < num1; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[i].pageNo == (*refDataSetPtr1)->pageInfo[j].pageNo ) {
                num3++; // count a duplicate.
                break;
            }
        }
    }
    pageNum = num1+num2-num3;
    arMalloc(pageInfo, KpmPageInfo, pageNum);
    
    for( i = 0; i < num1; i++ ) {
        
        pageInfo[i].pageNo = (*refDataSetPtr1)->pageInfo[i].pageNo;
        
        // Count the number of imageInfo records in the combined set for this pageNo.
        imageNum = (*refDataSetPtr1)->pageInfo[i].imageNum;
        for( j = 0; j < num2; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[j].pageNo == (*refDataSetPtr1)->pageInfo[i].pageNo ) {
                imageNum += (*refDataSetPtr2)->pageInfo[j].imageNum;
            }
        }
        arMalloc(pageInfo[i].imageInfo, KpmImageInfo, imageNum);
        
        // Copy the imageInfo records into the new set.
        l = (*refDataSetPtr1)->pageInfo[i].imageNum;
        for( j = 0; j < l; j++ ) {
            pageInfo[i].imageInfo[j] = (*refDataSetPtr1)->pageInfo[i].imageInfo[j];
        }
        for( j = 0; j < num2; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[j].pageNo == (*refDataSetPtr1)->pageInfo[i].pageNo ) {
                for( k = 0; k < (*refDataSetPtr2)->pageInfo[j].imageNum; k++ ) {
                    pageInfo[i].imageInfo[l+k] = (*refDataSetPtr2)->pageInfo[j].imageInfo[k];
                }
                break;
            }
        }
        pageInfo[i].imageNum = imageNum;
    }
    
    k = 0;
    for( i = 0; i < num2; i++ ) {
        for( j = 0; j < num1; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[i].pageNo == (*refDataSetPtr1)->pageInfo[j].pageNo ) {
                k++; // count a duplicate.
                break;
            }
        }
        if( j < num1 ) continue;
        // If we get to here, we have a page from refDataSetPtr2 which doesn't
        // have the same pageNo as any page from refDataSetPtr1.
        pageInfo[num1+i-k].pageNo = (*refDataSetPtr2)->pageInfo[i].pageNo;
        imageNum = (*refDataSetPtr2)->pageInfo[i].imageNum;
        arMalloc(pageInfo[num1+i-k].imageInfo, KpmImageInfo, imageNum);
        for( j = 0; j < imageNum; j++ ) {
            pageInfo[num1+i-k].imageInfo[j] = (*refDataSetPtr2)->pageInfo[i].imageInfo[j];
        }
        pageInfo[num1+i-k].imageNum = imageNum;
    }

    if ((*refDataSetPtr1)->pageInfo) {
        for( i = 0; i < (*refDataSetPtr1)->pageNum; i++ ) {
            free( (*refDataSetPtr1)->pageInfo[i].imageInfo );
        }
        free((*refDataSetPtr1)->pageInfo);
    }
    (*refDataSetPtr1)->pageInfo = pageInfo;
    (*refDataSetPtr1)->pageNum  = pageNum;

    kpmDeleteRefDataSet(refDataSetPtr2);

    return 0;
}

int kpmDeleteRefDataSet( KpmRefDataSet **refDataSetPtr )
{
    if (!refDataSetPtr) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr.\n");
        return (-1);
    }
    if (!*refDataSetPtr) return 0; // OK to call on already deleted handle.

    if ((*refDataSetPtr)->refPoint) free((*refDataSetPtr)->refPoint);
    
    for(int i = 0; i < (*refDataSetPtr)->pageNum; i++ ) {
        free( (*refDataSetPtr)->pageInfo[i].imageInfo );
    }
    free( (*refDataSetPtr)->pageInfo );
    free( *refDataSetPtr );
    *refDataSetPtr = NULL;

    return 0;
}


int kpmSaveRefDataSet( const char *filename, const char *ext, KpmRefDataSet  *refDataSet )
{
    FILE   *fp;
    char    fmode[] = "wb";
    int     i, j;

    if (!filename || !refDataSet) {
        ARLOGe("kpmSaveRefDataSet(): NULL filename/refDataSet.\n");
        return (-1);
    }

    fp = kpmFopen(filename, ext, fmode);
    if( fp == NULL ) {
        ARLOGe("Error saving KPM data: unable to open file '%s%s%s' for writing.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return -1;
    }

    if( fwrite(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    
    for(i = 0; i < refDataSet->num; i++ ) {
        if( fwrite(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadWrite;
#if BINARY_FEATURE
        if( fwrite(  &(refDataSet->refPoint[i].featureVec), sizeof(FreakFeature), 1, fp) != 1 ) goto bailBadWrite;
#else
        if( fwrite(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) goto bailBadWrite;
#endif
        if( fwrite(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    }

    if( fwrite(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fwrite( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        j = refDataSet->pageInfo[i].imageNum;
        if( fwrite(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadWrite;
    }

    fclose(fp);
    return 0;
    
bailBadWrite:
    ARLOGe("Error saving KPM data: error writing data.\n");
    fclose(fp);
    return -1;
}

int kpmLoadRefDataSet( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr )
{
    KpmRefDataSet  *refDataSet;
    FILE           *fp;
    char            fmode[] = "rb";
    int             i, j;

    if (!filename || !refDataSetPtr) {
        ARLOGe("kpmLoadRefDataSet(): NULL filename/refDataSetPtr.\n");
        return (-1);
    }

    fp = kpmFopen(filename, ext, fmode);
    if (!fp) {
        ARLOGe("Error loading KPM data: unable to open file '%s%s%s' for reading.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return (-1);
    }

    arMallocClear(refDataSet, KpmRefDataSet, 1);
    
    if( fread(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->num <= 0 ) goto bailBadRead;
    arMalloc(refDataSet->refPoint, KpmRefData, refDataSet->num); // each KpmRefData = 68 floats, 3 ints = 284 bytes.
    
    for(i = 0; i < refDataSet->num; i++ ) {
        if( fread(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
#if BINARY_FEATURE
        if( fread(  &(refDataSet->refPoint[i].featureVec), sizeof(FreakFeature), 1, fp) != 1 ) goto bailBadRead;
#else
        if( fread(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) goto bailBadRead;
#endif
        
        if( fread(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    }

    if( fread(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;

    if( refDataSet->pageNum <= 0 ) {
        refDataSet->pageInfo = NULL;
        goto bailBadRead;
    }
    arMalloc(refDataSet->pageInfo, KpmPageInfo, refDataSet->pageNum);
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fread( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        j = refDataSet->pageInfo[i].imageNum;
        arMalloc(refDataSet->pageInfo[i].imageInfo, KpmImageInfo, j);
        if( fread(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadRead;
    }

    *refDataSetPtr = refDataSet;

    fclose(fp);
    return 0;

bailBadRead:
    ARLOGe("Error loading KPM data: error reading data.\n");
    if (refDataSet->pageInfo) free(refDataSet->pageInfo);
    if (refDataSet->refPoint) free(refDataSet->refPoint);
    free(refDataSet);
    fclose(fp);
    return (-1);
}

int kpmLoadRefDataSetOld( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr )
{
#if !BINARY_FEATURE
    KpmRefDataSet  *refDataSet;
    FILE           *fp;
    char            fmode[] = "rb";
    uint32_t        dummy[2];
    int             i, j;
    
    if (!filename || !refDataSetPtr) {
        ARLOGe("kpmLoadRefDataSetOld(): NULL filename/refDataSetPtr.\n");
        return (-1);
    }
    
    fp = kpmFopen(filename, ext, fmode);
    if( fp == NULL ) {
        ARLOGe("Error loading KPM data: unable to open file '%s%s%s' for reading.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return -1;
    }
    
    arMallocClear(refDataSet, KpmRefDataSet, 1);
    
    if( fread(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->num <= 0 ) goto bailBadRead;
    arMalloc(refDataSet->refPoint, KpmRefData, refDataSet->num);

    if( fread(dummy, sizeof(uint32_t), 2, fp) != 2 ) goto bailBadRead; // Skip old (int)refDataSet->surfThresh and (int)refDataSet->coord3DFlag.

    for(i = 0; i < refDataSet->num; i++ ) {
        if( fread(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        //if( fread(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) {
        //    goto bailBadRead;
        //}
        if( fread( &(refDataSet->refPoint[i].featureVec.v[0]), sizeof(float), SURF_SUB_DIMENSION, fp) != SURF_SUB_DIMENSION ) goto bailBadRead;
        refDataSet->refPoint[i].featureVec.l = 2;
        if( fread(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        refDataSet->refPoint[i].pageNo     = 1;
        refDataSet->refPoint[i].refImageNo = 1;
    }
    
    if( fread(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->pageNum <= 0 ) {
        refDataSet->pageInfo = NULL;
        goto bailBadRead;
    }
    arMalloc(refDataSet->pageInfo, KpmPageInfo, refDataSet->pageNum);
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fread( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        //if( fread( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        refDataSet->pageInfo[i].imageNum = 1;
        j = refDataSet->pageInfo[i].imageNum;
        arMalloc(refDataSet->pageInfo[i].imageInfo, KpmImageInfo, j);
        //if( fread(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadRead;
        refDataSet->pageInfo[i].imageInfo->width  = 1000;
        refDataSet->pageInfo[i].imageInfo->height = 1000;
        refDataSet->pageInfo[i].imageInfo->imageNo = 1;
    }
    
    *refDataSetPtr = refDataSet;
    
    fclose(fp);
    return 0;
    
bailBadRead:
    ARLOGe("Error loading KPM data: error reading data.\n");
    if (refDataSet->pageInfo) free(refDataSet->pageInfo);
    if (refDataSet->refPoint) free(refDataSet->refPoint);
    free(refDataSet);
    fclose(fp);
    return (-1);
#else
    return 0;
#endif
    
}

int kpmChangePageNoOfRefDataSet ( KpmRefDataSet *refDataSet, int oldPageNo, int newPageNo )
{
    if (!refDataSet) {
        ARLOGe("kpmChangePageNoOfRefDataSet(): NULL refDataSet.\n");
        return (-1);
    }

    for(int i = 0; i < refDataSet->num; i++ ) {
        if( refDataSet->refPoint[i].pageNo == oldPageNo || (oldPageNo == KpmChangePageNoAllPages && refDataSet->refPoint[i].pageNo >= 0) ) {
            refDataSet->refPoint[i].pageNo = newPageNo;
        }
    }
    
    for(int i = 0; i < refDataSet->pageNum; i++ ) {
        if( refDataSet->pageInfo[i].pageNo == oldPageNo || (oldPageNo == KpmChangePageNoAllPages && refDataSet->pageInfo[i].pageNo >= 0) ) {
            refDataSet->pageInfo[i].pageNo = newPageNo;
        }
    }

    return 0;
}
