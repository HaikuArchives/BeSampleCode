/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include "lifeWin.h"

class lifeApp : public BApplication
{
 public: 
 	lifeApp();
 	void AboutRequested();
 	void MessageReceived(BMessage *msg); //voodoo
 private:
 	lifeWin *mw;
};

