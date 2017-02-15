/* ColorWin.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Window.h>

class ColorWin : public BWindow
{
 public:
 	ColorWin(BWindow* hand, rgb_color initialColor);
 	void MessageReceived(BMessage* msg); 
 	void Quit();
 	
 private:
 	BWindow* handler;
	BColorControl* cc;
	BView* swatch;
	
};
