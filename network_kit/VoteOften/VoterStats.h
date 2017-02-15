/* VoterStats.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef VOTER_STATS_H
#define VOTER_STATS_H

#include <Window.h>

class VoterStats : public BWindow {
	public:
						VoterStats(const char *request);
		virtual			~VoterStats();
		
	
		void			MessageReceived(BMessage *msg);
		bool			QuitRequested();
		
	private:
		int32			fState;
};

#define CHANGE_ACTIVITY 	'actv'
#define UPDATE_VOTECOUNT 	'vcnt'
#define STUFF_THE_BALLOT	'stuf'
#define CEASE_AND_DESIST	'cese'

#endif

