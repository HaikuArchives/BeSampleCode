/* TweakWin.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Window.h>

class TweakWin : public BWindow
{
 public:
 	TweakWin(BWindow* hand, int32 imass, int32 idrag, 
			 int32 iwidth, int32 isleepage);
 	void MessageReceived(BMessage* msg);
	void Quit();
	 
 private:
	BSlider* mass;
	BSlider* drag;
	BSlider* width;
	BSlider* sleepage;
	BCheckBox* fill;
	BCheckBox* angle;
	
	BWindow* handler;	
};

