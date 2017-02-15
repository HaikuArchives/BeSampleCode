/*
	
	ButtonView.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BUTTONVIEW_H_
#define _BUTTONVIEW_H_


#include <StringView.h>

#define BUTTON_MSG 'btnm'

class ButtonView : public BStringView 
{
public:
				ButtonView(BRect frame, const char *name, const char *text); 
virtual void 	MessageReceived(BMessage *message);
virtual void 	MouseDown(BPoint);

};

#endif
