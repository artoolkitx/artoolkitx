/*
 *  kpmMatching.cpp
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
 *  Author(s): Hirokazu Kato, Philip Lamb, Nalin Senthamil
 *
 */

#include <stdio.h>
#include <ARX/AR/ar.h>
#include <string>
#include <sstream>
#include <algorithm>

#include <ARX/KPM/kpm.h>
#include "kpmPrivate.h"
#if BINARY_FEATURE
extern "C" {
#  include <jpeglib.h>
}
#else
#  include "HomographyEst.h"
#  include "AnnMatch.h"
#  include "AnnMatch2.h"
#endif

int kpmUtilGetPose_binary( ARParamLT *cparamLT, const vision::matches_t &matchData, const std::vector<vision::Point3d<float> > &refDataSet, const std::vector<vision::FeaturePoint> &inputDataSet, float  camPose[3][4], float  *error );

template<typename T>
std::string arrayToString(T *v, size_t size){
    std::stringstream ss;
    for(size_t i = 0; i < size; ++i){
        if(i != 0)
            ss << ",";
        ss << v[i];
    }
    std::string s = ss.str();
    return s;
}

std::string arrayToString2(float pose[3][4]){
    std::stringstream ss;
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 4; ++j){
            if(j != 0 )
                ss << ",";
            ss << pose[i][j];
        }
        ss << "\n";
    }
    std::string s = ss.str();
    return s;
}

static unsigned char *kpmReadJPEGMono(FILE *fp, int *width, int *height)
{
    unsigned char                   *buf;
    struct jpeg_decompress_struct    cinfo;
    struct jpeg_error_mgr            jerr;
    unsigned char                  **decompressBufRowPtrs = NULL;
    int                              decompressBufRows, rowsToRead;
    int                              row;
    int                              i;
    
    if (!fp || !width || !height) return (NULL);
    
    memset(&cinfo, 0, sizeof(cinfo));
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    (void) jpeg_read_header(&cinfo, TRUE);
    
    // Adjust decompression parameters to match requested.
    if (cinfo.num_components != 3 && cinfo.num_components != 4 && cinfo.num_components != 1) {
        ARLOGe("JPEG file has unsupported %d-component pixels\n", cinfo.num_components);
        jpeg_destroy_decompress(&cinfo);
        return (NULL);
    }
    cinfo.out_color_space = JCS_GRAYSCALE;// Converting to mono, let libjpeg handle it.

    // Start decompression. This gives us access to the JPEG size.
    (void) jpeg_start_decompress(&cinfo);
    *height = cinfo.output_height;
    *width = cinfo.output_width;
    buf = (unsigned char *)malloc(*width * *height);
    if (!buf) {
        ARLOGe("Out of memory!!\n");
        jpeg_destroy_decompress(&cinfo);
        return (NULL);
    }
    
    // Decompression requires a bunch of pointers to where to output the decompressed pixels. Create an array to hold the pointers.
    decompressBufRows = cinfo.rec_outbuf_height;
    arMalloc(decompressBufRowPtrs, unsigned char *, decompressBufRows);
    
    // Decompress straight into user-supplied buffer (i.e. buf).
    row = 0;
    while (cinfo.output_scanline < *height) {
        rowsToRead = std::min<int>(*height - cinfo.output_scanline, decompressBufRows);
        // Update the set of pointers to decompress into.
        for (i = 0; i < rowsToRead; i++) decompressBufRowPtrs[i] = &(buf[*width * (row + i)]);
        // Decompress.
        row += jpeg_read_scanlines(&cinfo, decompressBufRowPtrs, rowsToRead);
    }
    
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    free(decompressBufRowPtrs);
    
    return (buf);
}

int kpmLoadImageDb(KpmHandle *kpmHandle, const char *filename)
{
    FILE                *fp;
    unsigned char       *image = NULL;
    int                  width, height;
    
    if (!kpmHandle || !filename) {
        ARLOGe("kpmSetRefDataSet(): NULL kpmHandle/filename.\n");
        return -1;
    }
    
    if ((fp = fopen(filename, "rb")) == NULL) {
        ARLOGe("Can't open JPEG file '%s'\n", filename);
        ARLOGperror(NULL);
        return (0);
    } else {
        image = kpmReadJPEGMono(fp, &width, &height);
        fclose(fp);
        if (!image) {
            ARLOGe("Can't read JPEG file '%s'\n", filename);
            return (0);
        }
    }

    kpmHandle->freakMatcher->addImage(image, width, height, 1);
    free(image);
    
    return 1;
}
        
int kpmSetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet *refDataSet )
{
#if !BINARY_FEATURE
    CAnnMatch2         *ann2;
#endif
    
    FeatureVector       featureVector;
    int                 i, j;
    
    if (!kpmHandle || !refDataSet) {
        ARLOGe("kpmSetRefDataSet(): NULL kpmHandle/refDataSet.\n");
        return -1;
    }
    if (!refDataSet->num) {
        ARLOGe("kpmSetRefDataSet(): refDataSet.\n");
        return -1;
    }
    
    // Copy the refPoints into the kpmHandle's dataset.
    if( kpmHandle->refDataSet.refPoint != NULL ) {
        // Discard any old points first.
        free( kpmHandle->refDataSet.refPoint );
    }
    if( refDataSet->num != 0 ) {
        arMalloc( kpmHandle->refDataSet.refPoint, KpmRefData, refDataSet->num );
        for( i = 0; i < refDataSet->num; i++ ) {
            kpmHandle->refDataSet.refPoint[i] = refDataSet->refPoint[i];
        }
    }
    else {
        kpmHandle->refDataSet.refPoint = NULL;
    }
    kpmHandle->refDataSet.num = refDataSet->num;

    // Copy the pageInfo into the kpmHandle's dataset.
    if( kpmHandle->refDataSet.pageInfo != NULL ) {
        // Discard any old pageInfo (and imageInfo) first.
        for( i = 0; i < kpmHandle->refDataSet.pageNum; i++ ) {
            if( kpmHandle->refDataSet.pageInfo[i].imageInfo != NULL ) {
                free( kpmHandle->refDataSet.pageInfo[i].imageInfo );
            }
        }
        free( kpmHandle->refDataSet.pageInfo );
    }
    if( refDataSet->pageNum != 0 ) {
        arMalloc( kpmHandle->refDataSet.pageInfo, KpmPageInfo, refDataSet->pageNum );
        for( i = 0; i < refDataSet->pageNum; i++ ) {
            kpmHandle->refDataSet.pageInfo[i].pageNo = refDataSet->pageInfo[i].pageNo;
            kpmHandle->refDataSet.pageInfo[i].imageNum = refDataSet->pageInfo[i].imageNum;
            if( refDataSet->pageInfo[i].imageNum != 0 ) {
                arMalloc( kpmHandle->refDataSet.pageInfo[i].imageInfo, KpmImageInfo, refDataSet->pageInfo[i].imageNum );
                for( j = 0; j < refDataSet->pageInfo[i].imageNum; j++ ) {
                    kpmHandle->refDataSet.pageInfo[i].imageInfo[j] = refDataSet->pageInfo[i].imageInfo[j];
                }
            }
            else {
                refDataSet->pageInfo[i].imageInfo = NULL;
            }
        }
    }
    else {
        kpmHandle->refDataSet.pageInfo = NULL;
    }
    kpmHandle->refDataSet.pageNum = refDataSet->pageNum;

    
    if( kpmHandle->result != NULL ) {
        free( kpmHandle->result );
        kpmHandle->result = NULL;
        kpmHandle->resultNum = 0;
    }
    if( refDataSet->pageNum > 0 ) {
        kpmHandle->resultNum = refDataSet->pageNum;
        arMalloc( kpmHandle->result, KpmResult, refDataSet->pageNum );
		for (int i = 0; i < refDataSet->pageNum;i++){
			kpmHandle->result[i].skipF = 0;
		}        
    }

    // Create feature vectors.
#if !BINARY_FEATURE
    if (kpmHandle->ann2) {
        delete (CAnnMatch2 *)(kpmHandle->ann2);
        kpmHandle->ann2 = NULL;
    }
    if (kpmHandle->refDataSet.num != 0) {
        ann2 = new CAnnMatch2();
        kpmHandle->ann2 = (void *)ann2;
        arMalloc( featureVector.sf, SurfFeature, kpmHandle->refDataSet.num );
        for( int l = 0; l < kpmHandle->refDataSet.num; l++ ) {
            featureVector.sf[l] = kpmHandle->refDataSet.refPoint[l].featureVec;
        }
        featureVector.num = kpmHandle->refDataSet.num;
        ann2->Construct(&featureVector);
        free(featureVector.sf);
    }
#else
    if (kpmHandle->refDataSet.num != 0) {
        featureVector.num = kpmHandle->refDataSet.num;
        
        int db_id = 0;
        for (int k = 0; k < kpmHandle->refDataSet.pageNum; k++) {
            for (int m = 0; m < kpmHandle->refDataSet.pageInfo[k].imageNum; m++) {
                std::vector<vision::FeaturePoint> points;
                std::vector<vision::Point3d<float> > points_3d;
                std::vector<unsigned char> descriptors;
            
                for (int i = 0; i < featureVector.num; i++) {
                    if (kpmHandle->refDataSet.refPoint[i].refImageNo == kpmHandle->refDataSet.pageInfo[k].imageInfo[m].imageNo
                    && kpmHandle->refDataSet.refPoint[i].pageNo == kpmHandle->refDataSet.pageInfo[k].pageNo) {
                        points.push_back(vision::FeaturePoint(kpmHandle->refDataSet.refPoint[i].coord2D.x,
                                                          kpmHandle->refDataSet.refPoint[i].coord2D.y,
                                                          kpmHandle->refDataSet.refPoint[i].featureVec.angle,
                                                          kpmHandle->refDataSet.refPoint[i].featureVec.scale,
                                                          kpmHandle->refDataSet.refPoint[i].featureVec.maxima));
                        points_3d.push_back(vision::Point3d<float>(kpmHandle->refDataSet.refPoint[i].coord3D.x,
                                                                   kpmHandle->refDataSet.refPoint[i].coord3D.y,                                                                  0));
                        for (int j = 0; j < FREAK_SUB_DIMENSION; j++)
                            descriptors.push_back(kpmHandle->refDataSet.refPoint[i].featureVec.v[j]);
                    }
                }
                ARLOGi("points-%d\n", points.size());
               kpmHandle->pageIDs[db_id] = kpmHandle->refDataSet.pageInfo[k].pageNo; kpmHandle->freakMatcher->addFreakFeaturesAndDescriptors(points,descriptors,points_3d,kpmHandle->refDataSet.pageInfo[k].imageInfo[m].width,kpmHandle->refDataSet.pageInfo[k].imageInfo[m].height,db_id++);
            }
        }
    }
#endif
    
    return 0;
}

int kpmSetRefDataSetFile( KpmHandle *kpmHandle, const char *filename, const char *ext )
{
    KpmRefDataSet   *refDataSet;
    
    if (!kpmHandle || !filename) {
        ARLOGe("kpmSetRefDataSetFile(): NULL kpmHandle/filename.\n");
        return -1;
    }
    
    if( kpmLoadRefDataSet(filename, ext, &refDataSet) < 0 ) return -1;
    if( kpmSetRefDataSet(kpmHandle, refDataSet) < 0 ) {
        kpmDeleteRefDataSet(&refDataSet);
        return -1;
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    return 0;
}

int kpmSetRefDataSetFileOld( KpmHandle *kpmHandle, const char *filename, const char *ext )
{
    KpmRefDataSet   *refDataSet;
    
    if( kpmHandle == NULL )  return -1;
    
    if( kpmLoadRefDataSetOld(filename, ext, &refDataSet) < 0 ) return -1;
    if( kpmSetRefDataSet(kpmHandle, refDataSet) < 0 ) {
        kpmDeleteRefDataSet(&refDataSet);
        return -1;
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    return 0;
}

int kpmSetMatchingSkipPage( KpmHandle *kpmHandle, int skipPages[], int num )
{
    int    i, j;
    
    if( kpmHandle == NULL ) return -1;
    
    for( i = 0; i < num; i++ ) {
        for( j = 0; j < kpmHandle->refDataSet.pageNum; j++ ) {
            if( skipPages[i] == kpmHandle->refDataSet.pageInfo[j].pageNo ) {
                kpmHandle->result[j].skipF = 1;
                break;
            }
        }
        if( j == kpmHandle->refDataSet.pageNum ) {
            ARLOGe("Cannot find the page for skipping.\n");
            return -1;
        }
    }
    
    return 0;
}

#if !BINARY_FEATURE
int kpmSetMatchingSkipRegion( KpmHandle *kpmHandle, SurfSubRect *skipRegion, int regionNum)
{
    if( kpmHandle->skipRegion.regionMax < regionNum ) {
        if( kpmHandle->skipRegion.region != NULL ) free(kpmHandle->skipRegion.region);
        kpmHandle->skipRegion.regionMax = ((regionNum-1)/10+1) * 10;
        arMalloc(kpmHandle->skipRegion.region, SurfSubSkipRegion, kpmHandle->skipRegion.regionMax);
    }
    kpmHandle->skipRegion.regionNum = regionNum;
    for(int i = 0; i < regionNum; i++ ) {
        kpmHandle->skipRegion.region[i].rect = skipRegion[i];
        for(int j = 0; j < 4; j++) {
            kpmHandle->skipRegion.region[i].param[j][0] = skipRegion[i].vertex[(j+1)%4].y - skipRegion[i].vertex[j].y;
            kpmHandle->skipRegion.region[i].param[j][1] = skipRegion[i].vertex[j].x - skipRegion[i].vertex[(j+1)%4].x;
            kpmHandle->skipRegion.region[i].param[j][2] = skipRegion[i].vertex[(j+1)%4].x * skipRegion[i].vertex[j].y
                                                        - skipRegion[i].vertex[j].x * skipRegion[i].vertex[(j+1)%4].y;
        }
    }
    return 0;
}
#endif

int kpmMatching(KpmHandle *kpmHandle, ARUint8 *inImageLuma)
{
    int               xsize, ysize;
    int               xsize2, ysize2;
    int               procMode;
    ARUint8          *imageLuma;
    int               imageLumaWasAllocated;
    int               i;
#if !BINARY_FEATURE
    FeatureVector     featureVector;
    int              *inlierIndex;
    CorspMap          preRANSAC;
    int               inlierNum;
    CAnnMatch2       *ann2;
    int              *annMatch2;
    int               knn;
    float             h[3][3];
    int               j;
#endif
    int               ret;
    
    if (!kpmHandle || !inImageLuma) {
        ARLOGe("kpmMatching(): NULL kpmHandle/inImageLuma.\n");
        return -1;
    }
    
    xsize           = kpmHandle->xsize;
    ysize           = kpmHandle->ysize;
    procMode        = kpmHandle->procMode;
    
    if (procMode == KpmProcFullSize) {
        imageLuma = inImageLuma;
        xsize2 = xsize;
        ysize2 = ysize;
        imageLumaWasAllocated = 0;
    } else {
        imageLuma = kpmUtilResizeImage(inImageLuma, xsize, ysize, procMode, &xsize2, &ysize2);
        if (!imageLuma) return -1;
        imageLumaWasAllocated = 1;
    }

#if BINARY_FEATURE
    kpmHandle->freakMatcher->query(imageLuma, xsize2, ysize2);
    kpmHandle->inDataSet.num = (int)kpmHandle->freakMatcher->getQueryFeaturePoints().size();
#else
    surfSubExtractFeaturePoint( kpmHandle->surfHandle, imageLuma, kpmHandle->skipRegion.region, kpmHandle->skipRegion.regionNum );
    kpmHandle->skipRegion.regionNum = 0;
    kpmHandle->inDataSet.num = featureVector.num = surfSubGetFeaturePointNum( kpmHandle->surfHandle );
#endif
    
    if( kpmHandle->inDataSet.num != 0 ) {
        if( kpmHandle->inDataSet.coord != NULL ) free(kpmHandle->inDataSet.coord);
#if !BINARY_FEATURE
        if( kpmHandle->preRANSAC.match != NULL ) free(kpmHandle->preRANSAC.match);
        if( kpmHandle->aftRANSAC.match != NULL ) free(kpmHandle->aftRANSAC.match);
#endif
        arMalloc( kpmHandle->inDataSet.coord, KpmCoord2D,     kpmHandle->inDataSet.num );
#if !BINARY_FEATURE
        arMalloc( kpmHandle->preRANSAC.match, KpmMatchData,   kpmHandle->inDataSet.num );
        arMalloc( kpmHandle->aftRANSAC.match, KpmMatchData,   kpmHandle->inDataSet.num );
#endif
#if BINARY_FEATURE
#else
        arMalloc( featureVector.sf,           SurfFeature,    kpmHandle->inDataSet.num );
        arMalloc( preRANSAC.mp,               MatchPoint,     kpmHandle->inDataSet.num );
        arMalloc( inlierIndex,                int,            kpmHandle->inDataSet.num );

        knn = 1;
        arMalloc( annMatch2,                  int,            kpmHandle->inDataSet.num*knn);
#endif
        
#if BINARY_FEATURE
        const std::vector<vision::FeaturePoint>& points = kpmHandle->freakMatcher->getQueryFeaturePoints();
        //const std::vector<unsigned char>& descriptors = kpmHandle->freakMatcher->getQueryDescriptors();
#endif
        if( procMode == KpmProcFullSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {

#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
#else
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
#endif
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x, y, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x;
                    kpmHandle->inDataSet.coord[i].y = y;
                }
            }
        }
        else if( procMode == KpmProcTwoThirdSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
#else
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
#endif
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*1.5f, y*1.5f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*1.5f;
                    kpmHandle->inDataSet.coord[i].y = y*1.5f;
                }
            }
        }
        else if( procMode == KpmProcHalfSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
#else
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
#endif
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*2.0f, y*2.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*2.0f;
                    kpmHandle->inDataSet.coord[i].y = y*2.0f;
                }
            }
        }
        else if( procMode == KpmProcOneThirdSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
#else
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
#endif
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*3.0f, y*3.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*3.0f;
                    kpmHandle->inDataSet.coord[i].y = y*3.0f;
                }
            }
        }
        else { // procMode == KpmProcQuatSize
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
#if BINARY_FEATURE
                float  x = points[i].x, y = points[i].y;
#else
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
#endif
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*4.0f, y*4.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*4.0f;
                    kpmHandle->inDataSet.coord[i].y = y*4.0f;
                }
            }
        }

#if !BINARY_FEATURE
        ann2 = (CAnnMatch2*)kpmHandle->ann2;
        ann2->Match(&featureVector, knn, annMatch2);
        for(int pageLoop = 0; pageLoop < kpmHandle->resultNum; pageLoop++ ) {
            kpmHandle->preRANSAC.num = 0;
            kpmHandle->aftRANSAC.num = 0;
            
            kpmHandle->result[pageLoop].pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            kpmHandle->result[pageLoop].camPoseF = -1;
            if( kpmHandle->result[pageLoop].skipF ) continue;

            int featureNum = 0;
            int *annMatch2Ptr = annMatch2;
            int pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            for( i = 0; i < kpmHandle->inDataSet.num; i++ ) {
                for( j = 0; j < knn; j++ ) {
                    if( *annMatch2Ptr >= 0 && kpmHandle->refDataSet.refPoint[*annMatch2Ptr].pageNo == pageNo ) {
                        kpmHandle->preRANSAC.match[featureNum].inIndex = i;
                        kpmHandle->preRANSAC.match[featureNum].refIndex = *annMatch2Ptr;
                        preRANSAC.mp[featureNum].x1 = kpmHandle->inDataSet.coord[i].x;
                        preRANSAC.mp[featureNum].y1 = kpmHandle->inDataSet.coord[i].y;
                        preRANSAC.mp[featureNum].x2 = kpmHandle->refDataSet.refPoint[*annMatch2Ptr].coord3D.x;
                        preRANSAC.mp[featureNum].y2 = kpmHandle->refDataSet.refPoint[*annMatch2Ptr].coord3D.y;
                        featureNum++;
                        annMatch2Ptr += knn-j;
                        break;
                    }
                    annMatch2Ptr++;
                }
            }
            //printf("Page[%d] %d\n", pageLoop, featureNum);
            preRANSAC.num = featureNum;
            if( featureNum < 6 ) continue;
            
            if( kpmRansacHomograhyEstimation(&preRANSAC, inlierIndex, &inlierNum, h) < 0 ) {
                inlierNum = 0;
            }
            //printf(" --> page[%d] %d  pre:%3d, aft:%3d\n", pageLoop, kpmHandle->inDataSet.num, preRANSAC.num, inlierNum);
            if( inlierNum < 6 ) continue;
            
            kpmHandle->preRANSAC.num = preRANSAC.num;
            kpmHandle->aftRANSAC.num = inlierNum;
            for( i = 0; i < inlierNum; i++ ) {
                kpmHandle->aftRANSAC.match[i].inIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].inIndex;
                kpmHandle->aftRANSAC.match[i].refIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].refIndex;
            }
            //printf(" ---> %d %d %d\n", kpmHandle->inDataSet.num, kpmHandle->preRANSAC.num, kpmHandle->aftRANSAC.num);
            if( kpmHandle->poseMode == KpmPose6DOF ) {
                //printf("----- Page %d ------\n", pageLoop);
                ret = kpmUtilGetPose(kpmHandle->cparamLT, &(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                     kpmHandle->result[pageLoop].camPose,  &(kpmHandle->result[pageLoop].error) );
                ARLOGi("Pose - %s\n",arrayToString2(kpmHandle->result[pageLoop].camPose).c_str());
                //printf("----- End. ------\n");
            }
            else {
                ret = kpmUtilGetPoseHomography(&(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                         kpmHandle->result[pageLoop].camPose,  &(kpmHandle->result[pageLoop].error) );
            }
            if( ret == 0 ) {
                kpmHandle->result[pageLoop].camPoseF = 0;
                kpmHandle->result[pageLoop].inlierNum = inlierNum;
                ARLOGi("Page[%d]  pre:%3d, aft:%3d, error = %f\n", pageLoop, preRANSAC.num, inlierNum, kpmHandle->result[pageLoop].error);
            }
        }
        free(annMatch2);
#else
        for (int pageLoop = 0; pageLoop < kpmHandle->resultNum; pageLoop++) {
            
            kpmHandle->result[pageLoop].pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            kpmHandle->result[pageLoop].camPoseF = -1;
            if( kpmHandle->result[pageLoop].skipF ) continue;
            
            
            const vision::matches_t& matches = kpmHandle->freakMatcher->inliers();
            int matched_image_id = kpmHandle->freakMatcher->matchedId();
            if (matched_image_id < 0) continue;

            ret = kpmUtilGetPose_binary(kpmHandle->cparamLT,
                                        matches ,
                                        kpmHandle->freakMatcher->get3DFeaturePoints(matched_image_id),
                                        kpmHandle->freakMatcher->getQueryFeaturePoints(),
                                        kpmHandle->result[pageLoop].camPose,
                                        &(kpmHandle->result[pageLoop].error) );
            //ARLOGi("Pose (freak) - %s\n",arrayToString2(kpmHandle->result[pageLoop].camPose).c_str());
            if( ret == 0 ) {
                kpmHandle->result[pageLoop].camPoseF = 0;
                kpmHandle->result[pageLoop].inlierNum = (int)matches.size();
                kpmHandle->result[pageLoop].pageNo = kpmHandle->pageIDs[matched_image_id];
                ARLOGi("Page[%d]  pre:%3d, aft:%3d, error = %f\n", pageLoop, (int)matches.size(), (int)matches.size(), kpmHandle->result[pageLoop].error);
            }
        }
#endif
#if !BINARY_FEATURE
        free(featureVector.sf);
        free(preRANSAC.mp);
        free(inlierIndex);
#endif
    }
    else {
#if !BINARY_FEATURE
        kpmHandle->preRANSAC.num = 0;
        kpmHandle->aftRANSAC.num = 0;
#endif
        for( i = 0; i < kpmHandle->resultNum; i++ ) {
            kpmHandle->result[i].camPoseF = -1;
        }
    }
    
    for( i = 0; i < kpmHandle->resultNum; i++ ) kpmHandle->result[i].skipF = 0;

    if (imageLumaWasAllocated) free(imageLuma);
    
    return 0;
}


int kpmUtilGetPose_binary(ARParamLT *cparamLT, const vision::matches_t &matchData, const std::vector<vision::Point3d<float> > &refDataSet, const std::vector<vision::FeaturePoint> &inputDataSet, float camPose[3][4], float *error)
{
    ICPHandleT    *icpHandle;
    ICPDataT       icpData;
    ICP2DCoordT   *sCoord;
    ICP3DCoordT   *wCoord;
    ARdouble       initMatXw2Xc[3][4];
    ARdouble       err;
    int            i;
    
    
    if( matchData.size() < 4 ) return -1;
    
    arMalloc( sCoord, ICP2DCoordT, matchData.size() );
    arMalloc( wCoord, ICP3DCoordT, matchData.size() );
    for( i = 0; i < matchData.size(); i++ ) {
        sCoord[i].x = inputDataSet[matchData[i].ins].x;
        sCoord[i].y = inputDataSet[matchData[i].ins].y;

        wCoord[i].x = refDataSet[matchData[i].ref].x;
        wCoord[i].y = refDataSet[matchData[i].ref].y;
        wCoord[i].z = 0.0;
    }
    
    icpData.num = i;
    icpData.screenCoord = &sCoord[0];
    icpData.worldCoord  = &wCoord[0];
    
    if( icpGetInitXw2Xc_from_PlanarData( cparamLT->param.mat, sCoord, wCoord, (int)matchData.size(), initMatXw2Xc ) < 0 ) {
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


