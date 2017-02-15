// ChannelMaskWin.h
// ----------------
// A panel that allows the user to turn transposition on/off on a
// channel-by-channel basis. Instead of quitting when you click on
// the close box (i.e. when QuitRequested is called), it simply
// hides itself.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _ChannelMaskWin_h
#define _ChannelMaskWin_h

#include <Window.h>
#include <List.h>

class ChannelMaskWin : public BWindow
{
public:
	ChannelMaskWin(uint16 startMask);
	void MessageReceived(BMessage* msg);
	bool QuitRequested();
		
private:
	uint16 CurrentMask() const;
	BList m_boxes;
};

#endif /* _ChannelMaskWin_h */
