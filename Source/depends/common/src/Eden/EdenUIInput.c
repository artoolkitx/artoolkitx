//
//  EdenUIInput.c
//
//  Copyright (c) 2001-2018 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
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
//	Includes
// ============================================================================

#include <Eden/EdenUIInput.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>				// malloc(), calloc(), free(), exit()
#include <stdbool.h>
#ifdef _WIN32
#  include <sys/timeb.h>        // struct _timeb, _ftime
#else
#  include <sys/time.h>         // gettimeofday(), struct timeval
#endif
#include <pthread.h>

// ============================================================================
//  Types and constants
// ============================================================================

struct _EdenUIInput {
    pthread_mutex_t lock;       // Protects buf.
    unsigned char *buf;         // Holds prompt, input string, and any trailing cursor character.
    unsigned char *bufPtr;      // Pointer into buffer to first character of input string.
    unsigned int inputLen;      // Holds length of input string.
    unsigned int promptLen;     //
    int cursorState;
    pthread_cond_t completeCond;
    bool complete;              // Set to true once input is complete.
    
    // Input constraints.
    unsigned int minLen;        // Minimum length of user input string.
    unsigned int maxLen;        // Maximum length of user input string.
    bool intOnly;
    bool fpOnly;
    bool alphaOnly;
};

#pragma mark -

// ============================================================================
//  Public functions
// ============================================================================

EdenUIInput_t EdenUIInputNew(const unsigned char *prompt, const unsigned int minLength, const unsigned int maxLength, const bool intOnly, const bool fpOnly, const bool alphaOnly)
{
    if (minLength > maxLength) return NULL;

    EdenUIInput_t input = (EdenUIInput_t)calloc(1, sizeof(struct _EdenUIInput));
    if (!input) return (NULL);
    
    input->minLen = minLength;
    input->maxLen = maxLength;
    input->intOnly = intOnly;
    input->fpOnly = fpOnly;
    input->alphaOnly = alphaOnly;
    if (prompt) {
        input->promptLen = (unsigned int)strlen((const char *)prompt);
    }
    input->buf = (unsigned char *)malloc(input->promptLen + input->maxLen + 2); // +1 for cursor and +1 for nul-terminator.
    if (!input->buf) goto bail;
    if (prompt) {
        memcpy((void *)input->buf, (const void *)prompt, input->promptLen);
    }
    input->bufPtr = input->buf + input->promptLen;
    input->bufPtr[0] = '\0';
    input->inputLen = 0;
    input->cursorState = -1;
    input->complete = false;
    pthread_mutex_init(&input->lock, NULL);
    pthread_cond_init(&input->completeCond, NULL);
        
    return (input);
    
bail:
    free(input);
    return (NULL);    
}

bool EdenUIInputProcessKeystrokes(EdenUIInput_t input, const unsigned char keyAsciiCode)
{
    if (!input) return (false);
    
    bool ret = true;
    
	pthread_mutex_lock(&input->lock);
    if (input->bufPtr && !input->complete) {
        switch (keyAsciiCode) {
            case EDEN_ASCII_ESC:
                free(input->buf);
                input->buf = input->bufPtr = NULL;
                input->inputLen = 0;
                input->complete = true;
                pthread_cond_signal(&input->completeCond);
                break;
            case EDEN_ASCII_CR:
                if (input->inputLen >= input->minLen) {
                    input->complete = true;
                    pthread_cond_signal(&input->completeCond);
                }
                break;
            case EDEN_ASCII_BS:
            case EDEN_ASCII_DEL:
                if (input->inputLen > 0) {
                    input->inputLen--;
                    input->bufPtr[input->inputLen] = '\0';
                }
                break;
            default:
                if (keyAsciiCode < ' ') break;	// Throw away all other control characters.
                if (input->intOnly && (keyAsciiCode < '0' || keyAsciiCode > '9')) break;
                if (input->fpOnly && (keyAsciiCode < '0' || keyAsciiCode > '9') && keyAsciiCode != '.') break;
                if (input->alphaOnly && (keyAsciiCode < 'A' || keyAsciiCode > 'Z') && (keyAsciiCode < 'a' || keyAsciiCode > 'z')) break;
                if (input->inputLen < input->maxLen) {
                    input->bufPtr[input->inputLen] = keyAsciiCode;
                    input->inputLen++;
                    input->bufPtr[input->inputLen] = '\0';
                }
                break;
        }
    }
    pthread_mutex_unlock(&input->lock);

	return (ret);
}

unsigned char *EdenUIInputGetInputForDrawing(EdenUIInput_t input)
{
#ifdef _WIN32
    struct _timeb sys_time;
#else
    struct timeval time;
#endif
    int cursorState;

    // Check if we need to show a blinking cursor, and if so, what state it is in.
    if (input) {
        pthread_mutex_lock(&input->lock);
        if (input->bufPtr && !input->complete) {
#ifdef _WIN32
            _ftime(&sys_time);
            if (sys_time.millitm < 500ul) cursorState = 1;
            else cursorState = 0;
#else
#  if defined(__linux) || defined(__APPLE__) || defined(EMSCRIPTEN)
            gettimeofday(&time, NULL);
#  else
            gettimeofday(&time);
#  endif
            if (time.tv_usec < 500000) cursorState = 1;
            else cursorState = 0;
#endif
            if (cursorState != input->cursorState) {
                input->cursorState = cursorState;
                if (cursorState == 1) {
                    input->bufPtr[input->inputLen] = '|';
                    input->bufPtr[input->inputLen + 1] = '\0';
                } else {
                    input->bufPtr[input->inputLen] = ' ';
                    input->bufPtr[input->inputLen + 1] = '\0';
                }
            }
        }
        pthread_mutex_unlock(&input->lock);
    }
    return input->buf;
}

bool EdenUIInputIsComplete(EdenUIInput_t input)
{
	if (!input) return (true);

    pthread_mutex_lock(&input->lock);
    bool ret = input->complete;
    pthread_mutex_unlock(&input->lock);
    return (ret);
}

void EdenUIInputWaitComplete(EdenUIInput_t input)
{
	if (!input) return;

	// Wait for signal that input is complete.
    pthread_mutex_lock(&input->lock);
    while (!input->complete) {
        pthread_cond_wait(&input->completeCond, &input->lock);
    }
    pthread_mutex_unlock(&input->lock);
}

unsigned char *EdenUIInputGetInput(EdenUIInput_t input)
{
    unsigned char *ret;
    
    if (!input) return NULL;
    
    pthread_mutex_lock(&input->lock);
    if (input->bufPtr && input->complete) {
        input->bufPtr[input->inputLen] = '\0'; // Overwrite any cursor character.
        ret = (unsigned char *)strdup((char *)input->bufPtr);
    } else ret = NULL;
    pthread_mutex_unlock(&input->lock);
    
    return (ret);
}

void EdenUIInputDelete(EdenUIInput_t *input_p)
{
	if (!input_p || !*input_p) return;
    
    pthread_mutex_destroy(&(*input_p)->lock);
    free((*input_p)->buf);
    pthread_cond_destroy(&(*input_p)->completeCond);
    free(*input_p);
    (*input_p) = NULL;
}
