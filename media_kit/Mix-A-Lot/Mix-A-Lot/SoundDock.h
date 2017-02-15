/*
	
	SoundDock.h

*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _SoundDock_h
#define _SoundDock_h

#include "FileDock.h"

class SoundDock : public FileDock
{
public:
	SoundDock(const char* name);

protected:
	bool IsValidRef(const entry_ref* ref);	
};

#endif /* _SoundDock_h */