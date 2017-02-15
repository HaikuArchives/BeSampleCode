// SimpleMidiFilter.cpp
// --------------------
// Implements the SimpleMidiFilter class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Debug.h>
#include <MidiProducer.h>
#include <MidiConsumer.h>
#include <MediaDefs.h>

#include "SimpleMidiFilter.h"
#include "SimpleMidiFilterConsumer.h"

SimpleMidiFilter::SimpleMidiFilter(const char* name, filter_result defaultAction)
	: m_name(name), m_defaultAction(defaultAction)
{
	BString endpointName;
	endpointName << m_name << " input";
	m_input = new SimpleMidiFilterConsumer(endpointName.String(), this);
	endpointName.SetTo("");
	endpointName << m_name << " output";
	m_output = new BMidiLocalProducer(endpointName.String());
}

SimpleMidiFilter::~SimpleMidiFilter()
{
	if (m_input) {
		m_input->Release();
		m_input = NULL;
	}
	if (m_output) {
		m_output->Release();
		m_output = NULL;
	}
}

BMidiLocalConsumer* SimpleMidiFilter::Input()
{
	return m_input;
}

BMidiLocalProducer* SimpleMidiFilter::Output()
{
	return m_output;
}

void SimpleMidiFilter::Register()
{
	if (m_input) m_input->Register();
	if (m_output) m_output->Register();
}

void SimpleMidiFilter::Unregister()
{
	if (m_input) m_input->Unregister();
	if (m_output) m_output->Unregister();
}

void SimpleMidiFilter::SprayData(void *data, size_t length, bool atomic, bigtime_t time) const
{ if (m_output) m_output->SprayData(data, length, atomic, time); }

void SimpleMidiFilter::SprayNoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time) const
{ if (m_output) m_output->SprayNoteOff(channel, note, velocity, time); }

void SimpleMidiFilter::SprayNoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time) const
{ if (m_output) m_output->SprayNoteOn(channel, note, velocity, time); }

void SimpleMidiFilter::SprayKeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time) const
{ if (m_output) m_output->SprayKeyPressure(channel, note, pressure, time); }

void SimpleMidiFilter::SprayControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time) const
{ if (m_output) m_output->SprayControlChange(channel, controlNumber, controlValue, time); }

void SimpleMidiFilter::SprayProgramChange(uchar channel, uchar programNumber, bigtime_t time) const
{ if (m_output) m_output->SprayProgramChange(channel, programNumber, time); }

void SimpleMidiFilter::SprayChannelPressure(uchar channel, uchar pressure, bigtime_t time) const
{ if (m_output) m_output->SprayChannelPressure(channel, pressure, time); }

void SimpleMidiFilter::SprayPitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time) const
{ if (m_output) m_output->SprayPitchBend(channel, lsb, msb, time); }

void SimpleMidiFilter::SpraySystemExclusive(void* data, size_t dataLength, bigtime_t time) const
{ if (m_output) m_output->SpraySystemExclusive(data, dataLength, time); }

void SimpleMidiFilter::SpraySystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time) const
{ if (m_output) m_output->SpraySystemCommon(statusByte, data1, data2, time); }

void SimpleMidiFilter::SpraySystemRealTime(uchar statusByte, bigtime_t time) const
{ if (m_output) m_output->SpraySystemRealTime(statusByte, time); }

void SimpleMidiFilter::SprayTempoChange(int32 bpm, bigtime_t time) const	
{ if (m_output) m_output->SprayTempoChange(bpm, time); }

filter_result SimpleMidiFilter::NoteOff(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::NoteOn(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::KeyPressure(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::ControlChange(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::ProgramChange(uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::ChannelPressure(uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::PitchBend(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::SystemExclusive(void*, size_t, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::SystemCommon(uchar, uchar, uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::SystemRealTime(uchar, bigtime_t)
{ return m_defaultAction; }

filter_result SimpleMidiFilter::TempoChange(int32, bigtime_t)
{ return m_defaultAction; }
