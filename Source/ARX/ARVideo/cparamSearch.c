/*
 *  cparamSearch.c
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
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.

#include "cparamSearch.h"

#if USE_CPARAM_SEARCH
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h> // malloc()
#include <pthread.h>
#include <time.h>
#include <sys/param.h> // MAXPATHLEN
#include <math.h> // log2f(), fabsf()
#ifndef _WIN32
#  include <unistd.h> // unlink()
#endif
#include <float.h> // FLT_MAX

#ifdef ANDROID
#  define LOG2F(x) (logf(x)/0.6931472f) // 0.6931472f = logf(2.0f)
#  include "Android/sqlite3.h"
#else
#  define LOG2F(x) log2f(x)
#  include <sqlite3.h>
#endif
#include <curl/curl.h>
#include <ARX/AR/ar.h>                  // arParamLoadFromBuffer().
#include <ARX/ARUtil/thread_sub.h>
#include <ARX/ARUtil/system.h>
#include <ARX/ARUtil/file_utils.h>

#include "nxjson.h"


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

#ifdef __APPLE__
#  include <CommonCrypto/CommonDigest.h>
#  define MD5 CC_MD5
#  define MD5_DIGEST_LENGTH CC_MD5_DIGEST_LENGTH
#  define MD5_COUNT_t CC_LONG
#else
//#include <openssl/md5.h>
// Rather than including full OpenSSL header tree, just provide prototype for MD5().
// Usage is here: https://www.openssl.org/docs/manmaster/man3/MD5.html .
#  define MD5_DIGEST_LENGTH 16
#  define MD5_COUNT_t size_t
#  ifdef __cplusplus
extern "C" {
#  endif
    unsigned char *MD5(const unsigned char *d, size_t n, unsigned char *md);
#  ifdef __cplusplus
}
#  endif
#endif

#define CACHE_INIT_DB_NAME "cparam_cache_init.db"
#define CACHE_DB_NAME "cparam_cache.db"
#define CACHE_TIME 31557600L // How long to keep values in the cache, in seconds. 86400L=1 day, 604800L=1 week, 31557600=1 year of 365.25 days.
#define CACHE_TIME_FALLBACK 1209600L // How long to keep fallback values in the cache, in seconds. 86400L=1 day, 604800L=1 week, 31557600=1 year of 365.25 days.
#define CACHE_FLUSH_INTERVAL 86400L // How many seconds between checks in which expired cache entries are flushed.
#define CALIBRATION_SERVER_DOWNLOAD_URL_DEFAULT "https://calibration.artoolkitx.org/app/calib_camera/download.php"
#define CALIBRATION_SERVER_AUTHENTICATION_TOKEN_DEFAULT "ARToolKit.Rocks"
#define RECEIVE_HTTP_BUFSIZE 65536 // Max. 64k.

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

struct _CPARAM_SEARCH_DATA {
    char *device_id;
    int camera_index;
    int camera_width;
    int camera_height;
    char *aspect_ratio;
    float focal_length;
    char *camera_para_base64;
    ARParam camera_para;
    CPARAM_SEARCH_CALLBACK callback;
    void *userdata;
    struct _CPARAM_SEARCH_DATA *next;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

static char *os_name = NULL;
static char *os_arch = NULL;
static char *os_version = NULL;

static sqlite3 *cacheDB = NULL;
static char *gCalibrationServerDownloadURL = NULL;
static char gCalibrationServerAuthenticationTokenMD5Ascii[MD5_DIGEST_LENGTH * 2 + 1] = {0}; // +1 for nul-term.
static time_t nextCacheFlushTime = 0L;

static struct _CPARAM_SEARCH_DATA *cparamSearchList = NULL;
static pthread_mutex_t cparamSearchListLock;
static THREAD_HANDLE_T *cparamSearchThread = NULL;

static char *receiveHTTPBuf = NULL;
static int receiveHTTPBufLength = 0;

static int internetState = -1;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

static void reportSQLite3Err(sqlite3 *db);
static sqlite3 *openCacheDB(const char *dbBasePath, const char *dbPath, const char *initDBBasePath, const char *initDBPath, bool reset);
static void *cparamSearchWorker(THREAD_HANDLE_T *threadHandle);

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

int cparamSearchInit(const char *cacheDir, const char *cacheInitDir, int resetCache, const char *calibrationServerDownloadURL, const char *calibrationServerAuthenticationToken)
{
    // CURL init.
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        ARLOGe("Unable to init libcurl.\n");
        return (-1);
    }
    
    // Init OS fields.
    os_name = arUtilGetOSName();
    os_version = arUtilGetOSVersion();
    os_arch = arUtilGetCPUName();
    ARLOGd("cparamSearchInit(): os_name = '%s', os_version = '%s', os_arch = '%s'.\n", os_name, os_version, os_arch);
    
    // Open cache DB.
    cacheDB = openCacheDB(cacheDir, CACHE_DB_NAME, cacheInitDir, CACHE_INIT_DB_NAME, (bool)resetCache);
    if (!cacheDB) {
        ARLOGe("Unable to open cache database.\n");
        goto bail;
    }
    
    nextCacheFlushTime = time(NULL) + CACHE_FLUSH_INTERVAL; // Flush once every 10 minutes.
    
    // Calibration server.
    gCalibrationServerDownloadURL = strdup(calibrationServerDownloadURL ? calibrationServerDownloadURL : CALIBRATION_SERVER_DOWNLOAD_URL_DEFAULT);
    const char *authToken = calibrationServerAuthenticationToken ? calibrationServerAuthenticationToken : CALIBRATION_SERVER_AUTHENTICATION_TOKEN_DEFAULT;
    unsigned char authTokenMD5[MD5_DIGEST_LENGTH];
    if (!MD5((unsigned char *)authToken, (MD5_COUNT_t)strlen(authToken), authTokenMD5)) {
        ARLOGe("Error calculating md5.\n");
        goto bail;
    } else {
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) snprintf(&(gCalibrationServerAuthenticationTokenMD5Ascii[i*2]), 3, "%.2hhx", authTokenMD5[i]);
    }
    
    internetState = -1;
    
    cparamSearchList = NULL;
    pthread_mutex_init(&cparamSearchListLock, NULL);
    cparamSearchThread = threadInit(0, NULL, cparamSearchWorker);
    if (!cparamSearchThread) {
        ARLOGe("Error creating cparam search worker thread.\n");
        pthread_mutex_destroy(&cparamSearchListLock);
        goto bail;
    }
    
    return (0);
    
bail:
    cparamSearchFinal();
    return (-1);
}

CPARAM_SEARCH_STATE cparamSearch(const char *device_id, int camera_index, int camera_width, int camera_height, float focal_length, CPARAM_SEARCH_CALLBACK callback, void *userdata)
{
    struct _CPARAM_SEARCH_DATA *searchData;
    struct _CPARAM_SEARCH_DATA *tail;

    // Create a new search request.
    searchData = (struct _CPARAM_SEARCH_DATA *)calloc(1, sizeof(struct _CPARAM_SEARCH_DATA));
    searchData->device_id = strdup(device_id);
    searchData->camera_index = camera_index;
    searchData->camera_width = camera_width;
    searchData->camera_height = camera_height;
    searchData->aspect_ratio = arVideoUtilFindAspectRatioName(camera_width, camera_height);
    searchData->focal_length = focal_length;
    searchData->callback = callback;
    searchData->userdata = userdata;

    // Find the queue tail and add it on.
    pthread_mutex_lock(&cparamSearchListLock);
    if (!cparamSearchList) {
        cparamSearchList = searchData;
    } else {
        tail = cparamSearchList;
        while (tail->next) tail = tail->next;
        tail->next = searchData;
    }
    pthread_mutex_unlock(&cparamSearchListLock);

    // Start processing.
    threadStartSignal(cparamSearchThread);

    return (CPARAM_SEARCH_STATE_INITIAL);
}

//--

static void reportSQLite3Err(sqlite3 *db)
{
    ARLOGe("sqlite3 error: %s (%d).\n", sqlite3_errmsg(db), sqlite3_errcode(db));
}

static sqlite3 *openCacheDB(const char *dbBasePath, const char *dbPath, const char *initDBBasePath, const char *initDBPath, bool reset)
{
    char *dbPath0 = NULL;
    char *initDBPath0 = NULL;
    int err;
    int sqliteErr;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt;
    bool copyOrCreate;

    // Form absolute paths if required.
    if (dbBasePath && dbBasePath[0]) {
        if (asprintf(&dbPath0, "%s/%s", dbBasePath, dbPath) == -1) {
            ARLOGperror(NULL);
            goto done;
        }
    } else {
        dbPath0 = strdup(dbPath);
    }
    if (initDBBasePath && initDBBasePath[0]) {
        if (asprintf(&initDBPath0, "%s/%s", initDBBasePath, initDBPath) == -1) {
            ARLOGperror(NULL);
            goto done;
        }
    } else {
        initDBPath0 = strdup(initDBPath);
    }
    if (!dbPath0 || !initDBPath0) goto done;
    ARLOGd("dbPath0 = '%s', initDBPath0 = '%s'.\n", dbPath0, initDBPath0);

    // Check if we already have a cache database.
    copyOrCreate = true;
    err = test_f(dbPath0, NULL);
    if (err < 0) {
        ARLOGe("Error looking for database file '%s'.\n", dbPath0);
        ARLOGperror(NULL);
        goto done;
    } else if (err == 1) {
        // File exists. If reset request, we need to delete it.
        if (reset) {
            if (unlink(dbPath0) == -1) {
                ARLOGe("Error deleting database at path '%s'.\n", dbPath0);
                ARLOGperror(NULL);
                goto done;
            }
        } else {
            copyOrCreate = false;
        }
    }

    if (copyOrCreate) {
        // No current database exists. Need to copy initial or create new.

        if (dbBasePath) {
            // First, ensure that we have a directory to put it in.
            int dbBasePathExists = test_d(dbBasePath);
            if (dbBasePathExists == -1) {
                // Some error other than "not found" occurred. Fail.
                ARLOGe("Error looking for cache directory '%s'.\n", dbBasePath);
                ARLOGperror(NULL);
                goto done;
            } else if (!dbBasePathExists) {
                // Create the directory.
                if (mkdir_p(dbBasePath) == -1) {
                    ARLOGe("Error creating cache directory '%s'.\n", dbBasePath);
                    ARLOGperror(NULL);
                    goto done;
                }
            }
        }

        // Check if we have an initial database to copy over.
        err = test_f(initDBPath0, NULL);
        if (err < 0) {
            ARLOGe("Error looking for initial database file '%s'.\n", initDBPath0);
            ARLOGperror(NULL);
            goto done;
        } else if (err == 1) {
            // Initial database exists. Copy to cache.
            err = cp_f(initDBPath0, dbPath0);
            if (err < 0) {
                ARLOGe("Error initialising database.\n");
                ARLOGperror(NULL);
                goto done;
            }
            ARLOGi("Initialising camera parameters database from '%s'.\n", initDBPath0);
            copyOrCreate = false;
        }
    }

    // Open the database.
    sqliteErr = sqlite3_open_v2(dbPath0, &db, (copyOrCreate ? SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE : SQLITE_OPEN_READWRITE), NULL);
    if (sqliteErr != SQLITE_OK) {
        ARLOGe("Error opening %s database '%s'.\n", (copyOrCreate ? "new" : "existing"), dbPath0);
        goto bail;
    }

    if (copyOrCreate) {
        ARLOGi("Created new camera parameters database.\n");
        // Create the table and columns needed.
        const char createTableSQL[] = "CREATE TABLE cache(device_id TEXT NOT NULL, camera_index INTEGER NOT NULL, camera_width INTEGER NOT NULL, camera_height INTEGER NOT NULL, aspect_ratio TEXT NOT NULL, focal_length REAL, camera_para_base64 TEXT NOT NULL, expires INTEGER);"; // Use default primary key ('rowid').
        sqliteErr = sqlite3_prepare_v2(db, createTableSQL, sizeof(createTableSQL), &stmt, NULL);
        if (sqliteErr != SQLITE_OK) {
            goto bail;
        }
        sqliteErr = sqlite3_step(stmt);
        if (sqliteErr != SQLITE_DONE) {
            goto bail;
        }
        sqliteErr = sqlite3_finalize(stmt);
        if (sqliteErr != SQLITE_OK) {
            goto bail;
        }
    }

    goto done;

bail:
    reportSQLite3Err(db);
    sqlite3_close(db);
    db = NULL;
done:
    free(dbPath0);
    free(initDBPath0);
    return (db);
}

int cparamSearchFinal()
{
    if (cparamSearchThread) {
        threadWaitQuit(cparamSearchThread);
        pthread_mutex_destroy(&cparamSearchListLock);
        threadFree(&cparamSearchThread);
    }
    cparamSearchList = NULL;

    internetState = -1;
    
    if (gCalibrationServerDownloadURL) {
        free(gCalibrationServerDownloadURL);
        gCalibrationServerDownloadURL = NULL;
    }

    if (cacheDB) {
        sqlite3_close(cacheDB);
        cacheDB = NULL;
    }

    free(os_name); os_name = NULL;
    free(os_arch); os_arch = NULL;
    free(os_version); os_version = NULL;

    // CURL final.
    curl_global_cleanup();

    return (0);
}

int cparamSearchSetInternetState(int state)
{
    internetState = state;
    return (0);
}

// Receive size*nmemb bytes of non-NULL terminated data.
// Return the number of bytes processed.
static size_t receiveHTTP(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    if (!ptr || !size || !nmemb) return (0); // If no data, do nothing.

    if (!receiveHTTPBuf) {
        receiveHTTPBuf = (char *)malloc(RECEIVE_HTTP_BUFSIZE * sizeof(char));
        if (!receiveHTTPBuf) return (0);
        *receiveHTTPBuf = '\0';
        receiveHTTPBufLength = 0;
    }

    // Check for response too big.
    if (receiveHTTPBufLength + size*nmemb > RECEIVE_HTTP_BUFSIZE - 1) {
        free(receiveHTTPBuf);
        receiveHTTPBuf = NULL;
        receiveHTTPBufLength = 0;
        return (0);
    }

    memcpy(&receiveHTTPBuf[receiveHTTPBufLength], ptr, size*nmemb);
    receiveHTTPBufLength += size*nmemb;
    receiveHTTPBuf[receiveHTTPBufLength] = '\0';

    return (size*nmemb);
}

static unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length)
{
    const char decoding_table[256] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  62,   0,   0,   0,  63,
        52,  53,  54,  55,  56,  57,  58,  59,  60,  61,   0,   0,   0,   0,   0,   0,
        0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
        15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,   0,   0,   0,   0,   0,
        0,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
        41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
    int i, j;

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (!decoded_data) return NULL;

    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}


static void *cparamSearchWorker(THREAD_HANDLE_T *threadHandle)
{
    CURL *curlHandle = NULL;
    CURLcode curlErr;
	char curlErrorBuf[CURL_ERROR_SIZE];

    time_t nowTime;
    bool timeSet = false;

    char *SQL;
    sqlite3_stmt *stmt;
    int sqliteErr;

	long http_response;
    CPARAM_SEARCH_STATE result;
    int i;

    ARLOGd("cparamSearch worker entered.\n");

    while (threadStartWait(threadHandle) == 0) {

        pthread_mutex_lock(&cparamSearchListLock);
        struct _CPARAM_SEARCH_DATA *searchData = cparamSearchList;
        pthread_mutex_unlock(&cparamSearchListLock);

        while (searchData) {

            nowTime = time(NULL);
            timeSet = true;
            result = CPARAM_SEARCH_STATE_IN_PROGRESS;

            ARLOGi("cparamSearch beginning search for %s, camera %d, aspect ratio %s.\n", searchData->device_id, searchData->camera_index, searchData->aspect_ratio);
            // Let observer know that we've started.
            if (searchData->callback) (*searchData->callback)(result, 0.0f, NULL, searchData->userdata);

            // First, check the cache.
            if (asprintf(&SQL, "SELECT camera_width, camera_height, focal_length, camera_para_base64 FROM cache WHERE device_id = ?1 AND camera_index = %d AND aspect_ratio = '%s' AND (expires IS NULL OR expires > %ld);", searchData->camera_index, searchData->aspect_ratio, nowTime) < 0) { // ?1 will be bound to the string value.
                ARLOGe("Out of memory!\n");
                result = CPARAM_SEARCH_STATE_FAILED_ERROR;
            } else {
                sqliteErr = sqlite3_prepare_v2(cacheDB, SQL, (int)strlen(SQL), &stmt, NULL);
                free(SQL);
                if (sqliteErr != SQLITE_OK) {
                    reportSQLite3Err(cacheDB);
                    result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                } else {
                    sqliteErr = sqlite3_bind_text(stmt, 1, searchData->device_id, -1, SQLITE_STATIC);
                    if (sqliteErr != SQLITE_OK) {
                        reportSQLite3Err(cacheDB);
                        result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                    } else {

                        // Retrieve all matching records and sort throught them.
                        float focal_length;
                        int camera_width;
                        int camera_height;
                        const unsigned char *camera_para_base64;

                        float bestSizeRatio = INFINITY;
                        float bestFocalLengthRatio = INFINITY;

                        do {

                            sqliteErr = sqlite3_step(stmt);

                            if (sqliteErr == SQLITE_ROW) {

                                camera_width = sqlite3_column_int(stmt, 0);
                                camera_height = sqlite3_column_int(stmt, 1);
                                focal_length = (float)sqlite3_column_double(stmt, 2);
                                camera_para_base64 = sqlite3_column_text(stmt, 3);

                                // Width, height and camera_para must never be NULL.
                                if (!camera_width || !camera_height || !camera_para_base64) {
                                    ARLOGe("Error in database: NULL field.\n");
                                } else {
                                    // Decode the ARParam.
                                    ARParam decodedCparam;
                                    size_t decodedLen;
                                    unsigned char *decoded = base64_decode((char *)camera_para_base64, strlen((char *)camera_para_base64), &decodedLen);
                                    if (!decoded) {
                                        ARLOGe("Error in database: bad base64.\n");
                                    } else if (arParamLoadFromBuffer(decoded, decodedLen, &decodedCparam) < 0) {
                                        ARLOGe("Error in database: bad ARParam.\n");
                                    } else {
                                        // If this result is better than a previous result, save it.
                                        // If it's the first result, it will always be better.
                                        float sizeRatio = fabsf(LOG2F(((float)camera_width)/((float)searchData->camera_width)));
                                        float focalLengthRatio = (focal_length ? fabsf(LOG2F(focal_length/searchData->focal_length)) : FLT_MAX);
                                        if (sizeRatio < bestSizeRatio || (sizeRatio == bestSizeRatio && focalLengthRatio < bestFocalLengthRatio)) {
                                            searchData->camera_para = decodedCparam;
                                            result = CPARAM_SEARCH_STATE_OK;
                                            ARLOGi("Matched cached camera calibration record (%dx%d, focal length %.2f).\n", camera_width, camera_height, focal_length);
                                            // Take note of sizeRatio and focalLengthRatio.
                                            bestSizeRatio = sizeRatio;
                                            bestFocalLengthRatio = focalLengthRatio;
                                        }
                                    }
                                    free(decoded);
                                }

                            } else if (sqliteErr != SQLITE_DONE) { // No records found is not an error.
                                reportSQLite3Err(cacheDB);
                                result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                                break;
                            }
                        } while (sqliteErr == SQLITE_ROW);

                        sqliteErr = sqlite3_finalize(stmt); // Invalidates cache_camera_para_base64.
                        if (sqliteErr != SQLITE_OK) {
                            reportSQLite3Err(cacheDB);
                            result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                        }
                    }
                }
            }

            // If no cache hit, do a network query.
            if (result == CPARAM_SEARCH_STATE_IN_PROGRESS) {

                // Init curl handle and network.
                if (!curlHandle) {
                    curlHandle = curl_easy_init();
                    if (!curlHandle) {
                        ARLOGe("Error initialising CURL.\n");
                        result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                    } else {
                        curlErr = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, curlErrorBuf);
                        if (curlErr != CURLE_OK) {
                            ARLOGe("Error setting CURL error buffer: %s (%d)\n", curl_easy_strerror(curlErr), curlErr);
                            result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                        } else {
                            if (internetState == 0) {
                                result = CPARAM_SEARCH_STATE_FAILED_NO_NETWORK;
                            } else if (internetState == -1) {
                                // First, attempt a connection to a well-known site. If this fails, assume we have no
                                // internet access at all.
                                curlErr = curl_easy_setopt(curlHandle, CURLOPT_URL, "http://www.google.com");
                                if (curlErr != CURLE_OK) {
                                    ARLOGe("Error setting CURL URL: %s (%d)\n", curl_easy_strerror(curlErr), curlErr);
                                    result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                                } else {
                                    if ((curlErr = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, NULL)) != CURLE_OK ||
                                        (curlErr = curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1L)) != CURLE_OK) { // Headers only.
                                        ARLOGe("Error setting CURL options: %s (%d)\n", curl_easy_strerror(curlErr), curlErr);
                                        result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                                    } else {
                                        curlErr = curl_easy_perform(curlHandle);
                                        if (curlErr != CURLE_OK) {
                                            // No need to report error, since we expect it (e.g.) when wifi and cell data are off.
                                            // Typical first error in these cases is failure to resolve the hostname.
                                            //ARLOGe("Error performing CURL network test: %s (%d). %s.\n", curl_easy_strerror(curlErr), curlErr, curlErrorBuf);
                                            result = CPARAM_SEARCH_STATE_FAILED_NO_NETWORK;
                                        }
                                    }
                                }
                            } // internetState
                        } // !curl_easy_setopt()
                    } // !curl_easy_init()
                } // !curlHandle

                if (result == CPARAM_SEARCH_STATE_IN_PROGRESS) {

                    // Network OK, proceed.
                    // Build the form.
                    struct curl_httppost* post = NULL;
                    struct curl_httppost* last = NULL;

                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "version", CURLFORM_COPYCONTENTS, "1", CURLFORM_END);
                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "os_name", CURLFORM_COPYCONTENTS, os_name, CURLFORM_END);
                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "os_arch", CURLFORM_COPYCONTENTS, os_arch, CURLFORM_END);
                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "os_version", CURLFORM_COPYCONTENTS, os_version, CURLFORM_END);

                    // Search authentication token.
                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "ss", CURLFORM_COPYCONTENTS, gCalibrationServerAuthenticationTokenMD5Ascii, CURLFORM_END);

                    // Get all records for this device_id.
                    curl_formadd(&post, &last, CURLFORM_COPYNAME, "device_id", CURLFORM_COPYCONTENTS, searchData->device_id, CURLFORM_END);

                    // Set options.
                    // WORK AROUND ISSUE OF MISSING CAfile (default: /etc/ssl/certs/ca-certificates.crt) AND EMPTY CApath.
                    if (((curlErr = curl_easy_setopt(curlHandle, CURLOPT_URL, gCalibrationServerDownloadURL)) != CURLE_OK) ||
                        ((curlErr = curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L)) != CURLE_OK) ||
                        ((curlErr = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, receiveHTTP)) != CURLE_OK) ||
                        ((curlErr = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, NULL)) != CURLE_OK) ||  // Set userdata pointer.
                        ((curlErr = curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, post)) != CURLE_OK)) { // Automatically sets CURLOPT_NOBODY to 0.

                        ARLOGe("CURL error: %s (%d)\n", curl_easy_strerror(curlErr), curlErr);
                        result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                    } else {

                        // Perform the transfer. Blocks until complete.
                        curlErr = curl_easy_perform(curlHandle);
                        curl_formfree(post); // Free the form resources, regardless of outcome.
                        if (curlErr != CURLE_OK) {
                            ARLOGe("Error performing CURL operation: %s (%d). %s.\n", curl_easy_strerror(curlErr), curlErr, curlErrorBuf);
                            // Interpret some types of connection error.
                            switch (curlErr) {
                                case CURLE_COULDNT_RESOLVE_PROXY:
                                case CURLE_COULDNT_RESOLVE_HOST:
                                case CURLE_COULDNT_CONNECT:
                                case CURLE_SSL_CONNECT_ERROR:
                                    result = CPARAM_SEARCH_STATE_FAILED_SERVICE_UNREACHABLE;
                                    break;
                                case CURLE_REMOTE_ACCESS_DENIED:
                                    result = CPARAM_SEARCH_STATE_FAILED_SERVICE_UNAVAILABLE;
                                    break;
                                case CURLE_SEND_ERROR:
                                case CURLE_RECV_ERROR:
                                default:
                                    result = CPARAM_SEARCH_STATE_FAILED_NETWORK_FAILED;
                                    break;
                            }
                        } else {
                            curl_easy_getinfo (curlHandle, CURLINFO_RESPONSE_CODE, &http_response);
                            if (http_response == 200) {

                                if (receiveHTTPBufLength <= 0) {
                                    result = CPARAM_SEARCH_STATE_RESULT_NULL;
                                } else {
                                    //ARLOGe("Got http:'%s'\n", receiveHTTPBuf);
                                    // Read the results.
                                    // JSON decode
                                    const nx_json *records = nx_json_parse_utf8(receiveHTTPBuf);
                                    if (!records || records->type != NX_JSON_ARRAY) {
                                        result = CPARAM_SEARCH_STATE_FAILED_ERROR;
                                    } else {
                                        float bestSizeRatio = INFINITY;
                                        float bestFocalLengthRatio = INFINITY;

                                        ARLOGi("Fetched %d camera calibration records from online database.\n", records->length);
                                        for (i = 0; i < records->length; i++) {
                                            // Get a record.
                                            const nx_json *record = nx_json_item(records, i);
                                            if (record->type != NX_JSON_OBJECT) continue;

                                            // Get its data.
                                            int camera_index = (int)(nx_json_get(record, "camera_index")->int_value);
                                            int camera_width = (int)(nx_json_get(record, "camera_width")->int_value);
                                            int camera_height = (int)(nx_json_get(record, "camera_height")->int_value);
                                            unsigned char *aspect_ratio = (unsigned char *)(nx_json_get(record, "aspect_ratio")->text_value);
                                            float focal_length = (float)(nx_json_get(record, "focal_length")->dbl_value);
                                            bool fallback = (nx_json_get(record, "fallback")->int_value ? true : false);
                                            unsigned char *camera_para_base64 = (unsigned char *)(nx_json_get(record, "camera_para_base64")->text_value);

                                            ARLOGd("camera_index=%d, camera_width=%d, camera_height=%d, aspect_ratio='%s', focal_length=%f, camera_para_base64='%s'%s.\n", camera_index, camera_width, camera_height, aspect_ratio, focal_length, camera_para_base64, (fallback ? ", fallback" : ""));

                                            // Check validity of values.
                                            if (!camera_width || !camera_height || !aspect_ratio || !camera_para_base64) {
                                                ARLOGe("Error in database: NULL field.\n");
                                            } else {
                                                // Decode the ARParam.
                                                ARParam decodedCparam;
                                                size_t decodedLen;
                                                unsigned char *decoded = base64_decode((char *)camera_para_base64, strlen((char *)camera_para_base64), &decodedLen);
                                                if (!decoded) {
                                                    ARLOGe("Error in database: bad base64.\n");
                                                } else if (arParamLoadFromBuffer(decoded, decodedLen, &decodedCparam) < 0) {
                                                    ARLOGe("Error in database: bad ARParam.\n");
                                                } else {
                                                    if (strcmp((char *)aspect_ratio, searchData->aspect_ratio) == 0) {
                                                        // If this result is better than a previous result, save it.
                                                        // If it's the first result, it will always be better.
                                                        float sizeRatio = fabsf(LOG2F(((float)camera_width)/((float)searchData->camera_width)));
                                                        float focalLengthRatio = (focal_length ? fabsf(LOG2F(focal_length/searchData->focal_length)) : FLT_MAX);
                                                        if (sizeRatio < bestSizeRatio || (sizeRatio == bestSizeRatio && focalLengthRatio < bestFocalLengthRatio)) {
                                                            searchData->camera_para = decodedCparam;
                                                            result = CPARAM_SEARCH_STATE_OK;
                                                            ARLOGi("Matched fetched camera calibration record (%dx%d, %.2f).\n", camera_width, camera_height, focal_length);
                                                            // Take note of sizeRatio and focalLengthRatio.
                                                            bestSizeRatio = sizeRatio;
                                                            bestFocalLengthRatio = focalLengthRatio;
                                                        }
                                                    }

                                                    // Put into cache.
                                                    if (asprintf(&SQL, "INSERT OR REPLACE INTO cache (device_id,camera_index,focal_length,camera_width,camera_height,aspect_ratio,camera_para_base64,expires) VALUES (?1, %d, %f, %d, %d, ?2, ?3, %ld);", camera_index, focal_length, camera_width, camera_height, nowTime + (fallback ? CACHE_TIME_FALLBACK : CACHE_TIME)) >= 0) { // ?n will be bound to string values.
                                                        sqliteErr = sqlite3_prepare_v2(cacheDB, SQL, (int)strlen(SQL), &stmt, NULL);
                                                        free(SQL);
                                                        if (sqliteErr != SQLITE_OK) {
                                                            reportSQLite3Err(cacheDB);
                                                        } else {
                                                            ;
                                                            if (((sqliteErr = sqlite3_bind_text(stmt, 1, searchData->device_id, -1, SQLITE_STATIC)) != SQLITE_OK) ||
                                                                ((sqliteErr = sqlite3_bind_text(stmt, 2, (char *)aspect_ratio, -1, SQLITE_STATIC)) != SQLITE_OK) ||
                                                                ((sqliteErr = sqlite3_bind_text(stmt, 3, (char *)camera_para_base64, -1, SQLITE_STATIC)) != SQLITE_OK)) {
                                                                reportSQLite3Err(cacheDB);
                                                            } else {
                                                                sqliteErr = sqlite3_step(stmt);
                                                                if (sqliteErr != SQLITE_DONE) {
                                                                    reportSQLite3Err(cacheDB);
                                                                }
                                                            }
                                                            sqliteErr = sqlite3_finalize(stmt);
                                                            if (sqliteErr != SQLITE_OK) {
                                                                reportSQLite3Err(cacheDB);
                                                            }
                                                        }
                                                    }

                                                } // base64 ok.
                                                free(decoded);
                                            } // no NULLs.
                                        } // for (records)

                                        // Check for case where we didn't match any of the downloaded records.
                                        if (result == CPARAM_SEARCH_STATE_IN_PROGRESS) {
                                            result = CPARAM_SEARCH_STATE_RESULT_NULL;
                                        }
                                    }
                                    nx_json_free(records);

                                }
                            } else if (http_response == 204) {
                                result = CPARAM_SEARCH_STATE_RESULT_NULL;
                            } else if (http_response == 403) {
                                result = CPARAM_SEARCH_STATE_FAILED_SERVICE_NOT_PERMITTED;
                            } else if (http_response == 400) {
                                result = CPARAM_SEARCH_STATE_FAILED_SERVICE_INVALID_REQUEST;
                            } else if (http_response == 404 || http_response == 503) {
                                result = CPARAM_SEARCH_STATE_FAILED_SERVICE_UNAVAILABLE;
                            } else {
                                ARLOGe("search failed: server returned response %ld.\n", http_response);
                                result = CPARAM_SEARCH_STATE_FAILED_SERVICE_FAILED;
                            }
                        } // curl_easy_perform OK.
                        if (receiveHTTPBuf) {
                            free(receiveHTTPBuf);
                            receiveHTTPBuf = NULL;
                        }
                        receiveHTTPBufLength = 0;
                    } //options set OK.
                } // curlHandle, curlErrorBuf, network OK.

            }

            // Return the result.
            if (searchData->callback) {
                ARLOGd("cparamSearch result ready, invoking callback.\n");
                (*searchData->callback)(result, 0.0f, &(searchData->camera_para), searchData->userdata);
                ARLOGd("cparamSearch back from callback.\n");
            } else {
                ARLOGw("Warning: cparamSearch result with no callback registered.\n");
            }

            // Remove the current query from the head of the list.
            pthread_mutex_lock(&cparamSearchListLock);
            cparamSearchList = searchData->next;
            if (searchData->device_id) free(searchData->device_id);
            if (searchData->aspect_ratio) free(searchData->aspect_ratio);
            if (searchData->camera_para_base64) free(searchData->camera_para_base64);
            free(searchData);
            searchData = cparamSearchList;
            pthread_mutex_unlock(&cparamSearchListLock);

        } // while(searchData)

        // Expire old cache records. Fail gracefully.
        if (timeSet && (nowTime >= nextCacheFlushTime)) {
            if (asprintf(&SQL, "DELETE FROM cache WHERE expires <= %ld;", nowTime) >= 0) {
                sqliteErr = sqlite3_prepare_v2(cacheDB, SQL, (int)strlen(SQL), &stmt, NULL);
                free(SQL);
                if (sqliteErr != SQLITE_OK) {
                    reportSQLite3Err(cacheDB);
                } else {
                    sqliteErr = sqlite3_step(stmt);
                    if (sqliteErr != SQLITE_DONE) {
                        reportSQLite3Err(cacheDB);
                    }
                    sqliteErr = sqlite3_finalize(stmt);
                    if (sqliteErr != SQLITE_OK) {
                        reportSQLite3Err(cacheDB);
                    }
                }
            }
            nextCacheFlushTime = nowTime + CACHE_FLUSH_INTERVAL;
        }

        threadEndSignal(threadHandle);
    }

    ARLOGd("cparamSearch worker exiting.\n");

    // Cleanup CURL handle on exit.
    if (curlHandle) {
        curl_easy_cleanup(curlHandle);
        curlHandle = NULL;
    }

    return (NULL);
}

#endif // USE_CPARAM_SEARCH
