/*
 *  thread_sub.c
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

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ARX/ARUtil/thread_sub.h>
//#define ARUTIL_DISABLE_PTHREADS // Uncomment to disable pthreads support.

#if !defined(_WINRT) && !defined(ARUTIL_DISABLE_PTHREADS)
#  include <pthread.h>
#else
#  define pthread_mutex_t               CRITICAL_SECTION
#  define pthread_mutex_init(pm, a)     InitializeCriticalSectionEx(pm, 4000, CRITICAL_SECTION_NO_DEBUG_INFO)
#  define pthread_mutex_lock(pm)        EnterCriticalSection(pm)
#  define pthread_mutex_unlock(pm)      LeaveCriticalSection(pm)
#  define pthread_mutex_destroy(pm)     DeleteCriticalSection(pm)
#  define pthread_cond_t                CONDITION_VARIABLE
#  define pthread_cond_init(pc, a)      InitializeConditionVariable(pc)
#  define pthread_cond_wait(pc, pm)     SleepConditionVariableCS(pc, pm, INFINITE)
#  define pthread_cond_signal(pc)       WakeConditionVariable(pc)
#  define pthread_cond_broadcast(pc)    WakeAllConditionVariable(pc)
#  define pthread_cond_destroy(pc)
#  ifdef _WINRT
#    include "thread_sub_winrt.h"
#  else
#    include <process.h> // _beginthread
#  endif
#endif

struct _THREAD_HANDLE_T {
    int             ID;
    int             startF; // 0 = no request pending, 1 = start please, 2 = quit please.
    int             endF;   // 0 = worker not started or worker running, 1 = worker completed, 2 = worker will quit (exit).
    int             busyF;  // 0 = worker not started or worker ended, 1 = worker busy.
    //pthread_t       thread;
    pthread_mutex_t mut;
    pthread_cond_t  cond1; // Signals from client that startF has changed.
    pthread_cond_t  cond2; // Signals from worker that endF has changed.
    void           *arg;
};

//
// Worker-side.
//

int threadStartWait( THREAD_HANDLE_T *flag )
{
    pthread_mutex_lock(&(flag->mut));
    while(flag->startF == 0) {
        pthread_cond_wait(&(flag->cond1), &(flag->mut));
    }
    if( flag->startF == 1 ) {
        flag->startF = 0;
        flag->busyF = 1;
        pthread_mutex_unlock(&(flag->mut));
        return 0;
    }
    else {
        flag->endF = 2;
        pthread_cond_signal(&(flag->cond2));
        pthread_mutex_unlock(&(flag->mut));
        return -1;
    }
}

int threadEndSignal( THREAD_HANDLE_T *flag )
{
    pthread_mutex_lock(&(flag->mut));
    flag->endF = 1;
    flag->busyF = 0;
    pthread_cond_signal(&(flag->cond2));
    pthread_mutex_unlock(&(flag->mut));
    return 0;
}

int threadGetID( THREAD_HANDLE_T *flag )
{
    return (flag->ID);
}

void *threadGetArg( THREAD_HANDLE_T *flag )
{
    return (flag->arg);
}

//
// Client-side.
//

// A construct to manage the difference in start routine signature between pthreads and windows threads.
#if defined(_WIN32) && !defined(_WINRT)
struct start_routine_proxy_arg {
	void *(*start_routine)(THREAD_HANDLE_T*);
	void *arg;
};
static void __cdecl start_routine_proxy(void *arg)
{
	struct start_routine_proxy_arg *arg0 = (struct start_routine_proxy_arg *)arg;
	(*(arg0->start_routine))(arg0->arg);
	free(arg0);
}
#endif

THREAD_HANDLE_T *threadInit( int ID, void *arg, void *(*start_routine)(THREAD_HANDLE_T*) )
{
    THREAD_HANDLE_T    *flag;
    int err;
#if !defined(_WINRT) && !defined(ARUTIL_DISABLE_PTHREADS)
    pthread_t           thread;
    pthread_attr_t      attr;
#endif
    if ((flag = malloc(sizeof(THREAD_HANDLE_T))) == NULL) return NULL;

    flag->ID     = ID;
    flag->startF = 0;
    flag->endF   = 0;
    flag->busyF  = 0;
    flag->arg    = arg;
    pthread_mutex_init( &(flag->mut), NULL );
    pthread_cond_init( &(flag->cond1), NULL );
    pthread_cond_init( &(flag->cond2), NULL );

#if !defined(_WINRT) && !defined(ARUTIL_DISABLE_PTHREADS)
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1); // Preclude the need to do pthread_join on the thread after it exits.
    err = pthread_create(&thread, &attr, (void *(*)(void*))start_routine, flag);
    pthread_attr_destroy(&attr);
#elif defined(_WIN32)
#  ifdef _WINRT
    err = arCreateDetachedThreadWinRT(start_routine, flag);
#  else
	struct start_routine_proxy_arg *srpa_p = malloc(sizeof(struct start_routine_proxy_arg));
	srpa_p->start_routine = start_routine;
	srpa_p->arg = flag;
	err = (_beginthread(start_routine_proxy, 0, srpa_p) == -1L);
#  endif
#else
#  error No routine available to create a thread.
#endif
    if (err == 0) {
        return flag;
    } else {
        threadFree(&flag);
        return NULL;
    }

}

int threadFree( THREAD_HANDLE_T **flag )
{
    pthread_mutex_destroy(&((*flag)->mut));
    pthread_cond_destroy(&((*flag)->cond1));
    pthread_cond_destroy(&((*flag)->cond2));
    free( *flag );
    *flag = NULL;
    return 0;
}

int threadStartSignal( THREAD_HANDLE_T *flag )
{
    pthread_mutex_lock(&(flag->mut));
    flag->startF = 1;
    pthread_cond_signal(&(flag->cond1));
    pthread_mutex_unlock(&(flag->mut));
    return 0;
}

int threadGetStatus( THREAD_HANDLE_T *flag )
{
    int  endFlag;
    pthread_mutex_lock(&(flag->mut));
    endFlag = flag->endF;
    pthread_mutex_unlock(&(flag->mut));

    return endFlag;
}

int threadGetBusyStatus( THREAD_HANDLE_T *flag )
{
    int  busyFlag;
    pthread_mutex_lock(&(flag->mut));
    busyFlag = flag->busyF;
    pthread_mutex_unlock(&(flag->mut));
    
    return busyFlag;
}

int threadEndWait( THREAD_HANDLE_T *flag )
{
    pthread_mutex_lock(&(flag->mut));
    while (flag->endF == 0) {
        pthread_cond_wait(&(flag->cond2), &(flag->mut));
    }
    flag->endF = 0;
    pthread_mutex_unlock(&(flag->mut));
    return 0;
}

int threadWaitQuit( THREAD_HANDLE_T *flag )
{
    pthread_mutex_lock(&(flag->mut));
    flag->startF = 2;
    pthread_cond_signal(&(flag->cond1));
    while (flag->endF != 2) {
        pthread_cond_wait(&(flag->cond2), &(flag->mut));
    }
    return 0;
}

int threadGetCPU(void)
{
#ifdef _WIN32
    SYSTEM_INFO   info;

#  ifndef _WINRT
    GetSystemInfo(&info);
#  else
    GetNativeSystemInfo(&info);
#  endif
    return info.dwNumberOfProcessors;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
