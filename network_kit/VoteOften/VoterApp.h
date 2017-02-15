/* VoterApp.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef VOTER_APP_H
#define VOTER_APP_H

#include <Application.h>

class voter;
class BList;
class VoterStats;

class VoterApp : public BApplication {
	public:
							VoterApp();
		virtual				~VoterApp();
		
		void				ReadyToRun();
		virtual void		ArgvReceived(int32 argc, char **argv);
		void				MessageReceived(BMessage *msg);
		void				Pulse();
		
		
	private:
		status_t			BribeVoters(int32 num);
		status_t			StuffTheBallot();
		status_t			CeaseAndDesist();
		status_t			ArrestVoters(int32 num);
		
		BList 				fPayroll;
		bool				fVoting;
		bool				fQuitting;	
		VoterStats *		fStats;
};

bool	cease(void *arg);
bool	stuff(void *arg);

#define	HIRE_VOTERS	'hire'
#define ARREST_VOTERS		'arst'	

#endif

