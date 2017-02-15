// MidiUtil.cpp
// ------------
// Implements assorted MIDI utility functions.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Debug.h>
#include <MidiRoster.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include "MidiUtil.h"

BMidiEndpoint* MidiUtil::FindEndpoint(const char* name)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster) {
		PRINT(("Couldn't find MIDI roster\n"));
		return NULL;
	}
			
	int32 id=0;
	BMidiEndpoint* object = roster->NextEndpoint(&id);
	while (object) {
		if (! strcmp(object->Name(), name))
			break;

		// Don't forget to decrement the refcount
		// from NextEndpoint!			
		object->Release();
		object = roster->NextEndpoint(&id);
	}

	return object;
}

BMidiProducer* MidiUtil::FindProducer(const char* name)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster) {
		PRINT(("Couldn't find MIDI roster\n"));
		return NULL;
	}
			
	int32 id=0;
	BMidiProducer* object = roster->NextProducer(&id);
	while (object) {
		if (! strcmp(object->Name(), name))
			break;
			
		// Don't forget to decrement the refcount
		// from NextProducer!			
		object->Release();
		object = roster->NextProducer(&id);
	}

	return object;
}

BMidiConsumer* MidiUtil::FindConsumer(const char* name)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster) {
		PRINT(("Couldn't find MIDI roster\n"));
		return NULL;
	}
			
	int32 id=0;
	BMidiConsumer* object = roster->NextConsumer(&id);
	while (object) {
		if (! strcmp(object->Name(), name))
			break;
			
		// Don't forget to decrement the refcount
		// from NextConsumer!			
		object->Release();
		object = roster->NextConsumer(&id);
	}

	return object;
}
