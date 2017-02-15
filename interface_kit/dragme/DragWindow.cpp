/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Invoker.h>
#include <Message.h>
#include <Bitmap.h>

#include <Directory.h>
#include <File.h>
#include <NodeInfo.h>
#include <DataIO.h>

#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <TranslatorFormats.h>

#include <stdio.h>
#include <string.h>

#include "DragApp.h"
#include "DragWindow.h"
#include "DragView.h"


DragWindow::DragWindow(
	const BRect & frame,
	const char * name) :
	BWindow(frame, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
		B_WILL_ACCEPT_FIRST_CLICK)
{
	//	Add the glitzy UI of our application's windows...
	BRect r(frame);
	r.OffsetTo(B_ORIGIN);
	m_view = new DragView(r);
	AddChild(m_view);
	//	Register this window with the app.
	BMessage msg(DragApp::ADD_WINDOW);
	msg.AddPointer("DragWindow", this);
	dynamic_cast<DragApp *>(be_app)->PostMessage(&msg);
}

DragWindow::~DragWindow()
{
	BMessage msg(DragApp::REMOVE_WINDOW);
	msg.AddPointer("DragWindow", this);
	dynamic_cast<DragApp *>(be_app)->PostMessage(&msg);
	//	If we were the last window when we're removed, the
	//	app automatically quits.
}

//	We have the window handling messages for the view.
//	In a bigger app, it might make more sense to dispatch
//	the messages directly to the views (using BMessengers
//	refering thereto).
void
DragWindow::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case B_COPY_TARGET: {
		ActionCopy(message);
		} break;
	case B_TRASH_TARGET: {
		ActionTrash(message);
		} break;
	case QUIT_ALERT_REPLY: {
		QuitAlertReply(message);
		} break;
	case DragView::SET_DRAG_MODE: {
		drawing_mode mode;
		if (!message->FindInt32("drawing_mode", (int32 *)&mode)) {
			m_view->m_mode = mode;
		}
		} break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool
DragWindow::QuitRequested()
{
	return true;
}

void
DragWindow::ActionCopy(
	BMessage * request)
{
	//	someone accepted our drag and requested one of the two
	//	types of data we can provide (in-message or in-file bitmap)
	const char * type = NULL;
	if (!request->FindString("be:types", &type)) {
		BBitmapStream strm(m_view->m_bitmap);
		if (!strcasecmp(type, B_FILE_MIME_TYPE)) {
			const char * name;
			entry_ref dir;
			if (!request->FindString("be:filetypes", &type) &&
					!request->FindString("name", &name) &&
					!request->FindRef("directory", &dir)) {
				//	write file
				BDirectory d(&dir);
				BFile f(&d, name, O_RDWR | O_TRUNC);
				BTranslatorRoster::Default()->Translate(m_view->m_translator,
						&strm, NULL, &f, m_view->m_type_code);
				BNodeInfo ni(&f);
				ni.SetType(m_view->m_the_type);
			}
		}
		else {
			//	put in message
			BMessage msg(B_MIME_DATA);
			BMallocIO f;
			BTranslatorRoster::Default()->Translate(m_view->m_translator,
					&strm, NULL, &f, m_view->m_type_code);
			msg.AddData(m_view->m_the_type, B_MIME_TYPE, f.Buffer(), f.BufferLength());
			request->SendReply(&msg);
		}
		strm.DetachBitmap(&m_view->m_bitmap);
	}
}

void
DragWindow::ActionTrash(
	BMessage * /*request*/)
{
	//	Trash the bitmap? That means closing the window, maybe.
	char text[100];
	sprintf(text, "Really close '%s'?", Title());
	BMessenger msgr(this);
	BInvoker * invoker = new BInvoker(new BMessage(QUIT_ALERT_REPLY), msgr);
	(new BAlert("", text, "No", "Yes"))->Go(invoker);
}

void
DragWindow::QuitAlertReply(
	BMessage * reply)
{
	int32 msg = 0;
	if (!reply->FindInt32("which", &msg) && msg == 1) {	//	index of Yes button
		//	Quitting will remove us from the app's list of windows.
		Quit();
	}
}


