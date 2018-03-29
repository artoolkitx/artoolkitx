//
//  EdenUtil.c
//
//  Various OS-related bits and pieces.
//
//  Copyright (c) 2004-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	
//	Rev		Date		Who		Changes
//	1.0.0	2004-04-23	PRL		Added functions for checking keyboard.
//

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
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
// @@END_EDEN_LICENSE_HEADER@@

// ============================================================================
//	Private includes
// ============================================================================
#include <Eden/EdenUtil.h>
#include <stdio.h>					// fprintf(), stderr, stdin, fileno(), perror()
#include <string.h>					// strdup(), strrchr()
#include <stdlib.h>                 // malloc()
#include <ctype.h>                  // isalpha(), tolower().
#if defined(EDEN_UNIX)
#  include <termios.h>				// struct termios, tcgetattr(), tcsetattr()
#  include <strings.h>				// bzero(), required for FD_ZERO().
#  define SELECT_IS_IN_SELECT_H 1
#  if SELECT_IS_IN_SELECT_H
#    include <sys/select.h>			// fd_set, FD_ZERO(), FD_SET(), select()
#  else
#    include <sys/types.h>
#    include <sys/time.h>
#  endif
#  include <sys/param.h>            // MAXPATHLEN
#  ifdef __APPLE__
#    include <mach-o/dyld.h>        // _NSGetExecutablePath()
#  endif
#  include <unistd.h>               // getcwd()
#elif defined(_WIN32)
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#  include <direct.h>               // _getcwd()
#  define getcwd _getcwd
#  include <conio.h>				// kbhit()
#endif // EDEN_UNIX

// ============================================================================
//	Private defines and types
// ============================================================================

// ============================================================================
//	Global variables
// ============================================================================
#ifdef EDEN_UNIX
static struct termios gEdenKeyboardHitTermiosSaved;
#endif

// ============================================================================
//	Private functions
// ============================================================================


// ============================================================================
//	Public functions
// ============================================================================

//
//  Do any required setup so that we can check for single keypresses.
//
EDEN_BOOL EdenKeyboardHitSetup(void)
{
#ifdef EDEN_UNIX
	struct termios t;
	
	// Put the terminal into single character mode.
	tcgetattr(fileno(stdin), &t);
	gEdenKeyboardHitTermiosSaved = t;  // Save current settings.
	t.c_lflag &= (~ICANON);
	t.c_cc[VTIME] = 0;
	t.c_cc[VMIN] = 1;
	if (tcsetattr(fileno(stdin), TCSANOW, &t) < 0) {
		EDEN_LOGperror("EdenKeyboardHitSetup(): Unable to set terminal to single character mode");
		return (FALSE);
	}
#endif	
	return (TRUE);
}

//
//  Check to see if the user has pressed a key on the keyboard.
//
EDEN_BOOL EdenKeyboardHit(void)
{
#if defined(EDEN_UNIX)
	fd_set fdset;
	struct timeval tv;
#endif
	
#if defined(EDEN_UNIX)
	FD_ZERO(&fdset);
	FD_SET(fileno(stdin), &fdset);
	tv.tv_sec = tv.tv_usec = 0;
	return (select(fileno(stdin) + 1, &fdset, NULL, NULL, &tv));
#elif defined(_WIN32)
	return (kbhit());
#else
	return (FALSE);
#endif
}

//
//  Do any required cleanup from our use of single keypress detection.
//
EDEN_BOOL EdenKeyboardHitCleanup(void)
{
#ifdef EDEN_UNIX
	if (tcsetattr(fileno(stdin), TCSANOW, &gEdenKeyboardHitTermiosSaved) < 0) {
		EDEN_LOGperror("EdenKeyboardHitCleanup(): Unable to set terminal to default mode");
		return (FALSE);
	}
#endif	
	return (TRUE);
}

//
//  Given a full or partial pathname passed in string path,
//  returns a pointer to the first char of the filename
//  portion of path.
//
char *EdenGetFileNameFromPath(const char *path)
{
	char *sep;
#ifdef _WIN32
    char *sep1;
#endif

    if (!path || !*path) return (NULL);
    
#if defined(EDEN_MACOS)
	sep = strrchr(path, ':');
#else
	sep = strrchr(path, '/');
#  ifdef _WIN32
    sep1 = strrchr(path, '\\');
    if (sep1 > sep) sep = sep1;
#  endif
#endif

	if (!sep) return ((char *)path);
	else return (sep + 1);
}

//
//  Given a full or partial pathname passed in string path,
//  returns a string with the extension portion of path,
//  i.e. the text after the rightmost '.' character, if any.
//  If the filename contains no '.', NULL is returned.
//  If convertToLowercase is TRUE, uppercase ASCII characters in
//  the extension will be converted to lowercase.
//
char *EdenGetFileExtensionFromPath(const char *path, int convertToLowercase)
{
    char *sep;
    size_t len;
    char *ret;
    int i;

    if (!path || !*path) return (NULL);

    sep = strrchr(EdenGetFileNameFromPath(path), '.');
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

//
//  Given a full or partial pathname passed in string path,
//  returns a string with the directory name portion of path.
//  The string is not terminated by the directory separator.
//  NB: The returned string must be freed by the caller.
//
char *EdenGetDirectoryNameFromPath(const char *path)
{
	char *dir;
	char *sep;
#ifdef _WIN32
    char *sep1;
#endif
	
	dir = strdup(path);
#if defined(EDEN_MACOS)
	sep = strrchr(dir, ':');
#else
	sep = strrchr(dir, '/');
#  ifdef _WIN32
    sep1 = strrchr(path, '\\');
    if (sep1 > sep) sep = sep1;
#  endif
#endif
			
	if (!sep) *dir = '\0';
	else *sep = '\0';
	return dir;
}

char *EdenGetExecutablePath(void)
{
#if defined(_WIN32)
	DWORD len;
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__linux)
	return (NULL); // Not implemented for this OS.
#else
    char *path = NULL;
	if (!(path = (char *)malloc(MAXPATHLEN * sizeof(char)))) return (NULL);
#  if defined(_WIN32)
	len = GetModuleFileName(NULL, path, MAX_PATH);    // NULL implies the current process.
	if (len) return (path);
    else {
        free (path);
        return (NULL);
	}
#  elif defined(__APPLE__)
    uint32_t size = MAXPATHLEN;
    if (_NSGetExecutablePath(path, &size) == 0) return (path);
    else {
        free (path);
        return (NULL);
    }
#  elif defined(__linux)
    ssize_t len = readlink("/proc/self/exe", path, MAXPATHLEN - 1); // -1 as it is not NULL terminated.
    if (len >= 0) {
        path[len] = '\0'; // NULL terminate.
        return (path);
    } else {        
        EDEN_LOGperror(NULL);
        free (path);
        return (NULL);
    }
#  endif
#endif
}

char *EdenGetFileURI(const char *path)
{
    const char method[] = "file://";
    char *abspath = NULL;
    char *uri = NULL;
    size_t pathlen, abspathlen = 0, urilen;
    EDEN_BOOL isAbsolute;
#ifdef _WIN32
    EDEN_BOOL isUNC;
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
        abspathlen = pathlen;
    } else {
        // For relative paths, concatenate with the current working directory.
        abspath = (char *)calloc(MAXPATHLEN, sizeof(char));
        if (!abspath) return (NULL);
        if (!getcwd(abspath, MAXPATHLEN)) goto bail;
        abspathlen = strlen(abspath);
        if (abspathlen < 1) goto bail;
        // Ensure current working directory path has a trailing slash.
#ifdef _WIN32
        if (abspath[abspathlen - 1] != '/' && abspath[abspathlen - 1] != '\\' )
#else
        if (abspath[abspathlen - 1] != '/')
#endif
        {
            abspath[abspathlen++] = '/'; abspath[abspathlen] = '\0';
        }
        if (abspathlen + pathlen >= MAXPATHLEN) goto bail;
        strncpy(abspath + abspathlen, path, MAXPATHLEN - abspathlen - 1); abspath[MAXPATHLEN - 1] = '\0';
        abspathlen += pathlen;
    }
    
#ifdef _WIN32
    // Windows UNC paths will be stripped of the leading two backslashes.
    if (abspath[0] == '\\' && abspath[1] == '\\') isUNC = TRUE;
    else isUNC = FALSE;
#endif
    
    // First pass. Work out how big everything needs to be.
    urilen = sizeof(method) - 1; // Begin with "file://"
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
#endif
        } else {
            sprintf(uri + urilen, "%%%02x", abspath[i]);
            urilen += 3; // URL encoded char is 3 chars.
        }
        i++;
    }
    uri[urilen] = '\0'; // nul termination.
    
bail:
    if (!isAbsolute) free(abspath);
    
    return (uri);
}

