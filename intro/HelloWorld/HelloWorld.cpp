/*
	
	HelloWorld.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_WINDOW_H
#include "HelloWindow.h"
#endif
#ifndef HELLO_WORLD_H
#include "HelloWorld.h"
#endif

int main(int, char**)
{	
	HelloApplication	myApplication;

	myApplication.Run();

	return(0);
}

HelloApplication::HelloApplication()
		  		  : BApplication("application/x-vnd.Be-HelloWorldSample")
{
	HelloWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 260, 120);
	aWindow = new HelloWindow(aRect);
			
	// make window visible
	aWindow->Show();
}
