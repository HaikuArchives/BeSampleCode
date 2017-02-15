/* StringApplication.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "StringApplication.h"
#include "StringLooper.h"
#include "StringHandlerProtocol.h"

#include <stdio.h>

/* phrases to test */
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



StringApplication::StringApplication()
	: 	BApplication("application/x-vnd-BeDTS-StringApplication"),
		//initalize the StringLooper
		fStrings(new StringLooper)
{
}


StringApplication::~StringApplication()
{
}

bool 
StringApplication::QuitRequested()
{
	//ask the StringLooper to quit
	if (fStrings) {
		BMessenger strings(fStrings);
		strings.SendMessage(B_QUIT_REQUESTED);
		return true;
	} else return true;
}

void 
StringApplication::ReadyToRun()
{
	//test each phrase
	TestPhrase("phrase_one", phrase1);
	TestPhrase("phrase_two", phrase2);
	
	//tell ourselves to quit
	PostMessage(B_QUIT_REQUESTED, this);
}

void 
StringApplication::TestPhrase(const char *name, const char *phrase)
{
	//create some useful items
	BMessenger strings(fStrings);
	BMessage get_info(GET_STRINGS);
	BMessage info;

	//check and print the current status of strings
	printf("Finding Strings...\n");
	strings.SendMessage(&get_info, &info);
	int32 count = 0;
	info.FindInt32("count", &count);
	printf("strings found: %ld\n", count);
	for (int32 ix = 0; ix < count; ix ++) {
		const char *name = NULL;
		const char *value = NULL;
		info.FindString("name", ix, &name);
		info.FindString("value", ix, &value);
		printf("%s = %s\n", name, value);
	}
	printf("\n");

	//create the new phrase:
	printf("creating ' %s '\n", name);		

	//send a NEw_STRING message with the name
	//and value of the new string to the StringLooper
	BMessage msg(NEW_STRING);
	msg.AddString("name", name);
	msg.AddString("value", phrase);
	BMessage reply;
	strings.SendMessage(&msg, &reply);

	if (reply.what != B_NO_REPLY) {
		BMessenger the_phrase;
		if (reply.FindMessenger("msgr", &the_phrase) == B_OK) {

			//the string was sucessfully created
			msg.MakeEmpty();
			reply.MakeEmpty();
			
			//get and print the original value of the string
			msg.what = GET_VALUE;
			the_phrase.SendMessage(&msg, &reply);
			const char *value = NULL;
			if (reply.FindString("value", &value) == B_OK) {
				printf("original:  %s\n", value);
			}
			
			/* make the string uppercase */
			printf("uppercase: ");
			msg.MakeEmpty();
			
			//send an UPPERCASE message to the StringHandler
			msg.what = UPPERCASE;
			the_phrase.SendMessage(&msg);
			
			msg.MakeEmpty();
			reply.MakeEmpty();
			
			//get and print the new value
			msg.what = GET_VALUE;
			the_phrase.SendMessage(&msg, &reply);
			value = NULL;
			if (reply.FindString("value", &value) == B_OK) {
				printf("%s\n", value);
			}

			/* make the string lowercase */
			printf("lowercase: ");
			msg.MakeEmpty();
			
			//send a LOWERCASE message to the StringHandler
			msg.what = LOWERCASE;
			the_phrase.SendMessage(&msg);
			
			msg.MakeEmpty();
			reply.MakeEmpty();
			
			//get and print the new value
			msg.what = GET_VALUE;
			the_phrase.SendMessage(&msg, &reply);
			value = NULL;
			if (reply.FindString("value", &value) == B_OK) {
				printf("%s\n", value);
			}
			
			/* make the  string mixed case */
			printf("mixedcase: ");
			msg.MakeEmpty();
			
			//send a MIXEDCASE message to the StringHandler
			msg.what = MIXEDCASE;
			the_phrase.SendMessage(&msg);
			
			msg.MakeEmpty();
			reply.MakeEmpty();

			//get and print the new value
			msg.what = GET_VALUE;
			the_phrase.SendMessage(&msg, &reply);
			value = NULL;
			if (reply.FindString("value", &value) == B_OK) {
				printf("%s\n", value);
			}
			
		}
		
	}
	printf("\n");
	
	//check and print the current status of strings
	printf("Finding Strings...\n");
	info.MakeEmpty();
	strings.SendMessage(&get_info, &info);
	count = 0;
	info.FindInt32("count", &count);
	printf("strings found: %ld\n", count);
	for (int32 ix = 0; ix < count; ix ++) {
		const char *name = NULL;
		const char *value = NULL;
		info.FindString("name", ix, &name);
		info.FindString("value", ix, &value);
		printf("%s = %s\n", name, value);
	}
	printf("\n");
}



int main() {
	StringApplication app;
	app.Run();
	return 0;
}