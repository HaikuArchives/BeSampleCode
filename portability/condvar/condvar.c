/*
	BeOS user-space condition variable compatibility library
	Written by Christopher Tate and Owen Smith

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	This implementation currently uses BeOS kernel semaphores as its basic
	locking primitive.  Using benaphores instead would make the code a bit
	more complex, but would improve the performance of the library in
	low-contention situations.
*/

#include "condvar.h"
#include <kernel/OS.h>

// mutex operations
static void lazy_init_mutex(be_mutex_t* mutex)
{
	int32 v = atomic_or(&mutex->init, 1);
	if (2000 == v)			// we're the first, so do the init
	{
		be_mutex_init(mutex, NULL);
	}
	else							// we're not the first, so wait until the init is finished
	{
		while (mutex->init != 9999) snooze(10000);
	}
}

status_t be_mutex_init(be_mutex_t* mutex, be_mutexattr_t* attr)
{
	// prevent an "unused parameter" warning; this will be optimized away
	if (attr) { /* do nothing */ }

	// check the arguments and whether it's already been initialized
	if (!mutex) return B_BAD_VALUE;
	if (mutex->init == 9999) return EALREADY;

	mutex->lock = create_sem(1, "BeMutex");
	mutex->owner = -1;
	mutex->init = 9999;
	return B_OK;
}

status_t be_mutex_destroy(be_mutex_t* mutex)
{
	if (!mutex) return B_BAD_VALUE;
	if (mutex->init != 9999) return B_NO_INIT;
	delete_sem(mutex->lock);
	return B_OK;
}

status_t be_mutex_lock(be_mutex_t* mutex)
{
	status_t err;

	if (!mutex) return B_BAD_VALUE;
	if (mutex->init < 2000) return B_NO_INIT;
	lazy_init_mutex(mutex);

	err = acquire_sem(mutex->lock);
	if (!err) mutex->owner = find_thread(NULL);
	return err;
}

status_t be_mutex_unlock(be_mutex_t* mutex)
{
	if (!mutex) return B_BAD_VALUE;
	if (mutex->init < 2000) return B_NO_INIT;
	lazy_init_mutex(mutex);

	if (mutex->owner != find_thread(NULL)) return ENOLCK;
	mutex->owner = -1;
	release_sem(mutex->lock);
	return B_OK;
}

//#pragma mark -

// condition variable operations

static void lazy_init_cond(be_cond_t* condvar)
{
	int32 v = atomic_or(&condvar->init, 1);
	if (2000 == v)			// we're the first, so do the init
	{
		be_cond_init(condvar, NULL);
	}
	else							// we're not the first, so wait until the init is finished
	{
		while (condvar->init != 9999) snooze(10000);
	}
}

status_t be_cond_init(be_cond_t* condvar, be_condattr_t* attr)
{
	// prevent an "unused parameter" warning
	if (attr) { /* do nothing */ }

	// now for bogus arguments and multiple initialization
	if (!condvar) return B_BAD_VALUE;
	if (condvar->init == 9999) return EALREADY;

	condvar->sem = create_sem(0, "CVSem");
	condvar->handshakeSem = create_sem(0, "CVHandshake");
	condvar->signalSem = create_sem(1, "CVSignal");
	condvar->ns = condvar->nw = 0;
	condvar->init = 9999;
	return B_OK;
}

status_t be_cond_destroy(be_cond_t* condvar)
{
	if (!condvar) return B_BAD_VALUE;
	if (condvar->init < 2000) return B_NO_INIT;

	condvar->init = 0;
	delete_sem(condvar->sem);
	delete_sem(condvar->handshakeSem);
	delete_sem(condvar->signalSem);
	return B_OK;
}

// wait on a condition variable -- must be called with the mutex held
status_t be_cond_wait(be_cond_t* condvar, be_mutex_t* mutex)
{
	status_t err;

	// validate the arguments and lazy-initialize the condition variable structure
	// if necessary
	if (!condvar) return B_BAD_VALUE;
	if (!mutex) return B_BAD_VALUE;
	if (condvar->init < 2000) return B_NO_INIT;
	lazy_init_cond(condvar);

	// record the fact that we're waiting on the semaphore.  This action is
	// protected by a mutex because exclusive access to the waiter count is
	// needed by both waiting threads and signalling threads.  If someone interrupts
	// us while we're waiting for the lock (e.g. by calling kill() or send_signal()), we
	// abort and return the appropriate failure code.
	if (acquire_sem(condvar->signalSem) == B_INTERRUPTED) return B_INTERRUPTED;
	condvar->nw += 1;
	release_sem(condvar->signalSem);

	// actually wait for a signal -- we have to unlock the mutex before calling the
	// underlying blocking primitive.  The potential preemption between unlocking
	// the mutex and calling acquire_sem() is why we needed to record, prior to
	// this point, that we're in the process of waiting on the condition variable.
	be_mutex_unlock(mutex);
	err = acquire_sem(condvar->sem);

	// we just awoke, either via a signal or by being interrupted.  If there's
	// a signaller running, he'll think he needs to handshake whether or not
	// we actually awoke due to his signal.  So, we reacquire the signalSem
	// mutex, and handshake if there's a positive signaller count.  It's critical
	// that we continue with the handshake process even if we've been interrupted,
	// so we just set the eventual error code and proceed with the CV state
	// unwinding in that case.
	while (acquire_sem(condvar->signalSem) == B_INTERRUPTED) { err = B_INTERRUPTED; }
	if (condvar->ns > 0)
	{
		release_sem(condvar->handshakeSem);
		condvar->ns -= 1;
	}
	condvar->nw -= 1;
	release_sem(condvar->signalSem);

	// always reacquire the mutex before returning, even in error cases
	while (be_mutex_lock(mutex) == B_INTERRUPTED) { err = B_INTERRUPTED; }
	return err;
}

// wait for a signal, with a timeout.  as with POSIX condition variables,
// the timeout is in ABSOLUTE time -- it's not a number of microseconds
// to wait, it's the system_time() at which to awaken.
status_t be_cond_timedwait(be_cond_t* condvar, be_mutex_t* mutex, bigtime_t expiration)
{
	status_t err;

	if (!condvar) return B_BAD_VALUE;
	if (!mutex) return B_BAD_VALUE;
	if (condvar->init < 2000) return B_NO_INIT;
	lazy_init_cond(condvar);

	if (acquire_sem(condvar->signalSem) == B_INTERRUPTED) return B_INTERRUPTED;
	condvar->nw += 1;
	release_sem(condvar->signalSem);
	be_mutex_unlock(mutex);

	err = acquire_sem_etc(condvar->sem, 1, B_ABSOLUTE_TIMEOUT, expiration);

	while (acquire_sem(condvar->signalSem) == B_INTERRUPTED)
	{
		if (!err) err = B_INTERRUPTED;		// preserve any "timed out" return code
	}
	if (condvar->ns > 0)
	{
		release_sem(condvar->handshakeSem);
		condvar->ns -= 1;
	}
	condvar->nw -= 1;
	release_sem(condvar->signalSem);

	while (be_mutex_lock(mutex) == B_INTERRUPTED)
	{
		if (!err) err = B_INTERRUPTED;
	}
	return err;
}

// be_cond_signal() - signal one waiter
status_t be_cond_signal(be_cond_t* condvar)
{
	status_t err = B_OK;

	if (!condvar) return B_BAD_VALUE;
	if (condvar->init < 2000) return B_NO_INIT;
	lazy_init_cond(condvar);

	// we need exclusive access to the waiter count while we figure out whether
	// we need a handshake with an awakening waiter thread
	if (acquire_sem(condvar->signalSem) == B_INTERRUPTED) return B_INTERRUPTED;

	// are there waiters to be awakened?
	if (condvar->nw > condvar->ns)
	{
		// inform the next awakening waiter that we need a handshake, then release
		// all the locks and block until we get the handshake.  We need to go through the
		// handshake process even if we're interrupted, to avoid breaking the CV, so we
		// just set the eventual return code if we are interrupted in the middle.
		condvar->ns += 1;
		release_sem(condvar->sem);
		release_sem(condvar->signalSem);
		while (acquire_sem(condvar->handshakeSem) == B_INTERRUPTED) { err = B_INTERRUPTED; }
	}
	else		// nobody is waiting, so the signal operation is a no-op
	{
		release_sem(condvar->signalSem);
	}
	return err;
}

// be_cond_broadcast() - signal all waiters
status_t be_cond_broadcast(be_cond_t* condvar)
{
	int32 handshakes;
	status_t err = B_OK;

	if (!condvar) return B_BAD_VALUE;
	if (condvar->init < 2000) return B_NO_INIT;
	lazy_init_cond(condvar);

	// as in be_cond_signal(), we need to lock access to the waiter count
	// while we're determining how many handshakes we'll need.
	if (acquire_sem(condvar->signalSem) == B_INTERRUPTED) return B_INTERRUPTED;

	// are there waiters to be awakened?
	if (condvar->nw > condvar->ns)
	{
		handshakes = condvar->nw - condvar->ns;		// condvar->nw will be invalid after releasing signalSem
		condvar->ns = condvar->nw;								// all waiters are accounted for now
		release_sem_etc(condvar->sem, handshakes, 0);		// signal all waiter threads
		release_sem(condvar->signalSem);

		// the waiters are running now, so we wait for all of the handshakes.  Again,
		// we need to retry in the case of interruptions, so we set the return code and
		// loop in that case.
		while (acquire_sem_etc(condvar->handshakeSem, handshakes, 0, 0) == B_INTERRUPTED) { err = B_INTERRUPTED; }
	}
	else
	{
		// when there are no waiters, the broadcast doesn't do anything, so we
		// just release the waiter-count lock and drop out of the function.
		release_sem(condvar->signalSem);
	}
	return err;
}
