/*
	ScriptWorld.cpp
	
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef SCRIPT_WINDOW_H
#include "ScriptWindow.h"
#endif
#ifndef SCRIPT_WORLD_H
#include "ScriptWorld.h"
#endif

int main(int, char**)
{	
	ScriptApplication	myApplication;

	myApplication.Run();

	return(0);
}

ScriptApplication::ScriptApplication()
		  		  : BApplication("application/x-vnd.Be-DTS.ScriptWorld")
{
	ScriptWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 260, 280);
	aWindow = new ScriptWindow(aRect);
			
	// make window visible
	aWindow->Show();
}
