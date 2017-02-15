/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __SIMPLE_PICKER__
#define __SIMPLE_PICKER__

#include <Window.h>

class BColorControl;

class SimpleColorPickerDialog : public BWindow {
public:
	SimpleColorPickerDialog(BMessage *);
	virtual ~SimpleColorPickerDialog();

protected:
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();

private:
	BColorControl *colorPalette;
	rgb_color initialColor;
	BMessenger target;
};

#endif
