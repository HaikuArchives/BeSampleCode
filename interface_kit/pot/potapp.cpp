/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Alert.h>
#include <FilePanel.h>

#include "constants.h"
#include "potapp.h"
#include "potwin.h"
#include "testwin.h"

int main(int, char**)
{
	PotApp theApp;
	theApp.Run();
	return 0;
}

PotApp::PotApp()
	: BApplication(APP_SIG), m_pOpenPanel(0)
{ }

void PotApp::AboutRequested()
{
	BAlert* pAlert = new BAlert("About PotApp", 
		"PotApp\na demonstration of asynchronous controls\n\n"
		"Usage:\n\n"
		"*\tClick and drag on a pot to adjust the\n"
		"\tcolor overlay.\n"
		"*\tClick on a check box to mark/unmark\n"
		"\ta pot.\n"
		"*\tCommand-click anywhere else and\n"
		"\tdrag to adjust all marked pots\n"
		"\tsimultaneously.", "OK");
	pAlert->Go();
}

void PotApp::ReadyToRun()
{
	int32 count = TestWin::CountWindows();
	if (count < 1)
		OnOpen();
}

void PotApp::ArgvReceived(int32 argc, char** argv)
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

void PotApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_FILE_OPEN:
		OnOpen();
		break;
	case B_CANCEL:
		if (TestWin::CountWindows() < 1)
			PostMessage(B_QUIT_REQUESTED);
		break;
	default:
		BApplication::MessageReceived(message);
		break;
	}
}

bool PotApp::QuitRequested()
{
	// Attempt to close all the document windows.
	bool ok = QuitDudeWinLoop();
	if (ok)
		// Everything's been saved, and only unimportant windows should remain.
		// Now we can forcibly blow those away.
		CloseAllWindows();
	
	return ok;
}

void PotApp::RefsReceived(BMessage* message)
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

void PotApp::OnOpen()
{
	if (! m_pOpenPanel) {
		m_pOpenPanel = new BFilePanel;
		m_pOpenPanel->Window()->SetTitle("Open Image File");
	}
	m_pOpenPanel->Show();
}

bool PotApp::QuitDudeWinLoop()
{
	bool ok = true;
	status_t err;
	int32 i=0;
	while (ok) {
		BWindow* pWin = WindowAt(i++);
		if (! pWin)
			break;
			
		TestWin* pTestWin = dynamic_cast<TestWin*>(pWin);
		if (pTestWin && pTestWin->Lock()) {
			BMessage quitMsg(B_QUIT_REQUESTED);
			BMessage reply;
			BMessenger winMsgr(pTestWin);
			pTestWin->Unlock();
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

void PotApp::CloseAllWindows()
{
	int32 i = 0;
	BWindow* pWin;
	for (pWin = WindowAt(i++); pWin && pWin->Lock(); pWin = WindowAt(i++)) {
		// don't take no for an answer
		pWin->Quit();
	}
}

void PotApp::Open(const entry_ref* ref)
{
	if (TestWin::NewWindow(ref) != B_OK) {
		char errStr[B_FILE_NAME_LENGTH + 50];
		sprintf(errStr, "Couldn't open file: %s",
			(ref && ref->name) ? ref->name : "???");
		BAlert* pAlert = new BAlert("file i/o error", errStr, "OK");
		pAlert->Go();
	}
}
