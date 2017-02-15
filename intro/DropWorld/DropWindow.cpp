/*

	DropWindow.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef DROP_WINDOW_H
#include "DropWindow.h"
#endif
#ifndef DROP_VIEW_H
#include "DropView.h"
#endif

DropWindow::DropWindow(BRect frame)
				: BWindow(frame, "Drop", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	DropView *dropView = new DropView( Bounds(), "myView", "Drag a file to me" );
	
	// add view to window
	AddChild(dropView);
}

bool DropWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}