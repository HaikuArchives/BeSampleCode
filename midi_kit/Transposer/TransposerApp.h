// TransposerApp.h
// ---------------
// The Transposer application class. It is in charge of maintaining
// the Transposer and other MIDI objects.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _TransposerApp_h
#define _TransposerApp_h

#include <Application.h>
#include <String.h>

class BMidiConsumer;
class BMidiProducer;
class Transposer;
class TransposerWin;
class ChannelMaskWin;

extern const uint32 MSG_CHANGE_TRANSPOSE;
extern const uint32 MSG_EDIT_CHANNEL_MASK;
extern const uint32 MSG_APPLY_CHANNEL_MASK;
extern const uint32 MSG_WINDOW_CLOSED;

class TransposerApp : public BApplication {
public:
	TransposerApp();
	void ArgvReceived(int32 argc, char** argv);
	void ReadyToRun();
	void MessageReceived(BMessage* message);
	bool QuitRequested();
	void Quit();	
	void EnableChannelRange(const char* rangeSpec, bool disable);
	void GetInitialTranspose(const char* arg);
	
private:
	void PrintUsage();
	
	BMidiConsumer* m_consumer;
	BMidiProducer* m_producer;
	Transposer* m_transposer;
	TransposerWin* m_transposerWin;
	ChannelMaskWin* m_channelMaskWin;
	BString m_name;
	int8 m_initialTranspose;
	uint16 m_channelMask;
	bool m_quit;
};

#endif /* _TransposerApp_h */
