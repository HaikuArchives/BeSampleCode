/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <OS.h>
#include <String.h>

#include <Resources.h>


#ifndef HELLO_VIEW_H
#include "UptimeView.h"
#endif

double uptime(void);

UptimeView::UptimeView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_PULSE_NEEDED)
{
	struct tm *time_stuff;
	long the_time;

	is_leap=0;
	PrevSec=-1;
	DateString="";
	xmas=357;

    time(&the_time);
    time_stuff = localtime(&the_time);

	our_year=(time_stuff->tm_year)+1900;

// is it leap year?
	if((!(our_year%4) && (our_year%100)) || !(our_year%400)) is_leap=1;
	if(is_leap==1) xmas=358;
	

	SetFont(be_bold_font);
	SetFontSize(16);

	SetHighColor(255,0,0);
	SetLowColor(0,0,0);
	SetViewColor(0,0,0);


}

void UptimeView::Draw(BRect /* updateRect */)
{
	
	DrawString(DateString.String(), BPoint(4, 20));
}

void UptimeView::Pulse(void)
{
	update();
}



void UptimeView::update(void)
{
	struct tm *time_stuff;
	long the_time;
	
	uint32 nDays, nHours, nMin, nSec;

    time(&the_time);
    time_stuff = localtime(&the_time);

	if(PrevSec != time_stuff->tm_sec) {    	
      nDays = ((xmas-time_stuff->tm_yday) <0) ? 365-(time_stuff->tm_yday-xmas) : xmas-time_stuff->tm_yday;
	  nHours = 23-(time_stuff->tm_hour);
	  nMin = 59-(time_stuff->tm_min);
      nSec = (59-time_stuff->tm_sec);
	    	
	  DateString="";
      DateString << nDays << " days, "
    	<< nHours << " hours, "
    	<< nMin << " minutes, "
    	<< nSec << " seconds until xmas";
      Invalidate();
	}    	

	PrevSec=time_stuff->tm_sec;
	


}

