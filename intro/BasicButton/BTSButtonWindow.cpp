/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BTSButtonWindow.h"
#include <Application.h>
#include <stdio.h>

const BRect kWindowFrame (100,100,300,300);
const BRect kButtonFrame (80, 90, 120, 110);
const char* kWindowName = "ButtonWindow";
const char* kButtonName = "Press";

BTSButtonWindow::BTSButtonWindow() :
	BWindow(kWindowFrame, kWindowName, B_TITLED_WINDOW,
			B_WILL_DRAW)
{
	fButton = new BButton(kButtonFrame, kButtonName, kButtonName,
					new BMessage(BUTTON_MSG));
	AddChild(fButton);
}


bool
BTSButtonWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

void
BTSButtonWindow::MessageReceived(BMessage* message)
{
	static int numPresses = 0;
	switch(message->what)
	{
		case BUTTON_MSG:
			char title[100];
			sprintf(title, "Presses: %d", ++numPresses);
			SetTitle(title);
		break;
		default:
			BWindow::MessageReceived(message);
	}
}