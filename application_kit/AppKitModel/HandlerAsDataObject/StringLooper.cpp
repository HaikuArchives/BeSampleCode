/* StringLooper.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Message.h>
#include <Messenger.h>

#include "StringLooper.h"
#include "StringHandlerProtocol.h"
#include "StringHandler.h"

StringLooper::StringLooper()
	: BLooper("StringLooper")
{
	//be sure to start the message loop
	Run();
}


StringLooper::~StringLooper()
{
}

void 
StringLooper::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		//start a new string
		case NEW_STRING:
			NewString(msg);
			break;
		
		//report back about existing strings
		case GET_STRINGS:
			GetStrings(msg);
			break;
		
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

bool 
StringLooper::QuitRequested()
{
	//clear out and delete all of the StringHandlers
	//remember that the looper is at Handler index 0
	while (CountHandlers() > 1) {
		StringHandler *handle = (StringHandler *) HandlerAt(1);
		RemoveHandler(handle);
		
		delete handle;
	}
	return true;
}

void 
StringLooper::NewString(BMessage *msg)
{

	const char *value = NULL;
	const char *name = NULL;
	
	if ((msg->FindString("value", &value) == B_OK)
		&& (msg->FindString("name", &name) == B_OK)) {
	
		//both a name and value found so create a new StringHandler
		StringHandler *string = new StringHandler(value, name);
		//add to our list of handlers
		AddHandler(string);
	
		//get a BMessenger for the handler and reply
		//all further contact will be through the
		//StringHandler
		BMessage reply(NEW_STRING);
		reply.AddMessenger("msgr", BMessenger(string));
		msg->SendReply(&reply);
	}
}

void 
StringLooper::GetStrings(BMessage *msg)
{
	//build a reply with all of our values
	BMessage reply(GET_STRINGS);
	int32 count = CountHandlers();
	reply.AddInt32("count", count - 1);
	
	for (int32 ix = 1; ix < count; ix++) {
		//use the standard MessageReceived interface to get this info
		BHandler *string = HandlerAt(ix);
		string->MessageReceived(&reply);
	}
	
	msg->SendReply(&reply);

}

