/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "algobase.h"
#include <stdio.h>
#include <string.h>
#include <Autolock.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SupportDefs.h>
#include "cmdtool.h"
#include "constants.h"
#include "doodle.h"
#include "dudedoc.h"
#include "dudeview.h"
#include "dudewin.h"

/////////////////////////////////////////////////////////////////////////////
// static variables

// windowDefaultRect: the default size of a window
static const BRect windowDefaultRect(0, 0, 400, 400);



/////////////////////////////////////////////////////////////////////////////
// DudeWin construction, destruction, operators

DudeWin::DudeWin()
	: BWindow(windowDefaultRect, "", B_DOCUMENT_WINDOW, 0),
	m_pDudeView(0), m_pDocument(0)
{
	// default placement
	MoveToCascadePosition();	
}



/////////////////////////////////////////////////////////////////////////////
// DudeWin overrides

// DudeWin::MessageReceived
// ------------------------
// The message dispatch routine for the window.
//
// MFC NOTE: Analagous to MFC's message map (but without the macros to
// hide the details, or VC++ to automatically generate the maps).
void DudeWin::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_DOC_TITLE_CHANGED:
		UpdateTitle();
		break;
	case MSG_WIN_NEW_WINDOW:
		NewWindow();
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

// DudeWin::QuitRequested
// ----------------------
// Checks to see if it's okay to close the window.
// Returns true if it is.
bool DudeWin::QuitRequested()
{
	if (m_pDocument && (m_pDocument->CountWindows() <= 1))
		// last window open for this document;
		// better get the document's permission
		return m_pDocument->QuitRequested();
	else
		return true;
}

// DudeWin::Quit
// -------------
// Quits the window.
void DudeWin::Quit()
{
	CmdToolbar* pTB = CmdToolbar::GetToolbar();
	if (pTB) {
		BAutolock tbLock(pTB);
		if (tbLock.IsLocked()) {
			// notify the toolbar that we're history
			pTB->Orphan(this);
		}
	}
	
	if (m_pDocument) {
		// notify the document that we're history
		m_pDocument->RemoveWindow(this);
	}
	BWindow::Quit();
}

// DudeWin::MenusBeginning
// -----------------------
// Some menu associated with this window is about to be displayed.
// This is our last chance to update the menus before it does.
void DudeWin::MenusBeginning()
{
	// reload the file menu from scratch
	BMenuItem* pItem = m_pMenuBar->FindItem("File");
	if (! pItem)
		return;
	
	BMenu* pMenu = pItem->Submenu();
	if (! pMenu)
		return;
	
	int32 i = pMenu->CountItems();
	while (--i >= 0)
		delete pMenu->RemoveItem(i);
	
	LoadFileMenu(pMenu);
}

// DudeWin::WindowActivated
// ------------------------
// Invoked when our window is being activated (active=true)
// or deactivated (active=false).
void DudeWin::WindowActivated(bool active)
{
	CmdToolbar* ptb = CmdToolbar::GetToolbar();
	if (! ptb)
		return;

	BAutolock tbLock(ptb);
	if (! tbLock.IsLocked())
		return;
				
	if (active) {
		// we're active; tell the toolbar to point at us
		ptb->Adopt(this);
	}
}



/////////////////////////////////////////////////////////////////////////////
// DudeWin accessors

BMenuBar* DudeWin::MenuBar()
{
	return m_pMenuBar;
}

DudeView* DudeWin::View()
{
	return m_pDudeView;
}

DudeDoc* DudeWin::Document()
{
	return m_pDocument;
}



/////////////////////////////////////////////////////////////////////////////
// DudeWin operations

// DudeWin::Init
// -------------
// Sets up the window to point to the specified document.
// Returns B_OK if everything was successful.
status_t DudeWin::Init(DudeDoc* pDoc)
{
	status_t res;
	
	// set up document pointer
	if (! pDoc)
		return B_BAD_VALUE;
	m_pDocument = pDoc;
	
	// create menu bar	
	m_pMenuBar = new BMenuBar(BRect(0,0,0,0), "menu bar");
	LoadMenus();
	AddChild(m_pMenuBar);

	// figure out available area for data view
	BRect viewFrame = Bounds();
	// take menu bar height into account
	viewFrame.top += m_pMenuBar->Frame().Height() + 1;
	// subtract scroll bar sizes from available area
	viewFrame.right -= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	// create the data view	
	m_pDudeView = new DudeView(viewFrame, "Doodle view");
	if ((res = m_pDudeView->Init(pDoc)) != B_OK) {
		delete m_pDudeView;
		m_pDudeView = 0;
		m_pDocument = 0;
		return res;
	}
	
	// wrap a scroll view (containing scroll bars) around the
	// view
	BScrollView* pScrollView = new BScrollView("Doodle scroller",
		m_pDudeView, B_FOLLOW_ALL, 0, true, true);
			
	AddChild(pScrollView);
	
	// set the window's min & max size limits
	// based on document's data bounds
	float maxWidth = m_pDocument->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
	float maxHeight = m_pDocument->Bounds().Height()
		+ m_pMenuBar->Frame().Height()
		+ B_H_SCROLL_BAR_HEIGHT + 1;
	float minWidth = min_c(maxWidth, 100.0f);
	float minHeight = min_c(maxHeight, 100.0f);

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
	m_pDocument->AddWindow(this);
	UpdateTitle();
	
	return B_OK;
}

// DudeWin::MoveToCascadePosition
// ------------------------------
// Moves the window to the next available cascade position.
// If resetToStart == true, puts the window at left top and
// resets the cascade position.
void DudeWin::MoveToCascadePosition(bool resetToStart)
{
	// cascadeDelta: default delta between windows in a cascade arrangement
	const BPoint cascadeDelta(20, 20);
	// startPt: the left top point for cascades
	const BPoint startPt(10, 30);
	// curPt: the next available point to place a cascade window
	static BPoint curPt(startPt);
	// bOverflow: true iff the window won't fit onscreen
	bool bOverflow = false;

	if (resetToStart)
		curPt = startPt;
			
	BRect wRect = Frame();
	BScreen screen(this);
	BRect sRect = screen.Frame();

	if ((wRect.Width() + curPt.x > sRect.right)
		|| (wRect.Height() + curPt.y > sRect.bottom))
	{
		// window doesn't fit
		bOverflow = true;
	}
	if (bOverflow) {
		// if window doesn't fit, start back at top
		curPt = startPt;
	}
	
	MoveTo(curPt);
	curPt.x += cascadeDelta.x;
	curPt.y += cascadeDelta.y;
}

// DudeWin::UpdateUI
// -----------------
// Updates various elements of the UI to be in sync
// with the document.
status_t DudeWin::UpdateUI()
{
	if (m_pDocument) {
		m_pDocument->UpdateMenus(m_pMenuBar);
		return B_OK;
	} else {
		return B_ERROR;
	}
}



/////////////////////////////////////////////////////////////////////////////
// DudeWin message handlers

// DudeWin::NewWindow
// ------------------
// Creates a new window that points to the same document.
void DudeWin::NewWindow()
{
	if (! (m_pDocument && m_pDocument->WriteLock()))
		return;
		
	DudeWin* newWin = new DudeWin;
	if (newWin->Init(m_pDocument) != B_OK) {
		delete newWin;
		m_pDocument->WriteUnlock();
		return;
	}
	
	if (m_pDocument->CountWindows() == 2) {
		// Special case: if we've just spawned the second
		// document window, we have to switch this doc window
		// to the new title scheme.
		PostMessage(MSG_DOC_TITLE_CHANGED);
	}
	
	m_pDocument->WriteUnlock();
	newWin->Show();
}

// DudeWin::UpdateTitle
// --------------------
// Responds to a notification that the document's
// state may have changed. Update the title bar.
void DudeWin::UpdateTitle()
{
	if (! m_pDocument) {
		SetTitle("");
		return;
	}
	
	if (! m_pDocument->ReadLock())
		return;
		
	const char* name = m_pDocument->Title();
	if (! name) {
		SetTitle("");
		m_pDocument->ReadUnlock();
		return;
	}
	char* title = new char[strlen(name) + 50];
	strcpy(title, name);
	if (m_pDocument->CountWindows() > 1) {
		// more than 1 window; add a unique window identifier
		int32 index = m_pDocument->IndexOfWindow(this) + 1;
		char indexStr[20];
		sprintf(indexStr, " : %ld", index);
		strcat(title, indexStr);
	}
	
	if (m_pDocument->IsModified()) {
		// document is modified; add an asterisk
		strcat(title, " *");
	}
	m_pDocument->ReadUnlock();
	SetTitle(title);
}



/////////////////////////////////////////////////////////////////////////////
// DudeWin implementation

void DudeWin::LoadMenus()
{
	BMenu* pMenu = new BMenu("File");
	BMenuItem* pItem;
	
	LoadFileMenu(pMenu);
		
	m_pMenuBar->AddItem(pMenu);

	pMenu = new BMenu("Edit");
	pItem = new BMenuItem("Clear All", new BMessage(MSG_EDIT_CLEAR_ALL));
	pItem->SetTarget(m_pDocument);
	pMenu->AddItem(pItem);
	
	m_pMenuBar->AddItem(pMenu);
	
	pMenu = new BMenu("Pen");
	pItem = new BMenuItem("Thick Line", new BMessage(MSG_PEN_THICK_OR_THIN));
	pItem->SetTarget(m_pDocument);
	pMenu->AddItem(pItem);
	pItem = new BMenuItem("Pen Widths...", new BMessage(MSG_PEN_WIDTHS));
	pItem->SetTarget(m_pDocument);
	pMenu->AddItem(pItem);
	
	m_pMenuBar->AddItem(pMenu);
	
	pMenu = new BMenu("Window");
	pItem = new BMenuItem("New Window", new BMessage(MSG_WIN_NEW_WINDOW));
	pMenu->AddItem(pItem);
	pMenu->AddSeparatorItem();
	pItem = new BMenuItem("Cascade", new BMessage(MSG_WIN_CASCADE));
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	pItem = new BMenuItem("Tile", new BMessage(MSG_WIN_TILE));
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	
	m_pMenuBar->AddItem(pMenu);
	
	pMenu = new BMenu("Help");
	pItem = new BMenuItem("About Doodle...", new BMessage(B_ABOUT_REQUESTED));
	pItem->SetTarget(be_app);
	pMenu->AddItem(pItem);
	
	m_pMenuBar->AddItem(pMenu);
}

// DudeWin::LoadFileMenu
// ---------------------
// Loads the menu with File menu items.
void DudeWin::LoadFileMenu(BMenu* menu)
{
	BMenuItem* pItem = new BMenuItem("New", new BMessage(MSG_FILE_NEW),
		'N', B_COMMAND_KEY);
	pItem->SetTarget(be_app);
	menu->AddItem(pItem);
	pItem = new BMenuItem("Open...", new BMessage(MSG_FILE_OPEN),
		'O', B_COMMAND_KEY);
	pItem->SetTarget(be_app);
	menu->AddItem(pItem);
	pItem = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED),
		'W', B_COMMAND_KEY);
	menu->AddItem(pItem);
	pItem = new BMenuItem("Save", new BMessage(MSG_FILE_SAVE),
		'S', B_COMMAND_KEY);
	pItem->SetTarget(m_pDocument);
	menu->AddItem(pItem);
	pItem = new BMenuItem("Save As...", new BMessage(MSG_FILE_SAVE_AS));
	pItem->SetTarget(m_pDocument);
	menu->AddItem(pItem);
	
	menu->AddSeparatorItem();
	
	pItem = new BMenuItem("Print...", new BMessage(MSG_FILE_PRINT),
		'P', B_COMMAND_KEY);
	pItem->SetTarget(m_pDudeView);
	menu->AddItem(pItem);
	pItem = new BMenuItem("Page Setup...", new BMessage(MSG_FILE_PAGE_SETUP));
	pItem->SetTarget(m_pDudeView);
	menu->AddItem(pItem);
	
	DudeApp* app = dynamic_cast<DudeApp*>(be_app);
	BAutolock appLock(app);
	if (appLock.IsLocked())
		app->AddMRUMenuItems(menu);
	else 
		menu->AddSeparatorItem();
	
	pItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY);
	pItem->SetTarget(be_app);
	menu->AddItem(pItem);
}
