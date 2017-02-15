/* TweakWin.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Box.h>
#include <Slider.h>
#include <CheckBox.h>
#include <Application.h>
#include "TweakWin.h"
#include "MsgVals.h"

const int32 MIN_SLEEP = 3000;
const int32 MAX_SLEEP = 30000;

/*-------------------------------------------------------------------------*/

TweakWin::TweakWin(BWindow* hand, int32 imass, int32 idrag, 
					int32 iwidth, int32 isleepage)
	: BWindow(BRect(610,50,760,500), "Tweakables", B_FLOATING_WINDOW, 0)
{
	handler = hand;
	
	BView* tv = new BView(Bounds(), "tweakview", B_FOLLOW_ALL, B_WILL_DRAW);

	/* fancy border box */	
	BBox* bb = new BBox(BRect(5,10,Bounds().Width()-5, 
								Bounds().Height()-10));
	tv->AddChild(bb);
	
	/* create sliders for mass, drag, etc... */	
	mass = new BSlider(BRect(5,20,135,50), "mass", "Mass", 
						 new BMessage(MASS_CHG), 0, 100);
	mass->SetHashMarks(B_HASH_MARKS_BOTTOM);
	mass->SetLimitLabels("Light", "Heavy");
	mass->SetValue(imass);
	bb->AddChild(mass);
	
	drag = new BSlider(BRect(5,90,135,120), "drag", "Drag", 
						 new BMessage(DRAG_CHG), 0, 100);
	drag->SetHashMarks(B_HASH_MARKS_BOTTOM);
	drag->SetLimitLabels("Glass", "Carpet");
	drag->SetValue(idrag);
	bb->AddChild(drag);

	width = new BSlider(BRect(5,160,135,190), "width", "Width",
						new BMessage(WIDTH_CHG), 0, 100);
	width->SetHashMarks(B_HASH_MARKS_BOTTOM);
	width->SetLimitLabels("Narrow", "Wide");
	width->SetValue(iwidth);
	bb->AddChild(width);

	sleepage = new BSlider(BRect(5,230,135,260), "sleepage", "Snooze Factor",
							new BMessage(SLEEPAGE_CHG), MIN_SLEEP, MAX_SLEEP);
	sleepage->SetHashMarks(B_HASH_MARKS_BOTTOM);
	sleepage->SetLimitLabels("Coma", "Catnap");
	sleepage->SetValue(isleepage);
	bb->AddChild(sleepage);

	/* create checkboxes for Wireframe and Fixed angle */	
	fill = new BCheckBox(BRect(25,320,135,340), "fill", "Wireframe",
						 new BMessage(FILL_CHG));
	fill->SetTarget(handler);
	bb->AddChild(fill);
	
	angle = new BCheckBox(BRect(25,350,135,370), "angle", "Fixed Angle",
						  new BMessage(ANGLE_CHG));
	angle->SetValue(1);
	angle->SetTarget(handler);
	bb->AddChild(angle);

	/* set the color to gray, add the child, and show the window */
	tv->SetViewColor(216,216,216);
	AddChild(tv);
	
	Show();
}
/*-------------------------------------------------------------------------*/

void 
TweakWin::MessageReceived(BMessage* msg)
{
	int32 val;
	switch(msg->what)
	{
	 case MASS_CHG:
	 	val = mass->Value();
	 	msg->AddInt32("Mass", val);
	 	handler->PostMessage(msg);
	 	break;	

	 case DRAG_CHG:
	 	val = drag->Value();
	 	msg->AddInt32("Drag", val);
	 	handler->PostMessage(msg);
	 	break;

	 case WIDTH_CHG:
	 	val = width->Value();
	 	msg->AddInt32("Width", val);
	 	handler->PostMessage(msg);
	 	break;

	 case SLEEPAGE_CHG:
	 	val = sleepage->Value();
	 	msg->AddInt32("Sleepage", val);
	 	handler->PostMessage(msg);
	 	break;

	 default:
	 	BWindow::MessageReceived(msg);
	 	break;
	}
}
/*-------------------------------------------------------------------------*/

void TweakWin::Quit() 
{
	/* alert the application of our impending demise */
	be_app->PostMessage(TWEAK_QUIT);
	BWindow::Quit();
}
/*-------------------------------------------------------------------------*/
