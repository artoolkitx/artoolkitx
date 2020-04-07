/*
 *  arosg.cpp
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
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2009-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "arosg.h"
#include <stdio.h>
#include <string.h>
#if ARX_TARGET_PLATFORM_IOS
#  include "osgPlugins.h"
#endif
#include "shaders.h"
#include <osg/Config>
#include <osg/Node>
#include <osg/FrontFace>
#include <osg/MatrixTransform>
#include <osg/LightModel>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgFX/Outline>
#include <osg/AnimationPath>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osgDB/FileNameUtils>
#include <osgUtil/Optimizer>


#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif

//#define DEBUG_AROSG_MODELLOADING // Uncomment to use color cubes rather than loading models.

struct _AROSG {
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::observer_ptr<osgViewer::GraphicsWindow> window;
    osg::ref_ptr<osg::Group> sg;
    int prevIndex;
    osg::ref_ptr<osg::MatrixTransform> models[AR_OSG_MODELS_MAX];
    int frontFaceWinding;
    double time;
#if !defined(OSG_GL_FIXED_FUNCTION_AVAILABLE)
    // global programs
    osg::ref_ptr<osg::Program> _vertColorProgram;
    osg::ref_ptr<osg::Program> _textureProgram;
#endif
};

static const std::string imageExtensions[] = {
    "jpeg", "jpg", "jpe", "tiff", "tif", "gif", "png", "psd", "tga",
    "bmp", "rgb", "rgba", "sgi", "int", "inta", "bw",
    "pnm", "ppm", "pgm", "pbm", "pic", "hdr", "dds",
    "mov", "avi", "mpeg", "mpg", "mpe", "mpv", "mp4", "m4v", "dv", "flv", "m2v", "m1v",
    "" // List must end with empty string.
};

//
// Shape drawables sometimes use gl_quads so use this function to convert
//
void optimizeNode(osg::Node* node) {
    osgUtil::Optimizer optimizer;
    optimizer.optimize(node, osgUtil::Optimizer::TRISTRIP_GEOMETRY);
}


class ManageAnimationNodesVisitor : public osg::NodeVisitor
{
public:
    
    ManageAnimationNodesVisitor(): osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }
    
    virtual ~ManageAnimationNodesVisitor()
    {
    }
    
    virtual void apply(osg::Transform& transform)
    {
        osg::AnimationPathCallback* apc = dynamic_cast<osg::AnimationPathCallback*>(transform.getUpdateCallback());
        if (apc) {
            _selectedCallbacks.push_back(apc);
        }
        
        traverse(transform); // Can have children, so need to traverse always.
    }
    
    void setLoopMode(osg::AnimationPath::LoopMode mode)
    {
        for (SelectedCallbacks::iterator itr = _selectedCallbacks.begin(); itr != _selectedCallbacks.end(); ++itr) {
            osg::AnimationPathCallback* apc = itr->get();
            osg::AnimationPath* ap = apc->getAnimationPath();
            if (ap) ap->setLoopMode(mode);
        }
    }
    
    void setPause(bool animationPaused, double simulationTime)
    {
        for (SelectedCallbacks::iterator itr = _selectedCallbacks.begin(); itr != _selectedCallbacks.end(); ++itr) {
            osg::AnimationPathCallback* apc = itr->get();
            apc->_latestTime = simulationTime;
            apc->setPause(animationPaused);
        }
    }
    
    double getAnimationTime(void)
    {
        if (_selectedCallbacks.size() < 1) return (0.0);
        return (_selectedCallbacks[0]->getAnimationTime()); // Hardcoded [0] = gets the time on only the first callback found.
    }
    
    typedef std::vector< osg::ref_ptr<osg::AnimationPathCallback> > SelectedCallbacks;
    SelectedCallbacks _selectedCallbacks;
    
}; // class ManageAnimationNodesVisitor.

// Reset is handled separately, as we actually need to have access to the node.
class ResetAnimationNodesVisitor : public osg::NodeVisitor
{
public:
    
    ResetAnimationNodesVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }
    
    virtual ~ResetAnimationNodesVisitor()
    {
    }
    
    
    virtual void apply(osg::Transform& transform)
    {
        osg::AnimationPathCallback* apc = dynamic_cast<osg::AnimationPathCallback*>(transform.getUpdateCallback());
        if (apc) {
            // Here we encounter a bug in OSG. If the animation path callback is paused, reset() doesn't actually reset the
            // animation itself, but resets only the animation time. The animation is only updated when the animation
            // is unpaused (and therefore update()s on the animation path callback are processed again).
            // To workaround this, we do a rather complicated sequence if it's paused. including an update().
            if (apc->getPause()) {
                apc->setPause(false); // Moves _firstTime forward by the amount of time since pause started.
                double animationTime = apc->getAnimationTime(); // (_latestTime-_firstTime)-_timeOffset
                apc->setTimeOffset(animationTime); // Now animationTime will calculate as 0.0, and pause is off, so update() will actually do something.
                apc->update(transform);
                apc->setTimeOffset(0.0); // Get rid if the offset we used temporarily.
                apc->reset(); // Reset the other times.
                apc->setPause(true);
            } else {
                apc->reset();
                apc->update(transform);
            }
        }
        
        traverse(transform); // Can have children, so need to traverse always.
    }
};

osg::Geometry* myCreateTexturedQuadGeometry(const osg::Vec3& pos,float width,float height, osg::Image* image, bool useTextureRectangle, bool xyPlane, bool option_flip);
osg::Geometry* myCreateTexturedQuadGeometry(const osg::Vec3& pos,float width,float height, osg::Image* image, bool useTextureRectangle, bool xyPlane, bool option_flip)
{
    bool flip = image->getOrigin()==osg::Image::TOP_LEFT;
    if (option_flip) flip = !flip;
    
    if (useTextureRectangle)
    {
        osg::Geometry* pictureQuad = osg::createTexturedQuadGeometry(pos,
                                                                     osg::Vec3(width,0.0f,0.0f), // widthVec
                                                                     xyPlane ? osg::Vec3(0.0f,height,0.0f) : osg::Vec3(0.0f,0.0f,height), // heightVec
                                                                     0.0f, flip ? image->t() : 0.0, image->s(), flip ? 0.0 : image->t()); // left, bottom, right, top
        
        osg::TextureRectangle* texture = new osg::TextureRectangle(image);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        
        
        pictureQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                                                                        texture,
                                                                        osg::StateAttribute::ON);
        
        return pictureQuad;
    }
    else
    {
        osg::Geometry* pictureQuad = osg::createTexturedQuadGeometry(pos,
                                                                     osg::Vec3(width,0.0f,0.0f),
                                                                     xyPlane ? osg::Vec3(0.0f,height,0.0f) : osg::Vec3(0.0f,0.0f,height),
                                                                     0.0f, flip ? 1.0f : 0.0f , 1.0f, flip ? 0.0f : 1.0f);
        
        osg::Texture2D* texture = new osg::Texture2D(image);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        
        
        pictureQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                                                                        texture,
                                                                        osg::StateAttribute::ON);
        
        return pictureQuad;
    }
}

extern "C" {
    
    
    unsigned int arOSGGetVersion()
    {
        return (0x10000000u * ((unsigned int)AR_HEADER_VERSION_MAJOR / 10u) +
                0x01000000u * ((unsigned int)AR_HEADER_VERSION_MAJOR % 10u) +
                0x00100000u * ((unsigned int)AR_HEADER_VERSION_MINOR / 10u) +
                0x00010000u * ((unsigned int)AR_HEADER_VERSION_MINOR % 10u) +
                0x00001000u * ((unsigned int)AR_HEADER_VERSION_TINY / 10u) +
                0x00000100u * ((unsigned int)AR_HEADER_VERSION_TINY % 10u) +
                0x00000010u * ((unsigned int)AR_HEADER_VERSION_DEV / 10u) +
                0x00000001u * ((unsigned int)AR_HEADER_VERSION_DEV % 10u)
                );        
    }

    ARG_API arOSGGetPreferredAPI()
    {
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
        return (ARG_API_GLES2);
#elif defined(OSG_GL3_AVAILABLE)
        return (ARG_API_GL3);
#elif defined(OSG_GL1_AVAILABLE) || defined(OSG_GL2_AVAILABLE)
        return (ARG_API_GL);
#else
        return (ARG_API_None);
#endif
    }
    
    AROSG *arOSGInit()
    {
        AROSG *arOsg;
        
        arOsg = (AROSG *)calloc(1, sizeof(AROSG));
        if (!arOsg) return (nullptr);
        
        arOsg->sg = new osg::Group;
        arOsg->prevIndex = AR_OSG_MODELS_MAX - 1;
        arOsg->time = USE_REFERENCE_TIME;

        // create our default programs
#if !defined(OSG_GL_FIXED_FUNCTION_AVAILABLE)
        arOsg->_vertColorProgram = new osg::Program();
        arOsg->_vertColorProgram->addShader( new osg::Shader(osg::Shader::VERTEX, ColorShaderVert));
        arOsg->_vertColorProgram->addShader( new osg::Shader(osg::Shader::FRAGMENT, ColorShaderFrag));
        
        arOsg->_textureProgram = new osg::Program();
        arOsg->_textureProgram->addShader( new osg::Shader(osg::Shader::VERTEX, TextureShaderVert));
        arOsg->_textureProgram->addShader( new osg::Shader(osg::Shader::FRAGMENT, TextureShaderFrag));
#endif

        return (arOsg);
    }
    
    static char *get_buff(char *buf, int n, FILE *fp, int skipblanks)
    {
        char *ret;
        
        do {
            ret = fgets(buf, n, fp);
            if (ret == NULL) return (NULL); // EOF or error.
            
            // Remove NLs and CRs from end of string.
            size_t l = strlen(buf);
            while (l > 0) {
                if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
                l--;
                buf[l] = '\0';
            }
        } while (buf[0] == '#' || (skipblanks && buf[0] == '\0')); // Reject comments and blank lines.
        
        return (ret);
    }
    
    
    int arOSGLoadModel2(AROSG *arOsg, const char *modelFilePath, const ARdouble translation[3], const ARdouble rotation[4], const ARdouble scale[3], const int textures)
    {
        if (!arOsg) return (-1);
        
        // Find a free slot in the models array.
        int index = arOsg->prevIndex;
        do {
            index++;
            if (index >= AR_OSG_MODELS_MAX) index = 0;
        } while (index != arOsg->prevIndex && arOsg->models[index] != nullptr);
        if (index == arOsg->prevIndex) {
            ARLOGe("Error: Unable to load model, maximum number of models (%d) already loaded.\n", AR_OSG_MODELS_MAX);
            return (-1);
        };
        
#ifndef DEBUG_AROSG_MODELLOADING
        osg::ref_ptr<osg::Node> model = nullptr;

        // Ask OSG to load the model.
        // Check if object file refers to an image.
        std::string ext = osgDB::getLowerCaseFileExtension(modelFilePath);
        int i = 0;
        while (imageExtensions[i] != ext && imageExtensions[i] != "") i++;
        if (imageExtensions[i] != "") {
            // Attempt to load as image.
            //ARLOGd("Reading image file '%s'.\n", modelFilePath);
            osg::Image *image = osgDB::readImageFile(modelFilePath);
            if (!image) {
                ARLOGe("Unable to read model image file '%s'.\n", modelFilePath);
                return (-1);
            } else {
                osg::ref_ptr<osg::Geode> geode = new osg::Geode();
                osg::Vec3 pos(0.0f, 0.0f, 0.0f);
                float scalef = 80.0f / (image->s() > image->t() ? image->s() : image->t());
                geode->addDrawable(myCreateTexturedQuadGeometry(pos, image->s() * scalef, image->t() * scalef, image, false, true, false));
                model = geode.get();
            }
        } else {
            // Attempt to load as node.
            //ARLOGd("Reading node file '%s'.\n", modelFilePath);
            model = osgDB::readNodeFile(modelFilePath);
            if (!model.valid()) {
                ARLOGe("Unable to read model node file '%s'.\n", modelFilePath);
                return (-1);
            }
            // attach shader program if needed
#  if !defined(OSG_GL_FIXED_FUNCTION_AVAILABLE)
            if (!textures) {
                model->getOrCreateStateSet()->setAttributeAndModes(arOsg->_vertColorProgram, osg::StateAttribute::ON);
            } else {
                model->getOrCreateStateSet()->setAttributeAndModes(arOsg->_textureProgram, osg::StateAttribute::ON);
            }
            optimizeNode(model);
#  endif
        }
#else
        // For debugging purposes: colour cube.
        int i;
        const osg::Vec3f cube_vertices[8] = {
            /* +z */ osg::Vec3f(0.5f, 0.5f, 0.5f), osg::Vec3f(0.5f, -0.5f, 0.5f), osg::Vec3f(-0.5f, -0.5f, 0.5f), osg::Vec3f(-0.5f, 0.5f, 0.5f),
            /* -z */ osg::Vec3f(0.5f, 0.5f, -0.5f), osg::Vec3f(0.5f, -0.5f, -0.5f), osg::Vec3f(-0.5f, -0.5f, -0.5f), osg::Vec3f(-0.5f, 0.5f, -0.5f) };
        const osg::Vec4f cube_vertex_colors[8] = {
            osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f), osg::Vec4f(1.0f, 1.0f, 0.0f, 1.0f), osg::Vec4f(0.0f, 1.0f, 0.0f, 1.0f), osg::Vec4f(0.0f, 1.0f, 1.0f, 1.0f),
            osg::Vec4f(1.0f, 0.0f, 1.0f, 1.0f), osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f), osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f), osg::Vec4f(0.0f, 0.0f, 1.0f, 1.0f) };
        const GLubyte cube_faces[6][4] = { /* ccw-winding */
            /* +z */ {3, 2, 1, 0}, /* -y */ {2, 3, 7, 6}, /* +y */ {0, 1, 5, 4},
            /* -x */ {3, 0, 4, 7}, /* +x */ {1, 2, 6, 5}, /* -z */ {4, 5, 6, 7} };
        osg::ref_ptr<osg::Geode> model = new osg::Geode();
        model->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        //model->getOrCreateStateSet()->setTextureMode(GL_TEXTURE_2D, osg::StateAttribute::OFF);
        osg::Geometry* facesGeom = new osg::Geometry();
        facesGeom->setVertexArray(new osg::Vec3Array(8, (osg::Vec3 *)cube_vertices));
        facesGeom->setColorArray(new osg::Vec4Array(8, (osg::Vec4 *)cube_vertex_colors));
        facesGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        for (i = 0; i < 6; i++) {
            facesGeom->addPrimitiveSet(new osg::DrawElementsUByte(osg::PrimitiveSet::TRIANGLE_FAN, 4, &(cube_faces[i][0])));
        }
        model->addDrawable(facesGeom);
        osg::Geometry* edgesGeom = new osg::Geometry();
        edgesGeom->setVertexArray(new osg::Vec3Array(8, (osg::Vec3 *)cube_vertices));
        edgesGeom->setColorArray(new osg::Vec4Array(1, (osg::Vec4 *)cube_vertex_colors));
        edgesGeom->setColorBinding(osg::Geometry::BIND_OVERALL);
        for (i = 0; i < 6; i++) {
            edgesGeom->addPrimitiveSet(new osg::DrawElementsUByte(osg::PrimitiveSet::LINE_LOOP, 4, &(cube_faces[i][0])));
        }
        model->addDrawable(edgesGeom);
#  if !defined(OSG_GL_FIXED_FUNCTION_AVAILABLE)
        model->getOrCreateStateSet()->setAttributeAndModes(arOsg->_vertColorProgram, osg::StateAttribute::ON);
        optimizeNode(drawable);
#  endif

#endif       
        
        //osg::BoundingSphere bs =  model->getBound();
        //ARLOGe("Model: radius %f, center (%f,%f,%f).\n", bs.radius(), bs.center().x(), bs.center().y(), bs.center().z());
        
        // Add the transform that will be used for the live pose (i.e. the modelview).
        arOsg->models[index] = new osg::MatrixTransform; // Save the pointer.
        arOsg->sg->addChild(arOsg->models[index].get());
        
        // Add the transform that will be used for the local pose.
        osg::ref_ptr<osg::MatrixTransform> modelTransform = new osg::MatrixTransform; // Save the pointer.
        arOsg->models[index]->addChild(modelTransform.get());
        modelTransform->setMatrix(osg::Matrix());
        
        // Add the transform for the model static configuration.
        osg::ref_ptr<osg::MatrixTransform> configTransform = new osg::MatrixTransform;
        modelTransform->addChild(configTransform.get());
        configTransform->setMatrix(osg::Matrix());
#ifndef DEBUG_AROSG_MODELLOADING
        if (translation) configTransform->preMult(osg::Matrix::translate(translation[0], translation[1], translation[2]));
        if (rotation) configTransform->preMult(osg::Matrix::rotate(osg::inDegrees(rotation[0]), rotation[1], rotation[2], rotation[3]));        
        if (scale) {
            configTransform->preMult(osg::Matrix::scale(scale[0], scale[1], scale[2]));
            configTransform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON); // Rescale normals.
        }
#else
        configTransform->preMult(osg::Matrix::translate(0.0f, 0.0f, 20.0f));
        configTransform->preMult(osg::Matrix::scale(40.0f, 40.0f, 40.0f));
        configTransform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON); // Rescale normals.
#endif
        
        // Add the model.
        configTransform->addChild(model.get());
        
        arOsg->prevIndex = index; // Save for next time.
        return (index);
    }
    
    int arOSGLoadModel(AROSG *arOsg, const char *modelDescriptionFilePath)
    {
        FILE             *fp;
        char             buf[MAXPATHLEN], modelFilePath[MAXPATHLEN];
        ARdouble         translation[3], rotation[4], scale[3];
                
        // Read in the .dat file and get model filename, pose and scale.
        if ((fp = fopen(modelDescriptionFilePath, "r")) == nullptr) {
            ARLOGe("Error: unable to open model description file '%s'.\n", modelDescriptionFilePath);
            return (-1);
        }
        
        // Read filename.
        if (!get_buff(buf, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error: unable to read model file name from model description file '%s'.\n", modelDescriptionFilePath);
            fclose(fp); return (-1);
        }
        if (!arUtilGetDirectoryNameFromPath(modelFilePath, modelDescriptionFilePath, sizeof(modelFilePath), 1)) { // Get directory prefix.
            fclose(fp); return (-1);
        } 
        strncat(modelFilePath, buf, sizeof(modelFilePath) - strlen(modelFilePath) - 1); // Add name of file to open.
        
        // Read translation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &translation[0], &translation[1], &translation[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &translation[0], &translation[1], &translation[2]) != 3)
#endif
        {
            fclose(fp); return (-1);
        }
        // Read rotation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f %f", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#else
        if (sscanf(buf, "%lf %lf %lf %lf", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#endif
        {
            fclose(fp); return (-1);
        }
        // Read scale.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &scale[0], &scale[1], &scale[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &scale[0], &scale[1], &scale[2]) != 3)
#endif
        {
            fclose(fp); return (-1);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        int lightingFlag = 1, transparencyFlag = -1, texturesFlag = 0;
        
        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "LIGHTING", 8) == 0) {
                if (sscanf(&(buf[8]), " %d", &lightingFlag) != 1) {
                    ARLOGe("Error in model description file: LIGHTING token must be followed by an integer >= 0. Discarding.\n");
                }
            } else if (strncmp(buf, "TRANSPARENT", 11) == 0) {
                if (sscanf(&(buf[11]), " %d", &transparencyFlag) != 1) {
                    ARLOGe("Error in model description file: TRANSPARENT token must be followed by an integer >= 0. Discarding.\n");
                }
            } else if (strncmp(buf, "TEXTURES", 8) == 0) {
                if (sscanf(&(buf[8]), " %d", &texturesFlag) != 1) {
                    ARLOGe("Error in model description file: TEXTURES token must be followed by an integer >= 0. Discarding.\n");
                }
            }
            // Unknown tokens are ignored.
        }
        fclose(fp);

        int index = arOSGLoadModel2(arOsg, modelFilePath, translation, rotation, scale, texturesFlag);
        
        if (index >= 0) {
            if (!lightingFlag) arOSGSetModelLighting(arOsg, index, 0);
            if (transparencyFlag != -1) arOSGSetModelTransparency(arOsg, index, transparencyFlag);
        }
        
        return (index);
    }
    
    int arOSGUnloadModel(AROSG *arOsg, const int index)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to unload a model.\n");
            return (-1);
        }
        
        arOsg->sg->removeChild(arOsg->models[index].get());
        arOsg->models[index] = nullptr; // This will delete the model, through the ref_ptr template's methods.
        return (0);
    }

    int arOSGSetModelVisibility(AROSG *arOsg, const int index, const int visible)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model visibility.\n");
            return (-1);
        }
        
        arOsg->models[index]->setNodeMask(visible ? 0xffffffff : 0x0);

        return (0);
    }
    
    int arOSGGetModelVisibility(AROSG *arOsg, const int index, int *visible)
    {
        if (!arOsg || !visible) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model visibility.\n");
            return (-1);
        }
        
        *visible = (arOsg->models[index]->getNodeMask() != 0x0);
        return (0);
    }
    
    int arOSGSetModelLighting(AROSG *arOsg, const int index, const int lit)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model lighting.\n");
            return (-1);
        }
        
        arOsg->models[index]->getOrCreateStateSet()->setMode(GL_LIGHTING, (lit ? osg::StateAttribute::ON : osg::StateAttribute::OFF));
        return (0);
    }
    
    int arOSGGetModelLighting(AROSG *arOsg, const int index, int *lit)
    {
        if (!arOsg || !lit) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model lighting.\n");
            return (-1);
        }
        
        *lit = (arOsg->models[index]->getOrCreateStateSet()->getMode(GL_LIGHTING) != osg::StateAttribute::OFF);
        return (0);
    }
    
    int arOSGSetModelTransparency(AROSG *arOsg, const int index, const int transparent)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model transparency.\n");
            return (-1);
        }
        
        osg::StateSet *ss = arOsg->models[index]->getOrCreateStateSet();
        //osg::ref_ptr<osg::BlendFunc> texture_blending_function = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
        //ss->setAttributeAndModes(texture_blending_function.get(), osg::StateAttribute::ON);
        ss->setMode(GL_BLEND, (transparent ? osg::StateAttribute::ON : osg::StateAttribute::OFF));
        ss->setRenderingHint(transparent ? osg::StateSet::TRANSPARENT_BIN: osg::StateSet::OPAQUE_BIN);
        return (0);
    }
    
    int arOSGSetModelAnimationPause(AROSG *arOsg, const int index, const int pause)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model animation pause.\n");
            return (-1);
        }
        
        ManageAnimationNodesVisitor manv;
        arOsg->models[index]->accept(manv);
        manv.setPause((pause ? true : false), arOsg->viewer->getFrameStamp()->getSimulationTime());

        return (0);
    }
    
    int arOSGGetModelAnimationTime(AROSG *arOsg, const int index, double *animationTime)
    {
        if (!arOsg || !animationTime) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model animation time.\n");
            return (-1);
        }
        
        ManageAnimationNodesVisitor manv;
        arOsg->models[index]->accept(manv);
        *animationTime = manv.getAnimationTime();
        
        return (0);
    }
    
    int arOSGSetModelAnimationReset(AROSG *arOsg, const int index)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to reset model animation.\n");
            return (-1);
        }
        
        ResetAnimationNodesVisitor ranv;
        arOsg->models[index]->accept(ranv);
        
        return (0);
    }
    
    int arOSGSetModelAnimationLoopModeOverride(AROSG *arOsg, const int index, const int mode)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model animation loop mode override.\n");
            return (-1);
        }

        ManageAnimationNodesVisitor manv;
        arOsg->models[index]->accept(manv);
        if (mode == 0) manv.setLoopMode(osg::AnimationPath::NO_LOOPING);
        else if (mode == 1) manv.setLoopMode(osg::AnimationPath::LOOP);
        else if (mode == 2) manv.setLoopMode(osg::AnimationPath::SWING);
        
        return (0);
    }
    
    int arOSGSetProjection(AROSG *arOsg, ARdouble p[16])
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        arOsg->viewer->getCamera()->setProjectionMatrix(osg::Matrix(p));

        return (0);
    }

#ifndef ARDOUBLE_IS_FLOAT
    int arOSGSetProjectionf(AROSG *arOsg, float p[16])
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        arOsg->viewer->getCamera()->setProjectionMatrix(osg::Matrixf(p));
        return (0);
    }
#endif // !ARDOUBLE_IS_FLOAT

    int arOSGGetProjection(AROSG *arOsg, ARdouble *p)
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        double *mtx = arOsg->viewer->getCamera()->getProjectionMatrix().ptr();
        for (int i = 0; i < 16; i++) p[i] = (ARdouble)(mtx[i]);
        return (0);
    }
    
    int arOSGSetView(AROSG *arOsg, ARdouble v[16])
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        arOsg->viewer->getCamera()->setViewMatrix(osg::Matrix(v));
        return (0);
    }

#ifndef ARDOUBLE_IS_FLOAT
    int arOSGSetViewf(AROSG *arOsg, float v[16])
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        arOsg->viewer->getCamera()->setViewMatrix(osg::Matrixf(v));
        return (0);
    }
#endif // !ARDOUBLE_IS_FLOAT

    int arOSGGetView(AROSG *arOsg, ARdouble *v)
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);
        
        double *mtx = arOsg->viewer->getCamera()->getViewMatrix().ptr();
        for (int i = 0; i < 16; i++) v[i] = (ARdouble)(mtx[i]);
        return (0);
    }

    int arOSGSetFrontFace(AROSG *arOsg, int winding)
    {
        if (!arOsg) return (-1);
        
        arOsg->frontFaceWinding = winding;
        if (arOsg->viewer.valid()) {
            osg::ref_ptr<osg::FrontFace> ff = new osg::FrontFace(arOsg->frontFaceWinding ? osg::FrontFace::CLOCKWISE : osg::FrontFace::COUNTER_CLOCKWISE);
            arOsg->viewer->getCamera()->getOrCreateStateSet()->setAttributeAndModes(ff, osg::StateAttribute::ON);
        }
		return (0);
    }
    
    int arOSGGetFrontFace(AROSG *arOsg, int *winding)
    {
        if (!arOsg || !winding) return (-1);
        
        *winding = arOsg->frontFaceWinding;
		return (0);
    }
    
    int arOSGSetModelPose(AROSG *arOsg, const int index, const ARdouble modelview[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }

        arOsg->models[index]->setMatrix(osg::Matrix(modelview));
        return (0);
    }

#ifndef ARDOUBLE_IS_FLOAT
    int arOSGSetModelPosef(AROSG *arOsg, const int index, const float modelview[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }
        
        arOsg->models[index]->setMatrix(osg::Matrixf(modelview));
        return (0);
    }
#endif // !ARDOUBLE_IS_FLOAT

    int arOSGGetModelPose(AROSG *arOsg, const int index, ARdouble *modelview)
    {
        if (!arOsg || !modelview) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = arOsg->models[index]->getMatrix().ptr();
#else
        const double *mtx = arOsg->models[index]->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) modelview[i] = (ARdouble)(mtx[i]);
        return (0);
    }
    
    int arOSGSetModelLocalPose(AROSG *arOsg, const int index, const ARdouble model[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }
        
        static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->setMatrix(osg::Matrix(model));
        return (0);
    }
    
    int arOSGSetModelOutline(AROSG *arOsg, const int index, const int width, const unsigned char rgba[4])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model outline.\n");
            return (-1);
        }

        osg::Group *configTransform = arOsg->models[index]->getChild(0)->asGroup()->getChild(0)->asGroup(); // arOsg->models[index] is transform, first child is localTransform, second child is configTransform.
        osgFX::Outline *modelAsOutline = dynamic_cast<osgFX::Outline*>(configTransform->getChild(0)); // Is an outline already in place?
        if (modelAsOutline) {
            if (width) {
                // Just change the current params.
                modelAsOutline->setWidth(width);
                modelAsOutline->setColor(osg::Vec4f(rgba[0],rgba[1],rgba[2],rgba[3]));
            } else {
                // Width = 0, remove the current outline.
                configTransform->addChild(modelAsOutline->getChild(0)); // Add the model to the outline's parent.
                modelAsOutline->removeChild(0, 1); // and remove it from the outline.
                configTransform->removeChild(modelAsOutline); // This will unref() modelAsOutline.
            }
        } else {
            if (width) {
                osg::ref_ptr<osgFX::Outline> outline = new osgFX::Outline;
                outline->setWidth(width);
                outline->setColor(osg::Vec4f(rgba[0],rgba[1],rgba[2],rgba[3]));
                outline->addChild(configTransform->getChild(0));
                configTransform->removeChild(0, 1);
                configTransform->addChild(outline);
            } else {
                // Do nothing.
            }
        }
        return (0);
    }
    
    int arOSGGetModelLocalPose(AROSG *arOsg, const int index, ARdouble *model)
    {
        if (!arOsg || !model) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#else
        const double *mtx = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) model[i] = (ARdouble)(mtx[i]);
        return (0);
    }
    
    int arOSGGetModelIntersection(AROSG *arOsg, const int index, const ARdouble p1[3], const ARdouble p2[3])
    {
        if (!arOsg || !p1 || !p2) return (-1);
        if (index < 0 || index >= AR_OSG_MODELS_MAX) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model intersection.\n");
            return (-1);
        }
        
        osgUtil::LineSegmentIntersector::Intersections intersections;        
        osg::ref_ptr< osgUtil::LineSegmentIntersector > intersector = new osgUtil::LineSegmentIntersector(osg::Vec3d(p1[0], p1[1], p1[2]), osg::Vec3d(p2[0], p2[1], p2[2]));
        osgUtil::IntersectionVisitor iv(intersector.get());
        arOsg->models[index]->accept(iv);
        if (intersector->containsIntersections()) {
            /*
             @param      nodeType If supplied, on return the location pointed to will be filled with a pointer to a C string
             containing the name of the node type, e.g. "Node" or "Geode", or filled with nullptr if this could not be determined.
             If this information is not required, pass nullptr for this parameter.
             @param      nodeName If supplied, on return the location pointed to will be filled with a pointer to a C string
             containing the user-defined name of the actual node, provided one was defined, e.g. "MyCoolNode",
             or filled with nullptr if no name had been assigned.
             If this information is not required, pass nullptr for this parameter.
             @param      intersectionCoords If supplied, should point to an array of 3 ARdoubles. On return the location pointed
             to will be filled with a the coordinates of the interection in world coordinate space.
             or filled with nullptr if no name had been assigned.
             If this information is not required, pass nullptr for this parameter.             
             */
            /*char **nodeType, char **nodeName, ARdouble *intersectionCoords
            intersections = intersector->getIntersections();
            for (osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin(); hitr != intersections.end(); ++hitr) {
                if (nodeType) {
                    if (hitr->drawable.valid()) {
                        *nodeType = strdup(hitr->drawable->className());
                    } else *nodeType = nullptr;
                }
                if (nodeName) {
                    if (!hitr->nodePath.empty() && !(hitr->nodePath.back()->getName().empty())) {
                        *nodeType = strdup(hitr->nodePath.back()->getName().c_str());
                    } else *nodeName = nullptr;
                }
                if (intersectionCoords) {
                    double *ic = hitr->getWorldIntersectPoint().ptr();
                    intersectionCoords[0] = ic[0]; intersectionCoords[1] = ic[1]; intersectionCoords[2] = ic[2];
                }
            }*/
            return (1);
        } else return (0);
    }
    
    const double AR_OSG_TIME_USE_REFERENCE_TIME = USE_REFERENCE_TIME;
    
    int arOSGSetDrawTime(AROSG *arOsg, double time)
    {
        if (!arOsg) return (-1);
        arOsg->time = time;
        return (0);
    }
    
    int arOSGDraw(AROSG *arOsg)
    {
        if (!arOsg) return (-1);
        if (arOsg->viewer.valid()) {
            //double referenceTime = arOsg->viewer->getViewerFrameStamp()->getReferenceTime();
            arOsg->viewer->frame(arOsg->time); // This call clobbers the OpenGL viewport.
        }
        return (0);
    }

    void arOSGHandleReshape(AROSG *arOsg, const int w, const int h)
    {
        arOSGHandleReshape2(arOsg, 0, 0, w, h);
    }
    
    void arOSGHandleReshape2(AROSG *arOsg, const int left, const int bottom, const int width, const int height)
    {
        if (!arOsg) return;
        
        // Update the window dimensions, in case the window has been resized.
        if (arOsg->window.valid()) {
            arOsg->window->resized(0, 0, width, height); // Sets viewport.
            arOsg->viewer->getCamera()->setViewport(new osg::Viewport(left, bottom, width, height)); // To allow negative values for left / bottom, need to manually set the viewport.
            arOsg->window->getEventQueue()->windowResize(0, 0, width, height); // Initiates GUIEventAdapter::RESIZE.
        } else {
            // Set up viewer.
            // By using default Viewer constructor, we get a detached viewer. Its osg::View sets up a
            // default perspective projection, a headlight, dark blue clear color, and a default camera
            // with global default stateset. Then its osgViewer::View sets up a renderer containing two sceneviews.
            arOsg->viewer = new osgViewer::Viewer;
            // setUpAsEmbeddedInWindow instantiates an osg::GraphicsContext and an osgGA::GUIActionAdapter
            // in an osgViewer::GraphicsWindow, and then adds no-op methods for the window-management related functions.
            arOsg->window = arOsg->viewer->setUpViewerAsEmbeddedInWindow(0, 0, width, height); // Sets viewport, and Projection to perspective with 30 degree FOV.
            // Override a bunch of defaults.
            arOsg->viewer->getCamera()->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
            arOsg->viewer->getCamera()->setProjectionResizePolicy(osg::Camera::FIXED);
            arOsg->viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR); // Don't let OSG choose the clipping values.
            arOsg->viewer->getCamera()->setClearMask(0L); // Don't let OSG clear any buffers.
            arOsg->viewer->getCamera()->setViewport(new osg::Viewport(left, bottom, width, height)); // To allow negative values for left / bottom, need to manually set the viewport.
            arOsg->viewer->getCamera()->setProjectionMatrix(osg::Matrix::identity());
            arOsg->viewer->getCamera()->setViewMatrix(osg::Matrix::identity());
            
            // Set lighting model.
#if defined(OSG_GL_FIXED_FUNCTION_AVAILABLE)
            osg::ref_ptr<osg::LightModel> lm = new osg::LightModel();
            lm->setAmbientIntensity(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
            //lm->setTwoSided(true);
            arOsg->viewer->getCamera()->getOrCreateStateSet()->setAttributeAndModes(lm, osg::StateAttribute::ON);
#endif
            // Make sure glFrontFace is set when the viewer is created.
            osg::ref_ptr<osg::FrontFace> ff = new osg::FrontFace(arOsg->frontFaceWinding ? osg::FrontFace::CLOCKWISE : osg::FrontFace::COUNTER_CLOCKWISE);
            arOsg->viewer->getCamera()->getOrCreateStateSet()->setAttributeAndModes(ff, osg::StateAttribute::ON);

            arOsg->viewer->setSceneData(arOsg->sg.get());
            arOsg->viewer->realize(); // Is a no-op when using embeded viewer.
        }
    }
    
    void arOSGHandleMouseDownUp(AROSG *arOsg, const int button, const int state, const int x, const int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            if (state == 0) arOsg->window->getEventQueue()->mouseButtonPress(x, y, button + 1);
            else arOsg->window->getEventQueue()->mouseButtonRelease(x, y, button + 1);
        }
    }
    
    void arOSGHandleMouseMove(AROSG *arOsg, int x, int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            arOsg->window->getEventQueue()->mouseMotion(x, y);
        }
    }
    
    void arOSGHandleKeyboard(AROSG *arOsg, int key, int x, int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            arOsg->window->getEventQueue()->keyPress((osgGA::GUIEventAdapter::KeySymbol)key);
            arOsg->window->getEventQueue()->keyRelease((osgGA::GUIEventAdapter::KeySymbol)key);
        }
    }
    
    void arOSGFinal(AROSG *arOsg)
    {
        if (!arOsg) return;
        
        //if (arOsg->viewer.valid()) arOsg->viewer->unref();
        //if (arOsg->sg.valid()) arOsg->sg->unref();
        free (arOsg);
    }
    
} // extern "C"
