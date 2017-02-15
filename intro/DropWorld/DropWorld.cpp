/*
	
	DropWorld.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "DropWindow.h"
#include "DropWorld.h"
#include <Entry.h>
#include <Window.h>
#include <View.h>

int main(int, char**)
{	
	// Create an application instance
	DropApplication myApplication;
	
	// Run the application looper
	myApplication.Run();
	
	return(0);
}

// DropApplication constructor
DropApplication::DropApplication()
		  		  : BApplication("application/x-vnd.Be-DTS.DropWorld")
{
	BRect			aRect;

	// set up a rect and instantiate a new window
	aRect.Set(100, 80, 300, 120);
	dropWindow = new DropWindow(aRect);
		
	// make window visible
	dropWindow->Show();
}

// DropApplication::RefsReceived overiding BApplication::RefsReceived
void DropApplication::RefsReceived(BMessage *message) 
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
        dropWindow->Lock();
        BView *view = dropWindow->ChildAt(0);
		//Unlock the window before calling PostMessage()
        dropWindow->Unlock();
        // Post the message via the window
       	dropWindow->PostMessage( &aMessage, view );
    }
}