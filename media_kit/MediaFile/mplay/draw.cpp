/*
  A simple program that puts up a window with a BView in it so
  you can draw things.

  Dominic Giampaolo
  dbg@be.com
	
*/

#include "draw.h"
#include "draw_window.h"
#include <FilePanel.h>
#include <Path.h>
#include <stdio.h>


int32 DrawApp::sNumWindows = 0;


DrawApp::DrawApp()
	: BApplication("application/x-vnd.Be.MediaPlayer")
{
	fOpenPanel = NULL;
}


DrawApp::~DrawApp()
{
	delete (fOpenPanel);
}


void
DrawApp::OpenOpenPanel()
{
	if (fOpenPanel != NULL)
		return;

	fOpenPanel = new BFilePanel();
	fOpenPanel->Window()->SetTitle("Select a media file to open:");
	fOpenPanel->Show();
}


void
DrawApp::ReadyToRun()
{
	if (sNumWindows > 0)
		return;

	OpenOpenPanel();	
}


void
DrawApp::ArgvReceived(
	int32	argc,
	char	**argv)
{
	for (int32 i = 1; i < argc; i++) {
		sNumWindows++;
		(new DrawWindow(BRect(100.0, 100.0, 400.0, 400.0), argv[i]))->Show();
	}
}


void
DrawApp::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			OpenOpenPanel();
			break;

		case B_CANCEL:
			if (fOpenPanel != NULL) {
				delete (fOpenPanel);
				fOpenPanel = NULL;
				
				if (sNumWindows < 1)
					PostMessage(B_QUIT_REQUESTED);
			}
			break;

		case msg_WindowClosed:
			if (--sNumWindows < 1) {
				if (fOpenPanel == NULL)
					PostMessage(B_QUIT_REQUESTED);
				else
					sNumWindows = 0;
			}
			break;

		default:
			break;
	}
}


void
DrawApp::RefsReceived(
	BMessage	*message)
{
	int32	count = 0;
	uint32	type = 0;
	message->GetInfo("refs", &type, &count);

	for (int32 i = 0; i < count; i++) {
		entry_ref	ref;

		if (message->FindRef("refs", i, &ref) == B_NO_ERROR) {
			BEntry entry(&ref);

			if (entry.InitCheck() == B_NO_ERROR) {
				BPath path;
				entry.GetPath(&path);

				sNumWindows++;
				(new DrawWindow(BRect(100.0, 100.0, 400.0, 400.0), path.Path()))->Show();
			}
		}
	}
}


int
main(int argc, char **argv)
{
	status_t  res;

	DrawApp *app = new DrawApp();
	app->Run();
	delete (app);
			
	return (B_NO_ERROR);
}
