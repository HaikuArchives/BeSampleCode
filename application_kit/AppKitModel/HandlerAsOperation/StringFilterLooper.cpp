/* StringFilterLooper.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "StringFilterLooper.h"
#include "StringFilterProtocol.h"
#include "StringFilter.h"

#include <string.h>
#include <Message.h>

StringFilterLooper::StringFilterLooper()
	: BLooper("StringFilters")
{
	//load all of the filters
	LoadFilters();
	//start up the message loop
	Run();
}


StringFilterLooper::~StringFilterLooper()
{
}

void 
StringFilterLooper::MessageReceived(BMessage *msg)
{

	switch(msg->what) {
		case OPCODES:
			GetOpcodes(msg);
			break;		
	
		case FILTER:
			Filter(msg);
			break;
	
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

bool 
StringFilterLooper::QuitRequested()
{
	//remove and delete all handlers
	//always start with the handler at index 1
	//as index 0 is ourselves, and cannot be removed
	while (CountHandlers() > 1) {
		BHandler *filter = HandlerAt(1);
		RemoveHandler(filter);
		delete filter;
	}
	return true;
}

/* tell each StringFilter to add its information to */
/* the message and then send it back */
void 
StringFilterLooper::GetOpcodes(BMessage *msg)
{
	int32 count = CountHandlers();
	//start with Handler 1 as Handler 0 is always the BLooper itself
	for (int ix = 1; ix < count; ix++) {
		BHandler *handler = HandlerAt(ix);
		handler->MessageReceived(msg);
	}
	//send the filled out message back
	msg->SendReply(msg);
}

/* These functions were broken out becuase they needed knowledge of the actual */
/* filters availble. */
/* This is simply for simplicity for the sample code.  Normally */
/* the looper class would instantiate the filters some other way.  */
/* A typical example would be through loading add-ons.  As I did not */
/* want the add-on loading code to complicate the issue, I have omitted it */
/* and simply instantiated the known filters by hand. */
/* In addition, the Looper would normally have a way to choose which */
/* filter to call for each specified opcode.  This separate list would */
/* also complicate the example, so was left out. */

#include "Filters.h"

/* instantiate all of the StringFilters */
void 
StringFilterLooper::LoadFilters()
{
	StringFilter *filter = NULL;
	
	filter = new UpperCase();
	AddHandler(filter);
	
	filter = new LowerCase();
	AddHandler(filter);
	
	filter = new MixedCase();
	AddHandler(filter);
}

/* determine which Filter to call for the given opcode */
/* this could easily be extended to do this action for */
/* multiple opcodes in order */
void 
StringFilterLooper::Filter(BMessage *msg)
{
	int32 opcode = 0;
	msg->FindInt32("opcode", &opcode);
	
	//match the opcode with a StringFilter name
	const char *name = NULL;
	switch(opcode) {
		case LOWERCASE:
			name = lowercase_name;
			break;
		
		case UPPERCASE:
			name = uppercase_name;
			break;
		
		case MIXEDCASE:
			name = mixedcase_name;
			break;
			
		default:
			break;
	}

	//step through each StringFilter to match the name
	int32 count = CountHandlers();
	for (int32 ix = 1; ix < count; ix++) {
		BHandler *filter = HandlerAt(ix);
		if (strcmp(filter->Name(), name) == 0) {
			//name found: pass along the message
			filter->MessageReceived(msg);
			//send the reply
			msg->SendReply(msg);
			ix = count;
		}
	}

}

