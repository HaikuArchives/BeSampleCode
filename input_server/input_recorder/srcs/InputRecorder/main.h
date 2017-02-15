/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __TAPP_H
#define __TAPP_H 1

#define PORT_NAME "InputRecorderPort"

#define T_SET_INPUT_LOCK 'tSIL'
#define T_START_INPUT 'tSTI'
#define T_TOGGLE_LOOP 'tLOP'
#define T_TOGGLE_LOCK 'tLCK'
#define T_RECORD 'tREC'
#define T_PLAY 'tPLY'
#define T_STOP 'tSTP'
#define T_SET_BUTTONS 'tSBT'

#define T_OPEN 'tOPN'
#define T_SAVE 'tSAV'
#define T_SAVEAS 'tSAA'

#define FILTER_ADD 'Fadd'
#define FILTER_REMOVE 'Frem'
#define FILTER_PASSMODE 'Fpas'
#define FILTER_INPUT 'Finp'

#include <Application.h>
#include <FilePanel.h>

class TApp : public BApplication
{
	public:
		TApp();	
		virtual void ArgvReceived(int32 argc, char **argv);
		virtual void RefsReceived(BMessage *msg);
		virtual void MessageReceived(BMessage *msg);
	private:
		BFilePanel *_Load;
		BFilePanel *_Save;
};

#endif