/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>
#include <Roster.h>
#include "main.h"

extern const char *INFO_WINDOW_TITLE = "Shelf Inspector";

/*------------------------------------------------------------*/

int main(int argc, char* argv[])
{
	BApplication	app("application/x-vnd.Be-shelf-inspector");
	
	be_roster->Launch("application/x-vnd.Be-MYTE");	// Launch the Container Demo
	
	BWindow *rw = new TInfoWindow(BRect(100, 50, 380, 400), INFO_WINDOW_TITLE);
	rw->Show();
	app.Run();
	return 0;
}