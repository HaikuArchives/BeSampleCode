/*
	
	SoundDock.cpp
	
	A draggable dock for sound files.

*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <MediaAddOn.h>
#include <MediaRoster.h>

#include "MixWin.h"
#include "SoundDock.h"

SoundDock::SoundDock(const char* name)
	: FileDock(BRect(0,0,1,1), name)
{	
}

bool SoundDock::IsValidRef(const entry_ref* ref)
{
	if (! ref)
		return false;
		
	// First, check with our parent to see whether
	// we are available.
	BWindow* win = Window();
	MixWin* mixwin = dynamic_cast<MixWin*>(win);
	if (mixwin && (! mixwin->ChannelAvailable(this)))
		return false;
		
	// Next, check to see whether the ref is a
	// sound file -- if it is, we won't allow
	// the file to be dropped on us. We'll call
	// this a sound file if we can find a node
	// that will read it and give us raw audio
	// output for it.
	dormant_node_info dni;
	status_t err = BMediaRoster::Roster()->SniffRef(*ref,
		B_BUFFER_PRODUCER | B_FILE_INTERFACE, &dni);
	if (err != B_OK)
		// we can't find anything that'll read this file
		return false;
	
	dormant_flavor_info flavor;
	err = BMediaRoster::Roster()->GetDormantFlavorInfoFor(dni, &flavor);
	if (err != B_OK)
		// some bad error occurred
		return false;
	
	// We found something that will read this. Now see if it'll
	// give us raw adudio output.
	for (int32 i=0; i<flavor.out_format_count; i++) {
		if (flavor.out_formats[i].type == B_MEDIA_RAW_AUDIO)
			return true;
	}
	
	return false;
}
