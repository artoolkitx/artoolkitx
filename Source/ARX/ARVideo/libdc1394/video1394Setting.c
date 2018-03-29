/*
 *  video1394Setting.c
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*
 *   Video capture subrutine for Linux/libdc1394 devices
 *   author: Hirokazu Kato (kato@sys.im.hiroshima-cu.ac.jp)
 *
 *   Revision: 3.0   Date: 2004/01/01
 */

#include "video1394.h"

#ifdef ARVIDEO_INPUT_LIBDC1394
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#ifndef _WIN32
#  include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dc1394/dc1394.h>

#include "video1394Private.h"


int ar2VideoGetAbsValue1394(AR2VideoParam1394T *vid, int paramName, ARdouble *value)
{
    float   v;

    switch (paramName) {
        case AR_VIDEO_1394_GAMMA:
            if (dc1394_feature_get_absolute_value(vid->camera, DC1394_FEATURE_BRIGHTNESS, &v) != DC1394_SUCCESS) {
                ARLOGe("unable to get brightness.\n");
                return -1;
            }
            *value = (float)v;
            return 0;
    }

    return -1;
}


int ar2VideoSetAbsValue1394(AR2VideoParam1394T *vid, int paramName, ARdouble value)
{
    switch (paramName) {
        case AR_VIDEO_1394_GAMMA:
            if (dc1394_feature_set_absolute_value(vid->camera, DC1394_FEATURE_BRIGHTNESS, (float)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set brightness to %f.\n", value);
                return -1;
            }
            return 0;
    }

    return -1;
}

int ar2VideoGetAbsMaxValue1394(AR2VideoParam1394T *vid, int paramName, ARdouble *value)
{
    dc1394feature_t   feature;
    float             min, max;

    switch (paramName) {
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_get_absolute_boundaries(vid->camera, feature, &min, &max) != DC1394_SUCCESS) {
        ARLOGe("unable to get max value.\n");
        return -1;
    }
    *value = (float)max;

    return 0;
}

int ar2VideoGetAbsMinValue1394(AR2VideoParam1394T *vid, int paramName, ARdouble *value)
{
    dc1394feature_t   feature;
    float             min, max;

    switch (paramName) {
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_get_absolute_boundaries(vid->camera, feature, &min, &max) != DC1394_SUCCESS) {
        ARLOGe("unable to get min value.\n");
        return -1;
    }
    *value = (float)min;

    return 0;
}

int ar2VideoSetValue1394(AR2VideoParam1394T *vid, int paramName, int value)
{
    unsigned int ub, vr;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_BRIGHTNESS, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set brightness to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_EXPOSURE:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_EXPOSURE, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set exposure to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_UB:
            if (dc1394_feature_whitebalance_get_value(vid->camera, &ub, &vr) != DC1394_SUCCESS) {
                ARLOGe("unable to get white balance.\n");
                return -1;
            }
            if (dc1394_feature_whitebalance_set_value(vid->camera, (uint32_t)value, vr) != DC1394_SUCCESS) {
                ARLOGe("unable to set white balance.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_VR:
            if (dc1394_feature_whitebalance_get_value(vid->camera, &ub, &vr) != DC1394_SUCCESS) {
                ARLOGe("unable to get white balance.\n");
                return -1;
            }
            if (dc1394_feature_whitebalance_set_value(vid->camera, ub, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set white balance.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_SHUTTER, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set shutter speed to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAIN:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_GAIN, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set gain to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_FOCUS:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_FOCUS, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set focus to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAMMA:
            if (dc1394_feature_set_value(vid->camera, DC1394_FEATURE_GAMMA, (uint32_t)value) != DC1394_SUCCESS) {
                ARLOGe("unable to set gamma to %d.\n", value);
                return -1;
            }
            return 0;
    }

    return -1;
}

int ar2VideoGetValue1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    unsigned int ub, vr;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_BRIGHTNESS, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get brightness.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_EXPOSURE:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_EXPOSURE, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get exposure.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_UB:
            if (dc1394_feature_whitebalance_get_value(vid->camera, (uint32_t *)value, &vr) != DC1394_SUCCESS) {
                ARLOGe("unable to get white balance ub.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_VR:
            if (dc1394_feature_whitebalance_get_value(vid->camera, &ub, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get white balance vr.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_SHUTTER, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get shutter speed.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAIN:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_GAIN, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get gain.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_FOCUS:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_FOCUS, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get focus.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAMMA:
            if (dc1394_feature_get_value(vid->camera, DC1394_FEATURE_GAMMA, (uint32_t *)value) != DC1394_SUCCESS) {
                ARLOGe("unable to get gamma.\n");
                return -1;
            }
            return 0;
    }

    return -1;
}



int ar2VideoGetAutoOn1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    dc1394feature_t        feature;
    dc1394feature_mode_t   mode;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_get_mode(vid->camera, feature, &mode) != DC1394_SUCCESS) {
        ARLOGe("unable to check auto mode.\n");
        return -1;
    }

    if (mode == DC1394_FEATURE_MODE_AUTO) return 1;
    else                                  return 0;
}

int ar2VideoSetAutoOn1394(AR2VideoParam1394T *vid, int paramName, int value)
{
    dc1394feature_t        feature;
    dc1394feature_mode_t   mode;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (value) mode = DC1394_FEATURE_MODE_AUTO;
    else       mode = DC1394_FEATURE_MODE_MANUAL;
    if (dc1394_feature_set_mode(vid->camera, feature, mode) != DC1394_SUCCESS) {
        ARLOGe("unable to set mode.\n");
        return -1;
    }

    return 0;
}

int ar2VideoGetFeatureOn1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    dc1394feature_t   feature;
    dc1394switch_t    pwr;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_is_present(vid->camera, feature, (dc1394bool_t *)value) != DC1394_SUCCESS) {
        ARLOGe("unable to check feature.\n");
        return -1;
    }
    if (*value == 0) return 0;

    if (dc1394_feature_get_power(vid->camera, feature, &pwr) != DC1394_SUCCESS) {
        ARLOGe("unable to check feature.\n");
        return -1;
    }
    if (pwr == DC1394_OFF) *value = 0;
    else                   *value = 1;

    return 0;
}

int ar2VideoSetFeatureOn1394(AR2VideoParam1394T *vid, int paramName, int value)
{
    dc1394feature_t   feature;
    dc1394switch_t    pwr;
    dc1394bool_t      v;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_is_present(vid->camera, feature, &v) != DC1394_SUCCESS) {
        ARLOGe("unable to check feature.\n");
        return -1;
    }
    if (value && v == DC1394_FALSE) {
        ARLOGe("unable to set ON.\n");
        return -1;
    }

    if (value) pwr = DC1394_ON;
    else       pwr = DC1394_OFF;
    if (dc1394_feature_set_power(vid->camera, feature, pwr) != DC1394_SUCCESS) {
        ARLOGe("unable to set feature.\n");
        return -1;
    }

    return 0;
}

int ar2VideoGetMaxValue1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    dc1394feature_t   feature;
    uint32_t          min, max;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_get_boundaries(vid->camera, feature, &min, &max) != DC1394_SUCCESS) {
        ARLOGe("unable to get max value.\n");
        return -1;
    }
    *value = max;

    return 0;
}

int ar2VideoGetMinValue1394(AR2VideoParam1394T *vid, int paramName, int *value)
{
    dc1394feature_t   feature;
    uint32_t          min, max;

    switch (paramName) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = DC1394_FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = DC1394_FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = DC1394_FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = DC1394_FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = DC1394_FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = DC1394_FEATURE_FOCUS;
            break;
        case AR_VIDEO_1394_GAMMA:
            feature = DC1394_FEATURE_GAMMA;
            break;
        default:
            return -1;
    }

    if (dc1394_feature_get_boundaries(vid->camera, feature, &min, &max) != DC1394_SUCCESS) {
        ARLOGe("unable to get max value.\n");
        return -1;
    }
    *value = min;

    return 0;
}

int ar2VideoSaveParam1394(AR2VideoParam1394T *vid, char *filename)
{
    FILE    *fp;
    int     value;

    if ((fp=fopen(filename, "w")) == NULL) return -1;

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS, &value);
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE, &value);
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, &value);
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_UB\t%d\n", value);
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, &value);
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_VR\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, &value);
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_GAIN_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_GAIN_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN, &value);
            fprintf(fp, "AR_VIDEO_1394_GAIN\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_GAIN_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_GAIN_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS_FEATURE_ON, &value);
    if (value == 1) {
        fprintf(fp, "AR_VIDEO_1394_FOCUS_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS_AUTO_ON, &value);
        if (value == 0) {
            fprintf(fp, "AR_VIDEO_1394_FOCUS_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS, &value);
            fprintf(fp, "AR_VIDEO_1394_FOCUS\t%d\n", value);
        } else {
            fprintf(fp, "AR_VIDEO_1394_FOCUS_AUTO_ON\t1\n");
        }
    } else {
        fprintf(fp, "AR_VIDEO_1394_FOCUS_FEATURE_ON\t0\n");
    }

    fclose(fp);

    return 0;
}

int ar2VideoLoadParam1394(AR2VideoParam1394T *vid, char *filename)
{
    FILE    *fp;
    int     value;
    char    buf[512], buf1[512];
    int     ret = 0;

    if ((fp = fopen(filename, "r")) == NULL) return -1;

    for (;;) {
        if (fgets(buf, 512, fp) == NULL) break;
        if (buf[0] == '#' || buf[0] == '\n') continue;
        buf1[0] = '\0';
        if (sscanf(buf, "%s %d", buf1, &value) != 2) {
            if (buf1[0] == '\0') continue;
            ARLOGe("Error: %s\n", buf);
            ret = -1;
            continue;
        }

        if (strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_EXPOSURE_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_EXPOSURE") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_UB") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_VR") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_GAIN_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_GAIN_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_GAIN") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_FOCUS_FEATURE_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS_FEATURE_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_FOCUS_AUTO_ON") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS_AUTO_ON, value);
        } else if (strcmp(buf1, "AR_VIDEO_1394_FOCUS") == 0) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS, value);
        } else {
            ARLOGe("Unknown command: %s\n", buf1);
            ret = -1;
            continue;
        }
    }

    fclose(fp);

    return ret;
}

#endif // ARVIDEO_INPUT_LIBDC1394

