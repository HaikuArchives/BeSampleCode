//
// Text World
//
// A sample program that's gradually evolving into
// a real text editor application.
//
// Written by: Eric Shepherd
//
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <File.h>
#include <Application.h>
#include <Messenger.h>
#include <Message.h>
#include <Roster.h>
#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <Menu.h>
#include <Entry.h>
#include <Path.h>
#include <MenuItem.h>
#include <TextView.h>
#include <FilePanel.h>
#include <ScrollView.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "textapp.h"
#include "textwindow.h"

//
// TextWindow::TextWindow
//
// Constructs the window we'll be drawing into.
//
TextWindow::TextWindow(BRect frame)
			: BWindow(frame, "Untitled ", B_DOCUMENT_WINDOW, 0) {
	_InitWindow();
	Show();
}

//
// TextWindow::TextWindow
//
// Create a window from a file.
//
TextWindow::TextWindow(BRect frame, entry_ref *ref)
			: BWindow(frame, "Untitled ", B_DOCUMENT_WINDOW, 0) {
	BFile file;
	
	_InitWindow();
	
	// Open the file
	
	if (file.SetTo(ref, B_READ_ONLY) == B_OK) {
		off_t length;
		char *text;
		file.GetSize(&length);		// Get the file length;
		text = (char *) malloc(length);
		if (text && (file.Read(text, length) >= B_OK)) {
			savemessage = new BMessage(B_SAVE_REQUESTED);
			if (savemessage) {
				BEntry entry(ref, true);
				BEntry parent;
				entry_ref parent_ref;
				char name[B_FILE_NAME_LENGTH];
				
				entry.GetParent(&parent);		// Get parent directory
				entry.GetName(name);
				parent.GetRef(&parent_ref);
				savemessage->AddRef("directory", &parent_ref);
				savemessage->AddString("name", name);
				textview->SetText(text, length);
				SetTitle(name);
			}
			free(text);
		}
	}	
	Show();
}


//
// TextWindow::_InitWindow
//
// Initialize the window.
//
void TextWindow::_InitWindow(void) {
	BRect r;
	BMenu *menu;
	BMenuItem *item;
	
	// Initialize variables
	
	savemessage = NULL;			// No saved path yet
	
	// Add the menu bar
	
	r = Bounds();
	menubar = new BMenuBar(r, "menu_bar");

	// Add File menu to menu bar
	
	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New", new BMessage(MENU_FILE_NEW), 'N'));
	menu->AddItem(item=new BMenuItem("Open" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_OPEN), 'O'));
	item->SetTarget(be_app);
	menu->AddItem(new BMenuItem("Close", new BMessage(MENU_FILE_CLOSE), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(saveitem=new BMenuItem("Save", new BMessage(MENU_FILE_SAVE), 'S'));
	saveitem->SetEnabled(false);
	menu->AddItem(new BMenuItem("Save as" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_SAVEAS)));
	menu->AddSeparatorItem();
	menu->AddItem(item=new BMenuItem("Page Setup" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_PAGESETUP)));
	item->SetEnabled(false);
	menu->AddItem(item=new BMenuItem("Print" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_PRINT), 'P'));
	item->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(MENU_FILE_QUIT), 'Q'));
	menubar->AddItem(menu);
	
	// Attach the menu bar to he window
	
	AddChild(menubar);

	// Add the text view
	
	BRect textframe = Bounds();
	textframe.top = menubar->Bounds().bottom + 1.0;
	textframe.right -= B_V_SCROLL_BAR_WIDTH;
	BRect textrect = textframe;
	textrect.OffsetTo(B_ORIGIN);
	r.InsetBy(3.0,3.0);
	textview = new BTextView(textframe, "text_view", textrect,
				B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED);
	AddChild(scrollview = new BScrollView("scroll_view", textview,
				B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER));
	textview->SetDoesUndo(true);
	textview->MakeFocus(true);

	// Add the Edit menu to the menu bar

	menu = new BMenu("Edit");
	menu->AddItem(item=new BMenuItem("Undo", new BMessage(B_UNDO), 'Z'));
	item->SetTarget(textview);
	menu->AddSeparatorItem();
	menu->AddItem(item=new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	item->SetTarget(textview);
	menu->AddItem(item=new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	item->SetTarget(textview);
	menu->AddItem(item=new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	item->SetTarget(textview);
	menu->AddSeparatorItem();
	menu->AddItem(item=new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A'));
	item->SetTarget(textview);
	menubar->AddItem(menu);

	// Create the save filepanel for this window
	
	savePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL,
				B_FILE_NODE, false);

	// Tell the application that there's one more window
	// and get the number for this untitled window.
	
	Register(true);
	Minimize(true);		// So Show() doesn't really make it visible
}


//
// TextWindow::FrameResized
//
// Adjust the size of the BTextView's text rectangle
// when the window is resized.
//
void TextWindow::FrameResized(float width, float height) {
	BRect textrect = textview->TextRect();
	
	textrect.right = textrect.left + (width - B_V_SCROLL_BAR_WIDTH - 3.0);
	textview->SetTextRect(textrect);
}


//
// TextWindow::~TextWindow
//
// Destruct the window.  This calls Unregister().
//
TextWindow::~TextWindow() {
	Unregister();
	if (savemessage) {
		delete savemessage;
	}
	delete savePanel;
}


//
// TextWindow::MessageReceived
//
// Called when a message is received by our
// application.
//
void TextWindow::MessageReceived(BMessage *message) {
	switch(message->what) {
		case WINDOW_REGISTRY_ADDED:
			{
				char s[22];
				BRect rect;
				if (message->FindInt32("new_window_number", &window_id) == B_OK) {
					if (!savemessage) {		// if it's untitled
						sprintf(s, "Untitled %ld", window_id);
						SetTitle(s);
					}
				}
				if (message->FindRect("rect", &rect) == B_OK) {
					MoveTo(rect.LeftTop());
					ResizeTo(rect.Width(), rect.Height());
				}
				Minimize(false);
			}
			break;
			
		case MENU_FILE_NEW:
			{
				BRect r;
				r = Frame();
				new TextWindow(r);
			}
			break;
		case MENU_FILE_CLOSE:
			Quit();
			break;
		case MENU_FILE_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case MENU_FILE_SAVEAS:
			savePanel->Show();
			break;
		case MENU_FILE_SAVE:
			Save(NULL);
			break;
		case B_SAVE_REQUESTED:
			Save(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


//
// TextWindow::Register
//
// Since MessageWorld can have multiple windows and
// we need to know when there aren't any left so the
// application can be shut down, this function is used
// to tell the application that a new window has been
// opened.
//
// If the need_id argument is true, we'll specify true
// for the "need_id" field in the message we send; this
// will cause the application to send back a
// WINDOW_REGISTRY_ADDED message containing the window's
// unique ID number.  If this argument is false, we won't
// request an ID.
//
void TextWindow::Register(bool need_id) {
	BMessenger messenger(APP_SIGNATURE);
	BMessage message(WINDOW_REGISTRY_ADD);
	
	message.AddBool("need_id", need_id);
	messenger.SendMessage(&message, this);
}


//
// TextWindow::Unregister
//
// Unregisters a window.  This tells the application that
// one fewer windows are open.  The application will
// automatically quit if the count goes to zero because
// of this call.
//
void TextWindow::Unregister(void) {
	BMessenger messenger(APP_SIGNATURE);
	
	messenger.SendMessage(new BMessage(WINDOW_REGISTRY_SUB));
}


//
// TextWindow::QuitRequested
//
// Here we just give permission to close the window.
//
bool TextWindow::QuitRequested() {
	return true;
}


//
// TextWindow::Save
//
// Save the contents of the window.  The message specifies
// where to save it (see BFilePanel in the Storage Kit chapter
// of the Be Book).
//
status_t TextWindow::Save(BMessage *message) {
	entry_ref ref;		// For the directory to save into
	status_t err;		// For the return code
	const char *name;	// For the filename
	BPath path;		// For the pathname
	BEntry entry;		// Used to make the path
	FILE *f;		// Standard Posix file
	
	// If a NULL is passed for the message pointer, use
	// the value we've cached; this lets us do saves without
	// thinking.
	
	if (!message) {
		message = savemessage;
		if (!message) {
			return B_ERROR;
		}
	}
	
	// Peel the entry_ref and name of the directory and
	// file out of the message.
	
	if ((err=message->FindRef("directory", &ref)) != B_OK) {
		return err;
	}
	if ((err=message->FindString("name", &name)) != B_OK) {
		return err;
	}
	
	// Take the directory and create a pathname out of it
	
	if ((err=entry.SetTo(&ref)) != B_OK) {
		return err;
	}
	entry.GetPath(&path);		// Create a pathname for the directory
	path.Append(name);			// Tack on the filename
	
	// Now we can save the file.
	
	if (!(f = fopen(path.Path(), "w"))) {
		return B_ERROR;
	}
	
	err = fwrite(textview->Text(), 1, textview->TextLength(), f);
	fclose(f);
	if (err >= 0) {
		SetTitle(name);
		saveitem->SetEnabled(true);
		if (savemessage != message) {
			delete savemessage;
			savemessage = new BMessage(*message);
		}
	}
	return err;
}
