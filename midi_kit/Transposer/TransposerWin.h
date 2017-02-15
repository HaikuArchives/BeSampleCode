// TransposerWin.h
// ---------------
// The main window associated with the Transposer object.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _TransposerWin_h
#define _TransposerWin_h

#include <Window.h>

class TransposerWin : public BWindow {
public:
	TransposerWin(const char* name, int8 transpose);
	void Quit();
};

#endif /* _TransposerWin_h */
