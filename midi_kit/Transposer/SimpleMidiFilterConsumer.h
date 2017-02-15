// SimpleMidiFilterConsumer.h
// --------------------------
// A consumer that handles input to a SimpleMidiFilter. It basically
// calls the SimpleMidiFilter hook methods whenever it receives
// data. If the result of the hook methods is B_DISPATCH_MESSAGE,
// it then tells the SimpleMidiFilter to spray the event to the
// output.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _SimpleMidiFilterConsumer_h
#define _SimpleMidiFilterConsumer_h

#include <MidiConsumer.h>

class SimpleMidiFilter;

class SimpleMidiFilterConsumer : public BMidiLocalConsumer
{
public:
	SimpleMidiFilterConsumer(const char* name, SimpleMidiFilter* owner);
	
	void NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time);
	void NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time);
	void KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time);
	void ControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time);
	void ProgramChange(uchar channel, uchar programNumber, bigtime_t time);
	void ChannelPressure(uchar channel, uchar pressure, bigtime_t time);
	void PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time);
	void SystemExclusive(void* data, size_t dataLength, bigtime_t time);
	void SystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time);
	void SystemRealTime(uchar statusByte, bigtime_t time);
	void TempoChange(int32 bpm, bigtime_t time);
	
private:
	SimpleMidiFilter* m_owner;
};

#endif /* _SimpleMidiFilterConsumer_h */
