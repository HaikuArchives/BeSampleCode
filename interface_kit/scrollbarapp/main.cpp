/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Application.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include "main.h"
#include "win.h"

// The Open Panel is a window which we do not want to count
#define WINDOWS_TO_IGNORE 1

TApp :: TApp(void) : BApplication(APP_SIG)
{
	SetPulseRate(100000);
	BPath path;
	load = new BFilePanel(B_OPEN_PANEL);
	find_directory(B_USER_DIRECTORY, &path);
	load->SetPanelDirectory(path.Path());
}

TApp :: ~TApp ()
{
	delete load;
}

void TApp :: ReadyToRun(void)
{
	if(CountWindows() == WINDOWS_TO_IGNORE){
		BRect rect(50, 50, 250, 250);
		BWindow *win;
		win = new TWin(rect, NULL);
		win->Show();
	}
}

void TApp :: Pulse(void)
{
	if(!IsLaunching() && CountWindows() <= WINDOWS_TO_IGNORE){
		PostMessage(B_QUIT_REQUESTED);
	}
}

void TApp :: ArgvReceived(int32 argc, char **argv)
{
	BRect rect(50, 50, 250, 250);
	BWindow *win;
	if(argc <= 1){
		win = new TWin(rect, NULL);
		win->Show();
	} else for(int i = 1; i < argc ; i++){
		win = new TWin(rect, argv[i]);
		win->Show();
	}
}

void TApp :: MessageReceived(BMessage *message)
{
	switch(message->what){
	case T_OPEN:
		load->Show();
		break;
	default:
		BApplication::MessageReceived(message);
	}
}

void TApp :: RefsReceived(BMessage *message)
{
	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;

	message->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 

	BRect rect(50, 50, 250, 250);
	BWindow *win;
	for ( long i = --count; i >= 0; i-- ) { 
		if ( message->FindRef("refs", i, &ref) == B_OK ) { 
			BEntry entry(&ref, true);
			if ( entry.IsFile() && entry.GetPath(&path)==B_OK){
				win = new TWin(rect, path.Path());
				win->Show();
			}
		} 
	} 
}

int main(int argc, char **argv)
{
	new TApp();
	be_app->Run();
	delete be_app;
}
