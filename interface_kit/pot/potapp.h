/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _potapp_h
#define _potapp_h

#include <Application.h>

class PotApp : public BApplication
{
public:
	PotApp();

public:
	virtual void AboutRequested();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage* message);

private:
	void OnOpen();
	bool QuitDudeWinLoop();
	void CloseAllWindows();
	void Open(const entry_ref* ref);
		
private:
	BFilePanel*	m_pOpenPanel;
};

#endif /* _potapp_h */
