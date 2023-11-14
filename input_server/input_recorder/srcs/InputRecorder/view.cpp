/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <stdio.h>

#include <CheckBox.h>
#include <Path.h>
#include <Window.h>
  
#include "view.h"
#include "main.h"

TView::TView(BRect rect, const char *name, bool rec, bool stop, bool play)
	: BBox(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
		B_PLAIN_BORDER)
{
// Spacing between each item
#define SPACE_Y 7
#define SPACE_X 5
// Length of each item
#define LEN_Y 20

	BRect r;
	BMessage *msg;
	BButton *but = NULL;

	int x;
	r = Bounds();
	float y = r.Height();
	float width = r.Width();

	y = (y - LEN_Y) - SPACE_Y;
	width /= 3;
	for( x = 0; x < 3; x ++){
		r.Set(SPACE_X + x*width,y,x * width + width - SPACE_X,y+LEN_Y);
		switch(x){
		case 0:
			msg = new BMessage(T_RECORD);
			but = new BButton(r, "RecordButton", "Record", msg);
			but->SetEnabled(rec);
			break;
		case 1:
			msg = new BMessage(T_STOP);
			but = new BButton(r, "StopButton", "Stop", msg);
			but->SetEnabled(stop);
			break;
		case 2:
			msg = new BMessage(T_PLAY);
			but = new BButton(r, "PlayButton", "Play", msg);
			but->SetEnabled(play);
			break;
		}
		AddChild(but);
	}
}

void TView::MessageReceived(BMessage *msg)
{
	static const char *names[3][2] = {{"RecordButton","Record"},{"StopButton","Stop"},
		{"PlayButton","Play"}};
	switch(msg->what){
		case T_SET_BUTTONS: {
			bool b;
			if(Window()->Lock()){
				BView *view;
				BControl *ctl;
				for(int i = 0; i < 3; i++){
					view = FindView(names[i][0]);
					if(view!=NULL) ctl = dynamic_cast<BControl*>(view);
					else ctl = NULL;
					if(ctl!=NULL && msg->FindBool(names[i][1],&b)==B_OK) {
						ctl->SetEnabled(b);
					}
				}
				Window()->Unlock();
			}
			break;
		}
		default: {
			BBox::MessageReceived(msg);
			break;
		}
	}
}

