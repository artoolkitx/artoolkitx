/*
 *  AssetFileTransfer.java
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

import android.content.res.AssetManager;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class AssetFileTransfer {

    private static final String TAG = "AssetFileTransfer";

    private File assetFile;
    private boolean assetAvailable;

    public File targetFile;
    private File targetDirectory;

    private boolean targetFileAlreadyExists;
    private long targetFileCRC;
    private File tempFile;
    private long tempFileCRC;


    private boolean
            assetCopied;

    private void copyContents(InputStream in, OutputStream out) throws IOException {

        final int bufferSize = 16384;
        byte[] buffer = new byte[bufferSize];

        int bytesRead;
        while ((bytesRead = in.read(buffer)) != -1) {
            out.write(buffer, 0, bytesRead);
        }

        out.flush();
    }


    public void copyAssetToTargetDir(AssetManager manager, String assetFilePath, String targetDirPath) throws AssetFileTransferException {

        assetFile = new File(assetFilePath);

        InputStream in;
        OutputStream out;

        try {
            in = manager.open(assetFilePath);
            assetAvailable = true;
        } catch (IOException e) {
            assetAvailable = false;
            throw new AssetFileTransferException("Unable to open the asset file: " + assetFilePath, e);
        }

        targetFile = new File(targetDirPath, assetFilePath);
        targetFileAlreadyExists = targetFile.exists();

        Log.i(TAG, "copyAssetToTargetDir(): [" + assetFilePath + "] -> [" + targetFile.getPath() + "]");

        if (targetFileAlreadyExists) {

            //Log.i(TAG, "Target file exists. Unpacking to temporary file first.");

            // Create temporary file to unpack to
            try {
                tempFile = File.createTempFile("unpacker", null, Environment.getExternalStorageDirectory());
                //Log.i(TAG, "Created temp file for unpacking: " + tempFile.getPath());
            } catch (IOException ioe) {
                throw new AssetFileTransferException("Error creating temp file: " + tempFile.getPath(), ioe);
            }

            // Copy asset to temporary file
            try {
                out = new FileOutputStream(tempFile);
            } catch (FileNotFoundException fnfe) {
                throw new AssetFileTransferException("Error creating temp file: " + tempFile.getPath(), fnfe);
            }
            try {
                copyContents(in, out);
                in.close();
                in = null;
                out.close();
                out = null;
            } catch (IOException ioe) {
                throw new AssetFileTransferException("Error copying asset to temp file: " + tempFile.getPath(), ioe);
            }

            // Get hashes for new temporary file and existing file
            try {

                //tempFileHash = Hasher.computeHash(tempFile.getPath());
                tempFileCRC = Hasher.computeCRC(tempFile.getPath());

                //targetFileHash = Hasher.computeHash(targetFile.getPath());
                targetFileCRC = Hasher.computeCRC(targetFile.getPath());

            } catch (HashComputationException hce) {
                throw new AssetFileTransferException("Error hashing files", hce);
            }

            if (tempFileCRC == targetFileCRC) {

                // The hashes match. The files are the same, so don't need to do anything.
                //Log.i(TAG, "The hashes match. Keeping existing file, removing temp file.");
                // Clean up temporary file
                tempFile.delete();

            } else {

                // The hashes do not match. Overwrite the existing file with the new one.
                targetFile.delete();
                //Log.i(TAG, "Deleted existing file");
                tempFile.renameTo(targetFile);
                //Log.i(TAG, "Moved temp file: " + tempFile.getPath() + " to " + targetFile.getPath());
                assetCopied = true;
            }

        } else {

            Log.i(TAG, "copyAssetToTargetDir(): Target file does not exist. Creating directory structure.");

            // Ensure parent directories exist so we can create the file
            targetDirectory = targetFile.getParentFile();
            targetDirectory.mkdirs();

            // Copy asset to target file
            try {
                out = new FileOutputStream(targetFile);
            } catch (FileNotFoundException fnfe) {
                throw new AssetFileTransferException("Error creating target file: " + targetFile.getPath(), fnfe);
            }
            try {
                copyContents(in, new FileOutputStream(targetFile));
                //Log.i(TAG, "Copied asset to target file");

                in.close();
                in = null;
                out.close();
                out = null;
            } catch (IOException ioe) {
                throw new AssetFileTransferException("Error copying asset to target file: " + targetFile.getPath(), ioe);
            }
            assetCopied = true;
        }
    }
}