/* StringFilter.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* Default base class for StringFilters */
/* handles calling the appropriate Hook functions */
/* for each message */

#include <String.h>
#include <Message.h>

#include "StringFilter.h"
#include "StringFilterProtocol.h"

StringFilter::StringFilter(const char *name)
	:	BHandler(name)
{
}


StringFilter::~StringFilter()
{
}

void 
StringFilter::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		
		case FILTER:
			FilterStrings(msg);
			break;

		case OPCODES:
			Opcodes(msg);
			break;

		default:
			BHandler::MessageReceived(msg);
			break;
	}
}


/* step through the message calling Filter on each string */
void 
StringFilter::FilterStrings(BMessage *msg)
{
	//determine how many strings we have
	int32 count = 0;
	type_code type = 0;
	msg->GetInfo("strings", &type, &count);
	
	//if we do not have strings, just bail out
	if (type != B_STRING_TYPE) return;
	
	//for each string found, create a BString and Filter it
	for (int ix = 0; ix < count; ix++) {
		const char *value = NULL;
		BString string(NULL);
		if (msg->FindString("strings", ix, &value) == B_OK) {
			string.SetTo(value);
			Filter(string);
		}
		msg->ReplaceString("strings", ix, string.String());
	}
}
