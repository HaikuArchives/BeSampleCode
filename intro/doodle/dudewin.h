/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _dudewin_h
#define _dudewin_h

/////////////////////////////////////////////////////////////////////////////
// Class: DudeWin
// --------------
// A window which is associated with a DudeDoc.
//
// MFC NOTE: This class is directly analagous to CWnd: notice, however,
// that each BWindow runs in its own thread. Also notice that this class
// contains both frame and child window behavior.

#include <Window.h>

class DudeDoc;
class DudeView;

class DudeWin : public BWindow
{
// construction, destruction, operators
public:
	DudeWin();

// overrides
public:
	virtual void 		MessageReceived(BMessage* message);	
	virtual bool 		QuitRequested();
	virtual void		Quit();
	virtual void		MenusBeginning();
	virtual void		WindowActivated(bool active);

// accessors
public:
	BMenuBar*			MenuBar();
	DudeView*			View();
	DudeDoc*			Document();

// operations
public:
	status_t			Init(DudeDoc* pDoc);
	void				MoveToCascadePosition(bool resetToStart = false);
	status_t			UpdateUI();

// message handlers
private:
	void				UpdateTitle();
	void				NewWindow();
		
// implementation
private:
	void				LoadMenus();
	void				LoadFileMenu(BMenu* menu);
	
// data members
private:
	DudeView*			m_pDudeView;
	DudeDoc*			m_pDocument;
	BMenuBar*			m_pMenuBar;	
};

#endif /* _dudewin_h */