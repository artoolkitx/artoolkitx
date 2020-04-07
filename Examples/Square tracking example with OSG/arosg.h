/*
 *  arosg.h
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

/*!
    @header arosg
    @abstract   C-interface to OpenSceneGraph for augmented reality applications.
    @discussion
        arosg is intended to provide access to the modern plugin-based scene graph   
        OpenSceneGraph, and its attendant model formats and graphical capabilities.
 
        arosg provides a C-based interface to a limited subset of the
        functionality of OpenSceneGraph. As of version 1.0 of arosg, the
        supported functionality is primarily 3D model loading and drawing.
    @availability Available ARToolKit v4.4.2 and later.
*/

#ifndef AR_OSG_H
#define AR_OSG_H

#include <ARX/AR/ar.h>
#include <ARX/ARG/arg.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#ifdef _WIN32
#  ifdef LIBAROSG_EXPORTS
#    define AR_OSG_EXTERN __declspec(dllexport)
#  else
#    ifdef _DLL
#      define AR_OSG_EXTERN __declspec(dllimport)
#    else
#      define AR_OSG_EXTERN extern
#    endif
#  endif
#else
#  define AR_OSG_EXTERN
#endif


#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#define AR_OSG_MODELS_MAX 64
#else
#define AR_OSG_MODELS_MAX 1024
#endif

typedef struct _AROSG AROSG; // (Forward definition of opaque structure).

/*!
    @function
    @abstract   Get the version of ARToolKit with which the arOSG library was built.
    @discussion
        It is highly recommended that
        any calling program that depends on features in a certain
        ARToolKit version, check at runtime that it is linked to a version
        of ARToolKit that can supply those features. It is NOT sufficient
        to check the ARToolKit SDK header versions, since with ARToolKit implemented
        in dynamically-loaded libraries, there is no guarantee that the
        version of ARToolKit installed on the machine at run-time will be as
        recent as the version of the ARToolKit SDK which the host
        program was compiled against.
    @result
        Returns the full version number of the ARToolKit version corresponding
        to this OSG library, in binary coded decimal (BCD) format.

        BCD format allows simple tests of version number in the caller
        e.g. if ((arGetVersion() >> 16) > 0x0272) printf("This release is later than 2.72\n");

        The major version number is encoded in the most-significant byte
        (bits 31-24), the minor version number in the second-most-significant
        byte (bits 23-16), the tiny version number in the third-most-significant
        byte (bits 15-8), and the build version number in the least-significant
        byte (bits 7-0).
    @availability Available in ARToolKit v4.4.2 and later.
 */
AR_OSG_EXTERN     unsigned int arOSGGetVersion();

AR_OSG_EXTERN     ARG_API arOSGGetPreferredAPI();

/*!
    @function
    @abstract   Create a settings structure for use with all other arOSG functions.
    @discussion
        All other arOSG functions require a reference to settings and global data.
        Use this function to create and initialise such a structure.
    @result      Pointer to the new AROSG settings structure.
*/
AR_OSG_EXTERN     AROSG *arOSGInit();

/*!
    @function
    @abstract   Load an OSG model using a "model description file".
    @discussion -
        The format of this file is a simple text file. Comments may be
        included by prefixing the line with a "#" character.
        The object definition consists of the following:
        <ul>
        <li> A line with the path to the object's data file, relative to the
            objects file. (This path may include spaces.)
        <li> A line with the position of the object's origin, relative to the
            parent coordinate system's origin, expressed as 3 floating point
            numbers separated by spaces, representing the offset in x, y, and z.
            This is the same format used by the glTranslate() function.
        <li> A line with the orientation of the object's coordinate system,
            relative to the parent coordinate system, expressed as 4 floating
            point numbers separated by spaces, representing an angle and an
            axis of a rotation from the parent. This is the same format used
            by the glRotate() function.
        <li> A line with the scale factor to apply to the object, expressed
            as 3 floating point numbers separated by spaces, representing the
            scale factor to apply in x, y, and z.
            This is the same format used by the glScale() function.
        <li> Zero or more lines with optional tokens representing additional
            information about the object. The following tokens are defined:
            <ul>
            <li> LIGHTING f: Enables or disables lighting calculations for this
                object. Note that disabling lighting will result in the
                object being drawn fully lit but without shading.
                f = 0 to disable, f = 1 to enable. Default is enabled.
            <li> TRANSPARENT: Provides a hint that this object includes transparent
                portions, and should be drawn alpha-blended. Default is
                that no transparency hint is provided.
            </ul>
        </ul>
    @param      arOsg Pointer to the AROSG settings structure into which the model should be loaded. (See arOSGInit().)
    @param      modelDescriptionFilePath
        A string holding the path to a "model description file". This file must contain a structured list of
        the following data: the relative path to the actual OSG model to be loaded, the translation (in x,y,z axes)
        to be applied to the model, the rotation (in angle/axis form, as degrees, angle of rotation x,y,z), and
        the scale factor to be applied to the model (in the x,y,z axes). See the sample files included
        in the ARToolKit distribution in the directory bin/OSG for examples.
    @result     An index value with which the loaded model can be referred to, in the range [0, AR_OSG_MODELS_MAX - 1],
        or, in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGLoadModel(AROSG *arOsg, const char *modelDescriptionFilePath);

/*!
    @function
    @abstract   Load an OSG model.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure into which the model should be loaded. (See arOSGInit().)
    @param      modelFilePath A string holding the path to a file readable by OSG (as a node, e.g. an .ive or .osg file).
    @param      translation The translation (in x,y,z axes) to be applied to the model, or NULL to apply no translation.
    @param      rotation The rotation (in angle/axis form, as degrees, angle of rotation x,y,z) to be applied to the model, or NULL to apply no rotation.
    @param      scale The scale factor to be applied to the model (in the x,y,z axes) or NULL to retain the scale at 1.0.
    @param      textures Provides a hint that the model uses textures. This is required when using programmable OpenGL pipelines (e.g. OpenGL ES 2.0+, or OpenGL 3.1+) with textured models.
    @result     An index value with which the loaded model can be referred to, in the range [0, AR_OSG_MODELS_MAX - 1],
        or, in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
 */
AR_OSG_EXTERN     int arOSGLoadModel2(AROSG *arOsg, const char *modelFilePath, const ARdouble translation[3], const ARdouble rotation[4], const ARdouble scale[3], const int textures);

/*!
    @function
    @abstract   Unload an OSG model.
    @discussion
        Frees the memory associated with an OSG model.
    @param      arOsg Pointer to the AROSG settings structure from which the model should be unloaded. (See arOSGInit().)
    @result     0, or in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGUnloadModel(AROSG *arOsg, const int index);

/*!
    @function
    @abstract   Show or hide an OSG-based model.
    @discussion
        Models are visible by default.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to adjust the visibility of. See arOSGLoadModel().
    @param      visible 0 to hide, or non-zero to show.
    @result     0, or in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGSetModelVisibility(AROSG *arOsg, const int index, const int visible);

/*!
    @function
    @abstract   Find out if an OSG-based model is shown or hidden.
    @discussion
        Models are visible by default.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to request the visibility of. See arOSGLoadModel().
    @param      visible Pointer to a location, which on return will contain 0 if hidden, or 1 if shown.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
*/
AR_OSG_EXTERN     int arOSGGetModelVisibility(AROSG *arOsg, const int index, int *visible);

/*!
    @function
    @abstract   Set lighting for a model on or off.
    @discussion
        By default, lighting is enabled for models.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to adjust the lighting of. See arOSGLoadModel().
    @param      lit 0 to disable lighting, or non-zero to enable.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
*/
AR_OSG_EXTERN     int arOSGSetModelLighting(AROSG *arOsg, const int index, const int lit);
    
/*!
    @function
    @abstract   Find out if lighting for model is on or off.
    @discussion
        By default, lighting is enabled for models.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to request the lighting state of. See arOSGLoadModel().
    @param      lit Pointer to a location, which on return will contain 0 if lighting disable, or 1 if enabled.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
*/
AR_OSG_EXTERN     int arOSGGetModelLighting(AROSG *arOsg, const int index, int *lit);
    
/*!
    @function
    @abstract   Set transparency for a model on or off.
    @discussion
        By default, transparency for models depends on the state with which the model was created.
        However, this setting allows overriding of the transparency settings. One example usage
        would be to force an image texture containing an alpha channel to draw with transparency,
        or alternately without.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to adjust the transparency of. See arOSGLoadModel().
    @param      transparent 0 to disable transparency, or non-zero to enable.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.6. and later.
*/
AR_OSG_EXTERN     int arOSGSetModelTransparency(AROSG *arOsg, const int index, const int transparent);

/*!
    @function
    @abstract   For a model with animation, pause or resume the animation.
    @discussion
        Models with animations may have the animation paused and later resumed.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model of which the animation should be paused or resumed. See arOSGLoadModel().
    @param      pause 0 to resume the animation, 1 to pause it.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.6.7 and later.
*/
AR_OSG_EXTERN     int arOSGSetModelAnimationPause(AROSG *arOsg, const int index, const int pause);

/*!
    @function
    @abstract   For a model with animation, discover the animation time.
    @discussion
        You may query the animation time of models with animations.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model of which the animation time should be queried. See arOSGLoadModel().
    @result     0, or in case of error, a value less than 0.
    @param animationTime A pointer to a double, which will be filled with the animation time (should one be available), or 0 if no animation time is defined.
    @availability Available in ARToolKit v4.6.7 and later.
*/
AR_OSG_EXTERN     int arOSGGetModelAnimationTime(AROSG *arOsg, const int index, double *animationTime);

/*!
    @function
    @abstract   For a model with animation, reset the animation.
    @discussion
        Models with animations may have the animation reset to the initial state.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model of which the animation should be reset. See arOSGLoadModel().
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.6.7 and later.
*/
AR_OSG_EXTERN     int arOSGSetModelAnimationReset(AROSG *arOsg, const int index);

/*!
    @function
    @abstract   For a model with animation, override the animation loop mode.
    @discussion
        Models with animations have a default mode set at time of creation. To
        override the mode at runtime, use this function.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model of which  the animation loop mode should be overridden. See arOSGLoadModel().
    @param      mode 0 to disable looping, 1 to enable looping (where at the end of the animation sequence, animation continues from the beginning again), or 2 to enable
        swinging animation (where at the end of animation, animation proceeds in reverse
        until the beginning is reached, at which point animation proceeds forward again,
        ad infinitum.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.6.7 and later.
*/
AR_OSG_EXTERN     int arOSGSetModelAnimationLoopModeOverride(AROSG *arOsg, const int index, const int mode);

/*!
    @function
    @abstract   Set the projection matrix used in OSG drawing.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      p A 4x4 OpenGL transform matrix (column-major order) representing the projection transform.
    @result     0, or in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGSetProjection(AROSG *arOsg, ARdouble p[16]);
#ifdef ARDOUBLE_IS_FLOAT
#define arOSGSetProjectionf arOSGSetProjection
#else
AR_OSG_EXTERN     int arOSGSetProjectionf(AROSG *arOsg, float p[16]);
#endif

/*!
    @function
    @abstract   Get the projection matrix used in OSG drawing.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      p A 4x4 OpenGL transform matrix (column-major order) representing the projection transform.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
 */
AR_OSG_EXTERN     int arOSGGetProjection(AROSG *arOsg, ARdouble *p);

/*!
    @function
    @abstract   Set the view matrix used in OSG drawing.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      v A 4x4 OpenGL transform matrix (column-major order) representing the viewing transform.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v5.4 and later.
*/
AR_OSG_EXTERN     int arOSGSetView(AROSG *arOsg, ARdouble v[16]);
#ifdef ARDOUBLE_IS_FLOAT
#define arOSGSetViewf arOSGSetView
#else
AR_OSG_EXTERN     int arOSGSetViewf(AROSG *arOsg, float v[16]);
#endif

/*!
    @function
    @abstract   Get the view matrix used in OSG drawing.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      v A 4x4 OpenGL transform matrix (column-major order) representing the view transform.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v5.4 and later.
 */
AR_OSG_EXTERN     int arOSGGetView(AROSG *arOsg, ARdouble *v);

/*!
    @function
    @abstract   Set the polygon winding for front-facing polygons.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      winding 0 to set counter-clockwise (the default), or 1 to set clockwise.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v5.0 and later.
 */
AR_OSG_EXTERN     int arOSGSetFrontFace(AROSG *arOsg, int winding);

/*!
    @function
    @abstract   Get the polygon winding for front-facing polygons.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      winding Pointer to a location, which on return will be 0 if counter-clockwise (the default), or 1 if clockwise.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v5.0 and later.
 */
AR_OSG_EXTERN     int arOSGGetFrontFace(AROSG *arOsg, int *winding);

/*!
    @function
    @abstract   Set the pose (position and orientation) of an OSG-based model.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to adjust the pose of. See arOSGLoadModel().
    @param      modelview A 4x4 OpenGL transform matrix (column-major order) representing the modelview transform of the model.
    @result     0, or in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGSetModelPose(AROSG *arOsg, const int index, const ARdouble modelview[16]);
#ifdef ARDOUBLE_IS_FLOAT
#define arOSGSetModelPosef arOSGSetModelPose
#else
AR_OSG_EXTERN     int arOSGSetModelPosef(AROSG *arOsg, const int index, const float modelview[16]);
#endif

/*!
    @function
    @abstract   Get the pose (position and orientation) of an OSG-based model.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to retrieve the pose of. See arOSGLoadModel().
    @param      modelview A 4x4 OpenGL transform matrix (column-major order) representing the modelview transform of the model.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
 */
AR_OSG_EXTERN     int arOSGGetModelPose(AROSG *arOsg, const int index, ARdouble *modelview);

/*!
    @function
    @abstract   Set the local pose (position and orientation) of an OSG-based model.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to adjust the pose of. See arOSGLoadModel().
    @param      model A 4x4 OpenGL transform matrix (column-major order) representing the local transform of the model.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.5 and later.
*/
AR_OSG_EXTERN     int arOSGSetModelLocalPose(AROSG *arOsg, const int index, const ARdouble model[16]);

/*!
    @function
    @abstract   Get the local pose (position and orientation) of an OSG-based model.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to retrieve the pose of. See arOSGLoadModel().
    @param      model A 4x4 OpenGL transform matrix (column-major order) representing the local transform of the model.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.5 and later.
 */
AR_OSG_EXTERN     int arOSGGetModelLocalPose(AROSG *arOsg, const int index, ARdouble *model);

/*!
    @function
    @abstract   Turn on or off 2D outlining of a model's boundary.
    @discussion
        Outlining allows a model to be highlighted by drawing a 2D outline of the models extreme boundary.
        Outlining is not currently supported by OSG on platforms using OpenGL ES, and calling this function
        will produce erroneous visual results if used on an OpenGL ES platform.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to retrieve the pose of. See arOSGLoadModel().
    @param      width The width, in pixels, of the outline to place around the model. To disable outlining, pass 0.
        Outlining is initially disabled.
    @param      rgba The colour of the outline, as an array of unsigned bytes in order red, green, blue, alpha.
        An opaque outline can be generated by passing 255 for the alpha value. Values less than 255 will result in a transparent outline.
    @result     0, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.6.3 and later.
 */
AR_OSG_EXTERN     int arOSGSetModelOutline(AROSG *arOsg, const int index, const int width, const unsigned char rgba[4]);

/*!
    @function
    @abstract   Determine if a model is intersected by a line segment.
    @discussion
        This calculates the intersection between a line segment (defined by two points in world coordinates)
        and a model (actually an OSG node and subnodes). If an intersection is found, different data types
        can be returned by providing pointers in parameters nodeType, nodeName, and intersectionCoords.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      index The index of the model to calculate the intersection of. See arOSGLoadModel().
    @param      p1 Required; a vector specifying the location of one end of the line segment, in world coordinates.
    @param      p2 Required; a vector specifying the location of the other end of the line segment, in world coordinates.
    @result     1 if an intersection was found, 0 if no intersection was found, or in case of error, a value less than 0.
    @availability Available in ARToolKit v4.5.1 and later.
*/
AR_OSG_EXTERN     int arOSGGetModelIntersection(AROSG *arOsg, const int index, const ARdouble p1[3], const ARdouble p2[3]);

/*!
    @constant
    @abstract   Variable to supply to arOSGSetDrawTime to direct OpenSceneGraph to use its internal reference time.
    @discussion
        OSG maintains its own internal reference time for animation and simulation 
        rendering. If you wish to render using OSG's reference time, supply this
        value to arOSGSetDrawTime.
    @seealso arOSGSetDrawTime arOSGSetDrawTime
*/
extern const double AR_OSG_TIME_USE_REFERENCE_TIME;

/*!
    @function
    @abstract   Process scenegraph-related events, including drawing all visible models.
    @discussion
        OSG maintains its own internal reference time for animation and simulation 
        rendering. If you wish to render at particular time instants or at non-unit
        timescales, you can manipulate the timeline for following arOSGDraw calls by
        calling this function with the time (in decimal seconds) supplied in the
        parameter. To revert to OSG's internal reference time, pass the value
        AR_OSG_TIME_USE_REFERENCE_TIME.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      time Decimal seconds value at which to render the next arOSGDraw call.
    @result     0, or in case of error, a value less than 0.
    @seealso AR_OSG_TIME_USE_REFERENCE_TIME AR_OSG_TIME_USE_REFERENCE_TIME
    @seealso arOSGDraw arOSGDraw
*/
AR_OSG_EXTERN     int arOSGSetDrawTime(AROSG *arOsg, double time);
    
/*!
    @function
    @abstract   Process scenegraph-related events, including drawing all visible models.
    @discussion
        This function leaves the values of the OpenGL machine in an indeterminate state, including
        clobbering the OpenGL viewport, projection and modelview matrices. The user
        should save any sensitive state prior to calling this function, and restore that state upon return.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @result     0, or in case of error, a value less than 0.
*/
AR_OSG_EXTERN     int arOSGDraw(AROSG *arOsg);

/*!
    @function
    @abstract   Inform OpenSceneGraph that a window reshape has occured (i.e. changes to window size have occured).
    @discussion
        This function should be called when the window is first created, and then
        whenever the operating system generates a notification
        that the user has changed the window size.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      w The new window width, in pixels.
    @param      h The new window height, in pixels.
 */
AR_OSG_EXTERN     void arOSGHandleReshape(AROSG *arOsg, const int w, const int h);
    
/*!
    @function
    @abstract   Inform OpenSceneGraph that a window reshape has occured (i.e. changes to window size have occured).
    @discussion
        This function should be called when the window is first created, and then
        whenever the operating system generates a notification
        that the user has changed the window size.
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      left The new viewport origin left coordinate, in pixels.
    @param      bottom The new viewport origin bottom coordinate, in pixels.
    @param      width The new viewport width, in pixels.
    @param      height The new viewport height, in pixels.
 */
AR_OSG_EXTERN     void arOSGHandleReshape2(AROSG *arOsg, const int left, const int bottom, const int width, const int height);

/*!
    @function
    @abstract   Pass mouse clicks to OpenSceneGraph.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      button The values 0, 1 or 2 for the left, middle and right mouse buttons, respectively.
        (For the convenience of users using GLUT, these values match the values of GLUT_LEFT_BUTTON,
        GLUT_MIDDLE_BUTTON and GLUT_RIGHT_BUTTON.)
    @param      state 0 for a mouse-button-down event, and 1 for a mouse-button-up (button released) event.
    @param      x The mouse x location in window relative coordinates when the mouse button was pressed.
    @param      y The mouse y location in window relative coordinates.
*/
AR_OSG_EXTERN     void arOSGHandleMouseDownUp(AROSG *arOsg, const int button, const int state, const int x, const int y);

/*!
    @function
    @abstract   Pass mouse motion events to OpenSceneGraph
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      x The mouse x location in window relative coordinates.
    @param      y The mouse y location in window relative coordinates.
*/
AR_OSG_EXTERN     void arOSGHandleMouseMove(AROSG *arOsg, int x, int y);

/*!
    @function
    @abstract   Pass key press events to OpenSceneGraph.
    @discussion -
    @param      arOsg Pointer to the AROSG settings structure. (See arOSGInit().)
    @param      key Key code. The handling of this is OSG-depdendent, but ASCII key codes (0x00 to 0x7f) at least
        can be passed in directly. For the appropriate key code to use for
        other keys, see the values defined in OpenSceneGraph. The values are listed in the OSG header file
        &lt;osgGA/GUIEventAdapter&gt; under the enum "KeyCode".
    @param      x The mouse x location in window relative coordinates when the key was pressed -- presently ignored.
    @param      y The mouse y location in window relative coordinates when the key was pressed -- presently ignored.
*/
AR_OSG_EXTERN     void arOSGHandleKeyboard(AROSG *arOsg, int key, int x, int y);

/*!
    @function
    @abstract   Dispose of an AROSG settings structure.
    @discussion
        If you have finished with an AROSG settings structure in your running program,
        you can unload its internal data by calling arOSGFinal.
    @param      arOsg Pointer to the AROSG settings structure to be disposed of. (See arOSGInit().)
*/
AR_OSG_EXTERN     void arOSGFinal(AROSG *arOsg);

#ifdef __cplusplus
}
#endif

#endif // !AR_OSG_H


