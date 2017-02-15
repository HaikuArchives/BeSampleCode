/* multilocker test */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEBUG 1

#include "MultiLocker.h"
#include <List.h>
#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint32 counter;
MultiLocker *lock;

int32 read_func(void *arg) {
	int32 num = (int32) arg;
	bool run = true;

//	printf("reader %d starting up\n", num);
	snooze(1000000);
	
	while (run == true) {
		//lock the locker
		if (lock->ReadLock()) {
//			if (counter%25 == 0) printf("reader %d> %d\n", num, counter);
			if (counter > 100) run = false;
//			if (counter > 100) printf("reader %d> counter 100+\n", num);
			lock->ReadUnlock();
			snooze(100000);
		} else printf("reader %ld> cannot lock multilock!\n", num);	
	}
	
//	printf("reader %d> quitting\n", num);
	return B_OK;
}

int32 write_func(void *arg) {

	bool run = true;
	
//	int32 num = (int32)arg;
//	printf("writer %d starting up\n", num);
	snooze(1000000);
	while (run == true) {

		//lock the locker
		if (lock->WriteLock()) {
			counter++;
//			printf("writer %d> count now %d\n", num, counter);

/**/
		//comment or uncomment this section to test for writer ReadLocking			
			lock->ReadLock();
			lock->ReadUnlock();
/**/			
/**/			
		//comment or uncomment to test for nested WriteLocks
			if (lock->WriteLock()) {
				lock->WriteUnlock();
			}
/**/
			
			if (counter > 100) run = false;
			lock->WriteUnlock();
			snooze(500000);
		} //else printf("writer %d> cannot lock multilock!\n", num);	
	}
	
//	printf("writer %d> quitting\n", num);
	return B_OK;
}


const int32 NUM_READERS = 10;
const int32 NUM_WRITERS = 2;

int main(int argc, char **argv) {

	int32 num_readers = 10;
	int32 num_writers = 2;
	
	if (argc > 2) {
		for (int i = 1; i < argc; i++) {
			char *param = argv[i];
			if (param[0] == 'r' && !strncmp(param, "read=", 5)) {
				num_readers = atoi(&param[5]);
			}
			else if (param[0] == 'w' && !strncmp(param, "write=", 6)) {
				num_writers = atoi(&param[6]);
			}
		}
	}

	printf("TestMultiLock\nReaders:%ld | Writers: %ld\n", num_readers, num_writers);
	//a multilocker to protect our counter
	lock = new MultiLocker();
	counter = 0;
	
	char name[64];
	
	printf("spawn readers\n");
	for (int i = 1; i <= num_readers; i++) {
		sprintf(name, "reader %d", i);
		thread_id thread = spawn_thread(read_func, name, B_LOW_PRIORITY, (void *)i);
		snooze(100000);
		resume_thread(thread);
	}			
	
	printf("spawn writers\n");
	for (int i = 1; i <= num_writers; i++) {
		sprintf(name, "writer %d", i);
		thread_id thread = spawn_thread(write_func, name, B_NORMAL_PRIORITY, (void *)i);
		snooze(100000);
		resume_thread(thread);
		printf("write_thread: %ld\n", thread);
	}
	
	while (counter <= 100) {
		snooze(1000000);
	}
	//snooze an extra second
	snooze(1000000);
	
	delete lock;
}

