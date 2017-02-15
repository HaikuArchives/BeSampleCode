/*
	
	UptimeWindow.cpp
	
*/

/*
	Copyright 1995-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef HELLO_WINDOW_H
#include "UptimeWindow.h"
#endif

void set_palette_entry(long i,rgb_color c);

UptimeWindow::UptimeWindow(BRect frame) : BWindow(frame, "Weihnachten", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_AVOID_FRONT | B_NOT_RESIZABLE)
{
	;	
}

bool UptimeWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

