/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __SIMPLE_PICKER_APP__
#define __SIMPLE_PICKER_APP__

#include <Application.h>

class SimpleColorPickerDialog;

class SimplePickerApp : public BApplication {
public:
	SimplePickerApp();

protected:
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage *);

private:
	SimpleColorPickerDialog *colorPicker;
};



#endif