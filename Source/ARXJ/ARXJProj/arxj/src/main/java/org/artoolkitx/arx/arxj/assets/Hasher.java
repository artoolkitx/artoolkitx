/*
 *  Hasher.java
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

package org.artoolkitx.arx.arxj.assets;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.zip.CRC32;

//import android.util.Log;

public class Hasher {

    //private final static String TAG = "Hasher";

    private final static String HEX = "0123456789ABCDEF";

    public static String toHex(byte[] buf) {

        if (buf == null) return "";

        StringBuffer result = new StringBuffer(2 * buf.length);

        for (int i = 0; i < buf.length; i++) {
            result.append(HEX.charAt((buf[i] >> 4) & 0x0f)).append(HEX.charAt(buf[i] & 0x0f));
        }

        return result.toString();
    }

    public static long computeCRC(String filename) throws HashComputationException {

        InputStream in = null;
        byte[] buffer = new byte[16384];
        int bytesRead = -1;
        CRC32 crc = new CRC32();

        try {
            in = new FileInputStream(filename);
        } catch (FileNotFoundException fnfe) {
            throw new HashComputationException("File not found: " + filename, fnfe);
        }

        //long crcStartTime = System.nanoTime();

        try {
            while ((bytesRead = in.read(buffer)) != -1) crc.update(buffer, 0, bytesRead);
            in.close();
            in = null;
        } catch (IOException ioe) {
            throw new HashComputationException("IOException while reading from file", ioe);
        }

        //long elapsedTime = System.nanoTime() - crcStartTime;
        //Log.i(TAG, "CRC time: " + (elapsedTime / 1000000.0f) + " ms");

        //Log.i(TAG, "CRC result of " + filename + ": " + value);

        return crc.getValue();

    }


    public static String computeHash(String filename) throws HashComputationException, IOException {

        InputStream in = null;
        MessageDigest digest = null;
        String algorithm = "SHA-1";
        byte[] buffer = new byte[16384];
        int bytesRead = -1;


        try {
            in = new FileInputStream(filename);
        } catch (FileNotFoundException fnfe) {
            throw new HashComputationException("File not found: " + filename, fnfe);
        }


        try {
            digest = MessageDigest.getInstance(algorithm);
        } catch (NoSuchAlgorithmException nsae) {
            try {
                in.close();
            } catch (IOException e) {
                throw e;
            }
            in = null;
            throw new HashComputationException("No such algorithm: " + algorithm, nsae);
        }

        //long hashStartTime = System.nanoTime();

        try {
            while ((bytesRead = in.read(buffer)) != -1) digest.update(buffer, 0, bytesRead);
            in.close();
            in = null;
        } catch (IOException ioe) {
            throw new HashComputationException("IOException while reading from file", ioe);
        }

        byte[] digestResult = digest.digest();

        //long elapsedTime = System.nanoTime() - hashStartTime;
        //Log.i(TAG, "Hash time: " + (elapsedTime / 1000000.0f) + " ms");

        String hash = toHex(digestResult);
        //Log.i(TAG, "Hash result of " + filename + ": " + hash);

        return hash;

    }

}
