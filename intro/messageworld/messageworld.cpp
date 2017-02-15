//
// Menu World
//
// A sample program demonstrating the basics of using
// the BMessage and BMessenger classes.
//
// Written by: Eric Shepherd
//

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Messenger.h>
#include <Message.h>
#include <Roster.h>
#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <string.h>
#include <stdio.h>

// Application's signature

const char *APP_SIGNATURE				= "application/x-vnd.Be-MessageWorld";

// Messages for window registry with application

const uint32 WINDOW_REGISTRY_ADD		= 'WRad';
const uint32 WINDOW_REGISTRY_SUB		= 'WRsb';
const uint32 WINDOW_REGISTRY_ADDED		= 'WRdd';

// Messages for menu commands

const uint32 MENU_FILE_NEW				= 'MFnw';
const uint32 MENU_FILE_OPEN				= 'MFop';
const uint32 MENU_FILE_CLOSE			= 'MFcl';
const uint32 MENU_FILE_SAVE				= 'MFsv';
const uint32 MENU_FILE_SAVEAS			= 'MFsa';
const uint32 MENU_FILE_PAGESETUP		= 'MFps';
const uint32 MENU_FILE_PRINT			= 'MFpr';
const uint32 MENU_FILE_QUIT				= 'MFqu';

const uint32 MENU_OPT_HELLO				= 'MOhl';

const char *STRING_HELLO				= "Hello World!";
const char *STRING_GOODBYE				= "Goodbye World!";

//
// HelloView class
//
// This class defines the view in which the "Hello World"
// message will be drawn.
//
class HelloView : public BView {
	public:
						HelloView(BRect frame);
		virtual void	Draw(BRect updateRect);
		void			SetString(const char *s);
	
	private:
		char			message[128];
};


//
// HelloView::HelloView
//
// Constructs the view we'll be drawing in.
// As you see, it doesn't do much.
//
HelloView::HelloView(BRect frame)
			: BView(frame, "HelloView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW) {
	SetString(STRING_HELLO);
}


//
// HelloView::SetString
//
// Sets the message to draw in the view.
//
void HelloView::SetString(const char *s) {
	if (strlen(s) < 127) {
		strcpy(message, s);
	}
}

//
// HelloView::Draw
//
// This function is called whenever our view
// needs to be redrawn.  This happens only because
// we specified B_WILL_DRAW for the flags when
// we created the view (see the constructor).
//
// The updateRect is the rectangle that needs to be
// redrawn.  We're ignoring it, but you can use it to
// speed up your refreshes for more complex programs.
//
void HelloView::Draw(BRect updateRect) {
	MovePenTo(BPoint(20,75));			// Move pen
	DrawString(message);
}


//
// HelloWindow class
//
// This class defines the hello world window.
//
class HelloWindow : public BWindow {
	public:
						HelloWindow(BRect frame);
						~HelloWindow();
		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage *message);
	
	private:
		void			Register(bool need_id);
		void			Unregister(void);
		
		BMenuBar		*menubar;
		HelloView		*helloview;
};


//
// HelloWindow::HelloWindow
//
// Constructs the window we'll be drawing into.
//
HelloWindow::HelloWindow(BRect frame)
			: BWindow(frame, "Untitled ", B_TITLED_WINDOW,
				B_NOT_RESIZABLE|B_NOT_ZOOMABLE) {
	BRect r;
	BMenu *menu;
	BMenuItem *item;
	
	// Add the menu bar
	
	r = Bounds();
	menubar = new BMenuBar(r, "menu_bar");
	AddChild(menubar);

	// Add File menu to menu bar
	
	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New", new BMessage(MENU_FILE_NEW), 'N'));
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_OPEN), 'O'));
	menu->AddItem(new BMenuItem("Close", new BMessage(MENU_FILE_CLOSE), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Save", new BMessage(MENU_FILE_SAVE), 'S'));
	menu->AddItem(new BMenuItem("Save as" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_SAVEAS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Page Setup" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_PAGESETUP)));
	menu->AddItem(new BMenuItem("Print" B_UTF8_ELLIPSIS,
					new BMessage(MENU_FILE_PRINT), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(MENU_FILE_QUIT), 'Q'));
	menubar->AddItem(menu);

	// Add Options menu to menu bar
	
	menu = new BMenu("Options");
	item=new BMenuItem("Say Hello", new BMessage(MENU_OPT_HELLO));
	item->SetMarked(true);
	menu->AddItem(item);
	menubar->AddItem(menu);
	
	// Add the drawing view
	
	r.top = menubar->Bounds().bottom+1;
	AddChild(helloview = new HelloView(r));
	
	// Tell the application that there's one more window
	// and get the number for this untitled window.
	
	Register(true);
	Show();
}


//
// HelloWindow::~HelloWindow
//
// Destruct the window.  This calls Unregister().
//
HelloWindow::~HelloWindow() {
	Unregister();
}


//
// HelloWindow::MessageReceived
//
// Called when a message is received by our
// application.
//
void HelloWindow::MessageReceived(BMessage *message) {
	switch(message->what) {
		case WINDOW_REGISTRY_ADDED:
			{
				char s[22];
				int32 id = 0;
				if (message->FindInt32("new_window_number", &id) == B_OK) {
					sprintf(s, "Untitled %ld", id);
					SetTitle(s);
				}
			}
			break;
			
		case MENU_FILE_NEW:
			{
				BRect r;
				r = Frame();
				r.OffsetBy(20,20);
				new HelloWindow(r);
			}
			break;
		case MENU_FILE_CLOSE:
			Quit();
			break;
		case MENU_FILE_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		
		case MENU_OPT_HELLO:
			{
				BMenuItem *item;
				const char *s;
				bool mark;
				
				message->FindPointer("source", (void **) &item);
				if (item->IsMarked()) {
					s = STRING_GOODBYE;
					mark = false;
				}
				else {
					s = STRING_HELLO;
					mark = true;
				}
				helloview->SetString(s);
				item->SetMarked(mark);
				helloview->Invalidate();
			}
			break;
		
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


//
// HelloWindow::Register
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
void HelloWindow::Register(bool need_id) {
	BMessenger messenger(APP_SIGNATURE);
	BMessage message(WINDOW_REGISTRY_ADD);
	
	message.AddBool("need_id", need_id);
	messenger.SendMessage(&message, this);
}


//
// HelloWindow::Unregister
//
// Unregisters a window.  This tells the application that
// one fewer windows are open.  The application will
// automatically quit if the count goes to zero because
// of this call.
//
void HelloWindow::Unregister(void) {
	BMessenger messenger(APP_SIGNATURE);
	
	messenger.SendMessage(new BMessage(WINDOW_REGISTRY_SUB));
}


//
// HelloWindow::QuitRequested
//
// Here we just give permission to close the window.
//
bool HelloWindow::QuitRequested() {
	return true;
}


//
// HelloApp class
//
// This class, derived from BApplication, defines the
// Hello World application itself.
//
class HelloApp : public BApplication {
	public:
						HelloApp();
		virtual void	MessageReceived(BMessage *message);
	
	private:
		int32			window_count;
		int32			next_untitled_number;
};


//
// HelloApp::HelloApp
//
// The constructor for the HelloApp class.  This
// will create our window.
//
HelloApp::HelloApp()
			: BApplication(APP_SIGNATURE) {
	BRect windowRect;
	windowRect.Set(50,50,349,399);
	
	window_count = 0;			// No windows yet
	next_untitled_number = 1;	// Next window is "Untitled 1"
	new HelloWindow(windowRect);
}


//
// HelloApp::MessageReceived
//
// Handle incoming messages.  In particular, handle the
// WINDOW_REGISTRY_ADD and WINDOW_REGISTRY_SUB messages.
//
void HelloApp::MessageReceived(BMessage *message) {
	switch(message->what) {
		case WINDOW_REGISTRY_ADD:
			{
				bool need_id = false;
				
				if (message->FindBool("need_id", &need_id) == B_OK) {
					if (need_id) {
						BMessage reply(WINDOW_REGISTRY_ADDED);

						reply.AddInt32("new_window_number", next_untitled_number);
						message->SendReply(&reply);
						next_untitled_number++;
					}
					window_count++;
				}
				break;
			}
		case WINDOW_REGISTRY_SUB:
			window_count--;
			if (!window_count) {
				Quit();
			}
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


//
// main
//
// The main() function's only real job in a basic BeOS
// application is to create the BApplication object
// and run it.
//
int main(void) {
	HelloApp theApp;		// The application object
	theApp.Run();
	return 0;
}



