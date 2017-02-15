/* StringHandler.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <String.h>
#include <Message.h>

#include "StringHandler.h"
#include "StringHandlerProtocol.h"

StringHandler::StringHandler(const char *value, const char *name)
	//initialize the handler using name if available, and value if not
	: BHandler((name != NULL) ? name : value),
	//initialize the BString
	fValue(value)
{
}


StringHandler::~StringHandler()
{
}

void 
StringHandler::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case LOWERCASE:
			LowerCase();
			break;
		
		case UPPERCASE:
			UpperCase();
			break;
		
		case MIXEDCASE:
			MixedCase();
			break;
			
		case GET_VALUE:
			GetValue(msg);
			break;
			
		case SET_VALUE:
			SetValue(msg);
			break;
		
		case GET_STRINGS:
			GetInfo(msg);
			break;
		
		default:
			BHandler::MessageReceived(msg);
			break;
	}
}

void 
StringHandler::UpperCase()
{
	fValue = fValue.ToUpper();
}

void 
StringHandler::LowerCase()
{
	fValue = fValue.ToLower();
}

void 
StringHandler::MixedCase()
{
	fValue = fValue.CapitalizeEachWord();
}

void 
StringHandler::GetValue(BMessage *msg) const
{
	//pack the value in a message and reply
	BMessage reply(GET_VALUE);
	reply.AddString("value", fValue.String());
	msg->SendReply(&reply);
}

void 
StringHandler::SetValue(BMessage *msg)
{
	//change the value
	const char *value = NULL;
	if (msg->FindString("value", &value) == B_OK)
		fValue = fValue.SetTo(value);
}

void 
StringHandler::GetInfo(BMessage *msg) const
{
	//pack the message with our info
	msg->AddString("name", Name());
	msg->AddString("value", fValue.String());
}


