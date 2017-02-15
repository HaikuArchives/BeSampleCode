//
// Text World
//
// A sample program that's gradually evolving into
// a real text editor application.
//
// Written by: Eric Shepherd
//
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Messenger.h>
#include <Message.h>
#include <Roster.h>
#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <FilePanel.h>
#include <Path.h>
#include <Entry.h>
#include <TextView.h>
#include <ScrollView.h>
#include <string.h>
#include <stdio.h>

#include "constants.h"
#include "textapp.h"
#include "textwindow.h"

// Application's signature

const char *APP_SIGNATURE				= "application/x-vnd.Be-TextWorld";

BRect windowRect(50,50,599,399);

//
// TextApp::TextApp
//
// The constructor for the TextApp class.  This
// will create our window.
//
TextApp::TextApp()
			: BApplication(APP_SIGNATURE) {
	
	window_count = 0;			// No windows yet
	next_untitled_number = 1;	// Next window is "Untitled 1"
	
	// Create the Open file panel
	
	openPanel = new BFilePanel;
	
	new TextWindow(windowRect);
}


//
// TextApp::MessageReceived
//
// Handle incoming messages.  In particular, handle the
// WINDOW_REGISTRY_ADD and WINDOW_REGISTRY_SUB messages.
//
void TextApp::MessageReceived(BMessage *message) {
	switch(message->what) {
		case WINDOW_REGISTRY_ADD:
			{
				bool need_id = false;
				BMessage reply(WINDOW_REGISTRY_ADDED);
				
				if (message->FindBool("need_id", &need_id) == B_OK) {
					if (need_id) {
						reply.AddInt32("new_window_number", next_untitled_number);
						next_untitled_number++;
					}
					window_count++;
				}
				reply.AddRect("rect", windowRect);
				windowRect.OffsetBy(20,20);
				message->SendReply(&reply);
				break;
			}
		case WINDOW_REGISTRY_SUB:
			window_count--;
			if (!window_count) {
				Quit();
			}
			break;
		case MENU_FILE_OPEN:
			openPanel->Show();		// Show the file panel
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

//
// TextApp::RefsReceived
//
// Handle a refs received message.
//
void TextApp::RefsReceived(BMessage *message) {
	entry_ref 	ref;		// The entry_ref to open
	status_t 	err;		// The error code
	int32		ref_num;	// The index into the ref list
	
	// Loop through the ref list and open each one

	ref_num = 0;
	do {
		if ((err = message->FindRef("refs", ref_num, &ref)) != B_OK) {
			return;
		}
		new TextWindow(windowRect, &ref);
		ref_num++;
	} while (1);
}


//
// main
//
// The main() function's only real job in a basic BeOS
// application is to create the BApplication object
// and run it.
//
int main(void) {
	TextApp theApp;		// The application object
	theApp.Run();
	return 0;
}



