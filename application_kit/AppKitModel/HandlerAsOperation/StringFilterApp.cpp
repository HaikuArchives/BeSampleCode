/* StringFilterApp.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "StringFilterApp.h"
#include "StringFilterLooper.h"
#include "StringFilterProtocol.h"

#include <stdio.h>

/* the test phrases */
const char *phrase1 = "Are we having fun yet?";
/*
Just a small note about this second phrase
(which many people might think is just a little odd.)
This quote comes from my favorite classic movie: "The Philadelphia Story".
Jimmy Stewart, amazed at the number of rooms in a mansion and curious
about the internal telephone system, calls a random room and responds to
the question of "Who is this?" with this quote:
*/
const char *phrase2 = 	"This is doom calling.  "
  						"Your days are numbered to the seventh sun of the seventh sign.";


StringFilterApp::StringFilterApp()
	: 	BApplication("application/x-vnd.BeDTS-StringFilterApp"),
		//initialize the StringFilterLooper
		fFilters(new StringFilterLooper)
{
}


StringFilterApp::~StringFilterApp()
{
}

void 
StringFilterApp::ReadyToRun()
{
	/* determine what operations are available */
	/* by sending an OPCODES message to the */
	/* StringFilterLooper */
	BMessenger msgr(fFilters);
	BMessage msg(OPCODES);
	BMessage info;
	msgr.SendMessage(&msg, &info);
	
		
	/* determine the number of available operations */
	int32 count = 0;
	type_code type;
	info.GetInfo("opcodes", &type, &count);
	
	/* print out the names and descriptions */
	/* of the available operations */
	printf("Available Operations:\n");
	for(int32 ix = 0; ix < count; ix++) {
		const char *op = NULL;
		const char *desc = NULL;
		
		info.FindString("operations", ix, &op);
		info.FindString("descriptions", ix, &desc);

		printf("\t%s: %s\n", op, desc);
	}
	printf("\n");
	
	/* create the basic message to be filtered */
	
	//set the what to FILTER
	BMessage to_filter(FILTER);
	
	//add the strings to be filtered
	to_filter.AddString("strings", phrase1);
	to_filter.AddString("strings", phrase2);
	
	// add an empty default opcode
	to_filter.AddInt32("opcode", 0);
	
	/* print out the oringal string values */
	printf("Original Strings:\n%s\n%s\n\n", phrase1, phrase2);
	
	/* for every available operation */
	/* send the strings to be filtered */
	for (int ix = 0; ix < count; ix++) {

		//print out the name of the operation
		const char *op = NULL;
		info.FindString("operations", ix, &op);
		printf("%s:\n", op);

		//find the opcode to be used
		int32 opcode = 0;
		info.FindInt32("opcodes", ix, &opcode);
		
		//specify the correct opcode
		to_filter.ReplaceInt32("opcode", opcode);
		
		//send off the message and wait for a reply
		BMessage reply;
		msgr.SendMessage(&to_filter, &reply);
		
		//find out how many strings have been returned
		int32 string_count = 0;
		reply.GetInfo("strings", &type, &string_count);
		
		//print out the returned strings
		for (int32 i = 0; i < string_count; i++) {
			const char *string = NULL;
			reply.FindString("strings", i, &string);
			printf("%s\n", string);
		}
		printf("\n");
	}
	
	//tell the application to quit
	PostMessage(B_QUIT_REQUESTED, this);
}

bool 
StringFilterApp::QuitRequested()
{
	//ask the StringFilterLooper to Quit
	if (fFilters) {
		BMessenger msgr(fFilters);
		msgr.SendMessage(B_QUIT_REQUESTED);
	}
	return true;
}

int main() {
	StringFilterApp app;
	app.Run();
	return 0;
}