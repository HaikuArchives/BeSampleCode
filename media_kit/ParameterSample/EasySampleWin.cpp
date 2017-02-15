// EasySampleWin.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "EasySampleWin.h"
#include <be/media/MediaRoster.h>
#include <be/media/MediaTheme.h>
#include <be/app/Application.h>
#include <be/interface/View.h>
#include <stdio.h>

// Parameter window implementation
EasySampleWin::EasySampleWin(BRect frame)
	: BWindow(frame, "EasySampleWin", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	media_node mixerNode;

	BMediaRoster* roster = BMediaRoster::Roster();
	roster->GetAudioMixer(&mixerNode);
	roster->GetParameterWebFor(mixerNode, &mParamWeb);

	// initialization is okay; now set up the controlling view!
	BMediaTheme* theme = BMediaTheme::PreferredTheme();
	BView* paramView = theme->ViewFor(mParamWeb);
	AddChild(paramView);

	// resize to match the view
	ResizeTo(paramView->Bounds().Width(), paramView->Bounds().Height());
}

EasySampleWin::~EasySampleWin()
{
	// we don't delete the BParameterWeb copy here because the BView owns it
	// when you use BMediaTheme::ViewFor()
}

bool 
EasySampleWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
EasySampleWin::MessageReceived(BMessage *msg)
{
	BWindow::MessageReceived(msg);
}

