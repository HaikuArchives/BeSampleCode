/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _doodle_h
#define _doodle_h

/////////////////////////////////////////////////////////////////////////////
// Class: DudeApp
// --------------
// The main application class.
//
// MFC NOTE: Comparable to CWinApp. Also handles some of the stuff that an
// frame window would in MFC, i.e. cascade/tiled window arrangement.

#include <Application.h>
#include <FilePanel.h>

class DudePrefs;

class DudeApp : public BApplication
{
// construction, destruction, operators
public:
	DudeApp();
	virtual ~DudeApp();
	
// overrides
public:
	virtual void AboutRequested();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);
	virtual void Pulse();
	virtual bool QuitRequested();
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage* message);

// operations
public:
	void AddMRUMenuItems(BMenu* menu);
	
// message handlers
private:
	void OnNew();
	void OnOpen();
	void OnCascade();
	void OnTile();
	void AddToMRUList(BMessage* message);

// implementation
private:
	bool QuitDudeWinLoop();
	void CloseAllWindows();
	void Open(const entry_ref* ref);
	
// data members
private:
	BFilePanel*	m_pOpenPanel;
	DudePrefs*	m_pPrefs;
};

#endif /* _doodle_h */
