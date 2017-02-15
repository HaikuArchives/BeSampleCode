/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <View.h>

#include "colorutils.h"
#include "drawutils.h"

void Stroke3DBox(BView* v, BRect r)
{
	v->PushState();
	BRect s = r;
	s.right--; s.bottom--;
	s.OffsetBy(1,1);
	v->SetHighColor(PURE_WHITE);
	v->StrokeRect(s);
	s.OffsetBy(-1,-1);
	v->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
	v->StrokeRect(s);	
	v->PopState();
}
