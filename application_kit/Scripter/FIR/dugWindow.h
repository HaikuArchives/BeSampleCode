/*
	dugWindow.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DUG_WINDOW_H
#define DUG_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class dugWindow : public BWindow 
{
public:
				dugWindow(BRect frame); 
virtual	bool	QuitRequested();
};

#endif //DUG_WINDOW_H
