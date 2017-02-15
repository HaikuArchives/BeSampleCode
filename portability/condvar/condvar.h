/*
	BeOS condition variable compatibility layer, POSIX-style
	Written by Christopher Tate and Owen Smith

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _be_condvar_h
#define _be_condvar_h 1

#include <kernel/OS.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct be_mutex_t
{
	int32			init;
	sem_id		lock;
	thread_id		owner;
} be_mutex_t;

#define BE_MUTEX_INITIALIZER { 2000, -1, -1 }

typedef struct be_cond_t
{
	int32				init;
	sem_id			sem;
	sem_id			handshakeSem;
	sem_id			signalSem;
	volatile int32	nw;
	volatile int32	ns;
} be_cond_t;

#define BE_COND_INITIALIZER { 2000, -1, -1, -1, -1, -1 }

/* attributes are currently unused */
typedef int32 be_mutexattr_t;
typedef int32 be_condattr_t;

/* mutex operations */
status_t be_mutex_init(be_mutex_t* mutex, be_mutexattr_t* attr);
status_t be_mutex_destroy(be_mutex_t* mutex);
status_t be_mutex_lock(be_mutex_t* mutex);
status_t be_mutex_unlock(be_mutex_t* mutex);

/* condition variable operations */
status_t be_cond_init(be_cond_t* condvar, be_condattr_t* attr);
status_t be_cond_destroy(be_cond_t* condvar);
status_t be_cond_wait(be_cond_t* condvar, be_mutex_t* mutex);
status_t be_cond_timedwait(be_cond_t* condvar, be_mutex_t* mutex, bigtime_t absolute_timeout_time);
status_t be_cond_signal(be_cond_t* condvar);
status_t be_cond_broadcast(be_cond_t* condvar);

#ifdef __cplusplus
}
#endif

#endif
