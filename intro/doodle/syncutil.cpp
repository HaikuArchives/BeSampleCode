/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Looper.h>
#include <Window.h>
#include <OS.h>
#include "syncutil.h"

// WaitForDelete
// -------------
// blocker: a semaphore which should have a count < 1
//
// Blocks the current thread, updating if it's a window, until the
// blocker semaphore is deleted.
status_t WaitForDelete(sem_id blocker)
{
	status_t	result;
	thread_id	this_tid = find_thread(NULL);
	BLooper		*pLoop;
	BWindow		*pWin = 0;

	pLoop = BLooper::LooperForThread(this_tid);
	if (pLoop)
		pWin = dynamic_cast<BWindow*>(pLoop);

	// block until semaphore is deleted (modal is finished)
	if (pWin) {
		do {
			// update the window periodically
			pWin->UpdateIfNeeded();
			result = acquire_sem_etc(blocker, 1, B_TIMEOUT, 50000);
		} while (result != B_BAD_SEM_ID);
	} else {
		do {
			// just wait for exit
			result = acquire_sem(blocker);
		} while (result != B_BAD_SEM_ID);
	}
	return result;
}
