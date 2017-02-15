/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <algobase.h>
#include <Application.h>
#include <Bitmap.h>
#include <Entry.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <TranslationUtils.h>
#include <Locker.h>

#include "constants.h"
#include "potview.h"
#include "potwin.h"
#include "testview.h"
#include "testwin.h"

BLocker TestWin::s_winListLocker("testwin list lock");
BList TestWin::s_winList;

status_t TestWin::NewWindow(const entry_ref* ref)
{
	BEntry entry(ref);
	if (entry.InitCheck() == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() == B_OK) {
			BBitmap* pBitmap = BTranslationUtils::GetBitmap(path.Path());
			if (pBitmap) {
				TestWin* pWin = new TestWin(ref, pBitmap);
				return pWin->InitCheck();			
			}
		}
	}
	return B_ERROR;		
}

int32 TestWin::CountWindows()
{
	int32 count = -1;
	if (s_winListLocker.Lock()) {
		count = s_winList.CountItems();
		s_winListLocker.Unlock();
	}
	return count;
}

TestWin::TestWin(const entry_ref* ref, BBitmap* pBitmap)
	: BWindow(BRect(50, 50, 250, 250), "", B_DOCUMENT_WINDOW, 0),
	m_pRef(0)
{
	// create menu bar	
	BMenuBar* pBar = new BMenuBar(BRect(0,0,0,0), "menu bar");
	LoadMenus(pBar);
	AddChild(pBar);

	BRect viewFrame = Bounds();
	viewFrame.top += pBar->Frame().Height() + 1;
	viewFrame.right -= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	// create the data view	
	m_pv = new TestView(viewFrame, "Test view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS);
	m_pv->SetBitmap(pBitmap);
	
	// wrap a scroll view (containing scroll bars) around the
	// view
	BScrollView* pScrollView = new BScrollView("Test scroller",
		m_pv, B_FOLLOW_ALL, 0, true, true);
			
	AddChild(pScrollView);
	
	// set the window's min & max size limits
	// based on document's data bounds
	float maxWidth = pBitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
	float maxHeight = pBitmap->Bounds().Height()
		+ pBar->Frame().Height()
		+ B_H_SCROLL_BAR_HEIGHT + 1;
	float minWidth = min(maxWidth, 100.0f);
	float minHeight = min(maxHeight, 100.0f);

	// adjust the window's current size based on new min/max values	
	float curWidth = Bounds().Width();
	float curHeight = Bounds().Height();	
	if (curWidth < minWidth) {
		curWidth = minWidth;
	} else if (curWidth > maxWidth) {
		curWidth = maxWidth;
	}
	if (curHeight < minHeight) {
		curHeight = minHeight;
	} else if (curHeight > maxHeight) {
		curHeight = maxHeight;
	}
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	ResizeTo(curWidth, curHeight);
	
	// finish creating window
	SetRef(ref);
	UpdateTitle();
	if (s_winListLocker.Lock()) {
		s_winList.AddItem(this);
		s_winListLocker.Unlock();
	}
	
	Show();
}

TestWin::~TestWin()
{
	if (m_pRef) {
		delete m_pRef;
	}
	
	if (s_winListLocker.Lock()) {
		s_winList.RemoveItem(this);
		s_winListLocker.Unlock();
	}
	if (CountWindows() < 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

void TestWin::WindowActivated(bool active)
{
	BMessage msg(MSG_CHANGE_FOCUS);
	if (active) {	
		msg.AddPointer("new_target", this);
		rgb_color color = m_pv->Color();
		msg.AddInt32("new_color", *((int32*) &color));
		PotWin* pWin = PotWin::Instance();
		pWin->PostMessage(&msg, pWin->GetPotView());
	}
}

status_t TestWin::InitCheck()
{
	if (! m_pRef) {
		return B_ERROR;
	} else {
		return B_OK;
	}
}

void TestWin::SetRef(const entry_ref* ref)
{
	if (! m_pRef) {
		m_pRef = new entry_ref(*ref);
	} else {
		*m_pRef = *ref;
	}
}

void TestWin::UpdateTitle()
{
	BEntry entry(m_pRef);
	if (entry.InitCheck() == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() == B_OK) {
			SetTitle(path.Path());
		}
	}		
}

void TestWin::LoadMenus(BMenuBar* pBar)
{
	BMenuItem* pItem;
	BMenu* pMenu = new BMenu("File");
	pItem = new BMenuItem("Open...", new BMessage(MSG_FILE_OPEN), 'O');
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	pItem = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W');
	pMenu->AddItem(pItem);
	pMenu->AddSeparatorItem();
	pItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	pBar->AddItem(pMenu);
	
	pMenu = new BMenu("Help");
	pItem = new BMenuItem("About PotApp...", new BMessage(B_ABOUT_REQUESTED));
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	pBar->AddItem(pMenu);
}
