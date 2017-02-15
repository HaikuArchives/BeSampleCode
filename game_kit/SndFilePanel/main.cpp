// main.cpp
// ------------
// Testbed for a BFilePanel derivative that lets you select and preview
// audio files.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include "SndFilePanel.h"

#include <Alert.h>
#include <Application.h>
#include <Roster.h>

class TestPanelApp : public BApplication {
public:
	TestPanelApp(void);
	~TestPanelApp(void);

	void ReadyToRun(void);
	void RefsReceived(BMessage* msg);
	void MessageReceived(BMessage* msg);
	
private:
	SndFilePanel* panel;
};
	

TestPanelApp::TestPanelApp(void)
	: BApplication("application/x-vnd.BeDTS-ImagePanel"),
		panel(new SndFilePanel(this))
{
}

TestPanelApp::~TestPanelApp(void)
{
	delete panel;
}

void TestPanelApp::ReadyToRun(void)
{
	panel->Show();
}

void TestPanelApp::RefsReceived(BMessage* msg)
{
	entry_ref ref;

	// The user selected a file. Do something with it.
	if (msg->FindRef("refs",&ref) == B_OK) {
		// this will open the selected file in whatever
		// app is configured to handle its MIME type.
		be_roster->Launch(&ref);
	}
}

void TestPanelApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_CANCEL:
		// BFilePanel always sends B_CANCEL when the
		// panel is hidden -- even if the user clicked
		// Open or Save. For the purposes of this simple
		// test app, if the user caused the panel to be hidden
		// we'll just quit. Inform the user so the timid
		// are not frightened.
		(new BAlert("quit_notice",
			"SoundFilePanel is going to quit now.",
			"Fine by me"))->Go();
		PostMessage(B_QUIT_REQUESTED);
		break;
	default:
		// Pass unhandled messages to the default
		// implementation.
		BApplication::MessageReceived(msg);
		break;
	}
}

int main(void)
{
	TestPanelApp app;
	
	return app.Run();
}