// Example program for Be condition variables, derived from the book
// _Programming With POSIX Threads_, David R. Butenhof, example 3.3.4
//
//	Copyright 1999, Be Incorporated.   All Rights Reserved.
//	This file may be used under the terms of the Be Sample Code License.

#include "condvar.h"
#include "errors.h"
#include <kernel/OS.h>
#include <stdio.h>

struct alarm_t
{
	alarm_t* link;
	int seconds;
	bigtime_t time;
	char message[64];
};

be_mutex_t alarm_mutex;
be_cond_t alarm_cond;
alarm_t* alarm_list = NULL;
volatile bigtime_t current_alarm = 0;

// alarm_insert() - insert the given alarm_t object into the sorted list
// of pending alarms.  The caller *must* lock alarm_mutex while calling
// this function!
void alarm_insert( alarm_t* alarm )
{
	status_t status;
	alarm_t **last, *next;

	last = &alarm_list;
	next = *last;

	// find the item before which to insert the new alarm
	while (next)
	{
		if (next->time >= alarm->time)
		{
			// "next" points to the first alarm that's further in the future than
			// the alarm argument, so we insert the new alarm before the
			// "next" element, then drop out of the insertion loop.
			alarm->link = next;
			*last = alarm;
			break;
		}

		// *next is still earlier than the new alarm, so continue traversing the list
		last = &next->link;
		next = next->link;
	}

	// if we reached the end of the list without finding a later alarm,
	// put the new one at the end of the list.
	if (next == NULL)
	{
		*last = alarm;
		alarm->link = NULL;
	}

#if DEBUG
	printf("[list: ");
	for ( next = alarm_list; next != NULL; next = next->link)
	{
		printf("%Ld(%Ld)[\"%s\"] ", next->time, next->time - system_time(), next->message);
	}
	printf("]\n");
#endif

	// now signal the condition variable, indicating that there is an alarm in the list.
	// as an optimization, we only signal when the alarm we just added is earlier
	// than the one currently being waited for
	if (current_alarm == 0 || alarm->time < current_alarm)
	{
		current_alarm = alarm->time;
		status = ::be_cond_signal(&alarm_cond);
		if (status != 0)
			err_abort(status, "Signal cond");
	}
	else
	{
	}
}

// this is the separate thread that issues alarm messages.  it waits on the alarm
// condition variable 
int32 alarm_thread(void*)		// we don't use the thread function's parameter
{
	alarm_t* alarm;
	bigtime_t cond_time;
	bigtime_t now;
	status_t status;
	bool expired;

	// loop forever, processing the alarm queue
	::be_mutex_lock(&alarm_mutex);
	while (true)
	{
		// the predicate associated with the condition variable is whether the
		// alarm list is empty.  Check that condition, and it it's not true wait
		// until someone signals to indicate that it *is* true.
		current_alarm = 0;
		while (alarm_list == NULL)
		{
			status = ::be_cond_wait(&alarm_cond, &alarm_mutex);
			if (status)
				err_abort(status, "Wait on cond");
		}

		// we reach here when the alarm condition variable was signalled to
		// indicate that the alarm list is non-empty.  We remove the earliest
		// alarm from the list, and decide how to handle it.
		alarm = alarm_list;
		alarm_list = alarm->link;
		now = system_time();

		// predicate:  is it time for an alarm to be reported?
		expired = false;

		// if the next alarm is still in the future, figure out when that alarm's
		// time is and wait on the condition variable until that time.  If we are
		// signalled before then, it means that a new alarm was added to the
		// list, and we should check that one to see if it's sooner than the one
		// we removed from the list upon being signalled.  If we time out, it
		// means we reached the next alarm's time, so we set the "expired" flag.
		if (alarm->time > now)
		{
#if DEBUG
			printf("[waiting: %Ld(%Ld)\"%s\"]\n",
				alarm->time, alarm->time - system_time(), alarm->message);
#endif
			cond_time = alarm->time;
			current_alarm = alarm->time;
			while (current_alarm == alarm->time)
			{
				status = ::be_cond_timedwait(&alarm_cond, &alarm_mutex, cond_time);
				if (status == B_TIMED_OUT)
				{
					// Aha!  We reached the timeout without another alarm being added to
					// the queue.  This means that it's time to report the alarm to the user, and
					// go on to the next alarm.
					expired = true;
					break;
				}
				if (status)
					err_abort(status, "Cond timedwait");
			}

			// if the timer didn't expire, it means that we were awakened by a signal.
			// This, in turn, means that the alarm we're holding isn't necessarily the earliest
			// any more, so we put it back into the queue and start over.
			if (!expired)
				alarm_insert(alarm);
		}
		else
		{
			// if we get here, it means that the alarm we're holding is stamped to occur at
			// some time in the past.  That means it's inherently expired, and we should report
			// it immediately.
			expired = true;
		}

		// if the alarm we've been holding has expired, we report it, free the
		// alarm_t structure, and continue waiting for alarms.
		if (expired)
		{
			printf("(%d) %s\n", alarm->seconds, alarm->message);
			free(alarm);
		}
	}
}

int main(int, char**)
{
	// Initialize the mutex and the condition variable
	::be_mutex_init(&alarm_mutex, NULL);
	::be_cond_init(&alarm_cond, NULL);

	// spin off the alarm thread
	resume_thread(spawn_thread(alarm_thread, "alarm thread", B_NORMAL_PRIORITY, NULL));

	// issue a few instructions
	printf("Enter a number of seconds to wait followed by a string to print\nonce that amount of time expires,\n"
		"e.g. \"5 this message will be printed after five seconds\"\n\nType Control-D to quit.\n\n");

	// keep asking the user for new alarm timeouts
	while (true)
	{
		char line[128];
		alarm_t* alarm = (alarm_t*) malloc(sizeof(alarm_t));
		if (!alarm)
			errno_abort("Allocate alarm");

		// Prompt and read an alarm line; exiting when EOF is reached (i.e. the user
		// types a Control-D)
		printf("Alarm> ");
		if (fgets(line, sizeof(line), stdin) == NULL)
		{
			printf("\n");
			exit(0);
		}
		if (strlen(line) <= 1) continue;

		// parse input line into seconds (%d) and a message (64 non-newline chars)
		if (sscanf(line, "%d %64[^\n]", &alarm->seconds, alarm->message) < 2)
		{
			fprintf(stderr, "Bad command\n");
			free(alarm);
		}
		else
		{
			// Fill out the new alarm with the time in the future provided by the user,
			// converted into the same time base that system_time() uses.
			alarm->time = system_time() + (1000000LL * alarm->seconds);		// microseconds

			// add the new alarm to the queue -- we must lock the queue access
			// mutex when calling alarm_insert()
			::be_mutex_lock(&alarm_mutex);
			alarm_insert(alarm);
			::be_mutex_unlock(&alarm_mutex);
		}
	}

	return 0;
}
