/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ColorControl.h>
#include <String.h>

#include "../simpleColorPicker/Protocol.h"
#include "SimplePicker.h"

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

SimpleColorPickerDialog::SimpleColorPickerDialog(BMessage *message)
	:	BWindow(InitialRect(kPickerFrame, message), "Pick a color", B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE)
{
	BBox *box = new BBox(Bounds(), "", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);

	BRect rect(box->Bounds());
	rect.InsetBy(10, 10);
	
	// add OK and Cancel buttons
	rect.SetLeftTop(rect.RightBottom() - kSmallButtonSize);
	BButton *button = new BButton(rect, "ok", "OK", new BMessage(M_OK));
	box->AddChild(button);
	button->MakeDefault(true);
	rect.OffsetBy(-rect.Width() - 10, 0);
	button = new BButton(rect, "cancel", "Cancel", new BMessage(B_CANCEL));
	box->AddChild(button);

	// add a color control
	colorPalette = new BColorControl(BPoint(10, 10), B_CELLS_32x8, 4, "",
		new BMessage(kColorControlValueChanged));
	colorPalette->SetValue(initialColor);
	box->AddChild(colorPalette);

	AddChild(box);

	// find the initial color value
	const rgb_color *color;
	ssize_t size;
	if (message && message->FindData(kInitialValue, B_RGB_COLOR_TYPE,
		reinterpret_cast<const void **>(&color), &size) == B_OK)
		colorPalette->SetValue(*color);
	
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

SimpleColorPickerDialog::~SimpleColorPickerDialog()
{
	target.SendMessage(kServerQuitting);
		// tell the client we can no longer serve it
		// because we are quitting
}

bool 
SimpleColorPickerDialog::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
		// we are a one window per app instance setup,
		// when window closes, kill the whole app
	return true;
}


void 
SimpleColorPickerDialog::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kColorControlValueChanged:
			{
				// color picker value changed, propagate to our client
				BMessage valueUpdate(B_VALUE_CHANGED);
				rgb_color color = colorPalette->ValueAsColor();
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
					colorPalette->SetValue(*color);
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

