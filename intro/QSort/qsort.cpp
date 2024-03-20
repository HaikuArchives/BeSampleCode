/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <OS.h>
#include <List.h>
#include <StopWatch.h>

long Partition(void *castToWorkUnit);

/* for experimentation purposes, these are global */
int32 SIZE;
int32 THREADS;
int32 THRESH;

class QSort;
bool Verify(int32*, int32);

class WorkUnit
{
 public:
	WorkUnit(int32 first, int32 second, QSort *qs, bool stat)
		: start(first), end(second), caller(qs), spawned(stat) { }
	int32 start;
	int32 end;
	QSort *caller;
	bool spawned;
};


class QSort
{
	friend long Partition(void*);

 public:
	static void Sort(int32 *vector, int32 count);
	
 private:

	QSort(int32 *vector, int32 count);

	static inline void Swap(int32 *x, int32 *y);
	void StartSort();

	int32 cpu_count;
	int32 *list;

	int32 num_elements;

	int32 done_counter;
	sem_id done, workthread;		
};


void
QSort::Sort(int32 *vector, int32 count)
{
	QSort(vector, count);
}


QSort::QSort(int32 *vector, int32 count)
{
	system_info info;
	get_system_info(&info);
	cpu_count = info.cpu_count;

	// this is our "pool" of worker threads...	
	workthread = create_sem(THREADS, "workthread"); // for experimenting
	// for real code, you'd have a thread count of cpu_count...
	// workthread = create_sem(cpu_count, "workthread");

	if (workthread < B_NO_ERROR)
	{
		fprintf(stderr, "%s\n", "Couldn't create workthread semaphore!");
		return;
	}

	list = vector;
	num_elements = count;
	
	// this is how many elements are yet to be sorted...
	done_counter = count;

	// the program will block on this semaphore until done_counter == 0
	done = create_sem(0, "done");
	if (done < B_NO_ERROR)
	{
		fprintf(stderr, "%s\n", "Couldn't create done semaphore!");
		return;
	}
	
	// begin sorting...
	StartSort();	
}

void
QSort::Swap(int32 *x, int32 *y)
{
	int32 tmp = *x;
	*x = *y;
	*y = tmp;
}

void
QSort::StartSort()
{
	thread_id tid;
	
	// decrease the number of available workthreads in the pool by 1...
	acquire_sem(workthread);

	// spawn the initial thread...
	tid = spawn_thread(Partition, "Partition", B_NORMAL_PRIORITY,
						new WorkUnit(0, num_elements-1, this, true));

	// even if spawn_thread fails, we can still continue...
	if (tid < 0)
	{
		Partition(new WorkUnit(0, num_elements-1, this, false));
	}
	else
	{
		resume_thread(tid);
	}
	
	// now wait for the sort to finish...
	acquire_sem(done);

}


long
Partition(void *castToWorkUnit)
{
	WorkUnit *unit = (WorkUnit*)castToWorkUnit;

	int32 start = unit->start;
	int32 end = unit->end;
	QSort *thisclass = unit->caller;
	int32 *list = thisclass->list;

	// if small enough, use insertition sort...
	if (end - start+1 < 20)
	{
		int32 i, j, key;
		for(j = start; j <= end; j++)
		{
			key = list[j];

			// NOTE!  Below we're comparing the LOGS of the elements rather
			// than the elements themselves!  This is NOT part of the algorithm!
			// Read the article to understand why we're doing this!
			
			for (i=j-1; i >= 0 && log((double)key) < log((double)list[i]); i--)
			{
				list[i+1] = list[i];
			}
			list[i+1] = key;
		}
		
		// we now have (end-start+1) more elements in place...
		atomic_add(&thisclass->done_counter, -(end-start+1));
		
		// check if we're done...
		if (!thisclass->done_counter)
		{
			release_sem(thisclass->done);
		}
						
		// if we were spawned, then "return" a thread back into the pool...
		if (unit->spawned)
		{
			release_sem(thisclass->workthread);
		}
		
		delete unit;
		return 0;
	}
	

	// calculate the pivot	
	int32 sum = 0;
	for (int x=start; x <= end; x++)
	{
		sum += list[x];
	}

	register int32 pivot = sum / (end-start+1);
	
	int32 i = start-1;
	int32 j = end+1;
	
	// now partition this sub-array around the pivot...
	while (true)
	{				
		// NOTE!  Below we're comparing the LOGS of the elements rather
		// than the elements themselves!  This is NOT part of the algorithm!
		// Read the article to understand why we're doing this!

		// find elements to swap...
		do i++; while(log((double)list[i]) < log((double)pivot));
		do j--; while(log((double)list[j]) > log((double)pivot));

		if (i < j)
		{
			QSort::Swap(&list[i], &list[j]);
		}
		else
		{
			// this sub-array is partitioned around the pivot...

			thread_id tid;
			bool big = (end-start+1 > THRESH);
			
			// only spawn a new thread if we have a "significant" amount of work
			// (as determined by THRESH) and if there's a thread available...
			
			if (big && acquire_sem_etc(thisclass->workthread, 1, B_TIMEOUT, 0) == B_NO_ERROR)
			{
				tid = spawn_thread(Partition, "Partition", B_NORMAL_PRIORITY,
									new WorkUnit(start, j, thisclass, true));
				
				// if spawn_thread failed, we can still continue...
				if (tid < 0)
				{
					Partition(new WorkUnit(start, j, thisclass, false));
				}
				else
				{				
					resume_thread(tid);
				}

			}
			else
			{
				Partition(new WorkUnit(start, j, thisclass, false));
			}

			if (big && acquire_sem_etc(thisclass->workthread, 1, B_TIMEOUT, 0) == B_NO_ERROR)
			{
				tid = spawn_thread(Partition, "Partition", B_NORMAL_PRIORITY,
									new WorkUnit(j+1, end, thisclass, true));
				
				// if spawn_thread failed, we can still continue...
				if(tid < 0)
				{
					Partition(new WorkUnit(j+1, end, thisclass, false));
				}				
				else
				{
					resume_thread(tid);
				}

			}
			else
			{
				Partition(new WorkUnit(j+1, end, thisclass, false));
			}
		
	
			// if we were spawned, then "return" a thread back into the pool...
			if (unit->spawned)
			{
				release_sem(thisclass->workthread);
			}
						
			delete unit;
			return 0;
		}	

	}
}



bool
Verify(int32 *vector, int32 size)
{
	for (int i=0; i < size-1; i++)
	{
		if (*(vector+i) > *(vector+i+1))
		{
			return 0;
		}
	}

	return 1;
}


int
main(int argc, char** argv)
{
	if (argc != 4)
	{
		printf("%s: <array size> <num threads> <threshold>\n", argv[0]);
		return 0;
	}
	else
	{
		SIZE = atoi(argv[1]);
		THREADS = atoi(argv[2]);
		THRESH = atoi(argv[3]);
	}

	// initialize the array with random ints...
	srand(find_thread(NULL));
	int32 *foo = new int32[SIZE];
	for (int32 i=0; i < SIZE; i++)
	{
		foo[i] = rand() % SIZE;
	}

	// do the sort, and time it!
	BStopWatch *watch = new BStopWatch("sortwatch");	

	QSort::Sort(foo, SIZE);

	delete watch;

	// just to prove our sort's working properly...
	bool v = Verify(foo, SIZE);
	printf("proper sort: %s\n", v ?  "true" : "false");

	return 0;
}
