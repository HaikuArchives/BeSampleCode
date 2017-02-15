// SimpleMidiFilterConsumer.cpp
// ----------------------------
// Implements the SimpleMidiFilterConsumer class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Debug.h>
#include "SimpleMidiFilterConsumer.h"
#include "SimpleMidiFilter.h"

SimpleMidiFilterConsumer::SimpleMidiFilterConsumer(const char* name, SimpleMidiFilter* owner)
	: BMidiLocalConsumer(name), m_owner(owner)
{
	ASSERT(m_owner);
}

void SimpleMidiFilterConsumer::NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->NoteOff(channel, note, velocity, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayNoteOff(channel, note, velocity, time);
		}
	}
}

void SimpleMidiFilterConsumer::NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->NoteOn(channel, note, velocity, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayNoteOn(channel, note, velocity, time);
		}
	}
}

void SimpleMidiFilterConsumer::KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->KeyPressure(channel, note, pressure, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayKeyPressure(channel, note, pressure, time);
		}
	}
}

void SimpleMidiFilterConsumer::ControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->ControlChange(channel, controlNumber, controlValue, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayControlChange(channel, controlNumber, controlValue, time);
		}
	}
}

void SimpleMidiFilterConsumer::ProgramChange(uchar channel, uchar programNumber, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->ProgramChange(channel, programNumber, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayProgramChange(channel, programNumber, time);
		}
	}
}

void SimpleMidiFilterConsumer::ChannelPressure(uchar channel, uchar pressure, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->ChannelPressure(channel, pressure, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayChannelPressure(channel, pressure, time);
		}
	}
}

void SimpleMidiFilterConsumer::PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->PitchBend(channel, lsb, msb, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayPitchBend(channel, lsb, msb, time);
		}
	}
}

void SimpleMidiFilterConsumer::SystemExclusive(void* data, size_t dataLength, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->SystemExclusive(data, dataLength, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SpraySystemExclusive(data, dataLength, time);
		}
	}
}

void SimpleMidiFilterConsumer::SystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->SystemCommon(statusByte, data1, data2, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SpraySystemCommon(statusByte, data1, data2, time);
		}
	}
}

void SimpleMidiFilterConsumer::SystemRealTime(uchar statusByte, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->SystemRealTime(statusByte, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SpraySystemRealTime(statusByte, time);
		}
	}
}

void SimpleMidiFilterConsumer::TempoChange(int32 bpm, bigtime_t time)
{
	if (m_owner) {
		filter_result result = m_owner->TempoChange(bpm, time);
		if (result == B_DISPATCH_MESSAGE) {
			m_owner->SprayTempoChange(bpm, time);
		}
	}
}
