// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

struct thread_time
{
	ulong	usertime;
	ulong	kerntime;
};
struct thread_data
{
	thread_id thread;
	char name[B_OS_NAME_LENGTH+1];
	bool displayit;
	bigtime_t totaluser;
	bigtime_t totalkern;
	thread_time *times;
};

extern thread_data thread[];
