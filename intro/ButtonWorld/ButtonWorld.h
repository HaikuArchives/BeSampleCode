/*
	
	ButtonWorld.h

*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _BUTTONWORLD_H_
#define _BUTTONWORLD_H_


#include <Application.h>

//ButtonApplication class
class ButtonApplication : public BApplication 
{
public:
				ButtonApplication();
virtual void 	RefsReceived( BMessage *message );

private:
	BWindow		*buttonWindow;
};

#endif

