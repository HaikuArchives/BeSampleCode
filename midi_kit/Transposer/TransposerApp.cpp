// TransposerApp.cpp
// -----------------
// Implements the Transposer application class and main().
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <Debug.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <Slider.h>

#include "ChannelMaskWin.h"
#include "MidiUtil.h"
#include "TransposerApp.h"
#include "TransposerWin.h"
#include "Transposer.h"

extern const uint32 MSG_CHANGE_TRANSPOSE = 'Tpos';
extern const uint32 MSG_EDIT_CHANNEL_MASK = 'Cmsk';
extern const uint32 MSG_APPLY_CHANNEL_MASK = 'Amsk';
extern const uint32 MSG_WINDOW_CLOSED = 'Wdie';

TransposerApp::TransposerApp()
	: BApplication("application/x-vnd.Be-DTS.Transposer"),
	m_consumer(NULL), m_producer(NULL), m_transposer(NULL),
	m_channelMaskWin(NULL)
{
	m_name = "Transposer";
	m_initialTranspose = 0;
	m_channelMask = 0xFFFF;
	m_quit = false;
}

void TransposerApp::PrintUsage()
{
	fprintf(stderr, "usage: Transposer [--help] [--name <name>] [--initial <transpose_value>]\n"
					"       [--enable <channel_range> | --disable <channel_range>] [<producer> [<consumer>]]\n");
}

void TransposerApp::ArgvReceived(int32 argc, char** argv)
{
	bool prodArg = false;
	bool consArg = false;
	for (int32 i=1; i<argc; i++) {
		const char* arg = argv[i];
		if (! strcmp(arg, "--help")) {
			PrintUsage();
			PostMessage(B_QUIT_REQUESTED);
			m_quit = true;
			break;
		} else if (! strcmp(arg, "--name")) {
			i++;
			if (i >= argc) {
				PrintUsage();
				break;
			}
			m_name = argv[i];
		} else if (! strcmp(arg, "--initial")) {
			i++;
			if (i >= argc) {
				PrintUsage();
				break;
			}
			GetInitialTranspose(argv[i]);
		} else if (! strcmp(arg, "--enable")) {
			i++;
			if (i >= argc) {
				PrintUsage();
				break;
			}
			EnableChannelRange(argv[i], false);
		} else if (! strcmp(arg, "--disable")) {
			i++;
			if (i >= argc) {
				PrintUsage();
				break;
			}
			EnableChannelRange(argv[i], true);
		} else if (! prodArg) {
			prodArg = true;
			m_producer = MidiUtil::FindProducer(arg);
			if (! m_producer) {
				fprintf(stderr, "Couldn't find a producer named \"%s\"\n", arg);
			} 
		} else if (! consArg) {
			consArg = true;
			m_consumer = MidiUtil::FindConsumer(arg);
			if (! m_consumer) {
				fprintf(stderr, "Couldn't find a consumer named \"%s\"\n", arg);
			}
		} else {
			PrintUsage();
			break;
		}
	}
}

void TransposerApp::GetInitialTranspose(const char* arg)
{
	int16 init;
	if (sscanf(arg, "%hd", &init)) {
		if (init > 36) init = 36;
		if (init <= -36) init = -36;
		m_initialTranspose = (int8) init;
	}
}

// Enables or disables a range of channels specified like: "1,3-9,11-16".
void TransposerApp::EnableChannelRange(const char* rangeSpec, bool disable)
{
	// start by assuming that we're enabling, so set all bits to 0
	uint16 mask = 0x0000;
	BString left(rangeSpec);
	while (left.Length())
	{
		// find next chunk, separated by commas
		int32 mark = left.FindFirst(',');
		if (mark == -1) mark = left.Length();

		// split chunk into separate string
		BString chunk;
		left.MoveInto(chunk, 0, mark);
		left.Remove(0, 1); // chomps the comma
		
		// look for pattern of the form "M-N"
		unsigned short from, to;
		int numRead = sscanf(chunk.String(), " %hu - %hu", &from, &to);
		if (numRead >= 1) {
			if (numRead == 1) to = from; // just one channel

			// clip range to channels 1-16
			if (from < 1) from = 1;
			if (to < 1) to = 1;
			if (from > 16) from = 16;
			if (to > 16) to = 16;

			// set the bits (0-based index)
			for (unsigned short ch = from-1; ch <= to-1; ch++) {
				mask |= (0x1 << ch);
			}
		}
	}
	
	// if disable is true, enabled channels should be disabled and vice-versa
	m_channelMask = (disable) ? ~mask : mask;
}

void TransposerApp::ReadyToRun()
{
	if (! m_quit) {
		m_transposer = new Transposer(m_name.String(), m_initialTranspose, m_channelMask);
		m_transposer->Register();
		
		status_t err;
		if (m_producer && m_transposer->Input()) {
			err = m_producer->Connect(m_transposer->Input());
			if (err != B_OK) {
				fprintf(stderr, "Couldn't connect input: %s\n", strerror(err));
			}
		}
		if (m_consumer && m_transposer->Output()) {
			err = m_transposer->Output()->Connect(m_consumer);
			if (err != B_OK) {
				fprintf(stderr, "Couldn't connect output: %s\n", strerror(err));
			}
		}
		
		m_channelMaskWin = new ChannelMaskWin(m_channelMask);
		m_transposerWin = new TransposerWin(m_name.String(), m_initialTranspose);
		m_transposerWin->Show();
	}
}

bool TransposerApp::QuitRequested()
{
	bool quit = false;
	if (m_transposerWin) {
		if (m_transposerWin->Lock()) {
			quit = m_transposerWin->QuitRequested();
			m_transposerWin->Unlock();
		} else {
			// transposer window is already gone or hosed
			quit = true;
		}
	} else {
		quit = true;
	}
	return quit;
}

void TransposerApp::Quit()
{
	delete m_transposer;

	// application cleans these up for us
	m_channelMaskWin = NULL;
	m_transposerWin = NULL;
	
	// decrement refcounts of objects we're connected to
	if (m_consumer) {
		m_consumer->Release();
		m_consumer = NULL;
	}
	if (m_producer) {
		m_producer->Release();
		m_producer = NULL;
	}
	
	BApplication::Quit();
}

void TransposerApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_CHANGE_TRANSPOSE:
		{
			BSlider* slider;
			if (message->FindPointer("source", (void**) &slider) == B_OK) {
				int8 val = (int8) slider->Value();
				if (m_transposer) m_transposer->SetTranspose(val);
			}
		}
		break;
	case MSG_EDIT_CHANNEL_MASK:
		if (m_channelMaskWin && m_channelMaskWin->Lock()) {
			if (m_channelMaskWin->IsHidden()) {
				m_channelMaskWin->Show();
			} else {
				m_channelMaskWin->Activate();
			}
			m_channelMaskWin->Unlock();
		}
		break;
	case MSG_APPLY_CHANNEL_MASK:
		{
			uint16 mask;
			if (m_transposer
				&& message->FindInt16("mask", (int16*) &mask) == B_OK)
			{
				m_transposer->SetChannelMask(mask);
			}
		}
		break;
	case MSG_WINDOW_CLOSED:
		m_transposerWin = NULL;
		PostMessage(B_QUIT_REQUESTED);
		break;
	default:
		BApplication::MessageReceived(message);
		break;
	}	
}

int main()
{
	TransposerApp pApp;
	pApp.Run();
	return 0;
}
