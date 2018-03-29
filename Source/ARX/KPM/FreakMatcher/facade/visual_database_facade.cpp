//
//  visual_database_facade.cpp
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#include "visual_database_facade.h"
#include <matchers/visual_database-inline.h>
#include <matchers/freak.h>
#include <matchers/keyframe.h>
#include <framework/image.h>
#include <matchers/visual_database-inline.h>

namespace vision {
    typedef VisualDatabase<FREAKExtractor, BinaryFeatureStore, BinaryFeatureMatcher<96> > vdb_t;
    typedef std::vector<vision::Point3d<float> > Point3dVector;
    typedef std::unordered_map<int, Point3dVector> point3d_map_t;
    
    class VisualDatabaseImpl{
    public:
        VisualDatabaseImpl(){
            mVdb.reset(new vdb_t());
        }
        ~VisualDatabaseImpl(){
        }
        
        std::unique_ptr<vdb_t> mVdb;
        point3d_map_t mPoint3d;
    };
    
    VisualDatabaseFacade::VisualDatabaseFacade(){
        mVisualDbImpl.reset(new VisualDatabaseImpl());
    }
    VisualDatabaseFacade::~VisualDatabaseFacade(){
        
    }
    
    void VisualDatabaseFacade::addImage(unsigned char* grayImage,
                                        size_t width,
                                        size_t height,
                                        int image_id) {
        Image img;
        img.deepCopy(Image(grayImage,IMAGE_UINT8,width,height,(int)width,1));
        mVisualDbImpl->mVdb->addImage(img, image_id);
    }
    
    void VisualDatabaseFacade::addFreakFeaturesAndDescriptors(const std::vector<FeaturePoint>& featurePoints,
                                                              const std::vector<unsigned char>& descriptors,
                                                              const std::vector<vision::Point3d<float> >& points3D,
                                                              size_t width,
                                                              size_t height,
                                                              int image_id){
        std::shared_ptr<Keyframe<96> > keyframe(new Keyframe<96>());
        keyframe->setWidth((int)width);
        keyframe->setHeight((int)height);
        keyframe->store().setNumBytesPerFeature(96);
        keyframe->store().points().resize(featurePoints.size());
        keyframe->store().points() = featurePoints;
        keyframe->store().features().resize(descriptors.size());
        keyframe->store().features() = descriptors;
        keyframe->buildIndex();
        mVisualDbImpl->mVdb->addKeyframe(keyframe, image_id);
        mVisualDbImpl->mPoint3d[image_id] = points3D;
    }
    
    void VisualDatabaseFacade::computeFreakFeaturesAndDescriptors(unsigned char* grayImage,
                                                                  size_t width,
                                                                  size_t height,
                                                                  std::vector<FeaturePoint>& featurePoints,
                                                                  std::vector<unsigned char>& descriptors){
        Image img = Image(grayImage,IMAGE_UINT8,width,height,(int)width,1);
        std::unique_ptr<vdb_t> tmpDb(new vdb_t());
        tmpDb->addImage(img, 1);
        featurePoints = tmpDb->keyframe(1)->store().points();
        descriptors = tmpDb->keyframe(1)->store().features();
    }
    
    bool VisualDatabaseFacade::query(unsigned char* grayImage,
                                     size_t width,
                                     size_t height){
        Image img = Image(grayImage,IMAGE_UINT8,width,height,(int)width,1);
        return mVisualDbImpl->mVdb->query(img);
    }
    
    bool VisualDatabaseFacade::erase(int image_id){
        return mVisualDbImpl->mVdb->erase(image_id);
    }
    
    const size_t VisualDatabaseFacade::databaseCount(){
        return mVisualDbImpl->mVdb->databaseCount();
    }
    
    int VisualDatabaseFacade::matchedId(){
        return mVisualDbImpl->mVdb->matchedId();
    }
    
    const float* VisualDatabaseFacade::matchedGeometry(){
        return mVisualDbImpl->mVdb->matchedGeometry();
    }
    
    const std::vector<FeaturePoint> &VisualDatabaseFacade::getFeaturePoints(int image_id) const{
        return mVisualDbImpl->mVdb->keyframe(image_id)->store().points();
    }
    
    const std::vector<unsigned char> &VisualDatabaseFacade::getDescriptors(int image_id) const{
        return mVisualDbImpl->mVdb->keyframe(image_id)->store().features();
    }
    
    const std::vector<vision::Point3d<float> >& VisualDatabaseFacade::get3DFeaturePoints(int image_id) const{
        return mVisualDbImpl->mPoint3d[image_id];
    }
    
    const std::vector<FeaturePoint>&VisualDatabaseFacade::getQueryFeaturePoints() const{
        return mVisualDbImpl->mVdb->queryKeyframe()->store().points();
    }
    
    const std::vector<unsigned char>&VisualDatabaseFacade::getQueryDescriptors() const{
        return mVisualDbImpl->mVdb->queryKeyframe()->store().features();
    }
    
    const matches_t& VisualDatabaseFacade::inliers() const{
        return mVisualDbImpl->mVdb->inliers();
    }
    
    int VisualDatabaseFacade::getWidth(int image_id) const{
        return mVisualDbImpl->mVdb->keyframe(image_id)->width();
    }
    int VisualDatabaseFacade::getHeight(int image_id) const{
        return mVisualDbImpl->mVdb->keyframe(image_id)->height();
    }
} // vision