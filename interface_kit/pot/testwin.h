/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _testwin_h
#define _testwin_h

#include <Window.h>
class TestView;

class TestWin : public BWindow
{
public:
	static status_t NewWindow(const entry_ref* ref);
	static int32 CountWindows();

	TestWin(const entry_ref* ref, BBitmap* pBitmap);
	virtual ~TestWin();
	
	virtual void WindowActivated(bool active);
	
	status_t InitCheck();
	TestView* GetTestView() const { return m_pv; }
	void SetRef(const entry_ref* ref);
	void UpdateTitle();
	void LoadMenus(BMenuBar* pBar);
	
private:
	entry_ref* m_pRef;
	TestView* m_pv;
	
	static BLocker s_winListLocker;
	static BList s_winList;
};

#endif /* _testwin_h */