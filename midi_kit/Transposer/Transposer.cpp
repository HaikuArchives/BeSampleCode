// Transposer.cpp
// --------------
// Implements the Transposer class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Debug.h>
#include <File.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <Roster.h>

#include "Transposer.h"

// Icons based on the Tracker icons for the app.
// These are stored in properties of the input and
// output endpoints. I'd like to see them become
// standards.
const char* LARGE_ICON_NAME = "be:large_icon";
const char* MINI_ICON_NAME = "be:mini_icon";
const uint32 LARGE_ICON_TYPE = 'ICON';
const uint32 MINI_ICON_TYPE = 'MICN';

Transposer::Transposer(const char* name, int8 transpose, uint16 channelMask)
	: SimpleMidiFilter(name), m_transpose(transpose), m_channelMask(channelMask)
{
	BBitmap* largeIcon = new BBitmap(BRect(0,0,31,31), B_CMAP8);
	BBitmap* miniIcon = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	GetIcons(largeIcon, miniIcon);
	
	BMessage msg;
	BMidiEndpoint* input = Input();
	if (input && input->GetProperties(&msg) == B_OK)
	{
		AddIcons(&msg, largeIcon, miniIcon);
		input->SetProperties(&msg);
	} 

	BMidiEndpoint* output = Output();
	if (output && output->GetProperties(&msg) == B_OK)
	{
		AddIcons(&msg, largeIcon, miniIcon);
		output->SetProperties(&msg);
	} 	
}

void Transposer::AddIcons(BMessage* msg, BBitmap* largeIcon, BBitmap* miniIcon) const
{
	if (! msg->HasData(LARGE_ICON_NAME, LARGE_ICON_TYPE)) {
		msg->AddData(LARGE_ICON_NAME, LARGE_ICON_TYPE, largeIcon->Bits(),
			largeIcon->BitsLength());
	} else {
		msg->ReplaceData(LARGE_ICON_NAME, LARGE_ICON_TYPE, largeIcon->Bits(),
			largeIcon->BitsLength());
	}
	if (! msg->HasData(MINI_ICON_NAME, MINI_ICON_TYPE)) {
		msg->AddData(MINI_ICON_NAME, MINI_ICON_TYPE, miniIcon->Bits(),
			miniIcon->BitsLength());
	} else {
		msg->ReplaceData(MINI_ICON_NAME, MINI_ICON_TYPE, miniIcon->Bits(),
			miniIcon->BitsLength());
	}
}

int8 Transposer::Transpose() const
{
	return m_transpose;
}

void Transposer::SetTranspose(int8 transpose)
{
	if (m_transpose != transpose) {
		m_transpose = transpose;
		// Terminate all currently playing notes on all
		// channels on which transposition is active, else
		// they are likely to hang (the closing NoteOff
		// may be transposed to a different note value
		// than the corresponding NoteOn).
		for (uint8 ch=0; ch<16; ch++) {
			if (m_channelMask & (0x1 << ch)) {
				SprayControlChange(ch, B_ALL_NOTES_OFF, 0, 0);
			}
		}
	}
}

uint16 Transposer::ChannelMask() const
{
	return m_channelMask;
}

void Transposer::SetChannelMask(uint16 channelMask)
{
	if (m_channelMask != channelMask
		&& m_transpose != 0)
	{
		// Terminate all currently playing notes on all
		// channels that have changed status, since that's
		// like applying a new transpose on that channel.
		uint16 diff = m_channelMask ^ channelMask;
		for (uint8 ch=0; ch<16; ch++) {
			if (diff & (0x1 << ch)) {
				// this channel has changed
				SprayControlChange(ch, B_ALL_NOTES_OFF, 0, 0);
			}
		}
		m_channelMask = channelMask;
	}
}

uchar Transposer::TransposeNote(uchar note)
{
	int16 newNote = note + m_transpose;
	if (newNote < 0) newNote = 0;
	if (newNote > 127) newNote = 127;
	return (uchar) newNote;
}

filter_result Transposer::NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	if (m_channelMask & (0x1 << channel)) {
		SprayNoteOff(channel, TransposeNote(note), velocity, time);
		return B_SKIP_MESSAGE;
	} else {
		return B_DISPATCH_MESSAGE;
	}
}

filter_result Transposer::NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	if (m_channelMask & (0x1 << channel)) {
		SprayNoteOn(channel, TransposeNote(note), velocity, time);
		return B_SKIP_MESSAGE;
	} else {
		return B_DISPATCH_MESSAGE;
	}
}

filter_result Transposer::KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	if (m_channelMask & (0x1 << channel)) {
		SprayKeyPressure(channel, TransposeNote(note), pressure, time);
		return B_SKIP_MESSAGE;
	} else {
		return B_DISPATCH_MESSAGE;
	}
}

void Transposer::GetIcons(BBitmap* largeIcon, BBitmap* miniIcon)
{
	app_info ai;
	status_t err;
	
	// get the icons from the app file info of the
	// currently running app
	err = be_app->GetAppInfo(&ai);
	if (err == B_OK) {
		BFile file(&(ai.ref), B_READ_ONLY);
		err = file.InitCheck();
		if (err == B_OK) {
			BAppFileInfo afi(&file);
			err = afi.InitCheck();
			if (err == B_OK) {
				afi.GetIcon(largeIcon, B_LARGE_ICON);
				afi.GetIcon(miniIcon, B_MINI_ICON);
			}
		}
	}
}
