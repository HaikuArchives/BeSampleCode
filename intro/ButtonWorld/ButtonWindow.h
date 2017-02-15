/*
	
	ButtonWindow.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BUTTONWINDOW_H_
#define _BUTTONWINDOW_H_


#include <Window.h>

// ButtonWindow class
class ButtonWindow : public BWindow  
{
public:
				ButtonWindow(BRect frame);
virtual	bool	QuitRequested();
};

#endif
