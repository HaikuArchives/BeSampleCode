/* ColorWin.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Message.h>
#include <ColorControl.h>
#include <Application.h>
#include "ColorWin.h"
#include "MsgVals.h"

/*-------------------------------------------------------------------------*/

ColorWin::ColorWin(BWindow* hand, rgb_color initialColor)
	: BWindow(BRect(610,550,620,560), "Pen Color", B_FLOATING_WINDOW, B_NOT_RESIZABLE)
{
	float w, h;
	
	/* where we should direct our messages */
	handler = hand;
	
	/* create a BColorControl object and attach it to the window */
	cc = new BColorControl(BPoint(50,1), B_CELLS_32x8, 5, "colorcntl",
							new BMessage(COLOR_CHG));
	cc->SetValue(initialColor);
	cc->GetPreferredSize(&w, &h);
	ResizeTo(w+50,h);
	AddChild(cc);
	
	/* create a "swatch" which show the color of the current settings */
	/* from the BColorControl object -- not "real time", though*/
	swatch = new BView(BRect(5,5,45,Bounds().Height()-5),"swatch",
						B_FOLLOW_ALL, B_WILL_DRAW);
	swatch->SetViewColor(cc->ValueAsColor());
	AddChild(swatch);
	
	Show();

}
/*-------------------------------------------------------------------------*/

void
ColorWin::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
	 case COLOR_CHG:
		/* get the current color settings, and attach to the message */
		/* and pass it along to the handler */
		rgb_color clr = cc->ValueAsColor();
		swatch->SetViewColor(clr);
		swatch->Invalidate();
	 	msg->AddInt16("red", clr.red);
		msg->AddInt16("green", clr.green);
		msg->AddInt16("blue", clr.blue);	
		handler->PostMessage(msg);
		break;
		
	 default:
	 	/* always pass messages along! */
	 	BWindow::MessageReceived(msg);
	 	break;
	}
}
/*-------------------------------------------------------------------------*/

void
ColorWin::Quit()
{
	/* inform the application that we are dying */
	be_app->PostMessage(COLOR_QUIT);
	BWindow::Quit();
}
/*-------------------------------------------------------------------------*/
