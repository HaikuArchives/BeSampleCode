// Copyright 1992-1999, Be Incorporated, All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
//
// send comments/suggestions/feedback to pavel@be.com
//

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Entry.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include <stdlib.h>
#include <string.h>

#include "CDButton.h"
#include "CDPanel.h"
#include "iconfile.h"

const char *app_signature = "application/x-vnd.Be-CDButton";
// the application signature used by the replicant to find the supporting
// code

static int
FindFirstCPPlayerDevice()
{
	// simple CD lookup, only works with a single CD, for multiple CD support
	// need to get more fancy
	return CDEngine::FindCDPlayerDevice();
}

//
//	New with release 4.6 is a new and better way of adding replicants
//	to Deskbar.  By adding it in this way it is persistant until
//	the host removes it.  After the first 'add' of the replicant
//	Deskbar handles the adding from then on.
//

//
//	This is the exported function that will be used by Deskbar
//	to create and add the replicant
//
extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new CDButton(BRect(0, 0, 16, 16), "CD", FindFirstCPPlayerDevice());
}

// CDButton can be used as a standalone application, as a simple replicant
// installed into any replicant shell or as a special Deskbar-compatible
// replicant.

CDButton::CDButton(BRect frame, const char *name, int devicefd,
	uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags),
		engine(devicefd)
{
	SetViewColor(200, 200, 200);
	BRect rect(Bounds());
	rect.OffsetTo(B_ORIGIN);
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	AddChild(new BDragger(rect, this, 0));
	segments = new BBitmap(BRect(0, 0, 64 - 1, 8 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDsmall64x8_raw, 64*8, 0, B_COLOR_8_BIT);
}

CDButton::CDButton(BMessage *message)
	:	BView(message),
		engine(FindFirstCPPlayerDevice())
{
	segments = new BBitmap(BRect(0, 0, 64 - 1, 8 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDsmall64x8_raw, 64*8, 0, B_COLOR_8_BIT);
}


CDButton::~CDButton()
{
	delete segments;
}

// archiving overrides
CDButton *
CDButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "CDButton"))
		return NULL;
	return new CDButton(data);
}

status_t 
CDButton::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", app_signature);
	return B_NO_ERROR;
}


void 
CDButton::Draw(BRect rect)
{
	BView::Draw(rect);
	int32 trackNum = engine.TrackStateWatcher()->GetTrack();
	
	// draw the track number
	float offset = BlitDigit(this, BPoint(2, 7),
		trackNum >= 0 ? trackNum / 10 + 1 : 0,
		BRect(0, 0, 4, 7), segments, ViewColor());

	BlitDigit(this, BPoint(offset + 2, 7),
		trackNum >= 0 ? trackNum % 10 + 1 : 0,
		BRect(0, 0, 4, 7), segments, ViewColor());
	
	BRect playStateRect(Bounds());
	playStateRect.OffsetBy(3, 1);
	playStateRect.right = playStateRect.left + 6;
	playStateRect.bottom = playStateRect.top + 4;
		
	SetDrawingMode(B_OP_COPY);
	const rgb_color color = {80, 80, 80, 0};

	// draw the play or pause icons
	SetHighColor(color);
	if (engine.PlayStateWatcher()->GetState() == kPlaying) {
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.left, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left + 1, playStateRect.top), 
			BPoint(playStateRect.left + 1, playStateRect.bottom));
		
		StrokeLine(BPoint(playStateRect.left + 4, playStateRect.top), 
			BPoint(playStateRect.left + 4, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left + 5, playStateRect.top), 
			BPoint(playStateRect.left + 5, playStateRect.bottom));
	} else {
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.left, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 1), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 2), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 3), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.bottom), 
			BPoint(playStateRect.right, playStateRect.top + 2));
	}
}

void
CDButton::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			// This responds to invoking the About CDButton from the replicants'
			// menu
			(new BAlert("About CDButton", "CDButton (Replicant)\n"
				"  Brought to you by Pavel Cisler.\n\n"
				"Use CDButton -deskbar from Terminal to install CDButton into the Deskbar.\n\n"
				"Copyright Be Inc., 1997-2000","OK"))->Go();
			break;

		case 'adRm':
			if (RunningInDeskbar())
				DeskbarAddRemove(false);
			else
				DeskbarAddRemove(true);
			break;

		default:
			if (!Observer::HandleObservingMessages(message))
				// just support observing messages
				BView::MessageReceived(message);
			break;		
	}
}

void
CDButton::NoticeChange(Notifier *)
{
	// respond to a notice by forcig a redraw
	Invalidate();
}

void 
CDButton::AttachedToWindow()
{
	// start observing
	engine.AttachedToLooper(Window());
	StartObserving(engine.TrackStateWatcher());
	StartObserving(engine.PlayStateWatcher());
}

void 
CDButton::Pulse()
{
	// pulse the CD engine
	engine.DoPulse();
}

bool 
CDButton::RunningInDeskbar()
{
	app_info info;
	be_app->GetAppInfo(&info);
	return strcmp(info.signature, "application/x-vnd.Be-TSKB") == 0;
}

const char* kDeskbarItemName = "CD";

void 
CDButton::DeskbarAddRemove(bool add)
{
	//	a simple wrapper routine that uses the new
	//	Deskbar class API
	//
	BDeskbar deskbar;
	bool exists = deskbar.HasItem(kDeskbarItemName);
	if (RunningInDeskbar())
		// work around a problem where synchronous calls to the Deskbar API
		// do not return correct results (ie when called from inside the Deskbar)
		//
		// if we are calling this from a replicant running inside Deskbar
		// <exists> is wrong here, just assume we always need to add/remove
		// the replicant in that case
		exists = !add;
		
	if (add && !exists) {
		BRoster roster;
		entry_ref ref;
		roster.FindApp(app_signature, &ref);
		int32 id;
		deskbar.AddItem(&ref, &id);
	} else if (!add && exists) 
		deskbar.RemoveItem(kDeskbarItemName);
}


void
CDButton::MouseDown(BPoint point)
{
	uint32 mouseButtons;
	for (int32 count = 0; count < 20; count++) {
		BPoint where;
		GetMouse(&where, &mouseButtons, true);
		
		// handle mouse click/press in the CDButton icon
		
		if (!mouseButtons) {
			// handle a few modifier click cases
			if (modifiers() & B_CONTROL_KEY)
				// Control - click ejects the CD
				engine.Eject();
			else if ((modifiers() & (B_COMMAND_KEY | B_OPTION_KEY)) == (B_COMMAND_KEY | B_OPTION_KEY))
				// Command/Alt - Option/Win click skips to previous track
				engine.SkipOneBackward();

			else if (modifiers() & B_COMMAND_KEY)
				// Command/Alt - click skips to next track
				engine.SkipOneForward();
			else
				// button just clicked, play, pause, eject or skip
				engine.PlayOrPause();
			return;
		}
		snooze(10000);
	}
	ConvertToScreen(&point);
	// button pressed, show the whole panel
	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		BPopUpMenu *menu = new BPopUpMenu("context menu");
		menu->AddItem(new BMenuItem("About CDButton", new BMessage(B_ABOUT_REQUESTED)));
		
		bool inDeskbar = RunningInDeskbar();
		menu->AddItem(new BMenuItem(inDeskbar ?
			"Remove from Deskbar" : "Add to Deskbar", new BMessage('adRm')));
		menu->SetTargetForItems(this);
		BPoint where(0, 0);
		ConvertToScreen(&where);
		menu->Go(where, true, false, true);
	} else
		(new CDPanelWindow(point, &engine))->Show();
//	do not call inherited here, we do not wan't to block by tracking the button
}

// Optionally the CDButton can get installed into the Deskbar using the
// replicant technology.
//
// This only works on PR2 if you do not have the mail daemon running
// On R3 however this feature is fully supported. The following code
// illustrates how to use it.

CDButtonApplication::CDButtonApplication()
	:	BApplication(app_signature)
{
	// Just run CDButton as a regular application/replicant hatch
	// show a window with the CD button
	BRect windowRect(100, 100, 120, 120);
	BWindow *window = new BWindow(windowRect, "", B_TITLED_WINDOW, 
			B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE);
	BRect buttonRect(window->Bounds());
	buttonRect.InsetBy(2, 2);
	
	BView *button = new CDButton(buttonRect, "CD", FindFirstCPPlayerDevice());
	window->AddChild(button);
	window->Show();
}


int
main(int, char **argv)
{
	if (argv[1] && strcmp(argv[1], "-deskbar") != 0) {
		// print a simple usage string
		printf(	"# %s (c) 1997-2000 Be Inc.\n"
				"# Usage: %s [-deskbar add | remove]\n",
			 argv[0], argv[0]);
		return 0;
	}

	bool install = argv[1] && (strcmp(argv[1], "-deskbar") == 0);
	if (install) {
		//
		//	replicant handling outside of the BApplication
		//	so that the application does not get listed
		//	in Deskbar, just a bit cleaner
		//
		if (argv[2] && (strcmp(argv[2], "remove") == 0))
			CDButton::DeskbarAddRemove(false);
		else
			CDButton::DeskbarAddRemove(true);
	} else
		(new CDButtonApplication())->Run();

	return 0;
}
