/*
	
	DropWorld.h

*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DROP_WORLD_H
#define DROP_WORLD_H

#include <Application.h>

//DropApplication class
class DropApplication : public BApplication 
{
public:
				DropApplication();
virtual void 	RefsReceived( BMessage *message );

private:
	BWindow		*dropWindow;
};

#endif //DROP_WORLD_H
