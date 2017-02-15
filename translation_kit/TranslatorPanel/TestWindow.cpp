#include "TestWindow.h"
#include "Common.h"
#include <app/Application.h>
#include <interface/MenuBar.h>
#include <interface/MenuItem.h>
#include <interface/Menu.h>
#include <interface/Alert.h>
#include <storage/Path.h>
#include <storage/Directory.h>
#include <storage/Entry.h>
#include <Roster.h>
#include <stdlib.h>
#include <stdio.h>

TestWindow::TestWindow(BRect rect, const char *name) :
	BWindow(rect, name, B_TITLED_WINDOW, 0, B_CURRENT_WORKSPACE) {
	
	BRect r(0, 0, 100, 10);
	BMenuBar *menubar = new BMenuBar(r, "MenuBar");
	BMenu *filemenu = new BMenu("File");
	
	BMenuItem *openitem = new BMenuItem("Open...", new BMessage(OPEN_IMAGE), 'O', 0);
	BMenuItem *saveitem = new BMenuItem("Save...", new BMessage(SAVE_IMAGE), 'S', 0);
	BMenuItem *quititem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q', 0);
	
	filemenu->AddItem(openitem);
	filemenu->AddItem(saveitem);
	filemenu->AddSeparatorItem();
	filemenu->AddItem(quititem);
	menubar->AddItem(filemenu);
	AddChild(menubar);
	
	r = Bounds();
	r.top = menubar->Frame().bottom + 1;
	testview = new TestView(r, "TestView");
	AddChild(testview);
	
	app_info ai;
	be_app->GetAppInfo(&ai);
	BEntry entry(&ai.ref);
	BPath path;
	entry.GetPath(&path);
	path.GetParent(&path);
	path.Append("addons");
	BDirectory directory(path.Path());
	
	BMenu *filtermenu = new BMenu("Filters");
	menubar->AddItem(filtermenu);
	
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		entry.GetPath(&path);
		image_id image = load_add_on(path.Path());
		if (image < 0) continue;

		ImageFilter *(*instantiate_filter)();
		if (get_image_symbol(image, "instantiate_filter", B_SYMBOL_TYPE_TEXT,
			(void **)&instantiate_filter) != B_OK) {
			
			printf("get_image_symbol failed for %s\n", path.Path());
			continue;
		}
		ImageFilter *filter = (*instantiate_filter)();
		if (filter != NULL) {
			BMessage *m = new BMessage(RUN_FILTER);
			m->AddInt32("filter_id", (int32)filter->GetId());
			BMenuItem *menuitem = new BMenuItem(filter->GetName(), m, 0, 0);
			filtermenu->AddItem(menuitem);
			testview->AddFilter(filter);
		}
	}
	
	savepanel = new TranslatorSavePanel("TranslatorSavePanel", new BMessenger(this), NULL, 0, false,
		new BMessage(SAVE_FILE_PANEL));
	openpanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 0, false,
		new BMessage(OPEN_FILE_PANEL), NULL, false, true);
	
	SetSizeLimits(200, 10000, 150, 10000);
}

void TestWindow::MessageReceived(BMessage *message) {
	switch (message->what) {
		case OPEN_IMAGE:
			openpanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			openpanel->Show();
			break;
		case SAVE_IMAGE:
			if (testview->HasImage()) {
				savepanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
				savepanel->Show();
			} else {
				BAlert *alert = new BAlert(NULL, "No image to save.", "OK");
				alert->Go();
			}
			break;
		case OPEN_FILE_PANEL:
		case B_SIMPLE_DATA:
			testview->SetImage(message);
			break;
		case SAVE_FILE_PANEL:
			testview->SaveImage(message);
			break;
		case B_CANCEL:
			break;
		case RUN_FILTER:
			if (!testview->HasImage()) {
				BAlert *alert = new BAlert(NULL, "No image to filter.", "OK");
				alert->Go();
				break;
			}
			testview->FilterImage(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool TestWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

TestWindow::~TestWindow() {
	delete savepanel;
	delete openpanel;
}