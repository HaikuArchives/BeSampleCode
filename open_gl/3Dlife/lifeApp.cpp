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


int main()
{
	lifeApp app;
	app.Run();
	return 0;
}
