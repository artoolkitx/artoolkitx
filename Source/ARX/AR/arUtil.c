/*
 *  arUtil.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 3.2
 * Date: 03/08/13
 *
 *******************************************************/

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#include <ARX/AR/ar.h>
#include <math.h>
#include <ctype.h>    // tolower()
#ifdef _WIN32
#  include <direct.h> // chdir(), getcwd()
#  ifndef _WINRT
#    include <shlobj.h> // SHGetFolderPath()
#  endif
#  define MAXPATHLEN MAX_PATH
#else
#  include <unistd.h> // chdir(), getcwd(), confstr()
#  include <sys/param.h> // MAXPATHLEN
#  include <pthread.h> // pthread_self(), pthread_equal()
#  ifdef __APPLE__
#    include <CoreFoundation/CoreFoundation.h>
#    include <mach-o/dyld.h> // _NSGetExecutablePath()
#    ifdef __OBJC__
#      import <Foundation/Foundation.h> // NSURL. N.B.: Including Foundation requires that this file be compiled as Objective-C.
#    else
#      warning arUtil.c not compiled as Objective C. Behaviour of function arUtilGetResourcesDirectoryPath will be different.
#    endif
#  endif
#endif
#include <ARX/ARUtil/file_utils.h>

// These are the load/unload handlers for the case when libAR is
// loaded as a native library by a Java virtual machine (e.g. when
// running on Android.
#ifdef ANDROID

// To call Java methods when running native code inside an Android activity,
// a reference is needed to the JavaVM.
static JavaVM *gJavaVM;

static char _AndroidDeviceID[32] = { '\0' };

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    gJavaVM = vm;
    return JNI_VERSION_1_6;
}

/*JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    // Here is the right place to release statically-allocated resources,
    // including any weak global references created in JNI_OnLoad().
    // N.B. This routine is called from an arbitrary thread, and Java-side
    // resources already invalid, so don't do any locking or class ops.
}*/

#endif // ANDROID

ARUint32 arGetVersion(char **versionStringRef)
{
	const char version[] = AR_HEADER_VERSION_STRING;
	char *s;

	if (versionStringRef) {
		arMalloc(s, char, sizeof(version));
		strncpy(s, version, sizeof(version));
		*versionStringRef = s;
	}
	// Represent full version number (major, minor, tiny, build) in
	// binary coded decimal. N.B: Integer division.
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

int arUtilGetSquareCenter( ARdouble vertex[4][2], ARdouble *x, ARdouble *y )
{
    ARdouble   x4x2, x3x1, x2x1, x1, x2;
    ARdouble   y4y2, y3y1, y2y1, y1, y2;
    ARdouble   w;

    x4x2 = vertex[3][0] - vertex[1][0];
    x3x1 = vertex[2][0] - vertex[0][0];
    x2x1 = vertex[1][0] - vertex[0][0];
    x2   = vertex[1][0];
    x1   = vertex[0][0];

    y4y2 = vertex[3][1] - vertex[1][1];
    y3y1 = vertex[2][1] - vertex[0][1];
    y2y1 = vertex[1][1] - vertex[0][1];
    y2   = vertex[1][1];
    y1   = vertex[0][1];

    w = y3y1 * x4x2 - y4y2 * x3x1;
    if( w == 0.0 ) return -1;

    *x = ( y2y1 * x3x1 * x4x2 + y3y1 * x4x2 * x1 - y4y2 * x3x1 * x2 ) / w;
    *y = ( x4x2 * y3y1 * y2 - x2x1 * y3y1 * y4y2 - x3x1 * y4y2 * y1 ) / w;

    return 0;
}

int arUtilMatMul( const ARdouble s1[3][4], const ARdouble s2[3][4], ARdouble d[3][4] )
{
    int     i, j;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++) {
            d[j][i] = s1[j][0] * s2[0][i]
                    + s1[j][1] * s2[1][i]
                    + s1[j][2] * s2[2][i];
        }
        d[j][3] += s1[j][3];
    }

    return 0;
}

#ifndef ARDOUBLE_IS_FLOAT
int arUtilMatMuldff( const ARdouble s1[3][4], const float s2[3][4], float d[3][4] )
{
    int     i, j;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++) {
            d[j][i] = (float)s1[j][0] * s2[0][i]
            + (float)s1[j][1] * s2[1][i]
            + (float)s1[j][2] * s2[2][i];
        }
        d[j][3] += (float)s1[j][3];
    }

    return 0;
}

int arUtilMatMulf( const float s1[3][4], const float s2[3][4], float d[3][4] )
{
    int     i, j;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++) {
            d[j][i] = s1[j][0] * s2[0][i]
            + s1[j][1] * s2[1][i]
            + s1[j][2] * s2[2][i];
        }
        d[j][3] += s1[j][3];
    }

    return 0;
}
#endif

int arUtilMatInv( const ARdouble s[3][4], ARdouble d[3][4] )
{
    ARMat       *mat;
    int         i, j;

    mat = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat->m[j*4+i] = s[j][i];
        }
    }
    mat->m[3*4+0] = 0; mat->m[3*4+1] = 0;
    mat->m[3*4+2] = 0; mat->m[3*4+3] = 1;
    arMatrixSelfInv( mat );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            d[j][i] = mat->m[j*4+i];
        }
    }
    arMatrixFree( mat );

    return 0;
}

#ifndef ARDOUBLE_IS_FLOAT
int arUtilMatInvf( const float s[3][4], float d[3][4] )
{
    ARMat       *mat;
    int         i, j;

    mat = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat->m[j*4+i] = (ARdouble)s[j][i];
        }
    }
    mat->m[3*4+0] = 0; mat->m[3*4+1] = 0;
    mat->m[3*4+2] = 0; mat->m[3*4+3] = 1;
    arMatrixSelfInv( mat );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            d[j][i] = (float)mat->m[j*4+i];
        }
    }
    arMatrixFree( mat );

    return 0;
}
#endif

int arUtilMat2QuatPos( const ARdouble m[3][4], ARdouble q[4], ARdouble p[3] )
{
    ARdouble   t, s;

#ifdef ARDOUBLE_IS_FLOAT
    t = m[0][0] + m[1][1] + m[2][2] + 1.0f;
    if (t > 0.0001f) {
        s = sqrtf(t) * 2.0f;
        q[0] = (m[1][2] - m[2][1]) / s;
        q[1] = (m[2][0] - m[0][2]) / s;
        q[2] = (m[0][1] - m[1][0]) / s;
        q[3] = 0.25f * s;
    } else {
        if (m[0][0] > m[1][1] && m[0][0] > m[2][2])  {	// Column 0:
            s  = sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
            q[0] = 0.25f * s;
            q[1] = (m[0][1] + m[1][0] ) / s;
            q[2] = (m[2][0] + m[0][2] ) / s;
            q[3] = (m[1][2] - m[2][1] ) / s;
        } else if (m[1][1] > m[2][2]) {			// Column 1:
            s  = sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
            q[0] = (m[0][1] + m[1][0] ) / s;
            q[1] = 0.25f * s;
            q[2] = (m[1][2] + m[2][1] ) / s;
            q[3] = (m[2][0] - m[0][2] ) / s;
        } else {						// Column 2:
            s  = sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
            q[0] = (m[2][0] + m[0][2] ) / s;
            q[1] = (m[1][2] + m[2][1] ) / s;
            q[2] = 0.25f * s;
            q[3] = (m[0][1] - m[1][0] ) / s;
        }
    }
#else
    t = m[0][0] + m[1][1] + m[2][2] + 1.0;
    if (t > 0.0001) {
        s = sqrt(t) * 2.0;
        q[0] = (m[1][2] - m[2][1]) / s;
        q[1] = (m[2][0] - m[0][2]) / s;
        q[2] = (m[0][1] - m[1][0]) / s;
        q[3] = 0.25 * s;
    } else {
        if (m[0][0] > m[1][1] && m[0][0] > m[2][2])  {	// Column 0:
            s  = sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2.0;
            q[0] = 0.25 * s;
            q[1] = (m[0][1] + m[1][0] ) / s;
            q[2] = (m[2][0] + m[0][2] ) / s;
            q[3] = (m[1][2] - m[2][1] ) / s;
        } else if (m[1][1] > m[2][2]) {			// Column 1:
            s  = sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2.0;
            q[0] = (m[0][1] + m[1][0] ) / s;
            q[1] = 0.25 * s;
            q[2] = (m[1][2] + m[2][1] ) / s;
            q[3] = (m[2][0] - m[0][2] ) / s;
        } else {						// Column 2:
            s  = sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2.0;
            q[0] = (m[2][0] + m[0][2] ) / s;
            q[1] = (m[1][2] + m[2][1] ) / s;
            q[2] = 0.25 * s;
            q[3] = (m[0][1] - m[1][0] ) / s;
        }
    }
#endif

    p[0] = m[0][3];
    p[1] = m[1][3];
    p[2] = m[2][3];

    return 0;
}

int arUtilQuatPos2Mat( const ARdouble q[4], const ARdouble p[3], ARdouble m[3][4] )
{
    ARdouble    x2, y2, z2;
    ARdouble    xx, xy, xz;
    ARdouble    yy, yz, zz;
    ARdouble    wx, wy, wz;

#ifdef ARDOUBLE_IS_FLOAT
    x2 = q[0] * 2.0f;
    y2 = q[1] * 2.0f;
    z2 = q[2] * 2.0f;
#else
    x2 = q[0] * 2.0;
    y2 = q[1] * 2.0;
    z2 = q[2] * 2.0;
#endif
    xx = q[0] * x2;
    xy = q[0] * y2;
    xz = q[0] * z2;
    yy = q[1] * y2;
    yz = q[1] * z2;
    zz = q[2] * z2;
    wx = q[3] * x2;
    wy = q[3] * y2;
    wz = q[3] * z2;

#ifdef ARDOUBLE_IS_FLOAT
    m[0][0] = 1.0f - (yy + zz);
    m[1][1] = 1.0f - (xx + zz);
    m[2][2] = 1.0f - (xx + yy);
#else
    m[0][0] = 1.0 - (yy + zz);
    m[1][1] = 1.0 - (xx + zz);
    m[2][2] = 1.0 - (xx + yy);
#endif
    m[1][0] = xy - wz;
    m[0][1] = xy + wz;
    m[2][0] = xz + wy;
    m[0][2] = xz - wy;
    m[2][1] = yz - wx;
    m[1][2] = yz + wx;

    m[0][3] = p[0];
    m[1][3] = p[1];
    m[2][3] = p[2];

    return 0;
}

int arUtilQuatNorm(ARdouble q[4])
{
    ARdouble mag2, mag;

    // Normalise quaternion.
    mag2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
    if (!mag2) return (-1);
#ifdef ARDOUBLE_IS_FLOAT
    mag = sqrtf(mag2);
#else
    mag = sqrt(mag2);
#endif
    q[0] /= mag;
    q[1] /= mag;
    q[2] /= mag;
    q[3] /= mag;

    return (0);
}

// N.B. This function is duplicated in libARvideo, so that libARvideo doesn't need to
// link to libAR. Therefore, if changes are made here they should be duplicated there.
int arUtilGetPixelSize( const AR_PIXEL_FORMAT arPixelFormat )
{
    switch( arPixelFormat ) {
        case AR_PIXEL_FORMAT_RGB:
        case AR_PIXEL_FORMAT_BGR:
            return 3;
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_ABGR:
        case AR_PIXEL_FORMAT_ARGB:
            return 4;
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420v: // Report only size of luma pixels (i.e. plane 0).
        case AR_PIXEL_FORMAT_420f: // Report only size of luma pixels (i.e. plane 0).
        case AR_PIXEL_FORMAT_NV21: // Report only size of luma pixels (i.e. plane 0).
            return 1;
        case AR_PIXEL_FORMAT_2vuy:
        case AR_PIXEL_FORMAT_yuvs:
        case AR_PIXEL_FORMAT_RGB_565:
        case AR_PIXEL_FORMAT_RGBA_5551:
        case AR_PIXEL_FORMAT_RGBA_4444:
            return 2;
        default:
            return (0);
    }
}

// N.B. This function is duplicated in libARvideo, so that libARvideo doesn't need to
// link to libAR. Therefore, if changes are made here they should be duplicated there.
const char *arUtilGetPixelFormatName(const AR_PIXEL_FORMAT arPixelFormat)
{
    const char *names[] = {
        "AR_PIXEL_FORMAT_RGB",
        "AR_PIXEL_FORMAT_BGR",
        "AR_PIXEL_FORMAT_RGBA",
        "AR_PIXEL_FORMAT_BGRA",
        "AR_PIXEL_FORMAT_ABGR",
        "AR_PIXEL_FORMAT_MONO",
        "AR_PIXEL_FORMAT_ARGB",
        "AR_PIXEL_FORMAT_2vuy",
        "AR_PIXEL_FORMAT_yuvs",
        "AR_PIXEL_FORMAT_RGB_565",
        "AR_PIXEL_FORMAT_RGBA_5551",
        "AR_PIXEL_FORMAT_RGBA_4444",
        "AR_PIXEL_FORMAT_420v",
        "AR_PIXEL_FORMAT_420f",
        "AR_PIXEL_FORMAT_NV21"
    };
    if ((int)arPixelFormat < 0 || (int)arPixelFormat > AR_PIXEL_FORMAT_MAX) {
        ARLOGe("arUtilGetPixelFormatName: Error, unrecognised pixel format (%d).\n", (int)arPixelFormat);
        return (NULL);
    }
    return (names[(int)arPixelFormat]);
}

const char *arUtilGetFileNameFromPath(const char *path)
{
	char *sep;
#ifdef _WIN32
    char *sep1;
#endif

    if (!path) return (NULL);
    if (!*path) return (NULL);

	sep = strrchr(path, '/');
#ifdef _WIN32
    sep1 = strrchr(path, '\\');
    if (sep1 > sep) sep = sep1;
#endif

	if (!sep) return (path);
	else return (sep + 1);
}

char *arUtilGetFileBasenameFromPath(const char *path, const int convertToLowercase)
{
    const char *file;
    char *sep;
    size_t len;
    char *ret;
    int i;

    if (!path || !*path) return (NULL);

    file = arUtilGetFileNameFromPath(path);
    sep = strrchr(file, '.');
    if (!sep) return (strdup(file));

    len = sep - file;
    ret = (char *)malloc(len + 1);
    if (!ret) {
        fprintf(stderr, "Out of memory.\n");
        return (NULL);
    }

    if (convertToLowercase) {
        for (i = 0; i < len; i++) ret[i] = tolower(file[i]);
    } else {
        for (i = 0; i < len; i++) ret[i] = file[i];
    }
    ret[i] = '\0';

    return (ret);
}

char *arUtilGetFileExtensionFromPath(const char *path, const int convertToLowercase)
{
    char *sep;
    size_t len;
    char *ret;
    int i;

    if (!path || !*path) return (NULL);

    sep = strrchr(arUtilGetFileNameFromPath(path), '.');
    if (!sep) return (NULL);

    sep++; // Move past '.'
    if (!*sep) return (NULL);

    len = strlen(sep);
    ret = (char *)malloc(len + 1);
    if (!ret) {
        fprintf(stderr, "Out of memory.\n");
        return (NULL);
    }

    if (convertToLowercase) {
        for (i = 0; i < len; i++) ret[i] = tolower(sep[i]);
    } else {
        for (i = 0; i < len; i++) ret[i] = sep[i];
    }
    ret[i] = '\0';

    return (ret);
}

char *arUtilGetDirectoryNameFromPath(char *dir, const char *path, const size_t n, const int addSeparator)
{
	char *sep;
#ifdef _WIN32
    char *sep1;
#endif
    size_t toCopy;

    if (!dir || !path || !n) return (NULL);

	sep = strrchr(path, '/');
#ifdef _WIN32
    sep1 = strrchr(path, '\\');
    if (sep1 > sep) sep = sep1;
#endif

	if (!sep) dir[0] = '\0';
    else {
        toCopy = sep + (addSeparator ? 1 : 0) - path;
        if (toCopy + 1 > n) return (NULL); // +1 because we need space for null-terminator.
        strncpy(dir, path, toCopy); // strlen(path) >= toCopy, so won't ever be null-terminated.
        dir[toCopy] = '\0';
    }
	return dir;
}

char *arUtilGetFileURI(const char *path)
{
    const char method[] = "file://";
    char *abspath = NULL;
    char *uri = NULL;
    size_t pathlen, abspathlen = 0, urilen;
    int isAbsolute; // bool
#ifdef _WIN32
    int isUNC; // bool
#endif
    int i;

    if (!path) return (NULL);
    if (!*path) return (NULL);

    pathlen = strlen(path);

    // First check if we've been passed an absolute path.
    isAbsolute = FALSE;
#ifdef _WIN32
    // Windows has two styles of absolute paths. The first (local Windows
    // file path) begins with a drive letter e.g. C:, the second (UNC Windows)
    // with a double backslash e.g. \\.
    if (pathlen >= 2) {
        if (isalpha(path[0]) && path[1] == ':') isAbsolute = TRUE;
        else if (path[0] == '\\' && path[1] == '\\') isAbsolute = TRUE;
    }
#else
    if (path[0] == '/') isAbsolute = TRUE;
#endif

    // Ensure we have an absolute path.
    if (isAbsolute) {
        abspath = (char *)path;
    } else {
#ifdef _WINRT
		ARLOGe("Error: relative paths not supported by Windows Runtime.\n");
		return (NULL);
#else
        // For relative paths, concatenate with the current working directory.
        abspath = (char *)calloc(MAXPATHLEN, sizeof(char));
        if (!abspath) return (NULL);
        if (!getcwd(abspath, MAXPATHLEN)) goto bail;
        abspathlen = strlen(abspath);
        if (abspathlen < 1) goto bail;
        // Ensure current working directory path has a trailing slash.
#  ifdef _WIN32
        if (abspath[abspathlen - 1] != '/' && abspath[abspathlen - 1] != '\\' )
#  else
        if (abspath[abspathlen - 1] != '/')
#  endif
        {
            abspath[abspathlen++] = '/'; abspath[abspathlen] = '\0';
        }
        if (abspathlen + pathlen >= MAXPATHLEN) goto bail;
        strncpy(abspath + abspathlen, path, MAXPATHLEN - abspathlen - 1); abspath[MAXPATHLEN - 1] = '\0';
#endif
    }

#ifdef _WIN32
    // Windows UNC paths will be stripped of the leading two backslashes.
    if (abspath[0] == '\\' && abspath[1] == '\\') isUNC = TRUE;
    else isUNC = FALSE;
#endif

    // First pass. Work out how big everything needs to be.
    urilen = sizeof(method) - 1; // Begin with "file://". Don't count the nul terminator.
#ifdef _WIN32
    if (isUNC) i = 2;
    else {
        urilen++; // Prepend a slash.
        i = 0;
    }
#else
    i = 0;
#endif
    while (abspath[i]) {
        // Characters not to URL encode.
        if ((abspath[i] == '/') || (abspath[i] >= 'A' && abspath[i] <= 'Z') || (abspath[i] >= 'a' && abspath[i] <= 'z') || (abspath[i] >= '0' && abspath[i] <= '9') || (abspath[i] == '-') || (abspath[i] == '.') || (abspath[i] == '_') || (abspath[i] == '~')) {
            urilen++;
#ifdef _WIN32
        // On Windows only, backslashes will be converted to forward slashes.
        } else if (abspath[i] == '\\') {
            urilen++;
        // On Windows only, a colon which is part of a drive letter will not be encoded.
        } else if (i == 1 && abspath[i] == ':' && isalpha(abspath[i-1])) {
            urilen++;
#endif
        } else {
            urilen += 3; // URL encoded char is 3 chars.
        }
        i++;
    }
    urilen++; // nul termination.
    uri = (char *)malloc(urilen * sizeof(char));

    // Second pass. Construct the URI.
    sprintf(uri, method);
    urilen = sizeof(method) - 1;
#ifdef _WIN32
    if (isUNC) i = 2;
    else {
        uri[urilen++] = '/'; // Prepend a slash.
        i = 0;
    }
#else
    i = 0;
#endif
    while (abspath[i]) {
        // Characters not to URL encode.
        if ((abspath[i] == '/') || (abspath[i] >= 'A' && abspath[i] <= 'Z') || (abspath[i] >= 'a' && abspath[i] <= 'z') || (abspath[i] >= '0' && abspath[i] <= '9') || (abspath[i] == '-') || (abspath[i] == '.') || (abspath[i] == '_') || (abspath[i] == '~')) {
            uri[urilen++] = abspath[i];
#ifdef _WIN32
        } else if (abspath[i] == '\\') {
            uri[urilen++] = '/';
        } else if (i == 1 && abspath[i] == ':' && isalpha(abspath[i-1])) {
            uri[urilen++] = ':';
#endif
        } else {
            sprintf(uri + urilen, "%%%02x", abspath[i]);
            urilen += 3; // URL encoded char is 3 chars.
        }
        i++;
    }
    uri[urilen] = '\0'; // nul termination.

#ifndef _WINRT
bail:
    if (!isAbsolute) free(abspath);
#endif

    return (uri);
}

#ifdef ANDROID
static char *getFilePath(JNIEnv *env, jobject objectFile)
{
    // Call method on File object to retrieve String object.
    jclass classFile = (*env)->GetObjectClass(env, objectFile);
    if (!classFile) goto bail;
    jmethodID methodIDgetAbsolutePath = (*env)->GetMethodID(env, classFile, "getAbsolutePath", "()Ljava/lang/String;");
    if (!methodIDgetAbsolutePath) goto bail;
    jstring stringPath = (*env)->CallObjectMethod(env, objectFile, methodIDgetAbsolutePath);
    jthrowable exception = (*env)->ExceptionOccurred(env);
    if (exception) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
    // Extract a C string from the String object.
    const char *wpath2 = (*env)->GetStringUTFChars(env, stringPath, NULL);
    if (!wpath2) goto bail;
    char *wpath1 = strdup(wpath2);
    (*env)->ReleaseStringUTFChars(env, stringPath, wpath2);
    return wpath1;
bail:
    ARLOGe("Error: JNI call failure.\n");
    return (NULL);
}
#endif

#ifdef ANDROID
char *arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, jobject instanceOfAndroidContext)
#else
char *arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior)
#endif
{
#ifndef _WINRT
    char *wpath1 = NULL;
    char *wpath2 = NULL;
#  ifdef _WIN32
	DWORD len;
#  endif
#endif
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behaviorW;

    if (behavior == AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST) {
#if defined(__APPLE__) || (defined(__linux) && !defined(ANDROID))
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR;
#elif defined(ANDROID)
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR;
#elif defined(_WIN32)
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_EXECUTABLE_DIR;
#else
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_CWD;
#endif
    } else {
        behaviorW = behavior;
    }

	switch (behaviorW) {

        //
        // EXECUTABLE_DIR
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_EXECUTABLE_DIR:
        {
#if (defined(_WIN32) && !defined(_WINRT)) || defined(__APPLE__) || defined(__linux)
            arMallocClear(wpath1, char, MAXPATHLEN);
#  if defined(_WIN32)
            len = GetModuleFileName(NULL, wpath1, MAXPATHLEN);    // NULL implies the current process.
            if (!len) {
                free (wpath1);
                return (NULL);
            }
#  elif defined(__APPLE__)
            uint32_t size = MAXPATHLEN;
            if (_NSGetExecutablePath(wpath1, &size) != 0) {
                free (wpath1);
                return (NULL);
            }
#  elif defined(__linux)
            ssize_t len = readlink("/proc/self/exe", wpath1, MAXPATHLEN - 1); // -1 as it is not NULL terminated.
            if (len == -1) {
                ARLOGperror(NULL);
                free (wpath1);
                return (NULL);
            }
            wpath1[len] = '\0'; // NULL terminate.
#  endif
            arMallocClear(wpath2, char, MAXPATHLEN);
            if (!arUtilGetDirectoryNameFromPath(wpath2, wpath1, MAXPATHLEN, 0)) {
                free (wpath1);
                free (wpath2);
                return (NULL);
            }
            free (wpath1);
            return (wpath2);
#else
            return (NULL); // Unsupported OS.
#endif
            break;
        }

        //
        // BUNDLE_RESOURCES_DIR
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR:
#if defined(__APPLE__)
        {
            // Change working directory to resources directory inside app bundle.
            wpath1 = NULL;
            CFURLRef pathCFURLRef = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle()); // Get relative path to resources directory.
            if (pathCFURLRef) {
                wpath1 = (char *)calloc(MAXPATHLEN, sizeof(char)); //getcwd(path, MAXPATHLEN);
                if (wpath1) {
                    if (!CFURLGetFileSystemRepresentation(pathCFURLRef, true, (UInt8*)wpath1, MAXPATHLEN)) { // true in param 2 resolves against base.
                        ARLOGe("Error: Unable to get file system representation of a CFURL.\n");
                        free(wpath1);
                        wpath1 = NULL;
                    }
                }
                CFRelease(pathCFURLRef);
            }
            return (wpath1);
        }
#elif defined(__linux)
        {
            // Form a relative URL to exe, consisting of a subdirectory of ../share named after the executable.
            arMallocClear(wpath1, char, MAXPATHLEN);
            ssize_t len = readlink("/proc/self/exe", wpath1, MAXPATHLEN - 1); // -1 as it is not NULL terminated.
            if (len == -1) {
                ARLOGperror(NULL);
                free (wpath1);
                return (NULL);
            }
            wpath1[len] = '\0'; // NULL terminate.
            arMallocClear(wpath2, char, MAXPATHLEN);
            if (!arUtilGetDirectoryNameFromPath(wpath2, wpath1, MAXPATHLEN, 0)) {
                free (wpath1);
                free (wpath2);
                return (NULL);
            }
            len = strlen(wpath2);
            if (snprintf(&wpath2[len], MAXPATHLEN - len, "/../share/%s", arUtilGetFileNameFromPath(wpath1)) >= MAXPATHLEN - len) {
                free (wpath1);
                free (wpath2);
                return (NULL);
            }
            free(wpath1);
            return (wpath2);
        }
#else
            return (NULL); // Unsupported OS.
#endif
            break;

        //
        // CWD
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_CWD:
#ifndef _WINRT

            arMallocClear(wpath1, char, MAXPATHLEN);
            if (!getcwd(wpath1, MAXPATHLEN)) {
                free(wpath1);
                return (NULL);
            }
            return (wpath1);
#else
			return (NULL); // Unsupported OS.
#endif
			break;

        //
        // USER_ROOT
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_USER_ROOT:
#if defined(_WIN32) && !defined(_WINRT)
            arMallocClear(wpath1, char, MAXPATHLEN);
            if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, wpath1))) {
                free (wpath1);
                return (NULL);
            }
            return (wpath1);
#elif defined(ANDROID)
        {
            // Make JNI calls to get the external storage directory.

            // To begin, get a reference to the env and attach to it.
            JNIEnv *env;
            int isAttached = 0;
            jthrowable exception;
            if (((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6)) < 0) {
                // Couldn't get JNI environment, so this thread is native.
                if (((*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL)) < 0) {
                    ARLOGe("Error: Couldn't attach to Java VM.\n");
                    return (NULL);
                }
                isAttached = 1;
            }

            // Get File object for the external storage directory.
            jclass classEnvironment = (*env)->FindClass(env, "android/os/Environment");
            if (!classEnvironment) goto bailAndroid;
            jmethodID methodIDgetExternalStorageDirectory = (*env)->GetStaticMethodID(env, classEnvironment, "getExternalStorageDirectory", "()Ljava/io/File;"); // public static File getExternalStorageDirectory ()
            if (!methodIDgetExternalStorageDirectory) goto bailAndroid;
            jobject objectFile = (*env)->CallStaticObjectMethod(env, classEnvironment, methodIDgetExternalStorageDirectory);
            exception = (*env)->ExceptionOccurred(env);
            if (exception) {
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
            }
            wpath1 = getFilePath(env, objectFile);
        bailAndroid:
            if (isAttached) (*gJavaVM)->DetachCurrentThread(gJavaVM); // Clean up.
            return (wpath1);
        }
#elif defined(__APPLE__) // iOS/OS X.
        {
#  ifdef __OBJC__
            NSString *nssHomeDir = NSHomeDirectory(); // CoreFoundation equivalent is CFCopyHomeDirectoryURL(), iOS 6.0+ only.
            if (!nssHomeDir) {
                return (NULL);
            }
            wpath1 = strdup([nssHomeDir UTF8String]);
            return wpath1;
        }
#  else
            size_t len = confstr(_CS_DARWIN_USER_DIR, NULL, 0);
            if (!len) return (NULL);
            wpath1 = (char *)malloc(len);
            len = confstr(_CS_DARWIN_USER_CACHE_DIR, wpath1, len);
            if (!len) return (NULL);
#  endif
#elif defined(__linux)
            if (!((wpath1 = getenv("HOME")))) {
                return (NULL);
            }
            return (strdup(wpath1));
#else
            return (NULL); // Unsupported OS.
#endif
            break;

        //
        // APP_CACHE_DIR
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR:
#ifdef _WINRT
            auto folder = Windows::Storage::ApplicationData::Current->LocalCacheFolder;
            wpath1 = strdup(folder->Path->Data().c_str());
            return (wpath1);
#elif defined(_WIN32)
            arMallocClear(wpath1, char, MAXPATHLEN);
            if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, wpath1))) {
                free (wpath1);
                return (NULL);
            }
            return (wpath1);
#elif defined(__APPLE__) // iOS/OS X.
        {
#  ifdef __OBJC__
            NSURL *cacheDir = [[[NSFileManager defaultManager] URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask] objectAtIndex:0];
            if (!cacheDir) {
                return (NULL);
            }
            NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
            if (bundleID) {
                cacheDir = [cacheDir URLByAppendingPathComponent:bundleID];
            }
            wpath1 = strdup([[cacheDir path] UTF8String]);
#  else
            size_t len = confstr(_CS_DARWIN_USER_CACHE_DIR, NULL, 0);
            if (!len) return (NULL);
            wpath1 = (char *)malloc(len);
            len = confstr(_CS_DARWIN_USER_CACHE_DIR, wpath1, len); // On OS X, returns a folder in the sandbox hierachy under /var/folders/.
            if (!len) return (NULL);
#  endif
            return (wpath1);
        }
#elif defined(ANDROID)
        {
            // Make JNI calls to get the Context's cache directory.

            // To begin, get a reference to the env and attach to it.
            JNIEnv *env;
            int isAttached = 0;
            int ret = 0;
            jthrowable exception;
            if (((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6)) < 0) {
                // Couldn't get JNI environment, so this thread is native.
                if (((*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL)) < 0) {
                    ARLOGe("Error: Couldn't attach to Java VM.\n");
                    return (NULL);
                }
                isAttached = 1;
            }

            // Get File object for the Context's files directory. This only works
            // if a subclass of Context is supplied.
            // e.g. JNIEXPORT void JNICALL Java_com_test_TestActivity_test(JNIEnv * env, jobject obj)
            // so make sure before call.
            jclass classOfSuppliedObject = (*env)->GetObjectClass(env, instanceOfAndroidContext);
            if (!classOfSuppliedObject) goto bailAndroid1;
            jclass classContext = (*env)->FindClass(env, "android/content/Context");
            if (!classContext) goto bailAndroid1;
            if (!(*env)->IsInstanceOf(env, instanceOfAndroidContext, classContext)) {
                ARLOGe("Error: supplied object is not an instance of android/content/Context.\n");
                goto bailAndroid1;
            }
            jmethodID methodGetDir = (*env)->GetMethodID(env, classOfSuppliedObject, "getCacheDir", "()Ljava/io/File;"); // public abstract File getCacheDir();
            if (!methodGetDir) goto bailAndroid1;
            jobject objectFile = (*env)->CallObjectMethod(env, instanceOfAndroidContext, methodGetDir);
            exception = (*env)->ExceptionOccurred(env);
            if (exception) {
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
            }
            wpath1 = getFilePath(env, objectFile);
        bailAndroid1:
            if (isAttached) (*gJavaVM)->DetachCurrentThread(gJavaVM); // Clean up.
            return (wpath1);
        }
#elif defined(__linux) // Linux
        {
            // To get a useable process name, first get command line, then take last element of first portion
            // and append it to ~/.cache.
            FILE *fp = fopen("/proc/self/cmdline", "rb");
            if (!fp) {
                ARLOGperror("Unable to determine process name");
                return (NULL);
            }
            arMallocClear(wpath1, char, MAXPATHLEN);
            wpath2 = NULL;
            ssize_t len = 0;
            while (fread(&wpath1[len], 1, 1, fp) == 1) {
                if (wpath1[len] == '\0') break; // Read only until first nul.
                len++;
            }
            if (ferror(fp)) {
                ARLOGperror(NULL);
            }
            fclose(fp);
            char *home = getenv("HOME");
            char *basedir;
            if (asprintf(&basedir, "%s%s", (home ? home : "/var/cache"), (home ? "/.cache" : "")) == -1) {
                ARLOGperror(NULL);
                wpath2 = NULL;
            } else {
                const char *wpath1_file = arUtilGetFileNameFromPath(wpath1);
                if (wpath1_file) {
                    if (asprintf(&wpath2, "%s/%s", basedir, wpath1_file) == -1) {
                        ARLOGperror(NULL);
                        wpath2 = NULL;
                    };
                } else {
                    wpath2 = strdup(basedir);
                }
                free(basedir);
            }
            free(wpath1);
            return(wpath2);
        }
#else
            return (NULL); // Unsupported OS.
#endif
            break;

        //
        // APP_DATA_DIR
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_DATA_DIR:
#ifdef _WINRT
			auto folder = Windows::Storage::ApplicationData::Current->RoamingFolder;
			wpath1 = strdup(folder->Path->Data().c_str());
			return (wpath1);
#elif defined(_WIN32)
            arMallocClear(wpath1, char, MAXPATHLEN);
            if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, wpath1))) {
                free (wpath1);
                return (NULL);
            }
            return (wpath1);
#elif defined(__APPLE__) && defined(__OBJC__)
        {
            NSURL *appSupportDir = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] objectAtIndex:0];
            if (!appSupportDir) {
                return (NULL);
            }
            NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
            if (bundleID) {
                appSupportDir = [appSupportDir URLByAppendingPathComponent:bundleID];
            }
            wpath1 = strdup([[appSupportDir path] UTF8String]);
            return (wpath1);
        }
#elif defined(ANDROID)
        {
            // Make JNI calls to get the Context's files directory.
            
            // To begin, get a reference to the env and attach to it.
            JNIEnv *env;
            int isAttached = 0;
            int ret = 0;
            jthrowable exception;
            if (((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6)) < 0) {
                // Couldn't get JNI environment, so this thread is native.
                if (((*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL)) < 0) {
                    ARLOGe("Error: Couldn't attach to Java VM.\n");
                    return (NULL);
                }
                isAttached = 1;
            }
            
            // Get File object for the Context's files directory. This only works
            // if a subclass of Context is supplied.
            // e.g. JNIEXPORT void JNICALL Java_com_test_TestActivity_test(JNIEnv * env, jobject obj)
            // so make sure before call.
            jclass classOfSuppliedObject = (*env)->GetObjectClass(env, instanceOfAndroidContext);
            if (!classOfSuppliedObject) goto bailAndroid1;
            jclass classContext = (*env)->FindClass(env, "android/content/Context");
            if (!classContext) goto bailAndroid1;
            if (!(*env)->IsInstanceOf(env, instanceOfAndroidContext, classContext)) {
                ARLOGe("Error: supplied object is not an instance of android/content/Context.\n");
                goto bailAndroid2;
            }
            jmethodID methodGetDir = (*env)->GetMethodID(env, classOfSuppliedObject, "getFilesDir", "()Ljava/io/File;"); // public abstract File getFilesDir();
            //jmethodID methodGetDir = (*env)->GetMethodID(env, classOfSuppliedObject, "getDir", "(Ljava/lang/String;)Ljava/io/File;"); // public abstract File getDir(String type);
            if (!methodGetDir) goto bailAndroid1;
            jobject objectFile = (*env)->CallObjectMethod(env, instanceOfAndroidContext, methodGetDir);
            exception = (*env)->ExceptionOccurred(env);
            if (exception) {
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
            }
            wpath1 = getFilePath(env, objectFile);
        bailAndroid2:
            if (isAttached) (*gJavaVM)->DetachCurrentThread(gJavaVM); // Clean up.
            return (wpath1);
        }
#elif defined(__linux) // Linux
        {
            // To get a useable process name, first get command line, then take last element of first portion
            // and append it to ~/.config.
            FILE *fp = fopen("/proc/self/cmdline", "rb");
            if (!fp) {
                ARLOGperror("Unable to determine process name");
                return (NULL);
            }
            arMallocClear(wpath1, char, MAXPATHLEN);
            wpath2 = NULL;
            ssize_t len = 0;
            while (fread(&wpath1[len], 1, 1, fp) == 1) {
                if (wpath1[len] == '\0') break; // Read only until first nul.
                len++;
            }
            if (ferror(fp)) {
                ARLOGperror(NULL);
            }
            fclose(fp);
            char *home = getenv("HOME");
            char *basedir;
            if (asprintf(&basedir, "%s%s", (home ? home : "/var/lib"), (home ? "/.config" : "")) == -1) {
                ARLOGperror(NULL);
                wpath2 = NULL;
            } else {
                const char *wpath1_file = arUtilGetFileNameFromPath(wpath1);
                if (wpath1_file) {
                    if (asprintf(&wpath2, "%s/%s", basedir, wpath1_file) == -1) {
                        ARLOGperror(NULL);
                        wpath2 = NULL;
                    };
                } else {
                    wpath2 = strdup(basedir);
                }
                free(basedir);
            }
            free(wpath1);
            return(wpath2);
        }
#else
			return (NULL); // Unsupported OS.
#endif
			break;

        //
        // TMP_DIR
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_TMP_DIR:
#ifdef _WINRT
            auto folder = Windows::Storage::ApplicationData::Current->TemporaryFolder;
            wpath1 = strdup(folder->Path->Data().c_str());
            return (wpath1);
#elif defined(_WIN32)
            arMallocClear(wpath1, char, MAXPATHLEN);
            DWORD len = GetTempPath(MAXPATHLEN, wpath1);
            if (len == 0) {
                free (wpath1);
                return (NULL);
            } else {
                wpath1[len - 1] = '\0'; // Strip trailing slash.
                return (wpath1);
            }
#elif defined(__APPLE__) && defined(__OBJC__) // iOS/macOS.
        {
            NSString *nssTempDir = NSTemporaryDirectory(); // CoreFoundation equivalent is CFCopyHomeDirectoryURL() + "/tmp", iOS 6.0+ only.
            if (!nssTempDir) {
                return (NULL);
            }
            wpath1 = strdup([nssTempDir UTF8String]);
            return wpath1;
        }
#elif defined(ANDROID)
            return (arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR, instanceOfAndroidContext));
#elif defined(__linux) || defined(__APPLE__) // Linux or non-ObjC iOS/macOS.
            if (!((wpath1 = getenv("TMPDIR")))) {
                return (strdup("/tmp"));
            }
            return (strdup(wpath1));
#else
            return (NULL); // Unsupported OS.
#endif
            break;

        //
        // SUPPLIED_PATH
        //
        case AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH:
        default:
            return (NULL); // Undefined behaviour.
            break;
    }
}

#ifdef ANDROID
char *arUtilGetAndCreateResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, jobject instanceOfAndroidContext)
#else
char *arUtilGetAndCreateResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior)
#endif
{
    char *wpath;
#ifdef ANDROID
    wpath = arUtilGetResourcesDirectoryPath(behavior, instanceOfAndroidContext);
#else
    wpath = arUtilGetResourcesDirectoryPath(behavior);
#endif
    if (!wpath) return NULL;
    int err = test_d(wpath);
    if (err < 0) {
        ARLOGperror("Error looking for resources directory path");
        free(wpath);
        return NULL;
    } else if (err == 0) {
        if (mkdir_p(wpath) < 0) {
            ARLOGperror("Error creating resources directory path");
            free(wpath);
            return NULL;
        }
    }
    return (wpath);
}

#ifndef _WINRT
#ifdef ANDROID
int arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, const char *path, jobject instanceOfAndroidContext)
#else
int arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behavior, const char *path)
#endif
{
    char *wpath;
    AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR behaviorW;

    if (behavior == AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST) {
#if defined(__APPLE__) || defined(__linux)
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR;
#elif defined(ANDROID)
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR;
#elif defined(_WIN32)
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_EXECUTABLE_DIR;
#else
        behaviorW = AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_CWD;
#endif
    } else {
        behaviorW = behavior;
    }

    if (behaviorW != AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH) {
#ifdef ANDROID
        wpath = arUtilGetResourcesDirectoryPath(behavior, instanceOfAndroidContext);
#else
        wpath = arUtilGetResourcesDirectoryPath(behavior);
#endif
        if (wpath) {
            if (chdir(wpath) != 0) {
                ARLOGe("Error: Unable to change working directory to '%s'.\n", wpath);
                ARLOGperror(NULL);
                free (wpath);
                return (-1);
            }
            free(wpath);
        }
    }
    if (path) {
        if (chdir(path) != 0) {
            ARLOGe("Error: Unable to change working directory to '%s'.\n", path);
            ARLOGperror(NULL);
            return (-1);
        }
    }

    return (0);
}
#endif // !_WINRT

//
// For AR2.
//

int arUtilReplaceExt( char *filename, int n, char *ext )
{
    int   i, j;

    for( i = j = 0; filename[i] != '\0'; i++ ) {
        if( filename[i] == '.' ) j = i;
    }
    if( j == 0 ) {
        if( i + (int)strlen(ext) + 2 > n ) return -1;
        j = i;
        filename[j] = '.';
    }
    else {
        if( j + (int)strlen(ext) + 2 > n ) return -1;
    }

    filename[j+1] = '\0';
    strcat( filename, ext );

    return 0;
}

int arUtilRemoveExt( char *filename )
{
    int   i, j;

    j = -1;
    for( i = 0; filename[i] != '\0'; i++ ) {
        if( filename[i] == '.' ) j = i;
    }
    if( j != -1 ) filename[j] = '\0';

    return 0;
}

int arUtilDivideExt( const char *filename, char *s1, char *s2 )
{
    int   j, k;

    for(j=0;;j++) {
        s1[j] = filename[j];
        if( s1[j] == '\0' || s1[j] == '.' ) break;
    }
    s1[j] = '\0';
    if( filename[j] == '\0' ) s2[0] = '\0';
    else {
        j++;
        for(k=0;;k++) {
            s2[k] = filename[j+k];
            if( s2[k] == '\0' ) break;
        }
    }

    return 0;
}

void arUtilPrintTransMat(const ARdouble trans[3][4])
{
    int i;
    for (i = 0; i < 3; i++) {
        ARPRINT("[% .3f % .3f % .3f] [% 6.1f]\n", trans[i][0], trans[i][1], trans[i][2], trans[i][3]);
    }
}

void arUtilPrintMtx16(const ARdouble mtx16[16])
{
    int i;
    for (i = 0; i < 4; i++) {
        ARPRINT("[% .3f % .3f % .3f] [% 6.1f]\n", mtx16[i], mtx16[i + 4], mtx16[i + 8], mtx16[i + 12]);
    }
}

