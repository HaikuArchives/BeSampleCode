/*
	
	ButtonWorld.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ButtonWindow.h"
#include "ButtonWorld.h"
#include <Entry.h>
#include <Looper.h>
#include <View.h>

int main(int, char**)
{	
	// Create an application instance
	ButtonApplication myApplication;
	
	// Run the application looper
	myApplication.Run();

	return(0);
}

// ButtonApplication constructor
ButtonApplication::ButtonApplication()
		  		  : BApplication("application/x-vnd.Be-ButtonWorldSample")
{
	BRect			aRect;

	// set up a rect and instantiate a new window
	aRect.Set(200, 200, 400, 300);
	buttonWindow = new ButtonWindow(aRect);
		
	// make window visible
	buttonWindow->Show();
}

// ButtonApplication::RefsReceived overiding BApplication::RefsReceived
void ButtonApplication::RefsReceived(BMessage *message) 
{ 
    entry_ref ref;
    
	// get the ref from the message
    if ( message->FindRef("refs", &ref) == B_OK ){
        
        // Make a new message
        BMessage aMessage( B_SIMPLE_DATA );
        
        // Copy the ref into it
        aMessage.AddRef( "refs", &ref );
        
		// Print the message to see its contents
		aMessage.PrintToStream();
		
		//Lock the window before calling ChildAt()  
        buttonWindow->Lock();
        BView *view = buttonWindow->ChildAt(0);
		//Unlock the window before calling PostMessage()
        buttonWindow->Unlock();
        // Post the message via the window
       	buttonWindow->PostMessage( &aMessage, view );
    }
}