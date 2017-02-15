/*
	
	dugApp.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DUG_WINDOW_H
#include "dugWindow.h"
#endif
#ifndef DUG_APP_H
#include "dugApp.h"
#endif

int main(int, char**)
{	
	dugApp	app;

	app.Run();

	return(0);
}

dugApp::dugApp()
		: BApplication("application/x-vnd.dugz-stuff")
{
	dugWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 360, 180);
	aWindow = new dugWindow(aRect);
			
	// make window visible
	aWindow->Show();
}
