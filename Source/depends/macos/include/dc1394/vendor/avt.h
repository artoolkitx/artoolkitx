/*
 * 1394-Based Digital Camera Control Library
 *
 * Allied Vision Technologies (AVT) specific extensions
 * 
 * Written by Pierre MOOS <pierre.moos@gmail.com>
 *
 * Copyright (C) 2005 Inria Sophia-Antipolis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DC1394_VENDOR_AVT_H__
#define __DC1394_VENDOR_AVT_H__

#include <stdint.h>
#include <dc1394/types.h>
#include <dc1394/log.h>

/*! \file dc1394/vendor/avt.h
    \brief No docs yet

    More details soon
*/
typedef struct __dc1394_avt_adv_feature_info_struct
{
    uint32_t feature_id;
    dc1394bool_t features_requested;
    /************************************************************************/
    dc1394bool_t MaxResolution;      //ADV_INQ_1 0
    dc1394bool_t TimeBase;           //ADV_INQ_1 1
    dc1394bool_t ExtdShutter;        //ADV_INQ_1 2
    dc1394bool_t TestImage;          //ADV_INQ_1 3
    dc1394bool_t FrameInfo;          //ADV_INQ_1 4
    dc1394bool_t Sequences;          //ADV_INQ_1 5
    dc1394bool_t VersionInfo;        //ADV_INQ_1 6
    //ADV_INQ_1 7
    dc1394bool_t Lookup_Tables;      //ADV_INQ_1 8
    dc1394bool_t Shading;            //ADV_INQ_1 9
    dc1394bool_t DeferredTrans;      //ADV_INQ_1 10
    dc1394bool_t HDR_Mode;           //ADV_INQ_1 11
    dc1394bool_t DSNU;               //ADV_INQ_1 12
    dc1394bool_t BlemishCorrection;  //ADV_INQ_1 13
    dc1394bool_t TriggerDelay;       //ADV_INQ_1 14
    dc1394bool_t MirrorImage;        //ADV_INQ_1 15
    dc1394bool_t SoftReset;          //ADV_INQ_1 16
    dc1394bool_t HSNR;               //ADV_INQ_1 17
    dc1394bool_t ColorCorrection;    //ADV_INQ_1 18
    dc1394bool_t UserProfiles;       //ADV_INQ_1 19
    //ADV_INQ_1 20
    dc1394bool_t UserSets;           //ADV_INQ_1 21
    dc1394bool_t TimeStamp;          //ADV_INQ_1 22
    dc1394bool_t FrmCntStamp;        //ADV_INQ_1 23
    dc1394bool_t TrgCntStamp;        //ADV_INQ_1 24
    //ADV_INQ_1 25-30
    dc1394bool_t GP_Buffer;          //ADV_INQ_1 31
    /************************************************************************/
    dc1394bool_t Input_1;            //ADV_INQ_2 0
    dc1394bool_t Input_2;            //ADV_INQ_2 1
    //ADV_INQ_2 2-7
    dc1394bool_t Output_1;           //ADV_INQ_2 8
    dc1394bool_t Output_2;           //ADV_INQ_2 9
    dc1394bool_t Output_3;           //ADV_INQ_2 10
    dc1394bool_t Output_4;           //ADV_INQ_2 11
    //ADV_INQ_2 12-15
    dc1394bool_t IntEnaDelay;        //ADV_INQ_2 16
    dc1394bool_t IncDecoder;         //ADV_INQ_2 17
    //ADV_INQ_2 18-31
    /************************************************************************/
    dc1394bool_t CameraStatus;       //ADV_INQ_3 0
    //ADV_INQ_3 1-3
    dc1394bool_t AutoShutter;        //ADV_INQ_3 4
    dc1394bool_t AutoGain;           //ADV_INQ_3 5
    dc1394bool_t AutoFunctionAOI;    //ADV_INQ_3 6
    //ADV_INQ_3 7-31
    /************************************************************************/
    dc1394bool_t HDRPike;            //ADV_INQ_4 0
    //ADV_INQ_4 1-31


} dc1394_avt_adv_feature_info_t;


#ifdef __cplusplus
extern "C" {
#endif
/**
 * Retrieve the firmware version, FPGA version and the camera ID
 */
dc1394error_t dc1394_avt_get_version(dc1394camera_t *camera,
                                     uint32_t *UCType,
                                     uint32_t *Version,
                                     uint32_t *Camera_ID,
                                     uint32_t *FPGA_Version);



/**
 * Retrieve the supported features
 */
dc1394error_t dc1394_avt_get_advanced_feature_inquiry(dc1394camera_t *camera,
                                                      dc1394_avt_adv_feature_info_t *adv_feature);

/**
 * Print the supported features requested
 */
dc1394error_t dc1394_avt_print_advanced_feature(dc1394_avt_adv_feature_info_t *adv_feature);

/**
 * Retrieve if shading is on and the number of frames used to compute the shading reference frame
 */
dc1394error_t dc1394_avt_get_shading(dc1394camera_t *camera,
                                     dc1394bool_t *on_off,
                                     dc1394bool_t *compute,
                                     dc1394bool_t *show, uint32_t *frame_nb);

/**
 * Set the shading to on/off and the number of frames used to compute the shading reference frame
 */
dc1394error_t dc1394_avt_set_shading(dc1394camera_t *camera,
                                     dc1394bool_t on_off, dc1394bool_t compute,
                                     dc1394bool_t show, uint32_t frame_nb);

/**
 * Retrieve write and read access mode of the shading reference frame
 */
dc1394error_t dc1394_avt_get_shading_mem_ctrl(dc1394camera_t *camera,
                                              dc1394bool_t *en_write,
                                              dc1394bool_t *en_read,
                                              uint32_t *addroffset);

/**
 * Set write and read access mode of the shading reference frame
 */
dc1394error_t dc1394_avt_set_shading_mem_ctrl(dc1394camera_t *camera,
                                              dc1394bool_t en_write,
                                              dc1394bool_t en_read,
                                              uint32_t addroffset);

/**
 * Retrieve the max size of a shading image
 */
dc1394error_t dc1394_avt_get_shading_info(dc1394camera_t *camera,
                                          uint32_t *MaxImageSize);

/**
 * Retrieve if on/off, the nb of kneepoints used and the kneepoints values
 */
dc1394error_t dc1394_avt_get_multiple_slope(dc1394camera_t *camera,
                                            dc1394bool_t *on_off,
                                            uint32_t *points_nb,
                                            uint32_t *kneepoint1,
                                            uint32_t *kneepoint2,
                                            uint32_t *kneepoint3);

/**
 * Set on/off, the nb of kneepoints to use and the kneepoints values
 */
dc1394error_t dc1394_avt_set_multiple_slope(dc1394camera_t *camera,
                                            dc1394bool_t on_off,
                                            uint32_t points_nb,
                                            uint32_t kneepoint1,
                                            uint32_t kneepoint2,
                                            uint32_t kneepoint3);

/**
 * Get the timebase value with an Id. See Manual for correspondance
 */
dc1394error_t dc1394_avt_get_timebase(dc1394camera_t *camera,
                                      uint32_t *timebase_id);

/**
 * Set the timebase value with an Id. See Manual for correspondance
 */
dc1394error_t dc1394_avt_set_timebase(dc1394camera_t *camera,
                                      uint32_t timebase_id);

/**
 * Get the extented shutter value in us
 */
dc1394error_t dc1394_avt_get_extented_shutter(dc1394camera_t *camera,
                                              uint32_t *timebase_id);

/**
 * Set the extented shutter value in us
 */
dc1394error_t dc1394_avt_set_extented_shutter(dc1394camera_t *camera,
                                              uint32_t timebase_id);

/**
 * Get the Max reachable resolution
 */
dc1394error_t dc1394_avt_get_MaxResolution(dc1394camera_t *camera,
                                           uint32_t *MaxHeight,
                                           uint32_t *MaxWidth);

/**
 * Get min and max shutter values for autoshutter
 */
dc1394error_t dc1394_avt_get_auto_shutter(dc1394camera_t *camera,
                                          uint32_t *MinValue,
                                          uint32_t *MaxValue);

/**
 * Set min and max shutter values for autoshutter
 */
dc1394error_t dc1394_avt_set_auto_shutter(dc1394camera_t *camera,
                                          uint32_t MinValue,
                                          uint32_t MaxValue);

/**
 * Get min and max gain values for autogain
 */
dc1394error_t dc1394_avt_get_auto_gain(dc1394camera_t *camera,
                                       uint32_t *MinValue,
                                       uint32_t *MaxValue);

/**
 * Set min and max gain values for autogain
 */
dc1394error_t dc1394_avt_set_auto_gain(dc1394camera_t *camera,
                                       uint32_t MinValue,
                                       uint32_t MaxValue);

/**
 * Get if trigger delay on and the trigger delay
 */
dc1394error_t dc1394_avt_get_trigger_delay(dc1394camera_t *camera,
                                           dc1394bool_t *on_off,
                                           uint32_t *DelayTime);

/**
 * Set trigger delay on/off  and the trigger delay value
 */
dc1394error_t dc1394_avt_set_trigger_delay(dc1394camera_t *camera,
                                           dc1394bool_t on_off,
                                           uint32_t DelayTime);

/**
 * Get mirror mode
 */
dc1394error_t dc1394_avt_get_mirror(dc1394camera_t *camera,
                                    dc1394bool_t *on_off);

/**
 * Set mirror mode
 */
dc1394error_t dc1394_avt_set_mirror(dc1394camera_t *camera,
                                    dc1394bool_t on_off);

/**
 * Get DSNU mode and num of frames used for computing  dsnu correction
 */
dc1394error_t dc1394_avt_get_dsnu(dc1394camera_t *camera,
                                  dc1394bool_t *on_off,
                                  uint32_t *frame_nb);

/**
 * Set DSNU mode, number of frames used for computing and launch the the computation of the dsnu frame
 */
dc1394error_t dc1394_avt_set_dsnu(dc1394camera_t *camera,
                                  dc1394bool_t on_off, dc1394bool_t compute,
                                  uint32_t frame_nb);

/**
 * Get Blemish mode and num of frames used for computing the correction
 */
dc1394error_t dc1394_avt_get_blemish(dc1394camera_t *camera,
                                     dc1394bool_t *on_off, uint32_t *frame_nb);

/**
 * Set Blemish mode, num of frames used for computing and launch the the computation of the blemish correction
 */
dc1394error_t dc1394_avt_set_blemish(dc1394camera_t *camera,
                                     dc1394bool_t on_off, dc1394bool_t compute,
                                     uint32_t frame_nb);

/**
 * Get the polarity, the mode, the state of the IO
 */
dc1394error_t dc1394_avt_get_io(dc1394camera_t *camera, uint32_t IO,
                                dc1394bool_t *polarity, uint32_t *mode,
                                dc1394bool_t *pinstate);

/**
 * Set the polarity, the mode and the state of the IO
 */
dc1394error_t dc1394_avt_set_io(dc1394camera_t *camera,uint32_t IO,
                                dc1394bool_t polarity, uint32_t mode,
                                dc1394bool_t pinstate);

/**
 * Reset the bus and the fpga
 */
dc1394error_t dc1394_avt_reset(dc1394camera_t *camera);

/**
 * Get on/off and the num of the current lut loaded
 */
dc1394error_t dc1394_avt_get_lut(dc1394camera_t *camera,
                                 dc1394bool_t *on_off, uint32_t *lutnb  );

/**
 * Set on/off and the num of the current lut to loa
 */
dc1394error_t dc1394_avt_set_lut(dc1394camera_t *camera,
                                 dc1394bool_t on_off, uint32_t lutnb);

/**
 * Get access mode of a lut
 */
dc1394error_t dc1394_avt_get_lut_mem_ctrl(dc1394camera_t *camera,
                                          dc1394bool_t *en_write,
                                          uint32_t * AccessLutNo,
                                          uint32_t *addroffset);

/**
 * Set access mode of a lut
 */
dc1394error_t dc1394_avt_set_lut_mem_ctrl(dc1394camera_t *camera,
                                          dc1394bool_t en_write,
                                          uint32_t AccessLutNo,
                                          uint32_t addroffset);

/**
 * Get num of luts present and the max size
 */
dc1394error_t dc1394_avt_get_lut_info(dc1394camera_t *camera,
                                      uint32_t *NumOfLuts, uint32_t *MaxLutSize);

/**
 * Get on/off and area
 */
dc1394error_t dc1394_avt_get_aoi(dc1394camera_t *camera,
                                 dc1394bool_t *on_off, int *left, int *top,
                                 int *width, int *height);

/**
 * Set on/off and area
 */
dc1394error_t dc1394_avt_set_aoi(dc1394camera_t *camera,
                                 dc1394bool_t on_off,int left, int top,
                                 int width, int height);

/**
 * Get current test image
 */
dc1394error_t dc1394_avt_get_test_images(dc1394camera_t *camera,
                                         uint32_t *image_no);

/**
 * Set num of test image
 */
dc1394error_t dc1394_avt_set_test_images(dc1394camera_t *camera,
                                         uint32_t image_no);

/**
 * Get the number of captured frames
 */
dc1394error_t dc1394_avt_get_frame_info(dc1394camera_t *camera,
                                        uint32_t *framecounter);

/**
 * Reset frame counter
 */
dc1394error_t dc1394_avt_reset_frame_info(dc1394camera_t *camera);

/**
 * Get the size of the buffer
 */
dc1394error_t dc1394_avt_get_gpdata_info(dc1394camera_t *camera,
                                         uint32_t *BufferSize);

/**
 * Get the fifo control mode
 */
dc1394error_t dc1394_avt_get_deferred_trans(dc1394camera_t *camera,
                                            dc1394bool_t *HoldImage,
                                            dc1394bool_t * FastCapture,
                                            uint32_t *FifoSize,
                                            uint32_t *NumOfImages );

/**
 * Set the fifo control mode
 */
dc1394error_t dc1394_avt_set_deferred_trans(dc1394camera_t *camera,
                                            dc1394bool_t HoldImage,
                                            dc1394bool_t  FastCapture,
                                            uint32_t FifoSize,
                                            uint32_t NumOfImages,
                                            dc1394bool_t SendImage );

/**
 * Read size number of bytes from GPData buffe
 */
dc1394error_t dc1394_avt_read_gpdata(dc1394camera_t *camera, unsigned char *buf,
                                     uint32_t size);

/**
 * Write size number of bytes to GPData buffer
 */
dc1394error_t dc1394_avt_write_gpdata(dc1394camera_t *camera,
                                      unsigned char *buf, uint32_t size);

/**
 * Read shading image from camera into buffer
 */
dc1394error_t dc1394_avt_read_shading_img(dc1394camera_t *camera,
                                          unsigned char *buf, uint32_t size);

/**
 * Write shading image from buffer to camera
 */
dc1394error_t dc1394_avt_write_shading_img(dc1394camera_t *camera,
                                           unsigned char *buf, uint32_t size);

/**
 * Read channel adjust (AVT Pike)
 */
dc1394error_t dc1394_avt_get_channel_adjust(dc1394camera_t *camera,
                                                int16_t *channel_adjust);

/**
 * Write channel adjust (AVT Pike)
 */
dc1394error_t dc1394_avt_set_channel_adjust(dc1394camera_t *camera,
                                                int16_t channel_adjust);

/**
 * Set Color Correction + Coefficients
 */
dc1394error_t dc1394_avt_set_color_corr(dc1394camera_t *camera, dc1394bool_t on_off, dc1394bool_t reset,
                int32_t Crr, int32_t Cgr, int32_t Cbr, int32_t Crg, int32_t Cgg, int32_t Cbg, int32_t Crb, int32_t Cgb, int32_t Cbb);

/**
 * Get Color Correction + Coefficients
 */
dc1394error_t dc1394_avt_get_color_corr(dc1394camera_t *camera, dc1394bool_t *on_off,
                int32_t *Crr, int32_t *Cgr, int32_t *Cbr, int32_t *Crg, int32_t *Cgg, int32_t *Cbg, int32_t *Crb, int32_t *Cgb, int32_t *Cbb);

/**
 * Get HSNR
 */
dc1394error_t dc1394_avt_get_hsnr(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *grabCount);

/**
 * Set HSNR
 */
dc1394error_t dc1394_avt_set_hsnr(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t grabCount);

#ifdef __cplusplus
}
#endif

#endif
