/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Application.h>
#include <Debug.h>

#include "CrayonPicker.h"
#include "../simpleColorPicker/Protocol.h"

const char *kSignature = "application/x-vnd.Be.CrayonColorPicker";

class CrayonPickerApp : public BApplication {
public:
	CrayonPickerApp();

protected:
	virtual void MessageReceived(BMessage *);
	virtual void ReadyToRun();

private:
	CrayonColorPickerDialog *colorPicker;
};


CrayonPickerApp::CrayonPickerApp()
	:	BApplication(kSignature),
		colorPicker(0)
{
}

void 
CrayonPickerApp::MessageReceived(BMessage *message)
{
	if (message->what == kInitiateConnection) {
		// This is the initial open message that ModuleProxy::Invoke is sending
		// us. Pass it on to the new color picker dialog which will find all
		// the details in it
		colorPicker = new CrayonColorPickerDialog(message);
		colorPicker->Show();
		return;
	}
	
	BApplication::MessageReceived(message);
}


void 
CrayonPickerApp::ReadyToRun()
{
	// looks like we just got launched by plain double clicking,
	// open the color picker dialog anyway, some of them work pretty
	// well on their own

	if (!colorPicker) {
		colorPicker = new CrayonColorPickerDialog(0);
		colorPicker->Show();
	}		
}

int
main()
{
	CrayonPickerApp app;
	app.Run();

	return 0;
}
