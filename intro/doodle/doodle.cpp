/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <Alert.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Screen.h>
#include "cmdtool.h"
#include "constants.h"
#include "doodle.h"
#include "dudedoc.h"
#include "dudeprefs.h"
#include "dudewin.h"

/////////////////////////////////////////////////////////////////////////////
// Main entry point
//
// MFC NOTE: Instead of declaring a global application object, we define
// a main() that instantiates and runs the (one and only) application object.
// Note that we could handle command-line arguments here, but it's much more
// convenient to do so in ArgvReceived -- especially since additional
// command-line arguments might be passed in later on during runtime.
int main(int, char**)
{
	DudeApp theApp;
	theApp.Run();
	return 0;
}



/////////////////////////////////////////////////////////////////////////////
// DudeApp construction, destruction, operators

DudeApp::DudeApp()
	: BApplication("application/x-vnd.Be-DTS.Doodle"), m_pOpenPanel(0)
{
	m_pPrefs = DudePrefs::Load();
	if (! m_pPrefs)
		m_pPrefs = new DudePrefs;
}

DudeApp::~DudeApp()
{
	m_pPrefs->Save();
	delete m_pPrefs;
}



/////////////////////////////////////////////////////////////////////////////
// DudeApp overrides

// DudeApp::AboutRequested
// -----------------------
// MFC NOTE: Unlike MFC, an overridable hook to B_ABOUT_REQUESTED is
// already provided by the BApplication class.
void DudeApp::AboutRequested()
{
	// MFC NOTE: instead of creating a custom About dialog,
	// we'll just use the standard alert dialog.
	BAlert* pAlert = new BAlert("About Dpuloodle", 
		"Doodle: a simple drawing application\n"
		"Inspired by the MFC tutorial \"Scribble\"", "OK");
	pAlert->Go();	// asynchronous -- the alert automatically
					// deletes itself when done
}

// DudeApp::ArgvReceived
// ---------------------
// Handles command line arguments by attempting to treat them all as file
// references.
//
// MFC NOTE: Analagous to CWinApp's ParseCommandLine and
// ProcessShellCommand. Also handles command line arguments that might be
// passed in through attempted launches of the application after it has
// already started.
void DudeApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage* message = 0;
	for (int32 i=1; i<argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (! message) {
				message = new BMessage;
				message->what = B_REFS_RECEIVED;
			}
			message->AddRef("refs", &ref);
		}
	}
	if (message) {
		RefsReceived(message);
	}
}

// DudeApp::MessageReceived
// ------------------------
// The main message dispatch routine for the application.
//
// MFC NOTE: Analagous to MFC's message map (but without the macros to
// hide the details, or VC++ to automatically generate the maps).
void DudeApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_FILE_NEW:
		OnNew();
		break;
	case MSG_FILE_OPEN:
		OnOpen();
		break;
	case MSG_WIN_CASCADE:
		OnCascade();
		break;
	case MSG_WIN_TILE:
		OnTile();
		break;
	case MSG_ADD_MRU:
		AddToMRUList(message);
		break;
	default:
		BApplication::MessageReceived(message);
		break;
	}
}

// DudeApp::Pulse
// --------------
// Perform a task at regularly scheduled intervals, which are set by
// SetPulseRate.
//
// MFC NOTE: You can use this and its BWindow equivalent to implement
// coarse timer events.
void DudeApp::Pulse()
{
	bigtime_t timeout = PULSE_EVENT_RATE - 2000;

	// update windows' user interfaces
	status_t res = B_OK;
	int32 i=0;
	while (res == B_OK) {
		BWindow* pWin = WindowAt(i++);
		if (! pWin)
			break;
			
		DudeWin* pDudeWin = dynamic_cast<DudeWin*>(pWin);
		if (pDudeWin) {
			res = pDudeWin->LockWithTimeout(timeout);
			if (res == B_OK) {
				res = pDudeWin->UpdateUI();
				pDudeWin->Unlock();
			}
		}
	}
	if (res != B_OK)
		return;

	// update the command toolbar's UI
	CmdToolbar* tb = CmdToolbar::GetToolbar();
	if (tb && (tb->LockWithTimeout(timeout) == B_OK)) {
		tb->UpdateUI();
		tb->Unlock();
	}
}

// DudeApp::QuitRequested
// ----------------------
// Implements the normal QuitRequested in a slightly different way.
// In this method, the DudeWin::QuitRequested calls are handled by
// the respective window threads in response to B_QUIT_REQUESTED
// messages, NOT by the application's interdiction. The result is
// that synchronous dialogs (commonly employed in QuitRequested
// situations) can update the window correctly while they're being
// displayed.
bool DudeApp::QuitRequested()
{
	// Attempt to close all the document windows.
	bool ok = QuitDudeWinLoop();
	if (ok)
		// Everything's been saved, and only unimportant windows should remain.
		// Now we can forcibly blow those away.
		CloseAllWindows();
	
	return ok;
}

// DudeApp::ReadyToRun
// -------------------
// Sets up the application and creates an untitled document
// if one isn't already created.
//
// MFC NOTE: Comparing to InitInstance:
// * No OLE, 3D control, or mainframe initialization is needed.
// * Command line processing is done beforehand, in ArgvReceived -- this
//   function handles the case where there's nothing on the command line,
//   in which case a new untitled document is created.
// * Instead of returning a bool, you can quit the application directly
//   if something goes screwy.
void DudeApp::ReadyToRun()
{
	CmdToolbar::Create(); // set up toolbar
	SetPulseRate(PULSE_EVENT_RATE); // set up pulses for UI update
	if (DudeDoc::CountDocuments() < 1)
		OnNew();
}

// DudeApp::RefsReceived
// ---------------------
// Handles any entry references that might be delievered to the app (through
// drag&drop, a File>Open panel, an MRU list entry, or otherwise).
//
// MFC NOTE: In MFC, CWinApp handles this through the magic of document
// templates, documents, and dynamic CObject creation.
void DudeApp::RefsReceived(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;
	
	for (int32 i = --count; i >= 0; --i) {
   		if (message->FindRef("refs", i, &ref) == B_OK) {
   			Open(&ref);
   		}
   	}
}



/////////////////////////////////////////////////////////////////////////////
// DudeApp operations

// DudeApp::AddMRUMenuItems
// ------------------------
// Adds items to the menu that correspond to the most recently used files
void DudeApp::AddMRUMenuItems(BMenu* menu)
{
	int32 numItems = m_pPrefs->CountMRUs();
	if (! numItems) {
		menu->AddSeparatorItem();
		return;
	}
	menu->AddSeparatorItem();
	for (int32 i=0; i<numItems; i++)
	{
		// Construct the message for this menu entry, and make it look
		// exactly like a RefsReceived message so it'll be handled in
		// the same place as File>Open messages.
		//
		// MFC NOTE: This BMessage stores all the data we'll need to
		// open the document, and is well-understood by most Be
		// applications. Compare this to the traditional Windows
		// messaging approach.
		const entry_ref* pRef = m_pPrefs->MRUAt(i);
		if (! pRef)
			continue;
		
		BMessage* pMsg = new BMessage(B_REFS_RECEIVED);
		pMsg->AddRef("refs", pRef);
	
		char menuText[B_FILE_NAME_LENGTH + 2];
		if (i < 9) {
			sprintf(menuText, "%ld %s", i, pRef->name);
		} else if (i == 9) {
			sprintf(menuText, "0 %s", pRef->name);
		} else {
			strcpy(menuText, pRef->name);
		}
		BMenuItem* pItem = new BMenuItem(menuText, pMsg);
		pItem->SetTarget(this);
		menu->AddItem(pItem);
	}
	menu->AddSeparatorItem();
}

/////////////////////////////////////////////////////////////////////////////
// DudeApp message handlers

// DudeApp::OnNew
// --------------
// Creates a new, untitled document.
void DudeApp::OnNew()
{
	DudeDoc::CreateDocument();
	
	// if no document has been opened successfully, quit
	// (in case the default untitled document didn't
	// open correctly)
	if (DudeDoc::CountDocuments() < 1) {
		PostMessage(B_QUIT_REQUESTED);
	}
}

// DudeApp::OnOpen
// ---------------
// Launches the File>Open panel. Note that we reuse the panel so that it
// remembers what directory it was last in. Selected files will be sent to
// RefsReceived.
//
// MFC NOTE: The system file panel is modeless, and communicates with
// the application through B_REFS_RECEIVED messages. In MFC, the file
// panel is modal, and you get the data directly.
void DudeApp::OnOpen()
{
	if (! m_pOpenPanel) {
		m_pOpenPanel = new BFilePanel;
	}
	m_pOpenPanel->Show();
}

// DudeApp::OnCascade
// ------------------
// Cascades the document windows in the application.
void DudeApp::OnCascade()
{
	bool bFirst = true; // true for the first window only
	int32 numDocs = DudeDoc::CountDocuments();
	for (int32 i=0; i<numDocs; i++) {
		DudeDoc* pDoc = DudeDoc::DocumentAt(i);
		int32 numWins = pDoc->CountWindows();
		for (int32 j=0; j<numWins; j++) {
			DudeWin* pWin = pDoc->WindowAt(j);
			pWin->MoveToCascadePosition(bFirst);
			bFirst = false;
		}
	}
}

// DudeApp::OnTile
// ---------------
// Tiles the document windows in the application.
void DudeApp::OnTile()
{
	// This unspeakably clever algorithm tiles the windows
	// to the same area, with roughly the same number
	// of windows horiz. and vertical.
	
	// Empirical window measurements, taken from the standard
	// document window 'wdef'
	BRect winMargin(4, 4, 6, 6);	// not counting the title bar
	float topMargin = 19;			// top row must take title bars into account
	
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect sRect = screen.Frame();
	sRect.top += topMargin;			// don't count title bars as 'useable' space
	
	int32 totDocWins=0;
	int32 numDocs = DudeDoc::CountDocuments();
	for (int32 i=0; i<numDocs; i++) {
		totDocWins += DudeDoc::DocumentAt(i)->CountWindows();
	}
	
	// Use a fun math hack to get the A x B matrix of windows we'll need
	int32 biggerDim = static_cast<int32>(ceil(sqrt(totDocWins)));
	int32 smallerDim = static_cast<int32>(ceil(sqrt(totDocWins+.25) - .5));
	
	// assign the larger # to the larger screen dimension (horiz. or vert.)
	int32 docsAcross, docsDown;
	bool bAcrossIsBigger = sRect.Width() > sRect.Height();
	docsAcross	= bAcrossIsBigger ? biggerDim : smallerDim;
	docsDown	= bAcrossIsBigger ? smallerDim : biggerDim;

	// now get our target window size
	BPoint docWinSize(floor(sRect.Width() / docsAcross),
		floor(sRect.Height() / docsDown));
	
	// now tile 'em, starting at left top
	BPoint startPoint(sRect.LeftTop());
	BPoint curPoint = startPoint;
	int32 curAcross = 0, curDown = 0;
	bool bTileDown = bAcrossIsBigger;
	for (int32 i=0; i<numDocs; i++) {
		DudeDoc* pDoc = DudeDoc::DocumentAt(i);
		int32 numWins = pDoc->CountWindows();
		for (int32 j=0; j<numWins; j++) {
			// move & size the window
			DudeWin* pWin = pDoc->WindowAt(j);
			pWin->MoveTo(curPoint.x + winMargin.left, curPoint.y + winMargin.top);
			pWin->ResizeTo(docWinSize.x - winMargin.left - winMargin.right - 1,
				docWinSize.y - winMargin.top - winMargin.bottom - 1);
				
			// set the next point
			if (bTileDown) {
				if ((++curDown) >= docsDown) {
					curDown = 0; curAcross++;
				}
			} else {
				if ((++curAcross) >= docsAcross) {
					curAcross = 0; curDown++;
				}
			}
			curPoint.Set(startPoint.x + curAcross*docWinSize.x,
				startPoint.y + curDown*docWinSize.y);
		}
	}
}

// DudeApp::AddToMRUList
// ---------------------
// Handles the MSG_ADD_MRU message by extracting the entry_ref and handing it
// to the global preferences.
void DudeApp::AddToMRUList(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;
	
	for (int32 i = --count; i >= 0; --i) {
   		if (message->FindRef("refs", i, &ref) == B_OK) {
   			m_pPrefs->AddMRUFile(&ref);
   		}
   	}
}



/////////////////////////////////////////////////////////////////////////////
// DudeApp message handlers

// DudeApp::QuitDudeWinLoop
// ------------------------
// Tries to quit all of the document windows. Returns true if it succeeds,
// or false if at least one shouldn't be closed yet.
bool DudeApp::QuitDudeWinLoop()
{
	bool ok = true;
	status_t err;
	int32 i=0;
	while (ok) {
		BWindow* pWin = WindowAt(i++);
		if (! pWin)
			break;
			
		DudeWin* pDudeWin = dynamic_cast<DudeWin*>(pWin);
		if (pDudeWin && pDudeWin->Lock()) {
			BMessage quitMsg(B_QUIT_REQUESTED);
			BMessage reply;
			BMessenger winMsgr(pDudeWin);
			pDudeWin->Unlock();
			err = winMsgr.SendMessage(&quitMsg, &reply);
			if (err == B_OK) {
				bool result;
				err = reply.FindBool("result", &result);
				if (err == B_OK) {
					ok = result;
				}
			}
		}
	}
	return ok;
}

// DudeApp::CloseAllWindows
// ------------------------
// Abruptly claims the lives of all windows associated with the application.
void DudeApp::CloseAllWindows()
{
	int32 i = 0;
	BWindow* pWin;
	for (pWin = WindowAt(i++); pWin && pWin->Lock(); pWin = WindowAt(i++)) {
		// don't take no for an answer
		pWin->Quit();
	}
}

// DudeApp::Open
// -------------
// Opens the given entry.
//
// MFC NOTE: When referring to a file in BeOS, it's much more common to
// use the more versatile entry_ref and BPath classes than a path string.
void DudeApp::Open(const entry_ref* ref)
{
	if (DudeDoc::CreateDocument(ref) == B_OK) {
		// opened successfully, add it to the MRU list
		BMessage message(MSG_ADD_MRU);
		message.AddRef("refs", ref);
		PostMessage(&message);
	} else {
		char errStr[B_FILE_NAME_LENGTH + 50];
		sprintf(errStr, "Couldn't open Doodle document: %s",
			(ref && ref->name) ? ref->name : "???");
		BAlert* pAlert = new BAlert("file i/o error", errStr, "OK");
		pAlert->Go();
	}
}
