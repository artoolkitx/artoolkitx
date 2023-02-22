/*
 *  AndroidUtils.java
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

package org.artoolkitx.arx.arxj;

import android.app.Activity;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.support.annotation.NonNull;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;

import org.artoolkitx.arx.arxj.camera.CameraAccessHandler;
import org.artoolkitx.arx.arxj.camera.CameraAccessHandlerImpl;

import java.io.File;
import java.text.DecimalFormat;

/**
 * A collection of utility functions for performing common and useful operations
 * specific to Android.
 */
@SuppressWarnings("WeakerAccess")
public class AndroidUtils {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = "AndroidUtils";

    public static final int VIEW_VISIBILITY = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;

    /**
     * Returns a String summarising the Android build information.
     *
     * @return The Android build information.
     */
    public static String androidBuildVersion() {

        String buf = ("Version\n *Release: " + Build.VERSION.RELEASE) +    // The user-visible version string. E.g., "1.0" or "3.4b5".
                "\n Incremental: " + Build.VERSION.INCREMENTAL +    // The internal value used by the underlying source control to represent this build.
                "\n Codename: " + Build.VERSION.CODENAME +        // The current development codename, or the string "REL" if this is a release build.
                "\n SDK: " + Build.VERSION.SDK_INT +                // The user-visible SDK version of the framework.
                "\n\n*Model: " + Build.MODEL +                    // The end-user-visible name for the end product..
                "\nManufacturer: " + Build.MANUFACTURER +        // The manufacturer of the product/hardware.
                "\nBoard: " + Build.BOARD +                        // The name of the underlying board, like "goldfish".
                "\nBrand: " + Build.BRAND +                        // The brand (e.g., carrier) the software is customized for, if any.
                "\nDevice: " + Build.DEVICE +                    // The name of the industrial design.
                "\nProduct: " + Build.PRODUCT +                    // The name of the overall product.
                "\nHardware: " + Build.HARDWARE +                // The name of the hardware (from the kernel command line or /proc).
                "\nCPU ABI: " + Build.CPU_ABI +                    // The name of the instruction set (CPU type + ABI convention) of native code.
                "\nCPU second ABI: " + Build.CPU_ABI2 +            // The name of the second instruction set (CPU type + ABI convention) of native code.
                "\n\n*Displayed ID: " + Build.DISPLAY +            // A build ID string meant for displaying to the user.
                "\nHost: " + Build.HOST +
                "\nUser: " + Build.USER +
                "\nID: " + Build.ID +                            // Either a changelist number, or a label like "M4-rc20".
                "\nType: " + Build.TYPE +                        // The type of build, like "user" or "eng".
                "\nTags: " + Build.TAGS +                        // Comma-separated tags describing the build, like "unsigned,debug".
                "\n\nFingerprint: " + Build.FINGERPRINT +        // A string that uniquely identifies this build. 'BRAND/PRODUCT/DEVICE:RELEASE/ID/VERSION.INCREMENTAL:TYPE/TAGS'.
                "\n\nItems with * are intended for display to the end user.";

        return buf;
    }

    /**
     * Returns whether or not there is an SD card mounted.
     *
     * @return true if an SD card is mounted, otherwise false.
     */
    public static boolean isSDCardMounted() {
        return android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
    }


    /**
     * Returns the number of bytes of external storage available.
     *
     * @return Bytes of external storage available.
     */
    static public long getAvailableExternalMemorySize() {
        if (isSDCardMounted()) {
            File path = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(path.getPath());
            long blockSize = stat.getBlockSize();
            long availableBlocks = stat.getAvailableBlocks();
            return availableBlocks * blockSize;
        } else {
            return -1;
        }
    }

    /**
     * Returns the total number of bytes of external storage.
     *
     * @return Bytes of external storage in total, or -1 if no SD card mounted.
     */
    static public long getTotalExternalMemorySize() {
        if (isSDCardMounted()) {
            File path = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(path.getPath());
            long blockSize = stat.getBlockSize();
            long totalBlocks = stat.getBlockCount();
            return totalBlocks * blockSize;
        } else {
            return -1;
        }
    }


    /**
     * Returns a formatted string representation of the number of bytes specified. The largest
     * suitable suffix up until GB will be used, with the returned value expressed to two
     * decimal places.
     *
     * @param bytes The number of bytes to be reported.
     * @return The specified number of bytes, formatted as a string, in bytes, KB, MB or GB.
     */
    static public String formatBytes(long bytes) {

        double val = 0;
        String units = "";

        if (bytes < 1024) {
            val = bytes;
            units = "bytes";
        } else if (bytes < 1048576) {
            val = (bytes / 1024.0f);
            units = "KB";
        } else if (bytes < 1073741824) {
            val = (bytes / 1048576.0f);
            units = "MB";
        } else {
            val = (bytes / 1073741824.0f);
            units = "GB";
        }

        DecimalFormat df = new DecimalFormat("###.##");
        return df.format(val) + " " + units;

    }

    /**
     * Reports to the log information about the device's display. This information includes
     * the width and height, and density (low, medium, high).
     *
     * @param activity The Activity to report on.
     */
    public static void reportDisplayInformation(Activity activity) {

        DisplayMetrics metrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        int displayWidth = metrics.widthPixels;
        int displayHeight = metrics.heightPixels;

        String density = "unknown";
        switch (metrics.densityDpi) {
            case DisplayMetrics.DENSITY_LOW:
                density = "Low";
                break;
            case DisplayMetrics.DENSITY_MEDIUM:
                density = "Medium";
                break;
            case DisplayMetrics.DENSITY_HIGH:
                density = "High";
                break;
        }

        Log.i(TAG, "reportDisplayInformation(): Display is " + displayWidth + "x" + displayHeight
                + ", Density: " + density);

    }
}
