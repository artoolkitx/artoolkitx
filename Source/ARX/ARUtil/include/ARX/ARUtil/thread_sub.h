/*
 *  thread_sub.h
 *  artoolkitX
 *
 *  Implements a basic client-worker threading system.
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*
    thread_sub.c, thread_sub.h

    Written by Hirokazu Kato
    kato@is.naist.jp   Apr.24 2007
 */

#ifndef __ARUtil_thread_sub_h__
#define __ARUtil_thread_sub_h__

#include <ARX/ARUtil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _THREAD_HANDLE_T THREAD_HANDLE_T;

//
// Client-side.
//

// Setup.
ARUTIL_EXTERN THREAD_HANDLE_T *threadInit( int ID, void *arg, void *(*start_routine)(THREAD_HANDLE_T*) ); // Create a new thread, and launch start_routine() on it. Returns NULL in case of failure.
ARUTIL_EXTERN int threadFree( THREAD_HANDLE_T **flag ); // Frees structures associated with the thread handle pointed to by the location pointed to by flag. Thread should already have terminated (i.e. threadWaitQuit() has returned). Location pointed to by flag is set to NULL. 

// Communication.
ARUTIL_EXTERN int threadStartSignal( THREAD_HANDLE_T *flag ); // Send the worker thread the "start processing" request.
ARUTIL_EXTERN int threadGetStatus( THREAD_HANDLE_T *flag );   // Find out (without waiting) whether a worker has ended. 0 = not started or started but not yet ended, 1 = ended.
ARUTIL_EXTERN int threadGetBusyStatus( THREAD_HANDLE_T *flag ); // Find out if a worker is currently busy. 0 = worker not started or worker ended, 1 = worker started but not yet ended.
ARUTIL_EXTERN int threadEndWait( THREAD_HANDLE_T *flag );     // Wait for thread to end processing.
ARUTIL_EXTERN int threadWaitQuit( THREAD_HANDLE_T *flag );    // Tell a thread waiting for the "start processing" request to quit (exit), and wait until this has happened.

// Example client structure:
//
//    THREAD_HANDLE_T *threadHandle = threadInit(0, NULL, worker);
//    if (!threadHandle) {
//        fprintf(stderr, "Error starting thread.\n");
//        exit(-1);
//    }
//
//    // Ready to do some work:
//    threadStartSignal(threadHandle);
//
//    // Wait for the results.
//    threadEndWait(threadHandle);
//
//    // If all done, quit and cleanup.
//    threadWaitQuit(threadHandle);
//    threadFree(&threadHandle);

//
// Worker-side.
//
    
ARUTIL_EXTERN int threadStartWait( THREAD_HANDLE_T *flag );   // Wait for the "start processing" request. Returns 0 if worker should start, or -1 if worker should quit (exit).
ARUTIL_EXTERN int threadEndSignal( THREAD_HANDLE_T *flag );   // Notify anyone waiting that the worker has ended.
ARUTIL_EXTERN int threadGetID( THREAD_HANDLE_T *flag );       // Get the 'ID' value passed to the thread's start routine.
ARUTIL_EXTERN void *threadGetArg( THREAD_HANDLE_T *flag );    // Get the 'arg' value passed to the thread's start routine.

// Example worker structure:
//
//    void *worker(THREAD_HANDLE_T *threadHandle)
//    {
//        void *arg = threadHandle->arg; // Get data passed to the worker.
//        while (threadStartWait(threadHandle) == 0) {
//            // Do work, probably on arg.
//            threadEndSignal(threadHandle);
//        }
//        // Do cleanup.
//        return (NULL);
//    }

ARUTIL_EXTERN int threadGetCPU(void); // Returns the number of online CPUs in the system.


#ifdef __cplusplus
}
#endif
#endif // !__ARUtil_thread_sub_h__
