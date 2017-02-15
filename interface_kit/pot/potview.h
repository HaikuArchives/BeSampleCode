/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _potview_h
#define _potview_h

#include <View.h>
#include "colorutils.h"

class BPot;

class PotView : public BView
{
public:
	PotView(BRect frame);
	
	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint where);
	virtual void MessageReceived(BMessage* msg);
	
private:
	void AddPotAndBox(BRect r, color_channel ch,
		rgb_color color, BPot** hPot, const char* name);
	void InformMouseDown(BPot* pot, BPoint where);
	
	BPot *m_red, *m_green, *m_blue, *m_alpha;
};

#endif /* _potview_h */