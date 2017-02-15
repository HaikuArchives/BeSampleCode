/*
	
	UptimeWindow.h
	
*/

/*
	Copyright 1995-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_WINDOW_H
#define HELLO_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class UptimeWindow : public BWindow {

public:
				UptimeWindow(BRect frame); 
virtual	bool	QuitRequested();
};

#endif
