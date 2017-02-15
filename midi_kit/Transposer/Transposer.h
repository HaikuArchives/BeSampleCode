// Transposer.h
// ------------
// A MIDI filter object that transposes all incoming note events by a
// fixed amount.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _Transposer_h
#define _Transposer_h

#include "SimpleMidiFilter.h"

class Transposer : public SimpleMidiFilter
{
public:
	Transposer(const char* name, int8 transpose=0, uint16 channelMask=0xFFFF);
	int8 Transpose() const;
	void SetTranspose(int8 transpose);
	uint16 ChannelMask() const;
	void SetChannelMask(uint16 channelMask);
	
	filter_result NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time);
	filter_result NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time);
	filter_result KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time);

private:
	void GetIcons(BBitmap* largeIcon, BBitmap* miniIcon);
	void AddIcons(BMessage* msg, BBitmap* largeIcon, BBitmap* miniIcon) const;
	
	uchar TransposeNote(uchar note);
	int8 m_transpose;
	uint16 m_channelMask;
};

#endif /* _Transposer_h */
