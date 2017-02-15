/*
	
	HelloView.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_VIEW_H
#define HELLO_VIEW_H

#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif

class HelloView : public BStringView 
{
public:
	HelloView(BRect frame, const char *name, const char *text); 
};

#endif //HELLO_VIEW_H
