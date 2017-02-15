#include <storage/Entry.h>
#include "FilmStripApp.h"
#include "MessageVals.h"

FilmStripApp::FilmStripApp(void)
	: BApplication("application/x-vnd.BeDTS-FilmStrip"),
		mFSWindow(NULL)
{
}

FilmStripApp::~FilmStripApp(void)
{
}

void FilmStripApp::RefsReceived(BMessage* msg)
{
	entry_ref ref;
	
	if (msg->FindRef("refs",&ref) == B_OK) {
		mFSWindow = new FilmStripWindow(&ref);
		mFSWindow->Show();
	}
}

void FilmStripApp::ReadyToRun(void)
{
	if (CountWindows() == 0) {
		mFSWindow = new FilmStripWindow();
		mFSWindow->Show();
	}
}
