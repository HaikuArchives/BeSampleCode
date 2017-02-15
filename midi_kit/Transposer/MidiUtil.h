// MidiUtil.h
// ----------
// Assorted MIDI utility functions.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _MidiUtil_h
#define _MidiUtil_h

namespace MidiUtil {
	// Find MIDI objects by name. (Note: this is not carte blanche
	// for hardcoding MIDI object names in your app. :-)
	BMidiEndpoint* FindEndpoint(const char* name);
	BMidiProducer* FindProducer(const char* name);
	BMidiConsumer* FindConsumer(const char* name);
}

#endif /* _MidiUtil_h */
