// cputime - a cpu-usage measuring tool
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Application.h>
#include <Entry.h>
#include <Roster.h>
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include "uiclasses.h"
#include "datadefs.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string.h>


bigtime_t starttime;
bigtime_t stoptime;


Ruler *ruler;
CPUView *cpuview;
Scroller *scroller;
DisplayView *displayview;

const ulong MAXTHREADS=40;		// tests only this much threads in team
const ulong MAXSAMPLES=6000;	// take only this much samples

thread_data thread[MAXTHREADS];
ulong numthreads=0;

team_info team;

ulong timeindex=2;

class mywin: public BWindow
{
	public:	mywin()
			: BWindow(BRect(100,100,500,300),"CPU history",B_TITLED_WINDOW,0)
			{
				SetSizeLimits(300,2000,200,2000);
			}

			virtual bool QuitRequested()
			{
				be_app->PostMessage(B_QUIT_REQUESTED);
				return true;
			};
};



void StoreThread(thread_info *info);
void StoreThread(thread_info *info)
{
	if(timeindex<MAXSAMPLES)
	{
		for(uint i=0;i<numthreads;i++)
		{
			if(thread[i].thread==info->thread)
			{
				// update array
				thread[i].times[timeindex].usertime=info->user_time-thread[i].totaluser;
				thread[i].totaluser=info->user_time;

				thread[i].times[timeindex].kerntime=info->kernel_time-thread[i].totalkern;
				thread[i].totalkern=info->kernel_time;
				return;
			}
		}
		// we arrive here if this is a new thread
		if(numthreads<MAXTHREADS)
		{
			thread[numthreads].thread=info->thread;
			thread[numthreads].displayit=true;
			strcpy(thread[numthreads].name,info->name);
			thread[numthreads].times=new thread_time[MAXSAMPLES];
			for(uint i=0;i<MAXSAMPLES;i++)
			{
				thread[numthreads].times[i].usertime=0;
				thread[numthreads].times[i].kerntime=0;
			}

			thread[numthreads].totaluser=thread[numthreads].times[timeindex].usertime=info->user_time;
			thread[numthreads].totalkern=thread[numthreads].times[timeindex].kerntime=info->kernel_time;

			numthreads++;
		}
	}
}




int main(int argc, char *argv[])
{
	BApplication myapp("application/x-marcone-cputime");
	team_id team;
	entry_ref exe;

	if(argc<=1)
	{
		fprintf(stderr,"usage: %s command [args]\n",argv[0]);
		exit(1);
	}
	get_ref_for_path(argv[1],&exe);
	if(B_OK!=be_roster->Launch(&exe,argc-2,(char**)&argv[2],&team))
	{
		fprintf(stderr,"error launching '%s'\n",argv[1]);
		exit(2);
	}

	starttime=system_time();
	bigtime_t nexttime=starttime;
	long threads_in_team;

	do {
		long cookie=0;
		threads_in_team=0;
		thread_info currentthread;

		printf("\033[H\033[J"); // clear screen
		while(B_OK==get_next_thread_info(team,&cookie,&currentthread))
		{
			StoreThread(&currentthread);
			if(timeindex%10==0) // print stats only every 10th sample
				printf("thread %d (%30.30s) cpu: %Ld/%Ld\n",
					currentthread.thread,
					currentthread.name,
					currentthread.user_time,
					currentthread.kernel_time);
			threads_in_team++;
		}
		timeindex++;
		if(timeindex>MAXSAMPLES)
		{
			printf("measurement stopped because sampling buffer is full\n");
			break;
		}
		nexttime+=100000;
		snooze_until(nexttime,B_SYSTEM_TIMEBASE);
	} while(threads_in_team!=0);
	timeindex--;

	stoptime=system_time()-100000;
	// total
	printf("team had a total of %d threads\n",numthreads);
	bigtime_t totaluser=0;
	bigtime_t totalkern=0;
	for(uint i=0;i<numthreads;i++)
	{
		for(uint j=2;j<timeindex;j++)
		{
			totaluser+=thread[i].times[j].usertime;
			totalkern+=thread[i].times[j].kerntime;
		}
	}
	printf("total user/kernel time: %Ld/%Ld (microseconds)\n",totaluser,totalkern);
	printf("total elapsed time:%Ld microseconds (%d samples taken)\n",stoptime-starttime,timeindex);
	printf("%f%% CPU usage\n",double(totaluser+totalkern)*100/(stoptime-starttime));

	mywin *win=new mywin();
	win->AddChild(cpuview=new CPUView());
	win->AddChild(ruler=new Ruler());
	win->AddChild(scroller=new Scroller());
	win->AddChild(new ControlView());
	win->AddChild(displayview=new DisplayView());
	win->Show();

	myapp.Run();
}

