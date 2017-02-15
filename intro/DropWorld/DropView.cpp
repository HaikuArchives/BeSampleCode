/*
	
	DropView.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "DropView.h"
#include <Message.h>
#include <Entry.h>

DropView::DropView(BRect rect, const char *name, const char* text)
	   	   : BStringView(rect, name, text, B_FOLLOW_ALL, B_WILL_DRAW)
{
	//Set up the font for the text
	SetFont(be_bold_font);
	SetFontSize(24);
}


void DropView::MessageReceived(BMessage *message)
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
   		default:
   			// Call inherited if we didn't handle the message
   			BStringView::MessageReceived(message);
   			break;
	}
}

// This is here to show a quick example of what to do with MouseDown message
void DropView::MouseDown(BPoint)
{
	//Call SetText() to change the string in the view
	SetText( "Don't click, drag!" );
}