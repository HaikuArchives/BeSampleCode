/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _cmdtool_h
#define _cmdtool_h

/////////////////////////////////////////////////////////////////////////////
// Class: CmdToolbar
// -----------------
// The command toolbar.
//
// MFC NOTE: In the MFC library, toolbars are associated with a frame
// window. However, there is no concept of frame windows in BeOS. Instead
// of forcing the application to manage the toolbar, it's more convenient
// to make this toolbar a global singleton object.

#include <Locker.h>
#include "toolbar.h"

class CmdToolbar : public Toolbar
{
// static methods
public:
	static void Create();
	static CmdToolbar* GetToolbar();

// construction, destruction, operators	
public:
	CmdToolbar();
	~CmdToolbar();

// overrides
public:
	virtual void Adopt(BWindow* pWin);
	
// implementation
private:
	void InitObject();

// static data
private:
	static CmdToolbar* _toolbar;
	static BLocker	_toolLock;
};

#endif /* _cmdtool_h */
