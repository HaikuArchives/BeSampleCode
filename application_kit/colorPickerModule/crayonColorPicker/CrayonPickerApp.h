/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __CRAYON_PICKER_APP__
#define __CRAYON_PICKER_APP__

#include <Application.h>

class CrayonColorPickerDialog;

class CrayonPickerApp : public BApplication {
public:
	CrayonPickerApp();

protected:
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage *);

private:
	CrayonColorPickerDialog *colorPicker;
};



#endif