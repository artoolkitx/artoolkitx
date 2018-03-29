/*
 *  file_utils.c
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
 *  Copyright 2013-2015 Daqri, LLC.
 *
 *  Author(s): Philip Lamb, Dan Bell
 *
 */


#ifdef __linux
#  define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#  define _XOPEN_SOURCE 500
#  define _FILE_OFFSET_BITS 64
#endif

#include <ARX/ARUtil/file_utils.h>

#include <stdio.h> // FILE, fprintf(), sprintf(), perror(), stderr, stdin, fileno(), getchar()
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#  ifndef _CRT_SECURE_NO_WARNINGS
#     define _CRT_SECURE_NO_WARNINGS
#  endif
#  include <windows.h>
#  include <sys/types.h>
#  include <direct.h> // mkdir()
#  include <io.h>
#  include <direct.h> // chdir(), getcwd()
#  include <Shellapi.h> // SHFileOperation()
#  ifndef S_ISDIR
#    define S_ISDIR(mode) (((mode)& S_IFMT) == S_IFDIR)
#  endif
#  define MAXPATHLEN MAX_PATH
#  define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#  define FTELLO_FUNC(stream) ftello64(stream)
#  define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#  include <conio.h> // getch()
#  define snprintf _snprintf
#else
#  include <unistd.h> // chdir()
#  include <sys/param.h> // MAXPATHLEN
#  include <utime.h> // struct utimbuf, utime()
#  if (ANDROID && __ANDROID_API__ < 21)
#    include "ftw.h"
#  else
#    include <ftw.h> // struct FTW, nftw()
#  endif
#  define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#  define FTELLO_FUNC(stream) ftello(stream)
#  define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#  include <termios.h> // struct termios, tcgetattr(), tcsetattr()
#endif
#include <sys/stat.h> // struct stat, stat(), mkdir()
#include <errno.h> // errno, ENOENT
#include <time.h> // struct tm, mktime()

#define WRITEBUFFERSIZE (8192)



#include "unzip.h"
#include "zip.h"

int test_f(const char *file, const char *dir)
{
    char *path;
    struct stat statResult;
    int ret;
    
    if (!file) return (0);

    if (dir) {
        path = (char *)malloc((strlen(dir) + 1 + strlen(file) + 1) * sizeof(char)); // +1 for '/', +1 for '\0'.
        sprintf(path, "%s/%s", dir, file);
    } else {
        path = (char *)file;
    }
    
    if (stat(path, &statResult) == 0) {
    	// Something exists at path. Check that it's a file.
    	if (!(statResult.st_mode & S_IFREG)) {
    		errno = EEXIST;
    		ret = -1;
    	} else {
            ret = 1;
        }
    } else {
        if (errno != ENOENT) {
            // Some error other than "not found" occurred. Fail.
            ret = -1;
        } else {
            ret = 0;
        }
    }

    if (dir) free(path);
    return (ret);
}

int test_d(const char *dir)
{
    struct stat statResult;
    int ret;
        
    if (!dir) return (0);

    if (stat(dir, &statResult) == 0) {
    	// Something exists at path. Check that it's a dir.
    	if (!(statResult.st_mode & S_IFDIR)) {
    		errno = EEXIST;
    		ret = -1;
    	} else {
            ret = 1;
        }
    } else {
        if (errno != ENOENT) {
            // Some error other than "not found" occurred. Fail.
            ret = -1;
        } else {
            ret = 0;
        }
    }

    return (ret);
}

int cp_f(const char *source_file, const char *target_file)
{
    FILE *fps;
    FILE *fpt;
#define BUFSIZE 16384
    unsigned char *buf;
    size_t count;
    int ret;
    
    if (!source_file || !target_file) {
        errno = EINVAL;
        return (-1);
    }
    fps = fopen(source_file, "rb");
    if (!fps) return (-1);
    fpt = fopen(target_file, "wb");
    if (!fpt) {
        fclose(fps);
        return (-1);
    }
    buf = (unsigned char *)malloc(BUFSIZE);
    if (!buf) {
        fclose(fpt);
        fclose(fps);
        return (-1);
    }
    ret = 0;
    do {
        count = fread(buf, 1, BUFSIZE, fps);
        if (count < BUFSIZE && ferror(fps)) {
            ret = -1;
            break;
        }
        if (fwrite(buf, 1, count, fpt) < count) {
            ret = -1;
            break;
        }
    } while (count == BUFSIZE);
    free(buf);
    fclose(fpt);
    fclose(fps);
    return (ret);
}

int rn_f(const char *source_file, const char *target_file)
{
    FILE *fps;
    FILE *fpt;
#define BUFSIZE 16384
    unsigned char *buf;
    size_t count;
    int ret;
    
    if (!source_file || !target_file) {
        errno = EINVAL;
        return (-1);
    }
    fps = fopen(source_file, "rb");
    if (!fps) return (-1);
    fpt = fopen(target_file, "wb");
    if (!fpt) {
        fclose(fps);
        return (-1);
    }
    buf = (unsigned char *)malloc(BUFSIZE);
    if (!buf) {
        fclose(fpt);
        fclose(fps);
        return (-1);
    }
    ret = 0;
    do {
        count = fread(buf, 1, BUFSIZE, fps);
        if (count < BUFSIZE && ferror(fps)) {
            ret = -1;
            break;
        }
        if (fwrite(buf, 1, count, fpt) < count) {
            ret = -1;
            break;
        }
    } while (count == BUFSIZE);
    free(buf);
    fclose(fpt);
    fclose(fps);
    rm_rf(source_file);
    return (ret);
}


int mkdir_p(const char *path)
{
    size_t len;
    char *pathCopy = NULL;
	struct stat sb;
    int ret;
	bool last;
	char *p;
    char sep;
    
    // Copy the path so that we can modify it.
    if (!(len = strlen(path))) {
        errno = ENOENT;
        return -1;
    }
    if (!(pathCopy = (char *)malloc(len + 1))) {
        errno = ENOMEM;
        return -1;
    }
    strcpy(pathCopy, path);
    
    ret = 0;
	p = pathCopy;
	if (p[0] == '/')		/* Skip leading '/'. */
		++p;
    // Find next path separator (or end of string), then replace it with '\0'
    // to get name of intermediate directory to create. Repeat until last path
    // component is reached.
	for (last = false; !last; p++) {
        
		if (p[0] == '\0') last = true;
		else if (p[0] != '/'
#ifdef _WIN32
                 && p[0] != '\\'
#endif
                 ) continue;
        sep = *p;
        *p = '\0'; // Overwrite path separator.
		if (p[1] == '\0') last = true;

#ifdef _WIN32
        if (_mkdir(pathCopy) < 0)
#else
        if (mkdir(pathCopy, S_IRWXU|S_IRWXG) < 0)
#endif
        {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(pathCopy, &sb) < 0) {
					ret = -1;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last) errno = EEXIST;
					else errno = ENOTDIR;
					ret = -1;
					break;
				}
			} else {
				ret = -1;
				break;
			}
		}
        
		if (!last) *p = sep; // Restore path separator.
	}
    
    free(pathCopy);
	return (ret);
}

#ifndef _WIN32
static int ftwCallback_rm(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    return (remove(fpath));
}
#endif

int rm_rf(const char *path)
{
#ifndef _WIN32
    if (!path) {
        errno = EINVAL;
        return (-1);
    }
    
    return (nftw(path, ftwCallback_rm, 64, FTW_DEPTH | FTW_PHYS));
#else
    /*
     typedef struct _SHFILEOPSTRUCT {
     HWND         hwnd;
     UINT         wFunc;
     PCZZTSTR     pFrom;
     PCZZTSTR     pTo;
     FILEOP_FLAGS fFlags;
     BOOL         fAnyOperationsAborted;
     LPVOID       hNameMappings;
     PCTSTR       lpszProgressTitle;
     } SHFILEOPSTRUCT, *LPSHFILEOPSTRUCT;
     */
    
    int ret = 0;
    char cwd[MAXPATHLEN];
    size_t len, len2;
    int needsep;
    char *buf;
    unsigned int i;
    int err;
    SHFILEOPSTRUCT opts;
    
    len = strlen(path);
    if (len + 1 > MAXPATHLEN) { // path + nul.
        //ARLOGe("Error: path too long.\n");
        return (-1);
    }
    buf = (char *)calloc(MAXPATHLEN + 1, sizeof(char)); // +1 because needs to be double-NULL terminated.
    
    // Check for absolute path (path beginning with forward/back slash).
    // If relative, get cwd and append path to it.
    if (path[0] == '\\' || path[0] == '/') {
        len2 = 0;
    } else {
        if (!_getcwd(cwd, MAXPATHLEN)) {
            //ARLOGe("Error: unable to get current working directory.\n");
            ret = -1;
            goto bail;
        }
        len2 = strlen(buf);
        needsep = (buf[len2 - 1] == '\\' ? 0 : 1); // Check if cwd ends in a slash (will be true if cwd is root).
        if (len2 + needsep + len + 1 > MAXPATHLEN) { // cwd + separator + path + nul.
            //ARLOGe("Error: path too long.\n");
            ret = -1;
            goto bail;
        }
        if (needsep) buf[len2++] = '\\';
    }
    strncpy(buf + len2, path, len);
    
    // SHFileOperation is old API. Convert forward slashes to backslashes.
    for (i = 0; i < len2 + len; i++) if (buf[i] == '/') buf[i] = '\\';
    
    opts.wFunc = FO_DELETE;
    opts.pFrom = buf;
    opts.pTo = NULL;
    opts.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

    err = SHFileOperationA(&opts);

    if (err || opts.fAnyOperationsAborted) {
        ret = -1;
    }
 
bail:
    free(buf);
    return (ret);
#endif
}

static int dosdate_to_tm(uint64_t dos_date, struct tm *ptm)
{
    uint64_t date = (uint64_t)(dos_date >> 16);

    ptm->tm_mday = (uint16_t)(date & 0x1f);
    ptm->tm_mon = (uint16_t)(((date & 0x1E0) / 0x20) - 1);
    ptm->tm_year = (uint16_t)(((date & 0x0FE00) / 0x0200) + 1980);
    ptm->tm_hour = (uint16_t)((dos_date & 0xF800) / 0x800);
    ptm->tm_min = (uint16_t)((dos_date & 0x7E0) / 0x20);
    ptm->tm_sec = (uint16_t)(2 * (dos_date & 0x1f));
    ptm->tm_isdst = -1;
    
#define datevalue_in_range(min, max, value) ((min) <= (value) && (value) <= (max))
    if (!datevalue_in_range(0, 11, ptm->tm_mon) ||
        !datevalue_in_range(1, 31, ptm->tm_mday) ||
        !datevalue_in_range(0, 23, ptm->tm_hour) ||
        !datevalue_in_range(0, 59, ptm->tm_min) ||
        !datevalue_in_range(0, 59, ptm->tm_sec))
    {
        /* Invalid date stored, so don't return it. */
        memset(ptm, 0, sizeof(struct tm));
        return -1;
    }
#undef datevalue_in_range
    return 0;
}

static void change_file_date(const char *path, uint32_t dos_date)
{
#ifdef _WIN32
    HANDLE handle = NULL;
    FILETIME ftm, ftm_local, ftm_create, ftm_access, ftm_modified;

    handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        GetFileTime(handle, &ftm_create, &ftm_access, &ftm_modified);
        DosDateTimeToFileTime((WORD)(dos_date >> 16), (WORD)dos_date, &ftm_local);
        LocalFileTimeToFileTime(&ftm_local, &ftm);
        SetFileTime(handle, &ftm, &ftm_access, &ftm);
        CloseHandle(handle);
    }
#else
    struct utimbuf ut;
    struct tm newdate;

    dosdate_to_tm(dos_date, &newdate);

    ut.actime = ut.modtime = mktime(&newdate);
    utime(path, &ut);
#endif
}

int unzip_od(const char *zipFilePathname, const char *dir)
{
    unzFile uf = NULL;
    char cwd[MAXPATHLEN];
    unz_file_info64 file_info = {0};
    char filename_inzip[256] = {0};
    FILE *fout = NULL;
    void *buf = NULL;
    uInt size_buf = WRITEBUFFERSIZE;
    int err = UNZ_OK;
    int errclose = UNZ_OK;
    char *filename_withoutpath = NULL;
    char *p = NULL;
    
    if (!zipFilePathname) {
        return (UNZ_PARAMERROR);
    }
    
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64A(&ffunc);
    uf = unzOpen2_64(zipFilePathname, &ffunc);
#else
    uf = unzOpen64(zipFilePathname);
#endif
    if (!uf) {
        //ARLOGe("Error: unable to open file at path '%s'.\n", zipFilePathname);
        return (1);
    }
    
    //ARLOGd("%s opened\n", zipFilePathname);
    
    if (dir) {
        if (!getcwd(cwd, MAXPATHLEN)) {
            //ARLOGe("Error: unable to get current working directory.\n");
            err = UNZ_ERRNO;
            goto bail;
        }
        if (chdir(dir)) {
            //ARLOGe("Error: unable to change to directory '%s'.\n", dir);
            err = UNZ_ERRNO;
            goto bail;
        }
    }
    
    err = unzGoToFirstFile2(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    if (err != UNZ_OK) {
        //ARLOGe("Error %d with zipfile in unzGoToFirstFile2\n", err);
        goto bail1;
    }
    
    if (!(buf = malloc(size_buf))) {
        //ARLOGe("Out of memory!\n");
        err = UNZ_ERRNO;
        goto bail1;
    }
    
    do {
        
        p = filename_withoutpath = filename_inzip;
        while (*p) {
            if ((*p == '/') || (*p == '\\'))
                filename_withoutpath = p + 1;
            p++;
        }
        
        if (!*filename_withoutpath) {
            
            // Zip entry is a directory.
#ifdef DEBUG
            //ARLOGd("Creating directory: %s\n", filename_inzip);
#endif
            if (mkdir_p(filename_inzip)) {
                err = UNZ_ERRNO;
            }
            
        } else {

            // Zip entry is a file.
            err = unzOpenCurrentFilePassword(uf, NULL); // NULL = password.
            if (err != UNZ_OK) {
                //ARLOGe("Error %d with zipfile in unzOpenCurrentFilePassword.\n", err);
            } else {
                // Open the target file.
                fout = FOPEN_FUNC(filename_inzip, "wb");
                /* Some zips don't contain directory alone before file */
                if (!fout && (filename_withoutpath != filename_inzip)) {
                    char c = *(filename_withoutpath - 1);
                    *(filename_withoutpath - 1) = '\0';
                    mkdir_p(filename_inzip);
                    *(filename_withoutpath - 1) = c;
                    fout = FOPEN_FUNC(filename_inzip, "wb");
                }
                
                if (!fout) {
                    //ARLOGe("Error: unable to open file '%s' for writing.\n", filename_inzip);
                } else {
#ifdef DEBUG
                    //ARLOGd(" extracting: %s\n", filename_inzip);
#endif
                    // Read from the zip to buffer and then write to target.
                    do {
                        err = unzReadCurrentFile(uf, buf, size_buf); // return value: <0 == error, 0 == EOF, >0 == number of bytes read.
                        if (err < 0) {
                            //ARLOGe("Error %d with zipfile in unzReadCurrentFile.\n", err);
                        } else if (err > 0) {
                            if (fwrite(buf, err, 1, fout) != 1) {
                                //ARLOGe("Error %d writing extracted file.\n", errno);
                                err = UNZ_ERRNO;
                            }
                        }
                    } while (err > 0);
                    fclose(fout);
                    
                    if (err == 0) change_file_date(filename_inzip, file_info.dos_date);
                }
                
                errclose = unzCloseCurrentFile(uf);
                if (errclose != UNZ_OK)
                {
                    //ARLOGe("Error %d with zipfile in unzCloseCurrentFile\n", errclose);
                }
            }
        }
        
        if (err == UNZ_OK) {
            err = unzGoToNextFile2(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
            if (err != UNZ_OK && err != UNZ_END_OF_LIST_OF_FILE) {
                //ARLOGe("Error %d with zipfile in unzGoToNextFile2\n", err);
            }
        }
    
    } while (err == UNZ_OK);
    
    if (err == UNZ_END_OF_LIST_OF_FILE) {
        err = UNZ_OK;
    }
    free(buf);
    
bail1:
    if (dir) {
        if (chdir(cwd)) {
            //ARLOGe("Error: unable to change to directory '%s'.\n", cwd);
            if (err == UNZ_OK) err = UNZ_ERRNO;
        }
    }
    
bail:
    unzClose(uf);
    return err;
}

int zip_od(char *zipFilePathname, const char *baseFilePath, const char **fileNames, int totalFiles)
{
    int opt_compress_level = Z_DEFAULT_COMPRESSION;
    int err = 0;
    int size_buf = 0;
    void *buf = NULL;
    
    if (!zipFilePathname || !zipFilePathname[0]) {
        return ZIP_PARAMERROR;
    }

    size_buf = WRITEBUFFERSIZE;
    buf = (void *)malloc(size_buf);
    if (!buf) {
        fprintf(stderr, "Error allocating memory.\n");
        return ZIP_INTERNALERROR;
    }
    
    zipFile zf;
    int errclose;
    zf = zipOpen(zipFilePathname, 0);
    if (zf == NULL) {
        fprintf(stderr, "Error opening '%s'.\n", zipFilePathname);
        err = ZIP_ERRNO;
        goto bail;
    } else {
        //ARLOGd("Creating '%s'.\n", zipFilePathname);
    }
    
    for (int i = 0; (i < totalFiles) && (err == ZIP_OK); i++) {
        if (((*(fileNames[i])) != '-') && ((*(fileNames[i])) != '/')) {

            const char *filenameinzip = fileNames[i];

            FILE * fin;
            int size_read;
            char *fullFilePath;
            size_t fullFilePathSize = (strlen(baseFilePath) + 1 + strlen(filenameinzip) + 1) * sizeof(char); // +1 for nul-terminator.
            fullFilePath = (char *)malloc(fullFilePathSize);
            if (snprintf(fullFilePath, fullFilePathSize, "%s/%s", baseFilePath, filenameinzip) < 0) {
                fprintf(stderr, "Error creating full file path from '%s' and '%s'", baseFilePath, filenameinzip);
                err = ZIP_PARAMERROR;
            } else {
                zip_fileinfo zi;
                zi.dos_date = 0;
                //get_file_date(fullFilePathSize, &zi.dos_date);
                zi.internal_fa = 0;
                zi.external_fa = 0;
                
                err = zipOpenNewFileInZip(zf, filenameinzip, &zi,
                                          NULL, 0, NULL, 0, NULL /* comment*/,
                                          (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                          opt_compress_level);
                if (err != ZIP_OK) {
                    fprintf(stderr, "Error in opening '%s' in zipfile.\n", filenameinzip);
                } else {
                    fin = fopen(fullFilePath, "rb");
                    if (!fin) {
                        err = ZIP_ERRNO;
                        fprintf(stderr, "Error in opening '%s' for reading\n", filenameinzip);
                    }
                    free(fullFilePath);
                }
                
                if (err == ZIP_OK) {
                    do {
                        err = ZIP_OK;
                        size_read = (int)fread(buf, 1, size_buf, fin);
                        if (size_read < size_buf) {
                            if (feof(fin) == 0) {
                                fprintf(stderr, "Error in reading '%s'.\n", filenameinzip);
                                err = ZIP_ERRNO;
                            }
                        }
                        
                        if (size_read > 0) {
                            err = zipWriteInFileInZip (zf,buf,size_read);
                            if (err < 0) {
                                fprintf(stderr, "Error in writing '%s' in the zipfile.\n", filenameinzip);
                            }
                        }
                    } while ((err == ZIP_OK) && (size_read > 0));
                }
                
                fclose(fin);
                if (err < 0) {
                    err = ZIP_ERRNO;
                } else {
                    err = zipCloseFileInZip(zf);
                    if (err != ZIP_OK) {
                        fprintf(stderr, "Error in closing %s in the zipfile.\n", filenameinzip);
                    }
                }
            }
        }
    }
    errclose = zipClose(zf,NULL);
    if (errclose != ZIP_OK) {
        fprintf(stderr, "Error in closing '%s'.\n", zipFilePathname);
    }

bail:
    free(buf);
    return err;
}


int64_t get_file_size(const char * pathName)
{
    struct stat st;
    if (stat(pathName, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

char *cat(const char *file, size_t *bufSize_p)
{
    FILE *fp;
    size_t len;
    char *s;
    
    if (!file) {
        errno = EINVAL;
        return NULL;
    }
    
    fp = fopen(file, "rb");
    if (!fp) {
        return (NULL);
    }
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (!(s = (char *)malloc(len + 1))) {
        fclose(fp);
        errno = ENOMEM;
        return (NULL);
    }
    
    if (fread(s, len, 1, fp) < 1) {
        free(s);
        fclose(fp);
        return (NULL);
    }
    s[len] = '\0';
    fclose(fp);

    if (bufSize_p) *bufSize_p = len + 1;
    return (s);
}

char read_sn1(void)
{
#if defined(__APPLE__) || defined(__linux) || defined(ANDROID)
    struct termios t, t_saved;
    
    // Set terminal to single character non-echo mode.
    tcgetattr(fileno(stdin), &t);
    t_saved = t;
    t.c_lflag &= (~ICANON & ~ECHO);
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &t);
    if (tcsetattr(fileno(stdin), TCSANOW, &t) < 0) {
        perror("Unable to set terminal to single character mode");
        return -1;
    }

    char c = getchar();
    
    // Restore terminal mode.
    if (tcsetattr(fileno(stdin), TCSANOW, &t_saved) < 0) {
        perror("Unable to restore terminal mode");
    }
    
    return c;
#elif defined(_WIN32)
    return (_getch());
#else
    return -1;
#endif
}
