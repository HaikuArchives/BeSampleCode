/* VoterStats.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "VoterStats.h"
#include <View.h>
#include <StringView.h>
#include <Button.h>
#include <string.h>
#include <stdio.h>
#include <Application.h>

VoterStats::VoterStats(const char *request)
		: BWindow(BRect(50,50,199,149), "Voter Stats", B_TITLED_WINDOW, 0),
			fState(STUFF_THE_BALLOT)
{
	BRect bounds = Bounds();
	bounds.OffsetTo(B_ORIGIN);
	BView * main_view = new BView(bounds, "Main", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);
	main_view->SetViewColor(245,245,245);
	
	bounds.OffsetTo(B_ORIGIN);
	bounds.InsetBy(5,5);
	bounds.SetRightBottom(bounds.RightBottom() + BPoint(0, -65));
	BStringView *req_view = new BStringView(bounds, "Request", request, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS);
	main_view->AddChild(req_view);
	
	bounds.OffsetBy(0, 15);
	BStringView *vote_view = new BStringView(bounds, "Votes", "Current Vote Count: 0",B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS);
	main_view->AddChild(vote_view);

	bounds.OffsetBy(0, 15);
	BStringView *voter_view = new BStringView(bounds, "Felons", "Current Felon Count: 0", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS);
	main_view->AddChild(voter_view);

	bounds.OffsetBy(0, 35);
	BRect rebounds = bounds;
	rebounds.Set(bounds.left, bounds.top, bounds.left + 120, bounds.top + 15);
	
	BMessage *msg = new BMessage(CHANGE_ACTIVITY);
	BButton *button = new BButton(rebounds, "Activity", "Stuff The Ballot!", msg);
	main_view->AddChild(button);
	button->SetTarget(this);
	
	AddChild(main_view);
}


VoterStats::~VoterStats()
{
}

void 
VoterStats::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case UPDATE_VOTECOUNT:
		{
			BStringView * view = (BStringView *)FindView("Votes");
			if (view) {
				int64 votes;
				msg->FindInt64("votes", &votes);
				char text[64];
				sprintf(text, "Current Vote Count: %Ld", votes);
				
				Lock();
				view->SetText(text);
				Unlock();
			}
			
			view = NULL;
			view = (BStringView *)FindView("Felons");
			if (view) {
				int32 felons;
				msg->FindInt32("felons", &felons);
				char text[64];
				sprintf(text, "Current Felon Count: %ld", felons);
				
				Lock();
				view->SetText(text);
				Unlock();
			}
			
			break;
		}
		
		case CHANGE_ACTIVITY:
		{
			//get the button
			BButton *button = NULL;
			msg->FindPointer("source", (void **)&button);
			if (button) {
				if(fState == CEASE_AND_DESIST) {
					fState = STUFF_THE_BALLOT;
					button->SetLabel("Stuff The Ballot!");
					BMessage cease(CEASE_AND_DESIST);
					be_app->PostMessage(&cease);
				
				} else if (fState == STUFF_THE_BALLOT) {
					fState = CEASE_AND_DESIST;
					button->SetLabel("Cease And Desist!");
					BMessage stuff(STUFF_THE_BALLOT);
					be_app->PostMessage(&stuff);
				}
			}
			break;
		}
		
		case CEASE_AND_DESIST:
		{
			printf("CeaseAndDesist rcvd\n");
			if (fState == CEASE_AND_DESIST) {
				BButton *button = NULL;
				button= (BButton *) FindView("Activity");
				if (button) {
					button->SetLabel("Stuff The Ballot!");
					fState = STUFF_THE_BALLOT;
				}
			}
			break;
		
		case STUFF_THE_BALLOT:
			printf("StuffTheBallot rcvd\n");
			if (fState == STUFF_THE_BALLOT) {
				BButton *button = NULL;
				button= (BButton *) FindView("Activity");
				if (button) {
					button->SetLabel("Cease And Desist!");
					fState = CEASE_AND_DESIST;
				}
			}
			break;
		}
			
		default:
			BWindow::MessageReceived(msg);
	}
}

bool 
VoterStats::QuitRequested()
{
	//tell the app to quit!
	BMessage msg(B_QUIT_REQUESTED);
	be_app->PostMessage(&msg, be_app);
	return true;
}

