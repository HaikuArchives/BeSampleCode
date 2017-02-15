/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Screen.h>
#include <stdio.h>

#include "DragApp.h"
#include "DragWindow.h"


DragApp::DragApp(
	const char * signature) :
	BApplication(signature),
	m_windowRect(100,100,250,200)
{
	m_windowCount = 0;
}

DragApp::~DragApp()
{
}

bool
DragApp::QuitRequested()
{
	if (m_windows.size() == 0) {
		return true;
	}
	for (set<DragWindow *>::iterator ptr(m_windows.begin());
			ptr != m_windows.end();
			ptr++) {
		//	Window will determine if it can quit or not.
		//	When all windows have gone away, the app will die.
		(*ptr)->PostMessage(B_QUIT_REQUESTED);
	}
	return false;
}

void
DragApp::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case ADD_WINDOW: {
		DragWindow * w = NULL;
		if (message->FindPointer("DragWindow", (void **)&w) == B_OK) {
			AddWindow(w);
		}
		} break;
	case REMOVE_WINDOW: {
		DragWindow * w = NULL;
		if (message->FindPointer("DragWindow", (void **)&w) == B_OK) {
			RemoveWindow(w);
		}
		} break;
	case NEW_WINDOW: {
		NewWindow();
		} break;
	default: {
		BApplication::MessageReceived(message);
		} break;
	}
}

void
DragApp::ReadyToRun()
{
	if (m_windows.size() == 0) {
		PostMessage(NEW_WINDOW);
	}
}

void
DragApp::AddWindow(
	DragWindow * window)
{
	m_windows.insert(window);
}

void
DragApp::RemoveWindow(
	DragWindow * window)
{
	m_windows.erase(window);
	if (m_windows.size() == 0) {
		Quit();
	}
}

void
DragApp::NewWindow()
{
	BScreen scrn;
	BRect r = m_windowRect;
	if ((r & scrn.Frame()) != r) {
		if (r.right > scrn.Frame().right) {
			r.OffsetBy(r.Width()-scrn.Frame().Width(), 0);
		}
		if (r.bottom > scrn.Frame().bottom) {
			r.OffsetBy(0, r.Height()-scrn.Frame().Height());
		}
		m_windowRect = r;
	}
	m_windowRect.OffsetBy(50,75);	//	next window position
	char name[30];
	m_windowCount += 1;
	sprintf(name, "DragWindow %d", m_windowCount);
	DragWindow * w = new DragWindow(r, name);
	w->Show();
}

