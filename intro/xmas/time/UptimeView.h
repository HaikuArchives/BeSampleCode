/*
	
	UptimeView.h
	
*/

/*
	Copyright 1995-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_VIEW_H
#define HELLO_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include <String.h>

class UptimeView : public BView {
private:
public:
				UptimeView(BRect frame, char *name);
				int PrevSec;
				BString DateString;
				int is_leap;
				int xmas;
				int our_year;
				 
virtual void	update(void);
virtual	void Draw(BRect updateRect);
virtual	void Pulse(void);



};

#endif

