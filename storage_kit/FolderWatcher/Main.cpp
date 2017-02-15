//
// Main.cpp
// --------
// Written by Scott Barta, Be Incorporated.
 
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Entry.h>
#include <stdio.h>
#include <string.h>

#include "FolderWatcher.h"


const char *path1 = "/boot/home/Folder1";
const char *path2 = "/boot/home/Folder2";
const char *fileType = "application/x-vnd.Be-bookmark";
const char *appSig = "application/x-vnd.Be-NPOS";


// FolderWatcherApp
// ----------------
class FolderWatcherApp : public BApplication {
public:
	FolderWatcherApp();
	
	virtual void MessageReceived(BMessage *msg);
	virtual void ReadyToRun();
	virtual bool QuitRequested();
private:
	FolderWatcher *mFW1;
	FolderWatcher *mFW2;
};


// FolderWatcherApp::FolderWatcherApp
// ----------------------------------
FolderWatcherApp::FolderWatcherApp() :
	BApplication("application/x-vnd.Be-FolderWatcher")
{
}


// FolderWatcherApp::MessageReceived
// ---------------------------------
void FolderWatcherApp::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		// If we get a notification message from one of our FolderWatchers,
		// spit out the contents of the message to stdout.
		case FolderWatcher::kFileAdded:
		case FolderWatcher::kFileChanged:
		case FolderWatcher::kFileRemoved: {
			const char *messageName = NULL;
			switch(msg->what) {
				case FolderWatcher::kFileAdded: messageName = "kFileAdded"; break;
				case FolderWatcher::kFileChanged: messageName = "kFileChanged"; break;
				case FolderWatcher::kFileRemoved: messageName = "kFileRemoved"; break;
			}

			const char *filename = "";
			const char *watcherName;
			FolderWatcher *folderWatcher;
			BMessage attrData;
			msg->FindString("Filename", &filename);
			msg->FindPointer("FolderWatcher", (void**)&folderWatcher);
			msg->FindMessage("AttrData", &attrData);
			if (folderWatcher == mFW1)
				watcherName = path1;
			else
				watcherName = path2;
			printf("Received %s from %s for \"%s\".", messageName, watcherName, filename);
			if (msg->what != FolderWatcher::kFileRemoved) {
				printf("  Data:\n");
				attrData.PrintToStream();
			} else
				printf("\n");
			printf("\n");
			break;
		}
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

// FolderWatcherApp::ReadyToRun
// ----------------------------
void FolderWatcherApp::ReadyToRun()
{
	// Initialize the FolderWatchers.
	printf("--------------------\n");

	BEntry entry1(path1);
	BEntry entry2(path2);
	mFW1 = new FolderWatcher(entry1, fileType, appSig, this, true);
	mFW2 = new FolderWatcher(entry2, fileType, appSig, this, true);
		
	status_t status;
	if ((status = mFW1->Init()) == B_NO_ERROR) {
		printf("FolderWatcher %s initialized.\n", path1);
	} else
		printf("FolderWatcher::Init() %s failed with result code 0x%lx (%s)\n", path1, status, strerror(status));

	if ((status = mFW2->Init()) == B_NO_ERROR) {
		printf("FolderWatcher %s initialized.\n", path2);
	} else
		printf("FolderWatcher::Init() %s failed with result code 0x%lx (%s)\n", path2, status, strerror(status));
	
	printf("--------------------\n");
	
	BMessage msg;

	// Add a few bogus files to the folders to demonstrate FolderWatcher.
	msg.AddString("META:url","http://www.be.com");
	mFW1->AddFile("File1", msg);
	mFW1->AddFile("File2", msg);
	mFW1->AddFile("File3", msg);
	mFW2->AddFile("Doc1", msg);
	mFW2->AddFile("Doc2", msg);
	
	mFW1->RemoveFile("File3");
	msg.AddString("META:url", "Blarr");
	mFW1->ChangeFile("File2", msg);
	
	BApplication::ReadyToRun();
}


// FolderWatcherApp::QuitRequested
// --------------------------------
bool FolderWatcherApp::QuitRequested()
{
	delete mFW1;
	delete mFW2;
	
	return BApplication::QuitRequested();
}

// main
// ----
int main(void)
{
	FolderWatcherApp app;
	app.Run();
	
	return 0;
}

