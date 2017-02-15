/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ColorControl.h>
#include <String.h>

#include "../simpleColorPicker/Protocol.h"
#include "CrayonPicker.h"

const BRect kPickerFrame(100, 100, 390, 205);
const uint32 M_OK = 'okok';
const uint32 kColorControlValueChanged = 'clch';
const BPoint kSmallButtonSize(60, 20);

static BRect
InitialRect(BRect defaultRect, BMessage *message)
{
	BRect result(defaultRect);
	BPoint offset;
	if (message && message->FindPoint(kInvokePoint, &offset) == B_OK) {
		// found the inital clickPoint
		offset += BPoint(50, -40);
		// make sure the color picker doesn't cover the spot we
		// just clicked at
		result.OffsetTo(offset);
	}

	return result;
}

CrayonColorPickerDialog::CrayonColorPickerDialog(BMessage *message)
	:	BWindow(InitialRect(kPickerFrame, message), "Pick a color", 
			B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, 
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE)
{
	BRect rect(Bounds());
	rect.InsetBy(-1,-1);
	BBox *box = new BBox(rect, "", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		B_NO_BORDER);

	// add OK and Cancel buttons
	rect.left = 0;rect.top=0;
	rect.right = 75; rect.bottom = 1;
	BButton *button = new BButton(rect, "ok", "OK", new BMessage(M_OK),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	box->AddChild(button);
	button->MakeDefault(true);
	button->MoveTo(Bounds().Width() - 75 - 10,
		Bounds().Height()-button->Frame().Height() - 10);

	rect = button->Frame();
	rect.right = rect.left - 10; rect.left = rect.right - 75;
	button = new BButton(rect, "cancel", "Cancel", new BMessage(B_CANCEL),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	box->AddChild(button);

	// add a color picker
	colorPicker = new TCrayonColorPicker(BPoint(2, 2));
	box->AddChild(colorPicker);
	
	// install a bunch of crayons
	colorPicker->AddCrayon(color(200,10,10));
	colorPicker->AddCrayon(color(200,30,60));
	colorPicker->AddCrayon(color(200,50,110));
	colorPicker->AddCrayon(color(200,70,160));

	colorPicker->AddCrayon(color(10,200,10));
	colorPicker->AddCrayon(color(30,200,60));
	colorPicker->AddCrayon(color(50,200,110));
	colorPicker->AddCrayon(color(70,200,160));

	colorPicker->AddCrayon(color(10,10,200));
	colorPicker->AddCrayon(color(60,30,200));
	colorPicker->AddCrayon(color(110,50,200));
	colorPicker->AddCrayon(color(160,70,200));

	colorPicker->AddCrayon(color(100,100,100));
	colorPicker->AddCrayon(color(140,140,140));
	colorPicker->AddCrayon(color(180,180,180));
	colorPicker->AddCrayon(color(220,220,220));

	colorPicker->ResizeToPreferred();

	AddChild(box);

	float width, height;
	colorPicker->GetPreferredSize(&width, &height);
	ResizeTo(width+4, 2 + height + button->Frame().Height() + 20);

	// find the initial color value
	const rgb_color *color;
	ssize_t size;
	if (message && message->FindData(kInitialValue, B_RGB_COLOR_TYPE,
		reinterpret_cast<const void **>(&color), &size) == B_OK)
		colorPicker->SetCurrentColor(*color);
	
	// use a specific window title
	const char *title;
	if (message && message->FindString(kTargetName, &title) == B_OK) {
		BString newTitle;
		newTitle << "Color for " << title;
		SetTitle(newTitle.String());
	}
	
	if (message) {
		// keep the address of our client
		BMessenger tmp;
		if (message->FindMessenger(kClientAddress, &tmp) == B_OK)
			target = tmp;
		
		// let the client know we are ready to serve it
		BMessage reply(kOpenConnection);
		reply.AddMessenger(kServerAddress, BMessenger(this));
		reply.AddInt32(kProvidedValues, B_RGB_COLOR_TYPE);
			// let the client know we can supply color values

		target.SendMessage(&reply);
	}
}

CrayonColorPickerDialog::~CrayonColorPickerDialog()
{
	target.SendMessage(kServerQuitting);
		// tell the client we can no longer serve it
		// because we are quitting
}

bool 
CrayonColorPickerDialog::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
		// we are a one window per app instance setup,
		// when window closes, kill the whole app
	return true;
}


void 
CrayonColorPickerDialog::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kColorControlValueChanged:
			{
				// color picker value changed, propagate to our client
				BMessage valueUpdate(B_VALUE_CHANGED);
				rgb_color color = colorPicker->CurrentColor();
				valueUpdate.AddData(kNewValue, B_RGB_COLOR_TYPE,
					&color, sizeof(color)); 
				target.SendMessage(&valueUpdate);
			}
			break;

		case kActivateWindow:
			Activate();
			break;

		case B_VALUE_CHANGED:
			{
				// client color changed without our intervention,
				// sync up
				rgb_color *color;
				ssize_t size;
				if (message->FindData(kNewValue, B_RGB_COLOR_TYPE,
					(const void**) &color, &size) == B_OK)
					colorPicker->SetCurrentColor(*color);
			}
			break;

		case M_OK:
			target.SendMessage(kApplyValue);
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case B_CANCEL:
			target.SendMessage(B_CANCEL);
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kCloseConnection:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

