//
// Preferences sample project
//
// by Eric Shepherd
//
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TPreferences.h"

int main(void) {
	TPreferences prefs("PrefsSample_prefs");			// Preferences
	
	// If the prefs object didn't initialize properly,
	// use default preference values.
	
	if (prefs.InitCheck() != B_OK) {
		prefs.SetInt64("last_used", real_time_clock());
		prefs.SetInt32("use_count", 0);
	}
	
	// Show the preference values.
	
	prefs.PrintToStream();
	
	// Change the preferences.
	
	int32 count;
	if (prefs.FindInt32("use_count", &count) != B_OK) {
		count = 0;
	}
	prefs.SetInt64("last_used", real_time_clock());
	prefs.SetInt32("use_count", ++count);
	
	// The preferences are saved automatically when the object
	// is deleted, so we don't have to do anything.
	
	return 0;
}