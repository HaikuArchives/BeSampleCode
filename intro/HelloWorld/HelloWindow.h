/*
	
	HelloWindow.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_WINDOW_H
#define HELLO_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class HelloWindow : public BWindow 
{
public:
				HelloWindow(BRect frame); 
virtual	bool	QuitRequested();
};

#endif //HELLO_WINDOW_H
