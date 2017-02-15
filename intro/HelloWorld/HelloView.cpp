/*
	
	HelloView.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_VIEW_H
#include "HelloView.h"
#endif

HelloView::HelloView(BRect rect, const char *name, const char *text)
	   	   : BStringView(rect, name, text, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetFont(be_bold_font);
	SetFontSize(24);
}