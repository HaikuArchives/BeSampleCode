/*
	
	DropView.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DROP_VIEW_H
#define DROP_VIEW_H

#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif

class DropView : public BStringView 
{
public:
				DropView(BRect frame, const char *name, const char *text); 
virtual void 	MessageReceived(BMessage *message);
virtual void 	MouseDown(BPoint);

};

#endif //DROP_VIEW_H
