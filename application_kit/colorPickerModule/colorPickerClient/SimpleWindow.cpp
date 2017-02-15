/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Button.h>

#include "SimpleWindow.h"
#include "ColorLabel.h"

const BPoint kButtonSize(70, 20);
const uint32 kStop = 'stop';
const uint32 kTextHeight = 18;


SimpleView::SimpleView(BRect frame)
	:	BBox(frame, "", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER)
{
	BRect rect(Bounds());
	rect.InsetBy(20, 20);
	rect.bottom = rect.top + 14;
	rect.right = rect.left + 60;
	AddChild(new ColorLabel(rect, "day", "day:", Color(180, 0, 0), 0));
	rect.OffsetBy(0, 22);
	AddChild(new ColorLabel(rect, "night", "night:", Color(0, 0, 160), 0));
}

void 
SimpleView::AttachedToWindow()
{
}

void 
SimpleView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			_inherited::MessageReceived(message);
	}
}

BRect kWindowRect(0, 0, 240, 100);

SimpleWindow::SimpleWindow(BPoint offset)
	:	BWindow(kWindowRect.OffsetToSelf(offset), "Simple Color Picking App",
			B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	AddChild(new SimpleView(Bounds()));
}

bool 
SimpleWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
SimpleWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			_inherited::MessageReceived(message);
	}
}

