#include "Common.h"
#include "TranslatorSavePanel.h"
#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Alert.h>
#include <interface/ScrollBar.h>
#include <interface/Button.h>
#include <TranslationKit.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

TranslatorMenuItem::TranslatorMenuItem(const char *name, BMessage *message,
	translator_id id, uint32 format) : BMenuItem(name, message) {

	this->id = id;
	this->format = format;
}

TranslatorSavePanel::TranslatorSavePanel(const char *name, BMessenger *target, entry_ref *start_directory,
	uint32 node_flavors, bool allow_multiple_selection, BMessage *message, BRefFilter *filter, bool modal,
	bool hide_when_done) :
	
	BFilePanel(B_SAVE_PANEL, target, start_directory, node_flavors, allow_multiple_selection, message, filter,
		modal, hide_when_done), BHandler(name) {

	configwindow = NULL;
	if (Window()->Lock()) {
		Window()->SetTitle("Save Image");
		
		// Find all the views that are in the way and move up them up 10 pixels
		BView *background = Window()->ChildAt(0);
		BView *poseview = background->FindView("PoseView");
		if (poseview) poseview->ResizeBy(0, -10);
		BButton *cancel = (BButton *)background->FindView("cancel button");
		if (cancel) cancel->MoveBy(0, -10);
		BButton *insert = (BButton *)background->FindView("default button");
		if (insert) insert->MoveBy(0, -10);
		BScrollBar *hscrollbar = (BScrollBar *)background->FindView("HScrollBar");
		if (hscrollbar) hscrollbar->MoveBy(0, -10);
		BScrollBar *vscrollbar = (BScrollBar *)background->FindView("VScrollBar");
		if (vscrollbar) vscrollbar->ResizeBy(0, -10);
		BView *countvw = (BView *)background->FindView("CountVw");
		if (countvw) countvw->MoveBy(0, -10);
		BView *textview = (BView *)background->FindView("text view");
		if (textview) textview->MoveBy(0, 10);
		
		// Add the new BHandler to the window's looper
		Window()->AddHandler(this);
		
		if (!cancel || !textview || !hscrollbar) {
			printf("Couldn't find necessary controls.\n");
			return;
		}
		
		// Build the "Settings" button relative to the cancel button
		BRect rect = cancel->Frame();
		rect.right = rect.left - 10;
		float width = be_plain_font->StringWidth("Settings...") + 20;
		rect.left = (width > 75) ? rect.right - width : rect.right - 75;
		BButton *settings = new BButton(rect, "Settings", "Settings...", new BMessage(SAVE_FILE_PANEL_SETTINGS),
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
		background->AddChild(settings);
		settings->SetTarget(this);
		
		BuildMenu();
		
		// Position the menu field relative to the other GUI elements, and make it the
		// same length as the textview
		rect = textview->Frame();
		rect.top = hscrollbar->Frame().bottom + 5;
		rect.bottom = rect.top + 10;
		formatmenufield = new BMenuField(rect, "FormatMenuField", "Format:", formatpopupmenu,
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
		background->AddChild(formatmenufield);
		formatmenufield->SetDivider(be_plain_font->StringWidth("Format:") + 7);
		
		// Set the file panel's message to the first available translator
		// Use the 'what' field of the supplied message as a model
		what = message->what;
		BMessage *m = new BMessage(what);
		TranslatorMenuItem *item = GetCurrentMenuItem();
		if (item != NULL) {
			m->AddData("translator_id", B_RAW_TYPE, &(item->id), sizeof(translator_id));
			m->AddInt32("translator_format", item->format);
		}
		SetMessage(m);
		
		// Make sure the smallest window won't draw the "Settings" button over anything else
		float min_window_width = Window()->Bounds().right - settings->Frame().left + 10 + textview->Frame().right;
		Window()->SetSizeLimits(min_window_width, 10000, 250, 10000);
		if (Window()->Bounds().IntegerWidth() + 1 < min_window_width)
			Window()->ResizeTo(min_window_width, 300);
	
		Window()->Unlock();
	}
}

// Handle messages from controls we've added
void TranslatorSavePanel::MessageReceived(BMessage *message) {
	switch (message->what) {
		case SAVE_FILE_PANEL_FORMAT: {
			BMessage *message = new BMessage(what);
			TranslatorMenuItem *item = GetCurrentMenuItem();
			if (item != NULL) {
				message->AddData("translator_id", B_RAW_TYPE, &(item->id), sizeof(translator_id));
				message->AddInt32("translator_format", item->format);
			}
			SetMessage(message);
			break;
		}
		case SAVE_FILE_PANEL_SETTINGS:
			TranslatorSettings();
			break;
		default:
			BHandler::MessageReceived(message);
			break;
	}
}

TranslatorMenuItem *TranslatorSavePanel::GetCurrentMenuItem() {
	return (TranslatorMenuItem *)formatpopupmenu->FindMarked();
}

void TranslatorSavePanel::TranslatorSettings() {
	TranslatorMenuItem *item = GetCurrentMenuItem();
	if (item == NULL) return;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BView *view;
	BRect rect(0, 0, 239, 239);
	
	// Build a window around this translator's configuration view
	status_t err = roster->MakeConfigurationView(item->id, NULL, &view, &rect);
	if (err != B_OK || view == NULL) {
		BAlert *alert = new BAlert(NULL, strerror(err), "OK");
		alert->Go();
	} else {
		if (configwindow != NULL) {
			if (configwindow->Lock()) configwindow->Quit();
		}
		configwindow = new BWindow(rect, "Translator Settings", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
		configwindow->AddChild(view);
		// Just to make sure
		view->MoveTo(0, 0);
		view->ResizeTo(rect.Width(), rect.Height());
		configwindow->MoveTo(100, 100);
		configwindow->Show();
	}
}

void TranslatorSavePanel::BuildMenu() {
	formatpopupmenu = new BPopUpMenu("FormatPopUpMenu");
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_id *translators;
	int32 count;

	// Find all translators on the system
	roster->GetAllTranslators(&translators, &count);
	const translation_format *format;
	int32 format_count;
	
	for (int x = 0; x < count; x++) {
		// Determine which formats this one can write
		roster->GetOutputFormats(translators[x], &format, &format_count);
		for (int y = 0; y < format_count; y++) {
			// Check if this is an image translator
			if (format[y].group == B_TRANSLATOR_BITMAP) {
				// If this format saves to some native format build a menu item for it
				if (format[y].type == B_TRANSLATOR_BITMAP) continue;
				TranslatorMenuItem *item = new TranslatorMenuItem(format[y].name,
					new BMessage(SAVE_FILE_PANEL_FORMAT), translators[x], format[y].type);
				item->SetTarget(this);
				formatpopupmenu->AddItem(item);
				break;
			}
		}
	}
	delete [] translators;
	
	// Pick the first translator in the list
	TranslatorMenuItem *item = (TranslatorMenuItem *)formatpopupmenu->ItemAt(0);
	if (item != NULL) item->SetMarked(true);
}

TranslatorSavePanel::~TranslatorSavePanel() {
	
}