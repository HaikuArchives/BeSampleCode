// SimpleMidiFilter.h
// ------------------
// A MIDI filter object that sends data from one input connection to one
// output connection. To use it, do the following:
//
// 1. Create your own class that derives from SimpleMidiFilter.
//    a) Override the NoteOn, NoteOff, and other hook methods to
//       intercept incoming events. These are similar in function to
//       BMidiLocalConsumer methods, but they return a filter_result
//       to indicate how the original event should be dispatched.
//       Return B_DISPATCH_MESSAGE if you want the original event
//       to be sent to the output, B_SKIP_MESSAGE otherwise. (The
//       default implementation of each of these methods returns
//       the default action specified by SetDefaultAction, or
//       B_DISPATCH_MESSAGE if no default action is specified).
//    b) To send modified or new MIDI events, call SprayNoteOff,
//       SprayNoteOn, and the other BMidiLocalProducer-like
//       functions. They behave as you'd expect.
// 2. Connect a producer up to the input, and a consumer up to the output,
//    using ConnectToInput and ConnectToOutput. If you need to disconnect
//    at any time, there are methods for this as well.
// 3. Don't forget to delete the filter object when you're done!
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _SimpleMidiFilter_h
#define _SimpleMidiFilter_h

#include <String.h>
#include <MessageFilter.h> /* for filter_result */

class SimpleMidiFilterConsumer;
class BMidiLocalProducer;

class SimpleMidiFilter
{
public:
	friend class SimpleMidiFilterConsumer;

	SimpleMidiFilter(const char* name, filter_result defaultAction = B_DISPATCH_MESSAGE);
	virtual ~SimpleMidiFilter();
	
	const char* Name() const { return m_name.String(); }
	void SetName(const char* name) { m_name = name; }
	BMidiLocalConsumer* Input();
	BMidiLocalProducer* Output();
	void Register();
	void Unregister();
	
	filter_result DefaultAction() const { return m_defaultAction; }
	void SetDefaultAction(filter_result action) { m_defaultAction = action; }
		
	// these are the same as their BMidiLocalProducer counterparts;
	// you call them to send data to the ouptut
	void SprayData(void *data, size_t length, bool atomic = false, bigtime_t time = 0) const;
	void SprayNoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time = 0) const;
	void SprayNoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time = 0) const;
	void SprayKeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time = 0) const;
	void SprayControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time = 0) const;
	void SprayProgramChange(uchar channel, uchar programNumber, bigtime_t time = 0) const;
	void SprayChannelPressure(uchar channel, uchar pressure, bigtime_t time = 0) const;
	void SprayPitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time = 0) const;
	void SpraySystemExclusive(void* data, size_t dataLength, bigtime_t time = 0) const;
	void SpraySystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time = 0) const;
	void SpraySystemRealTime(uchar statusByte, bigtime_t time = 0) const; 
	void SprayTempoChange(int32 bpm, bigtime_t time = 0) const;	
	 
protected:
	// these are similar to their BMidiLocalConsumer counterparts,
	// and get called when data arrives at the input.
	virtual	filter_result NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time);
	virtual	filter_result NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time);
	virtual	filter_result KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time);
	virtual	filter_result ControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time);
	virtual	filter_result ProgramChange(uchar channel, uchar programNumber, bigtime_t time);
	virtual	filter_result ChannelPressure(uchar channel, uchar pressure, bigtime_t time);
	virtual	filter_result PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time);
	virtual	filter_result SystemExclusive(void* data, size_t dataLength, bigtime_t time);
	virtual	filter_result SystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time);
	virtual	filter_result SystemRealTime(uchar statusByte, bigtime_t time);
	virtual	filter_result TempoChange(int32 bpm, bigtime_t time);
	
private:
	BString m_name;
	SimpleMidiFilterConsumer* m_input;
	BMidiLocalProducer* m_output;
	filter_result m_defaultAction;
};

#endif /* _SimpleMidiFilter_h */
