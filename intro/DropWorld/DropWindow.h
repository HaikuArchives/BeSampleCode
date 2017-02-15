/*
	
	DropWindow.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DROP_WINDOW_H
#define DROP_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

// DropWindow class
class DropWindow : public BWindow  
{
public:
				DropWindow(BRect frame);
virtual	bool	QuitRequested();
};

#endif
