/*

	ButtonWindow.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Picture.h>
#include <PictureButton.h>
#include "ButtonWindow.h"
#include "ButtonView.h"
#include "blue4x4.h"
#include "blue4x4on.h"

ButtonWindow::ButtonWindow(BRect frame)
				: BWindow(frame, "ButtonWorld", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect rect;
	rect.Set(5, 5, 195, 35);
	//Create a text view
	ButtonView *buttonView = new ButtonView( rect, "button", "Drag a file to me" );
	
	//Create some buttons
		//Plain ol' BButton...
		rect.Set( 5, 45, 95, 75 );
		BMessage *plainmsg = new BMessage( BUTTON_MSG );
		plainmsg->AddString("text", "Plain Button");
		BButton* plainButton = new BButton( rect, 
											"plain", 
         									"plain", 
         								 	plainmsg);
			
		//BPictureButton...
			rect.Set(0,0,47,47);
			//bitmaps for the pictures
			BBitmap onBitmap(rect, B_COLOR_8_BIT );
			BBitmap offBitmap(rect, B_COLOR_8_BIT );
			//fill bitmap
			onBitmap.SetBits(blue4x4on, 18432, 0, B_COLOR_8_BIT);
			offBitmap.SetBits(blue4x4, 18432, 0, B_COLOR_8_BIT);
			//tempview for creating the picture
			BView *tempView = new BView( rect, "temp", B_FOLLOW_NONE, B_WILL_DRAW );
			AddChild(tempView);
			//create on picture
		   	BPicture *on;
   			tempView->BeginPicture(new BPicture); 
   			tempView->DrawBitmap(&onBitmap);
   			on = tempView->EndPicture();
   			//create off picture
   			BPicture *off;
   			tempView->BeginPicture(new BPicture); 
   			tempView->DrawBitmap(&offBitmap);
   			off = tempView->EndPicture();
   			//get rid of tempview
   			RemoveChild(tempView);
   			delete tempView;
   			//create a message for the button
   			BMessage *pictmsg = new BMessage(BUTTON_MSG);
   			pictmsg->AddString("text", "Picture Button");
			//create a picture button using the two pictures
			rect.Set( 120, 45, 167, 92 );
			BPictureButton* pictureButton = new BPictureButton(rect, 
																"picture", 
																off, 
																on, 
																pictmsg, 
																B_TWO_STATE_BUTTON);
			
		
	// add view and buttons to window
	AddChild(buttonView);
	AddChild(plainButton);
	AddChild(pictureButton);
	
	// make the view the target of the buttons
	plainButton->SetTarget(buttonView);
	pictureButton->SetTarget(buttonView);
}

bool ButtonWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}