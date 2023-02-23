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
 *  Copyright 2020 Mozilla.
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
#include <osg/GL>
#include <osg/Node>
#include <osg/ShapeDrawable>
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
#include <osgText/Text>
#include <osg/ComputeBoundsVisitor>
#if ARX_TARGET_PLATFORM_IOS || ARX_TARGET_PLATFORM_ANDROID || defined(__EMSCRIPTEN__)
#  include "osgPlugins.h"
#endif
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
#  include "shaders.h"
#endif

#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#ifdef __EMSCRIPTEN__
#  include <emscripten/html5.h>
#endif

#define AR_OSG_NODE_MASK_DRAWABLE 0x01
#define AR_OSG_NODE_MASK_SELECTABLE 0x02

class Logger;

// Debugging hint. To get more log detail from OSG, wrap the call with these directives:
// osg::setNotifyLevel(osg::DEBUG_INFO);
// (OSG call here)
// osg::setNotifyLevel(osg::NOTICE);

struct _AROSG {
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::observer_ptr<osgViewer::GraphicsWindow> window;
    osg::ref_ptr<osg::Group> sg;
    int prevIndex;
    int maxModels;
    osg::ref_ptr<osg::MatrixTransform> *models;
    osg::ref_ptr<osg::MatrixTransform> rays[AR_OSG_RAYS_MAX];
    bool rayHit[AR_OSG_RAYS_MAX];
    osg::Vec3f rayHitPos[AR_OSG_RAYS_MAX];
    osg::Vec3f rayHitNorm[AR_OSG_RAYS_MAX];
    int rayHitModelIndex[AR_OSG_RAYS_MAX];
    int frontFaceWinding;
    double time;
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
    // global programs
    osg::ref_ptr<osg::Program> _vertColorProgram;
    osg::ref_ptr<osg::Program> _textureProgram;
#endif
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webGLCtx;
#endif
    osg::ref_ptr<Logger> logger;
    osg::ref_ptr<osgText::Font> font;
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

#ifdef __EMSCRIPTEN__
class SwizzleBGRTexturesVisitor : public osg::NodeVisitor
{
public:

    SwizzleBGRTexturesVisitor():  osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }

    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());

        for(unsigned int i = 0; i < node.getNumDrawables(); ++i) {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet()) apply(*drawable->getStateSet());
        }

        traverse(node);
    }

    virtual void apply(osg::StateSet& stateset)
    {
        // Search for the existence of any textures in BGR format.
        for (unsigned int i = 0; i < stateset.getTextureAttributeList().size(); ++i) {
            osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset.getTextureAttribute(i, osg::StateAttribute::TEXTURE));
            if (texture) {
                _textureSet.insert(texture);
            }
        }
    }

    void swizzle()
    {
        osg::ref_ptr<osg::State> state = new osg::State;
        state->initializeExtensionProcs();
        int swizzleCount = 0;

        for (TextureSet::iterator itr = _textureSet.begin(); itr != _textureSet.end(); ++itr) {
            osg::Texture* texture = const_cast<osg::Texture*>(itr->get());

            osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(texture);
            if (texture2D) {
                osg::ref_ptr<osg::Image> image = texture2D->getImage();
                if (image && image.valid() && (image->getPixelFormat() == GL_BGR || image->getPixelFormat() == GL_BGRA)) {
                    int pixelSize = image->getPixelFormat() == GL_BGR ? 3 : 4;
                    for (osg::Image::DataIterator img_itr(image); img_itr.valid(); ++img_itr) {
                        uint8_t *p = (uint8_t *)img_itr.data();
                        size_t c = img_itr.size() / pixelSize;
                        for (size_t i = 0; i < c; i++) {
                            uint8_t t = *p;
                            *p = *(p + 2);
                            *(p + 2) = t;
                            p += pixelSize;
                        }
                    }
                    image->setPixelFormat(image->getPixelFormat() == GL_BGR ? GL_RGB : GL_RGBA);
                    texture->apply(*state);
                    swizzleCount++;
                }
            }
        }
        if (swizzleCount) ARLOGi("Swizzled %d textures from BGR format.\n", swizzleCount);
    }

    typedef std::set< osg::ref_ptr<osg::Texture> > TextureSet;
    TextureSet                          _textureSet;
};
#endif // __EMSCRIPTEN__

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

// This class prints OpenSceneGraph notifications to console.
class Logger : public osg::NotifyHandler
{
    public:
        Logger() { }
        virtual ~Logger() { }

        // Override NotifyHandler::notify() to receive OpenSceneGraph notifications.
        void notify(osg::NotifySeverity severity, const char *message)
        {
            switch (severity) {
                case osg::ALWAYS:
                case osg::FATAL:
                    ARLOGe("%s", message);
                    break;
                case osg::WARN:
                    ARLOGw("%s", message);
                    break;
                case osg::NOTICE:
                case osg::INFO:
                    ARLOGi("%s", message);
                    break;
                case osg::DEBUG_INFO:
                case osg::DEBUG_FP:
                    ARLOGd("%s", message);
                    break;
                default:
                    break;
            }
        }
};


extern "C" {

    
    unsigned int AR_OSG_EXTDEF arOSGGetVersion()
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

    ARG_API AR_OSG_EXTDEF arOSGGetPreferredAPI()
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
    
    AROSG AR_OSG_EXTDEF *arOSGInit(int maxModels)
    {
        AROSG *arOsg;
        
        arOsg = (AROSG *)calloc(1, sizeof(AROSG));
        if (!arOsg) return (nullptr);
        
        arOsg->models = (osg::ref_ptr<osg::MatrixTransform> *)calloc(maxModels, sizeof(osg::ref_ptr<osg::MatrixTransform>));
        if (!arOsg->models) {
            free(arOsg);
            return (nullptr);
        }
        arOsg->maxModels = maxModels;
        
#ifdef __EMSCRIPTEN__
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.enableExtensionsByDefault = 1;
//         attrs.majorVersion = 2;
//         attrs.minorVersion = 0;
//         attr.depth = 1;
//         attr.alpha = attr.stencil = attr.antialias = 0;
        arOsg->webGLCtx = emscripten_webgl_create_context("#canvas", &attrs);
        if (arOsg->webGLCtx == 0) {
            ARLOGe("arOSGInit: Can't create WebGL context.\n");
            free (arOsg);
            return (nullptr);    
        } else {
            ARLOGi("arOSGInit got WebGL context %d.\n", arOsg->webGLCtx);
            if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
                ARLOGe("Error in emscripten_webgl_make_context_current().\n");
                free (arOsg);
                return (nullptr);    
            };
        }
#endif

        arOsg->logger = new Logger;
        osg::setNotifyHandler(arOsg->logger);
        osg::setNotifyLevel(osg::NOTICE);
            
        arOsg->sg = new osg::Group;
        arOsg->prevIndex = maxModels - 1;
        arOsg->time = USE_REFERENCE_TIME;

        // create our default programs
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
        arOsg->_vertColorProgram = new osg::Program();
        arOsg->_vertColorProgram->addShader( new osg::Shader(osg::Shader::VERTEX, ColorShaderVert));
        arOsg->_vertColorProgram->addShader( new osg::Shader(osg::Shader::FRAGMENT, ColorShaderFrag));
        
        arOsg->_textureProgram = new osg::Program();
        arOsg->_textureProgram->addShader( new osg::Shader(osg::Shader::VERTEX, TextureShaderVert));
        arOsg->_textureProgram->addShader( new osg::Shader(osg::Shader::FRAGMENT, TextureShaderFrag));
#endif

        return (arOsg);
    }

    void AR_OSG_EXTDEF arOSGFinal(AROSG *arOsg)
    {
        if (!arOsg) return;
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
        //if (arOsg->viewer.valid()) arOsg->viewer->unref();
        //if (arOsg->sg.valid()) arOsg->sg->unref();
        osg::setNotifyHandler(nullptr);
        arOsg->logger = nullptr;
        free (arOsg->models);
        free (arOsg);
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
    
    static int AR_OSG_EXTDEF arOSGLoadInternal(AROSG *arOsg, osg::ref_ptr<osg::Node> model, const double translation[3], const double rotation[4], const double scale[3])
    {
        // Find a free slot in the models array.
        int index = arOsg->prevIndex;
        do {
            index++;
            if (index >= arOsg->maxModels) index = 0;
        } while (index != arOsg->prevIndex && arOsg->models[index] != nullptr);
        if (index == arOsg->prevIndex) {
            ARLOGe("Error: Unable to load model, maximum number of models (%d) already loaded.\n", arOsg->maxModels);
            return (-1);
        };
        arOsg->prevIndex = index; // Save for next time.
        
        //osg::BoundingSphere bs = model->getBound();
        //ARLOGd("Model: radius %f, center (%f,%f,%f).\n", bs.radius(), bs.center().x(), bs.center().y(), bs.center().z());
        
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
        if (translation) configTransform->preMult(osg::Matrix::translate(translation[0], translation[1], translation[2]));
        if (rotation) configTransform->preMult(osg::Matrix::rotate(osg::inDegrees(rotation[0]), rotation[1], rotation[2], rotation[3]));        
        if (scale) {
            configTransform->preMult(osg::Matrix::scale(scale[0], scale[1], scale[2]));
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
            configTransform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON); // Rescale normals.
#endif
        }
#ifdef __EMSCRIPTEN__
        // Fixes for a WebGL-compatible model.
        // Enable VBOs.
        osgUtil::GLObjectsVisitor glov;
        osgUtil::GLObjectsVisitor::Mode mode = glov.getMode();
        mode = mode | osgUtil::GLObjectsVisitor::SWITCH_ON_VERTEX_BUFFER_OBJECTS;
        mode = mode & ~osgUtil::GLObjectsVisitor::SWITCH_OFF_VERTEX_BUFFER_OBJECTS;
        mode = mode & ~osgUtil::GLObjectsVisitor::SWITCH_ON_DISPLAY_LISTS;
        glov.setMode(mode);
        model->accept(glov);
        
        // Swizzle BGR textures.
        SwizzleBGRTexturesVisitor stv;
        model->accept(stv);
        stv.swizzle();
#endif
        
        // Add the model.
        configTransform->addChild(model.get());
        
        return (index);
    }
    
    int AR_OSG_EXTDEF arOSGLoadModel2(AROSG *arOsg, const char *modelFilePath, const double translation[3], const double rotation[4], const double scale[3], const int textures)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
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
            //VBOSetupVisitor vbo;
            //model->accept(vbo);
#  endif
        }

        return arOSGLoadInternal(arOsg, model, translation, rotation, scale);
    }
    
    int AR_OSG_EXTDEF arOSGCreateCubeModel(AROSG *arOsg)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
        const double t[3] = {0.0f, 0.0f, 20.0f};
        const double s[3] = {40.0f, 40.0f, 40.0f};
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
        optimizeNode(model);
        //VBOSetupVisitor vbo;
        //model->accept(vbo);
#endif
        
        return arOSGLoadInternal(arOsg, model, t, nullptr, s);
    }

    int AR_OSG_EXTDEF arOSGLoadModel(AROSG *arOsg, const char *modelDescriptionFilePath)
    {
        FILE             *fp;
        char             buf[MAXPATHLEN], modelFilePath[MAXPATHLEN];
        double           translation[3], rotation[4], scale[3];
                
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
        if (sscanf(buf, "%lf %lf %lf", &translation[0], &translation[1], &translation[2]) != 3) {
            fclose(fp); return (-1);
        }
        // Read rotation.
        get_buff(buf, MAXPATHLEN, fp, 1);
        if (sscanf(buf, "%lf %lf %lf %lf", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4) {
            fclose(fp); return (-1);
        }
        // Read scale.
        get_buff(buf, MAXPATHLEN, fp, 1);
        if (sscanf(buf, "%lf %lf %lf", &scale[0], &scale[1], &scale[2]) != 3) {
            fclose(fp); return (-1);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        int lightingFlag = 1, transparencyFlag = -1, texturesFlag = 0, selectableFlag = 1;
        
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
            } else if (strncmp(buf, "SELECTABLE", 10) == 0) {
                if (sscanf(&(buf[10]), " %d", &selectableFlag) != 1) {
                    ARLOGe("Error in model description file: SELECTABLE token must be followed by an integer >= 0. Discarding.\n");
                }
            }
            // Unknown tokens are ignored.
        }
        fclose(fp);

        int index = arOSGLoadModel2(arOsg, modelFilePath, translation, rotation, scale, texturesFlag);
        
        if (index >= 0) {
            if (!lightingFlag) arOSGSetModelLighting(arOsg, index, 0);
            if (transparencyFlag != -1) arOSGSetModelTransparency(arOsg, index, transparencyFlag);
            if (selectableFlag != 1) arOSGSetModelSelectable(arOsg, index, selectableFlag);
        }
        
        return (index);
    }
    
    int AR_OSG_EXTDEF arOSGUnloadModel(AROSG *arOsg, const int index)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to unload a model.\n");
            return (-1);
        }
        
        arOsg->sg->removeChild(arOsg->models[index].get());
        arOsg->models[index] = nullptr; // This will delete the model, through the ref_ptr template's methods.
        return (0);
    }

    int AR_OSG_EXTDEF arOSGSetModelVisibility(AROSG *arOsg, const int index, const int visible)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model visibility.\n");
            return (-1);
        }
        
        arOsg->models[index]->setNodeMask(visible ? 0xffffffff : 0x0);

        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelVisibility(AROSG *arOsg, const int index, int *visible)
    {
        if (!arOsg || !visible) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model visibility.\n");
            return (-1);
        }
        
        *visible = (arOsg->models[index]->getNodeMask() != 0x0);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelLighting(AROSG *arOsg, const int index, const int lit)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model lighting.\n");
            return (-1);
        }
        
        arOsg->models[index]->getOrCreateStateSet()->setMode(GL_LIGHTING, (lit ? osg::StateAttribute::ON : osg::StateAttribute::OFF));
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelLighting(AROSG *arOsg, const int index, int *lit)
    {
        if (!arOsg || !lit) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model lighting.\n");
            return (-1);
        }
        
        *lit = (arOsg->models[index]->getOrCreateStateSet()->getMode(GL_LIGHTING) != osg::StateAttribute::OFF);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelTransparency(AROSG *arOsg, const int index, const int transparent)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
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
    
    int AR_OSG_EXTDEF arOSGSetModelAnimationPause(AROSG *arOsg, const int index, const int pause)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model animation pause.\n");
            return (-1);
        }
        
        ManageAnimationNodesVisitor manv;
        arOsg->models[index]->accept(manv);
        manv.setPause((pause ? true : false), arOsg->viewer->getFrameStamp()->getSimulationTime());

        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelAnimationTime(AROSG *arOsg, const int index, double *animationTime)
    {
        if (!arOsg || !animationTime) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model animation time.\n");
            return (-1);
        }
        
        ManageAnimationNodesVisitor manv;
        arOsg->models[index]->accept(manv);
        *animationTime = manv.getAnimationTime();
        
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelAnimationReset(AROSG *arOsg, const int index)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to reset model animation.\n");
            return (-1);
        }
        
        ResetAnimationNodesVisitor ranv;
        arOsg->models[index]->accept(ranv);
        
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelAnimationLoopModeOverride(AROSG *arOsg, const int index, const int mode)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
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
    
    int AR_OSG_EXTDEF arOSGSetProjection(AROSG *arOsg, double p[16])
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        arOsg->viewer->getCamera()->setProjectionMatrix(osg::Matrixd(p));

        return (0);
    }

    int AR_OSG_EXTDEF arOSGSetProjectionf(AROSG *arOsg, float p[16])
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        arOsg->viewer->getCamera()->setProjectionMatrix(osg::Matrixf(p));

        return (0);
    }

    int AR_OSG_EXTDEF arOSGGetProjection(AROSG *arOsg, double *p)
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        double *mtx = arOsg->viewer->getCamera()->getProjectionMatrix().ptr();
        for (int i = 0; i < 16; i++) p[i] = (double)(mtx[i]);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetProjectionf(AROSG *arOsg, float *p)
    {
        if (!arOsg || !p) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        double *mtx = arOsg->viewer->getCamera()->getProjectionMatrix().ptr();
        for (int i = 0; i < 16; i++) p[i] = (float)(mtx[i]);
		return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetView(AROSG *arOsg, double v[16])
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        arOsg->viewer->getCamera()->setViewMatrix(osg::Matrixd(v));
        return (0);
    }

    int AR_OSG_EXTDEF arOSGSetViewf(AROSG *arOsg, float v[16])
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        arOsg->viewer->getCamera()->setViewMatrix(osg::Matrixf(v));
        return (0);
    }

    int AR_OSG_EXTDEF arOSGGetView(AROSG *arOsg, double *v)
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        double *mtx = arOsg->viewer->getCamera()->getViewMatrix().ptr();
        for (int i = 0; i < 16; i++) v[i] = (double)(mtx[i]);
        return (0);
    }

    int AR_OSG_EXTDEF arOSGGetViewf(AROSG *arOsg, float *v)
    {
        if (!arOsg || !v) return (-1);
        if (!arOsg->viewer.valid()) return (-1);

        double *mtx = arOsg->viewer->getCamera()->getViewMatrix().ptr();
        for (int i = 0; i < 16; i++) v[i] = (float)(mtx[i]);
        return (0);
    }

    int AR_OSG_EXTDEF arOSGSetFrontFace(AROSG *arOsg, int winding)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
        arOsg->frontFaceWinding = winding;
        if (arOsg->viewer.valid()) {
            osg::ref_ptr<osg::FrontFace> ff = new osg::FrontFace(arOsg->frontFaceWinding ? osg::FrontFace::CLOCKWISE : osg::FrontFace::COUNTER_CLOCKWISE);
            arOsg->viewer->getCamera()->getOrCreateStateSet()->setAttributeAndModes(ff, osg::StateAttribute::ON);
        }
		return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetFrontFace(AROSG *arOsg, int *winding)
    {
        if (!arOsg || !winding) return (-1);
        
        *winding = arOsg->frontFaceWinding;
		return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelPose(AROSG *arOsg, const int index, const double modelview[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }

        arOsg->models[index]->setMatrix(osg::Matrixd(modelview));
        return (0);
    }

    int AR_OSG_EXTDEF arOSGSetModelPosef(AROSG *arOsg, const int index, const float modelview[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }

        arOsg->models[index]->setMatrix(osg::Matrixf(modelview));
        return (0);
    }

    int AR_OSG_EXTDEF arOSGGetModelPose(AROSG *arOsg, const int index, double *modelview)
    {
        if (!arOsg || !modelview) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = arOsg->models[index]->getMatrix().ptr();
#else
        const double *mtx = arOsg->models[index]->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) modelview[i] = (double)(mtx[i]);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelPosef(AROSG *arOsg, const int index, float *modelview)
    {
        if (!arOsg || !modelview) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = arOsg->models[index]->getMatrix().ptr();
#else
        const double *mtx = arOsg->models[index]->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) modelview[i] = (float)(mtx[i]);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelLocalPose(AROSG *arOsg, const int index, const double model[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }
        
        static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->setMatrix(osg::Matrixd(model));
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelLocalPosef(AROSG *arOsg, const int index, const float model[16])
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model pose.\n");
            return (-1);
        }
        
        static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->setMatrix(osg::Matrixf(model));
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGSetModelOutline(AROSG *arOsg, const int index, const int width, const unsigned char rgba[4])
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model outline.\n");
            return (-1);
        }

        const unsigned char rgbaDefault[4] = {255, 255, 0, 204};
        const unsigned char *rgba0 = rgba ? rgba : rgbaDefault;
        osg::Group *configTransform = arOsg->models[index]->getChild(0)->asGroup()->getChild(0)->asGroup(); // arOsg->models[index] is transform, first child is localTransform, second child is configTransform.
        osgFX::Outline *modelAsOutline = dynamic_cast<osgFX::Outline*>(configTransform->getChild(0)); // Is an outline already in place?
        if (modelAsOutline) {
            if (width) {
                // Just change the current params.
                modelAsOutline->setWidth(width);
                modelAsOutline->setColor(osg::Vec4f(rgba0[0],rgba0[1],rgba0[2],rgba0[3]));
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
                outline->setColor(osg::Vec4f(rgba0[0],rgba0[1],rgba0[2],rgba0[3]));
                outline->addChild(configTransform->getChild(0));
                configTransform->removeChild(0, 1);
                configTransform->addChild(outline);
            } else {
                // Do nothing.
            }
        }
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelLocalPose(AROSG *arOsg, const int index, double *model)
    {
        if (!arOsg || !model) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#else
        const double *mtx = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) model[i] = (double)(mtx[i]);
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelLocalPosef(AROSG *arOsg, const int index, float *model)
    {
        if (!arOsg || !model) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model pose.\n");
            return (-1);
        }
        
#ifdef OSG_USE_FLOAT_MATRIX
        const float *mtx  = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#else
        const double *mtx = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getMatrix().ptr();
#endif
        for (int i = 0; i < 16; i++) model[i] = (float)(mtx[i]);
        return (0);
    }
    
    
    void AR_OSG_EXTDEF arOSGSetModelSelectable(AROSG *arOsg, const int index, const int selectable)
    {
        if (!arOsg) return;
        if (index < 0 || index >= arOsg->maxModels) return;
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model selectability.\n");
            return;
        }
        
        static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->setNodeMask(selectable ? 0xffffffff : ~AR_OSG_NODE_MASK_SELECTABLE);
    }
    
    int AR_OSG_EXTDEF arOSGGetModelSelectable(AROSG *arOsg, const int index)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to get model selectability.\n");
            return (-1);
        }
        
        unsigned int nm  = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0))->getNodeMask();
        return (nm & AR_OSG_NODE_MASK_SELECTABLE ? 1 : 0);
    }

    
    int AR_OSG_EXTDEF arOSGGetModelIntersectionf(AROSG *arOsg, const int index, const float p1[3], const float p2[3])
    {
        if (!arOsg || !p1 || !p2) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            fprintf(stderr, "[error] Error: model not found while attempting to get model intersection.\n");
            return (-1);
        }
        
        osgUtil::LineSegmentIntersector::Intersections intersections;        
        osg::ref_ptr< osgUtil::LineSegmentIntersector > intersector = new osgUtil::LineSegmentIntersector(osg::Vec3f(p1[0], p1[1], p1[2]), osg::Vec3f(p2[0], p2[1], p2[2]));
        osgUtil::IntersectionVisitor iv(intersector.get());
        arOsg->models[index]->accept(iv);
        if (intersector->containsIntersections()) {
            return (1);
        } else return (0);
    }
    
    const double AR_OSG_TIME_USE_REFERENCE_TIME = USE_REFERENCE_TIME;
    
    int AR_OSG_EXTDEF arOSGSetDrawTime(AROSG *arOsg, double time)
    {
        if (!arOsg) return (-1);
        arOsg->time = time;
        return (0);
    }
    
    int AR_OSG_EXTDEF arOSGDraw(AROSG *arOsg)
    {
        if (!arOsg) return (-1);
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        if (arOsg->viewer.valid()) {
            //double referenceTime = arOsg->viewer->getViewerFrameStamp()->getReferenceTime();
            arOsg->viewer->frame(arOsg->time); // This call clobbers the OpenGL viewport.
        }
        return (0);
    }

    void AR_OSG_EXTDEF arOSGHandleReshape(AROSG *arOsg, const int w, const int h)
    {
        arOSGHandleReshape2(arOsg, 0, 0, w, h);
    }
    
    void AR_OSG_EXTDEF arOSGHandleReshape2(AROSG *arOsg, const int left, const int bottom, const int width, const int height)
    {
        if (!arOsg) return;
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
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
#ifdef __EMSCRIPTEN__
            arOsg->viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
#endif
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
    
    void AR_OSG_EXTDEF arOSGHandleMouseDownUp(AROSG *arOsg, const int button, const int state, const int x, const int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            if (state == 0) arOsg->window->getEventQueue()->mouseButtonPress(x, y, button + 1);
            else arOsg->window->getEventQueue()->mouseButtonRelease(x, y, button + 1);
        }
    }
    
    void AR_OSG_EXTDEF arOSGHandleMouseMove(AROSG *arOsg, int x, int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            arOsg->window->getEventQueue()->mouseMotion(x, y);
        }
    }
    
    void AR_OSG_EXTDEF arOSGHandleKeyboard(AROSG *arOsg, int key, int x, int y)
    {
        if (!arOsg) return;

        if (arOsg->window.valid()) {
            arOsg->window->getEventQueue()->keyPress((osgGA::GUIEventAdapter::KeySymbol)key);
            arOsg->window->getEventQueue()->keyRelease((osgGA::GUIEventAdapter::KeySymbol)key);
        }
    }
    
    #define AR_OSG_RAYS_MAX 2
    void AR_OSG_EXTDEF arOSGShowRayAndSetPose(AROSG *arOsg, int ray, float pose[16])
    {
        const float thickness = 0.002f;
        const float color[4] = {0.0f, 0.8f, 1.0f, 0.8f};
        const float maxRayLength = 30.0f;
        
        static float demo[AR_OSG_RAYS_MAX] = {0.0f};
        if (!arOsg || ray < 0 || ray >= AR_OSG_RAYS_MAX || !pose) return;
#ifdef __EMSCRIPTEN__
        if (emscripten_webgl_make_context_current(arOsg->webGLCtx) != EMSCRIPTEN_RESULT_SUCCESS) {
            ARLOGw("Error in emscripten_webgl_make_context_current().\n");
        };
#endif
        
        if (!arOsg->rays[ray]) {
            arOsg->rays[ray] = new osg::MatrixTransform;
            osg::ref_ptr<osg::ShapeDrawable> raySD = new osg::ShapeDrawable;
            raySD->setShape(new osg::Box(osg::Vec3(0.0f, 0.0f, -0.5f), thickness, thickness, 1.0f));
            raySD->setColor(osg::Vec4f(color[0], color[1], color[2], color[3]));
            osg::ref_ptr<osg::Geode> rayG = new osg::Geode;
            rayG->addDrawable(raySD.get());
            arOsg->rays[ray]->addChild(rayG.get());
            arOsg->sg->addChild(arOsg->rays[ray].get());
        }
        arOsg->rays[ray]->setMatrix(osg::Matrixf(pose));
        arOsg->rays[ray]->setNodeMask(~AR_OSG_NODE_MASK_SELECTABLE); // Set all bits except AR_OSG_NODE_MASK_SELECTABLE.
        
        // Now work out what the ray is hitting, if anything.
        // Near end of line segment is just origin of ray pose.
        // Far end of line segment is near end, plus transformed -z unit vector
        // multiplied by -maxRayLength.
        osg::Vec3f p0 = osg::Vec3f(pose[12], pose[13], pose[14]);
        osg::Vec3f p1 = p0 - osg::Vec3f(pose[8], pose[9], pose[10])*maxRayLength;
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(p0, p1);
        osgUtil::IntersectionVisitor iv(intersector.get());
        iv.setTraversalMask(AR_OSG_NODE_MASK_SELECTABLE); // Intersect only items with AR_OSG_NODE_MASK_SELECTABLE set.
        arOsg->sg->accept(iv);
        arOsg->rayHitModelIndex[ray] = -1;
        if (intersector->containsIntersections()) {
            const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersector->getIntersections().begin());
            arOsg->rayHit[ray] = true;
            arOsg->rayHitPos[ray] = p1 = intersection.getWorldIntersectPoint();
            arOsg->rayHitNorm[ray] = intersection.getWorldIntersectNormal();
            for (int index = 0; index < arOsg->maxModels; index++) {
                osg::NodePath::const_iterator itr = std::find(intersection.nodePath.begin(), intersection.nodePath.end(), arOsg->models[index].get());
                if (itr != intersection.nodePath.end()) {
                    arOsg->rayHitModelIndex[ray] = index;
                    break;
                }
            }
        } else {
            arOsg->rayHit[ray] = false;
        }
        osg::ref_ptr<osg::ShapeDrawable> raySD = static_cast<osg::ShapeDrawable *>(static_cast<osg::Geode *>(arOsg->rays[ray]->getChild(0))->getDrawable(0)); // We can just use static casts here because we know the node types.
        
        float length = (p1 - p0).length();
        raySD->setShape(new osg::Box(osg::Vec3(0.0f, 0.0f, -length/2.0f), thickness, thickness, length)); // Ray points in -z direction.
        raySD->build();
    }
    
    void AR_OSG_EXTDEF arOSGHideRay(AROSG *arOsg, int ray)
    {
        if (!arOsg || ray < 0 || ray >= AR_OSG_RAYS_MAX) return;
        if (!arOsg->rays[ray]) return;
        arOsg->rays[ray]->setNodeMask(0x0);
    }
    
    int AR_OSG_EXTDEF arOSGGetRayHit(AROSG *arOsg, int ray, float pos[3], float norm[3], int *modelIndexPtr)
    {
        if (!arOsg || ray < 0 || ray >= AR_OSG_RAYS_MAX) return -1;
        
        if (!arOsg->rays[ray]) return 0;
        if (!arOsg->rayHit[ray]) return 0;
        if (pos) {
            pos[0] = arOsg->rayHitPos[ray].x();
            pos[1] = arOsg->rayHitPos[ray].y();
            pos[2] = arOsg->rayHitPos[ray].z();
        }
        if (norm) {
            norm[0] = arOsg->rayHitNorm[ray].x();
            norm[1] = arOsg->rayHitNorm[ray].y();
            norm[2] = arOsg->rayHitNorm[ray].z();
        }
        if (modelIndexPtr) *modelIndexPtr = arOsg->rayHitModelIndex[ray];
        return 1;
    }
    
    int AR_OSG_EXTDEF arOSGSetModelLabel(AROSG *arOsg, const int index, const char *labelText)
    {
        if (!arOsg) return (-1);
        if (index < 0 || index >= arOsg->maxModels) return (-1);
        if (!arOsg->models[index]) {
            ARLOGe("Error: model not found while attempting to set model label.\n");
            return (-1);
        }
        
        // Check if existing label already in place.
        // First get the local pose transform. Then check for a second child.
        osg::ref_ptr<osg::MatrixTransform> localTransform = static_cast<osg::MatrixTransform *>(arOsg->models[index]->getChild(0));
        osg::ref_ptr<osg::Geode> geode;
        if (localTransform->getNumChildren() > 1) {
            geode = localTransform->getChild(1)->asGeode();
        }
        
        if (!labelText || !labelText[0]) {
            // Remove existing label.
            if (geode.valid()) localTransform->removeChild(1);
        } else {
            if (!arOsg->font.valid()) {
                arOsg->font = osgText::readFontFile("fonts/Vera.ttf");
                if (!arOsg->font) arOsg->font = osgText::Font::getDefaultFont();
            }
            if (!geode.valid()) {
                geode = new osg::Geode;
                localTransform->addChild(geode);
            } else {
                geode->removeDrawables(0, geode->getNumDrawables());
            }

            osg::ComputeBoundsVisitor cbv;
            localTransform->accept(cbv);
            osg::BoundingBox bb = cbv.getBoundingBox(); // in local coords.
            float width = bb.xMax() - bb.xMin();
            float height = bb.yMax() - bb.yMin();
            float depth = bb.zMax() - bb.zMin();
            osg::Vec3 center(bb.xMin() + width/2.0f, bb.yMin() + height/2.0f, bb.zMin() + depth/2.0f);
            osg::Vec3 pos(center.x(), center.y() + height/2.0f + 0.2f, center.z());
            
            osg::ref_ptr<osgText::Text> text = new osgText::Text;
            text->setFont(arOsg->font.get());
            text->setCharacterSize(0.12f);
            text->setPosition(pos);
            text->setAxisAlignment(osgText::Text::XY_PLANE);
            text->setText(labelText);
            text->setBackdropType(osgText::Text::BackdropType::DROP_SHADOW_BOTTOM_RIGHT);
            text->setAlignment(osgText::Text::AlignmentType::CENTER_BOTTOM_BASE_LINE);
            text->setMaximumWidth(width);
            text->setLineSpacing(0.2f);
            geode->addDrawable(text);
        }
        return (1);
    }
} // extern "C"
