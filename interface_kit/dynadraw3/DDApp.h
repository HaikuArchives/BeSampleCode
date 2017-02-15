/* DDApp.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Application.h>
#include "DDWindow.h"
#include "TweakWin.h"
#include "ColorWin.h"

class DDApp : public BApplication
{
 public:
 	DDApp();
 	void MessageReceived(BMessage* msg);
 	void AboutRequested();
 	
 private:
 	DDWindow* ddWin;
 	ColorWin* colorWin;
 	TweakWin* tweakWin;
};