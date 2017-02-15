/*
	
	HelloWindow.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef HELLO_WINDOW_H
#include "HelloWindow.h"
#endif
#ifndef HELLO_VIEW_H
#include "HelloView.h"
#endif

HelloWindow::HelloWindow(BRect frame)
				: BWindow(frame, "Hello", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	HelloView	*aView;
	// set up a rectangle and instantiate a new view
	BRect aRect( Bounds() );
	aView = new HelloView(aRect, "HelloView", "Hello World!");
	// add view to window
	AddChild(aView);
}

bool HelloWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
