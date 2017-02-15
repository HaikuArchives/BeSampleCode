/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __CRAYON_PICKER__
#define __CRAYON_PICKER__

#include <Window.h>

#include "CrayonColorPicker.h"

class CrayonColorPickerDialog : public BWindow {
public:
	CrayonColorPickerDialog(BMessage *);
	virtual ~CrayonColorPickerDialog();

protected:
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();

private:
	TCrayonColorPicker* colorPicker;
	rgb_color initialColor;
	BMessenger target;
};

#endif
