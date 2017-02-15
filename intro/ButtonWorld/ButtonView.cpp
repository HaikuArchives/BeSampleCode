/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/*
	
	ButtonView.cpp
	
*/

#include "ButtonView.h"
#include <Message.h>
#include <Entry.h>

ButtonView::ButtonView(BRect rect, const char *name, const char* text)
	   	   : BStringView(rect, name, text, B_FOLLOW_ALL, B_WILL_DRAW)
{
	//Set up the font for the text
	SetFont(be_bold_font);
	SetFontSize(24);
}


void ButtonView::MessageReceived(BMessage *message)
{
	// Print the message to see its contents
	message->PrintToStream();
   	entry_ref ref;
 	switch ( message->what ){
   		case B_SIMPLE_DATA:
   			// Look for a ref in the message
   			if( message->FindRef("refs", &ref) == B_OK ){
				// Call SetText() to change the string in the view
   				SetText( ref.name );
   			}else{
   				// Call inherited if we didn't handle the message
   				BStringView::MessageReceived(message);
   			}
   			break;
   		case BUTTON_MSG:
   			SetText( message->FindString("text") );
   			break;   				
   		default:
   			// Call inherited if we didn't handle the message
   			BStringView::MessageReceived(message);
   			break;
	}
}

// This is here to show a quick example of what to do with MouseDown message
void ButtonView::MouseDown(BPoint)
{
	//Call SetText() to change the string in the view
	SetText( "Don't click, drag!" );
}