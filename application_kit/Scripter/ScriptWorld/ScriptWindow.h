/*
	ScriptWindow.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef SCRIPT_WINDOW_H
#define SCRIPT_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class ScriptWindow : public BWindow 
{
public:
				ScriptWindow(BRect frame); 
virtual	bool	QuitRequested();
};

#endif //SCRIPT_WINDOW_H
