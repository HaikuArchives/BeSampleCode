/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "lifeApp.h"
#include <Alert.h>

lifeApp::lifeApp()
	: BApplication("application/x-vnd.Be-3dlife")
{
	mw = new lifeWin;
	mw->Show();
}

void
lifeApp::AboutRequested()
{
	(new BAlert("","3D Life -- A Small OpenGL Example", "sweet"))->Go();
}

/* added for voodoo hardware demo */
/* hadles message to switch to and from fullscreen mode */

void lifeApp::MessageReceived(BMessage *msg) {
	switch(msg->what)
	{
	 case FULL_SCREEN:
	 	if(mw->IsFullScreen() == true)
	 		mw->SetFullScreen(false);
	 	else mw->SetFullScreen(true);
		break;

	 default:
	 	BApplication::MessageReceived(msg);
	}
}



int main()
{
	lifeApp app;
	app.Run();
	return 0;
}
